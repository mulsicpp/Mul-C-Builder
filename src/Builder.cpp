#include "Builder.h"

#define PY_SSIZE_T_CLEAN
#include "../Python311/include/Python.h"

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

void Builder::passArgs(int argc, char *argv[])
{
    mode.os = Os::OS;
    mode.arch = sizeof(void *) == 8 ? Arch::X64 : (sizeof(void *) == 4 ? Arch::X86 : Arch::UNKNOWN);
    mode.config = Config::RELEASE;

    FlagIterator it = FlagIterator(argc, (const char **)argv);

A:
    while (it.hasNext())
    {
        it.next();
        for (auto flag : cliOptions.flags)
            if (flag.process(&it))
                goto A;
        printf("unknown flag ignored\n");
    }
}

void Builder::generateBuildInfo(void)
{
    wchar_t x;

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