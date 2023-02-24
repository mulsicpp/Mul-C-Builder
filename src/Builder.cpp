#if defined(_WIN32)
#include "windows.h"
#endif

#if defined(__linux__)
#include <unistd.h>
#include <linux/limits.h>
#endif

#include <filesystem>

#include <string>

#define PY_SSIZE_T_CLEAN
#include "../Python311/include/Python.h"

#include "Builder.h"
#include "utils.h"

#include <stdlib.h>

using Os = BuildMode::OperatinSystem;
using Arch = BuildMode::Architecture;
using Config = BuildMode::Configuration;

static void add_path_if_free(std::vector<std::string> &list, std::string element)
{
    for (const auto &el : list)
    {
        if (el == element)
            return;
    }
    list.push_back(element);
}

void Builder::initPaths(void)
{
    initialPath = std::filesystem::canonical(std::filesystem::current_path());

    #if defined(_WIN32)
    char result[MAX_PATH];
    GetModuleFileNameA(NULL, result, MAX_PATH);
#endif

#if defined(__linux__)
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
#endif
    builderPath = std::filesystem::canonical(std::filesystem::path(result).parent_path());
}

void Builder::run(int argc, char *argv[])
{
    initPaths();
    passArgs(argc, argv);
    if (cliOptions.action == CLIOptions::Action::SETUP)
    {
        setup();
    }
    generateBuildInfo();
    if (cliOptions.action == CLIOptions::Action::CLEAR)
        clear();
    else
        build();
}

void Builder::passArgs(int argc, char *argv[])
{
    mode.os = Os::OS;
    mode.arch = sizeof(void *) == 8 ? Arch::X64 : Arch::X86;
    mode.config = Config::RELEASE;

    if (mode.os == Os::UNKNOWN)
        error("This operating system is not supported");

    FlagIterator it = FlagIterator(argc, argv);

    // skip first parameter (program name)
    if (it.hasNext())
        it.next();

    if (it.hasNext())
    {
        const char *action = it.next();
        if (strcmp(action, "build") == 0)
            cliOptions.action = CLIOptions::Action::BUILD;
        else if (strcmp(action, "setup") == 0)
            cliOptions.action = CLIOptions::Action::SETUP;
        else if (strcmp(action, "clear") == 0)
            cliOptions.action = CLIOptions::Action::CLEAR;
        else
            error("Action \'%s\' is not allowed. Has to be \'build\', \'setup\' or \'clear\'", action);
    }
    else
    {
        error("No action specified");
    }

A:
    while (it.hasNext())
    {
        auto flagName = it.next();
        for (auto flag : cliOptions.flags)
            if (flag.process(&it))
                goto A;
        warning("Unknown flag \'%s\' ignored", flagName);
    }
}

void Builder::generateBuildInfo(void)
{
    bool python = true;

    if (std::filesystem::is_directory(cliOptions.path))
    {
        std::vector<std::filesystem::path> pyFiles;
        std::vector<std::filesystem::path> jsonFiles;

        auto iterator = std::filesystem::directory_iterator(cliOptions.path);
        for (const auto &entry : iterator)
        {
            if (entry.is_regular_file())
            {
                std::string s = entry.path().string();
                int py_length = sizeof("mulc.py") - 1;
                int json_length = sizeof("mulc.json") - 1;
                if (s.length() > py_length && s.substr(s.length() - py_length) == "mulc.py")
                    pyFiles.push_back(entry.path());
                else if (s.length() > json_length && s.substr(s.length() - json_length) == "mulc.json")
                    jsonFiles.push_back(entry.path());
            }
        }
        if (pyFiles.size() == 1)
        {
            buildFilePath = pyFiles[0].string();
            python = true;
        }
        else if (pyFiles.size() == 0)
        {
            if (jsonFiles.size() == 1)
            {
                buildFilePath = jsonFiles[0].string();
                python = false;
            }
            else if (jsonFiles.size() == 0)
            {
                error("No build file found");
            }
            else
            {
                error("Build file is ambiguous");
            }
        }
        else
        {
            error("Build file is ambiguous");
        }
    }
    else
    {
        std::string s = cliOptions.path;
        int py_length = sizeof("mulc.py") - 1;
        int json_length = sizeof("mulc.json") - 1;
        if (s.length() > py_length && s.substr(s.length() - py_length) == "mulc.py")
        {
            buildFilePath = cliOptions.path;
            python = true;
        }
        else if (s.length() > json_length && s.substr(s.length() - json_length) == "mulc.json")
        {
            buildFilePath = cliOptions.path;
            python = false;
        }
        else
        {
            error("The specified file \'%s\' is not a build file", cliOptions.path.c_str());
        }
    }

    if (python)
    {
        FILE* file = fopen(buildFilePath.string().c_str(), "r");
        if(file == nullptr)
            error("The build file \'%s\' could not be opened", buildFilePath.string().c_str());
        //mbstowcs()
        wchar_t nameW[] = L"mulC";
        wchar_t targetFileW[1024];
        wchar_t varsW[4096];
        wchar_t modeW[128];
        wchar_t *args[4] = {nameW, targetFileW, modeW, varsW};

        buildFilePath.replace_extension(".json");
        mbstowcs(targetFileW, buildFilePath.string().c_str(), 1024);

        char mode[128];
        sprintf(mode, "%s/%s/%s", this->mode.os == Os::WINDOWS ? "windows" : "linux", this->mode.arch == Arch::X64 ? "x64" : "x86", this->mode.config == Config::RELEASE ? "release" : "debug");
        mbstowcs(modeW, mode, 128);


        char vars[4096];
        char *ptr = vars;
        for(const auto& [name, value] : cliOptions.vars)
            ptr += sprintf(ptr, "%s=%s;", name.c_str(), value.c_str());
        mbstowcs(varsW, vars, 4096);

        printf("'%s'\n", vars);

        Py_SetProgramName(args[0]);
        Py_Initialize();

        PySys_SetArgv(4, args);
        int x = 0;

        fseek(file, 0, SEEK_END);
        int length = ftell(file);
        fseek (file, 0, SEEK_SET);

        char *buffer = new char[length + 1];
        fread(buffer, 1, length, file);
        buffer[length] = 0;

        PyRun_SimpleString("import sys");
        char cmd[4096];
        sprintf(cmd, "sys.path.append(R\'%s\')", builderPath.string().c_str());
        PyRun_SimpleString(cmd);

        fclose(file);

        if(PyRun_SimpleString(buffer) != 0)
            error("Python script has errors");

        Py_Finalize();
    }
}

void Builder::build(void)
{
}

void Builder::setup(void)
{
}

void Builder::clear(void)
{
}