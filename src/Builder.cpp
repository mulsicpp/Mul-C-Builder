#if defined(_WIN32)
#include "windows.h"
#endif

#if defined(__linux__)
#include <unistd.h>
#include <linux/limits.h>
#endif

#include <filesystem>

#define EXISTS std::filesystem::exists

#include <fstream>
#include <string>

#if defined(_WIN32)
#define PY_SSIZE_T_CLEAN
#include "../Python311_windows/include/Python.h"
#elif defined(__linux__)
#define PY_SSIZE_T_CLEAN
#include "../python3.8_linux/include/Python.h"
#endif

#include "jsonUtils.h"

using json = nlohmann::json;

#include "Builder.h"
#include "utils.h"

#include <stdlib.h>

using Os = BuildMode::OperatinSystem;
using Arch = BuildMode::Architecture;
using Config = BuildMode::Configuration;

static void addPathIfFree(std::vector<std::string> &list, std::string element)
{
    const auto &canonical = std::filesystem::canonical(element);
    for (const auto &el : list)
    {
        if (std::filesystem::canonical(el) == canonical)
            return;
    }
    list.push_back(element);
}

static void addElementIfFree(std::vector<std::string> &list, std::string element)
{
    for (const auto &el : list)
    {
        if (el == element)
            return;
    }
    list.push_back(element);
}

static bool isSubPath(const std::filesystem::path &base, const std::filesystem::path &sub)
{
    std::string relative = std::filesystem::relative(sub, base).string();
    // Size check for a "." result.
    // If the path starts with "..", it's not a subdirectory.
    return relative.size() == 1 || relative[0] != '.' && relative[1] != '.';
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
        setup();
    else if (cliOptions.action == CLIOptions::Action::CLEAR)
        clear();
    else
    {
        generateBuildInfo();
        build();
    }
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
    if (cliOptions.action != CLIOptions::Action::SETUP)
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

    buildFilePath = std::filesystem::canonical(buildFilePath);
}

