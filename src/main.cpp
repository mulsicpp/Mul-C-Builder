#define PY_SSIZE_T_CLEAN
#include "../Python311/include/Python.h"

void main()
{
    FILE* file;
    int argc;
    wchar_t a[100] = L"a";
    wchar_t b[100] = L"b";
    wchar_t c[100] = L"c";
    wchar_t *args[3] = { a, b, c };

    argc = 3;

    Py_SetProgramName(args[0]);
    Py_Initialize();
    
    PySys_SetArgv(argc, args);
    PyRun_SimpleString("import sys\n\nprint(sys.argv[0])");
    Py_Finalize();

    return;
}