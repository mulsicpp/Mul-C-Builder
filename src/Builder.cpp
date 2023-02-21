#include <filesystem>

#define PY_SSIZE_T_CLEAN
#include "../Python311/include/Python.h"

#include "Builder.h"
#include "utils.h"

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

void Builder::run(int argc, char *argv[]) {
    passArgs(argc, argv);
    if(cliOptions.action == CLIOptions::Action::SETUP) {
        setup();
    }
    generateBuildInfo();
    if(cliOptions.action == CLIOptions::Action::CLEAR)
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
        if     (strcmp(action, "build") == 0)
            cliOptions.action = CLIOptions::Action::BUILD;
        else if(strcmp(action, "setup") == 0)
            cliOptions.action = CLIOptions::Action::SETUP;
        else if(strcmp(action, "clear") == 0)
            cliOptions.action = CLIOptions::Action::CLEAR;
        else
            error("Action \'%s\' is not allowed. Has to be \'build\', \'setup\' or \'clear\'", action);
    } else {
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
    std::string path = "";
    bool python = false;


    FILE *file;
    int argcount;
    wchar_t a[100] = L"a";
    wchar_t b[100] = L"b";
    wchar_t c[100] = L"c";
    wchar_t *args[3] = {a, b, c};

    argcount = 3;

    Py_SetProgramName(args[0]);
    Py_Initialize();

    PySys_SetArgv(argcount, args);
    PyRun_SimpleString("import sys\n\nprint(sys.argv[0])");
    Py_Finalize();
}

void Builder::build(void)
{
}