void Builder::generateBuildInfo(void)
{
    std::filesystem::current_path(buildFilePath.parent_path());

    if (python)
    {
        FILE *file = fopen(buildFilePath.filename().string().c_str(), "rb");
        if (file == nullptr)
            error("The build file \'%s\' could not be opened", buildFilePath.filename().string().c_str());
        // mbstowcs()
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
        for (const auto &[name, value] : cliOptions.vars)
            ptr += sprintf(ptr, "%s=%s;", name.c_str(), value.c_str());
        mbstowcs(varsW, vars, 4096);

        Py_SetProgramName(args[0]);
        Py_Initialize();

        PySys_SetArgv(4, args);
        int x = 0;

        fseek(file, 0, SEEK_END);
        int length = ftell(file);
        fseek(file, 0, SEEK_SET);

        char *buffer = new char[length + 1];
        fread(buffer, 1, length, file);
        buffer[length] = 0;

        PyRun_SimpleString("import sys");
        char cmd[4096];
        sprintf(cmd, "sys.path.append(R\'%s\')", builderPath.string().c_str());
        PyRun_SimpleString(cmd);

        fclose(file);

        if (PyRun_SimpleString(buffer) != 0)
            error("Python script has errors");

        Py_Finalize();

        delete[] buffer;
    }

    std::filesystem::current_path(buildFilePath.parent_path());

    std::ifstream jsonFile(buildFilePath.filename().string());
    json jsonData = json::parse(jsonFile);

    const json mode = jsonRequire<json>(jsonData, "mode");

    jsonTryOn<std::string>(mode, "os", [this](const std::string str)
                           {
        this->mode.os = str == "windows" ? Os::WINDOWS : (str == "linux" ? Os::LINUX : Os::UNKNOWN);
        if(this->mode.os == Os::UNKNOWN)
            error("The operating system \'%s\' is unknown", str);
        if(this->mode.os != Os::OS)
            error("The selected operating system \'%s\' does not match the current operating system", str); });

    jsonTryOn<std::string>(mode, "arch", [this](const std::string str)
                           { this->mode.arch = str == "x64" ? Arch::X64 : (str == "x86" ? Arch::X86 : Arch::UNKNOWN); });

    jsonTryOn<std::string>(mode, "config", [this](const std::string str)
                           {
                                if(str == "release")
                                    this->mode.config = Config::RELEASE;
                                else if(str == "debug")
                                    this->mode.config = Config::DEBUG;
                                else
                                    error("The configuration \'%s\' is unknown", str.c_str()); });

    const json compile = jsonRequire<json>(jsonData, "compile");

    jsonTryOn<std::string>(jsonData, "group", [this](std::string group)
                           {
                                if(group.size() > 0)
                                    info.group = group; });

    jsonOnEach<std::string>(compile, "sources", [this](const std::string str)
                            {
                                std::filesystem::path source(str);
                                if(!EXISTS(source))
                                    error("The source \'%s\' does not exist", source.string().c_str());
                                if(std::filesystem::is_directory(source)){
                                    auto iterator = std::filesystem::recursive_directory_iterator(source);
                                    for(const auto &entry : iterator) {
                                        if(entry.path().extension() == ".cpp" ||entry.path().extension() == ".c")
                                            info.compile.translationUnits.push_back({entry.path().string(), ""});
                                    }
                                } else {
                                    info.compile.translationUnits.push_back({source.string(), ""});
                                } });

    jsonTryOnEach<std::string>(compile, "sourceBlackList", [this](const std::string str)
                               {
                                std::filesystem::path source(str);
                                if(!EXISTS(source))
                                    error("The source \'%s\' does not exist", source.string().c_str());
                                if(std::filesystem::is_directory(source)){
                                    for(int i = 0; i < info.compile.translationUnits.size();)
                                        if(isSubPath(source, info.compile.translationUnits[i].cFilePath))
                                            info.compile.translationUnits.erase(info.compile.translationUnits.begin() + i);
                                        else i++;
                                } else {
                                    source = std::filesystem::canonical(source);
                                    for(int i = 0; i < info.compile.translationUnits.size();)
                                        if(source == std::filesystem::canonical(info.compile.translationUnits[i].cFilePath))
                                            info.compile.translationUnits.erase(info.compile.translationUnits.begin() + i);
                                        else i++;
                                } });

    jsonTryOn<std::string>(compile, "std", [this](const std::string str)
                           { info.compile.std = str; });

    jsonTryOnEach<std::string>(compile, "includePaths", [this](const std::string str)
                               { addPathIfFree(info.compile.includePaths, str); });

    jsonTryOnEach<std::string>(compile, "defines", [this](const std::string str)
                               { info.compile.defines.push_back(str); });

    jsonTryOnEach<std::string>(compile, "additionalFlags", [this](const std::string str)
                               { info.compile.additionalFlags.push_back(str); });

    const json output = jsonRequire<json>(jsonData, "output");

    jsonOn<std::string>(output, "type", [this](const std::string str)
                        {
                            if(str == "app")
                                info.output.type = BuildInfo::Output::Type::APP;
                            else if(str == "lib")
                                info.output.type = BuildInfo::Output::Type::LIB;
                            else if(str == "dll")
                                info.output.type = BuildInfo::Output::Type::DLL;
                            else
                                error("The type \'%s\' is unknown", str.c_str()); });

    jsonOn<std::string>(output, "path", [this](const std::string str)
                        { info.output.path = str; });

    jsonTryOnEach<std::string>(output, "libraryPaths", [this](const std::string str)
                               { addPathIfFree(info.output.libPaths, str); });

    jsonTryOnEach<std::string>(output, "libs", [this](const std::string str)
                               { addElementIfFree(info.output.libs, str); });

    jsonTryOnEach<std::string>(output, "namedLibs", [this](const std::string str)
                               { addElementIfFree(info.output.namedLibs, str); });

    jsonTryOnEach<std::string>(output, "additionalFlags", [this](const std::string str)
                               { info.output.additionalFlags.push_back(str); });

    jsonTryOnEach<std::string>(jsonData, "require", [this](const std::string str)
                               { addPathIfFree(info.requirements, str); });

    jsonTryOnEach<std::string>(jsonData, "preBuildCommands", [this](const json cmd)
                               {
        BuildInfo::Command command;
        command.appPath = jsonRequire<std::string>(cmd, "appPath");
        command.args = "";

        jsonOnEach<std::string>(cmd, "agrs", [&](std::string arg) {
            command.args += " " + arg;
        });

        info.preBuildCommands.push_back(command); });

    jsonTryOnEach<json>(jsonData, "postBuildCommands", [this](const json cmd)
                        {
        BuildInfo::Command command;
        command.appPath = jsonRequire<std::string>(cmd, "appPath");
        command.args = "";

        jsonOnEach<std::string>(cmd, "agrs", [&](std::string arg) {
            command.args += " " + arg;
        });

        info.postBuildCommands.push_back(command); });

    jsonTryOnEach<json>(jsonData, "exports", [this](const json e)
                        {
        BuildInfo::Export exp;

        const auto type_str = jsonRequire<std::string>(e, "type");
        if(type_str == "file")
            exp.type = BuildInfo::Export::Type::FILE;
        else if(type_str == "headers")
            exp.type = BuildInfo::Export::Type::HEADERS;
        else
            error("The export type \'%s\' is unknown", type_str.c_str());
        exp.srcPath = jsonRequire<std::string>(e, "srcPath");
        exp.dstPath = jsonRequire<std::string>(e, "dstPath");
        
        info.exports.push_back(exp); });

    jsonTryOn<json>(jsonData, "exportSettings", [this](json settings)
                    { info.exportSettings = settings.dump(4); });
}

void Builder::build(void)
{
    for (const auto &tu : info.compile.translationUnits)
        printf("%s\n", tu.cFilePath.c_str());
}

void Builder::setup(void)
{
}

void Builder::clear(void)
{
    std::filesystem::path path("mulC.build");

    if (cliOptions.group.size() > 0)
        path += "/" + cliOptions.group;

    if (EXISTS(path))
        std::filesystem::remove_all(path);
}