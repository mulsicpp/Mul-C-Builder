#if defined(_WIN32)
#include <Windows.h>
#include <io.h>
#include <fcntl.h>
#elif defined(__linux__)
#include <unistd.h>
#include <linux/limits.h>
#endif

#include "SystemInterface.h"
#include "utils.h"

#include <filesystem>
#include <vector>
#include <string>
#include <algorithm>
#include <string.h>

using Os = BuildMode::OperatinSystem;
using Arch = BuildMode::Architecture;
using Config = BuildMode::Configuration;

#if defined(_WIN32)

static FILE *win_popen(const char *proc, char *args, HANDLE *out, PROCESS_INFORMATION *pi)
{
    HANDLE outWrite = NULL;

    SECURITY_ATTRIBUTES saAttr;

    // Set the bInheritHandle flag so pipe handles are inherited.

    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    // Create a pipe for the child process's STDOUT.

    if (!CreatePipe(out, &outWrite, &saAttr, 0))
        exit(-1);

    // Ensure the read handle to the pipe for STDOUT is not inherited.

    if (!SetHandleInformation(*out, HANDLE_FLAG_INHERIT, 0))
        exit(-1);

    STARTUPINFOA si;
    BOOL bSuccess = FALSE;

    // Set up members of the PROCESS_INFORMATION structure.

    ZeroMemory(pi, sizeof(PROCESS_INFORMATION));

    // Set up members of the STARTUPINFO structure.
    // This structure specifies the STDIN and STDOUT handles for redirection.

    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);
    si.hStdError = outWrite;
    si.hStdOutput = outWrite;
    si.hStdInput = NULL;
    si.dwFlags |= STARTF_USESTDHANDLES;

    // Create the child process.

    bSuccess = CreateProcessA(proc,
                              args, // command line
                              NULL, // process security attributes
                              NULL, // primary thread security attributes
                              TRUE, // handles are inherited
                              0,    // creation flags
                              NULL, // use parent's environment
                              NULL, // use parent's current directory
                              &si,  // STARTUPINFO pointer
                              pi);  // receives PROCESS_INFORMATION

    if (bSuccess)
    {
        CloseHandle(outWrite);

        int nHandle = _open_osfhandle((long)*out, _O_RDONLY);

        if (nHandle != -1)
        {
            FILE *p_file = _fdopen(nHandle, "r");
            return p_file;
        }
    }
    return NULL;
}

static int win_pclose(HANDLE *out, PROCESS_INFORMATION *pi)
{
    // Wait for the process to exit
    WaitForSingleObject(pi->hProcess, INFINITE);

    // Process has exited - check its exit code
    DWORD exitCode;
    GetExitCodeProcess(pi->hProcess, &exitCode);

    // At this point exitCode is set to the process' exit code

    // Handles must be closed when they are no longer needed
    CloseHandle(pi->hThread);
    CloseHandle(pi->hProcess);

    CloseHandle(*out);

    return exitCode;
}

#endif

static std::string &trim(std::string &str)
{
    str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](unsigned char ch)
                                        { return !std::isspace(ch); }));

    str.erase(std::find_if(str.rbegin(), str.rend(), [](unsigned char ch)
                           { return !std::isspace(ch); })
                  .base(),
              str.end());

    return str;
}

static std::string generateCompileFlags(BuildInfo::Compile *compile) {
    std::string ret = "";

    if(compile->std.size() > 0)
        ret += OS_STD(compile->std);
    for(const auto &str : compile->includePaths)
        ret += OS_INCLUDE_PATH(str);
    for(const auto &str : compile->defines)
        ret += OS_DEFINE(str);
    for(const auto &str : compile->additionalFlags)
        ret += str + " ";

    return ret;
}

static std::string generateLinkerFlags(BuildInfo::Output *output) {
    std::string ret = "";
    for(const auto &str : output->libPaths)
        ret += OS_LIBRARY_PATH(str);
    for(const auto &str : output->namedLibs)
        ret += OS_NAMED_LIBRARY(str);
    for(const auto &str : output->additionalFlags)
        ret += str + " ";
}

int SystemInterface::compile(BuildInfo::Compile::TranslationUnit tu, BuildInfo *buildInfo, BuildMode mode, std::string *output)
{
    char buffer[1024];

    const unsigned int pref_length = 21;
    char *path;
    std::filesystem::path current_include;
    int include_length = 0;

    std::vector<std::string> headers;
#if defined(_WIN32)
    HANDLE out;
    PROCESS_INFORMATION pi;

    std::string str;
    
    FILE *pipe = win_popen(((mode.arch == Arch::X64 ? compiler64 : compiler32) + "\\cl.exe").c_str(), (char *)(std::string(mode.config == Config::DEBUG ? "/DEBUG:FASTLINK " : "") + generateCompileFlags(&(buildInfo->compile)) + systemIncludePaths + "/c /showIncludes /EHsc /MD /Fo\"" + tu.oFilePath + "\" \"" + tu.cFilePath + "\"").c_str(), &out, &pi);

    if (!pipe)
    {
        return ERROR_CODE;
    }

    for (int i = 0; i < 3; i++)
        fgets(buffer, 1024, pipe);

    while (fgets(buffer, 1024, pipe))
    {
        if (memcmp(buffer, "Note: including file:", pref_length) == 0)
        {
            path = buffer + 21;

            while ((++path)[0] == ' ')
                ;
            include_length = strlen(path);
            while (path[include_length - 1] == '\n' || path[include_length - 1] == '\r')
            {
                path[include_length - 1] = 0;
                include_length = strlen(path);
            }

            auto rel = std::filesystem::relative(path, std::filesystem::current_path());
            if (!rel.empty() && rel.native()[0] != '.')
            {
                headers.push_back(std::filesystem::proximate(path).string());
                continue;
            }
            for (const auto &include_Path : buildInfo->compile.includePaths)
            {
                auto rel = std::filesystem::relative(path, include_Path);
                if (!rel.empty() && rel.native()[0] != '.')
                {
                    headers.push_back(std::filesystem::proximate(path).string());
                    break;
                }
            }
        }
        else{
            str = buffer;
            trim(str);
            if(str != std::filesystem::path(tu.cFilePath).filename())
                *output += buffer;
        }
    }

    buildInfo->headerDependencies[tu.cFilePath] = headers;

    return win_pclose(&out, &pi) == 0 ? UPDATED_CODE : ERROR_CODE;
#elif defined(__linux__)
    FILE *pipe = popen(("g++ -march=x86-64 " + std::string(mode.arch == Arch::X86 ? "-m32 " : "-m64 ") + generateCompileFlags(&(buildInfo->compile)) + "-c -H -o \"" + tu.oFilePath + "\" \"" + tu.cFilePath + "\" 2>&1").c_str(), "r");

    if (!pipe)
    {
        return ERROR_CODE;
    }

    while (fgets(buffer, 1024, pipe))
    {
        // check include files
        if (buffer[0] == '.')
        {
            path = buffer + 1;
            while (path[0] == '.')
                path++;
            if (path[0] == ' ')
            {
                path++;
                include_length = strlen(path);
                path[include_length - 1] = 0;
                auto rel = std::filesystem::relative(path, std::filesystem::current_path());
                if (!rel.empty() && rel.native()[0] != '.')
                {
                    headers.push_back(std::filesystem::proximate(path).string());
                    continue;
                }
                for (const auto &include_Path : buildInfo->compile.includePaths)
                {
                    auto rel = std::filesystem::relative(path, include_Path);
                    if (!rel.empty() && rel.native()[0] != '.')
                    {
                        headers.push_back(std::filesystem::proximate(path).string());
                        break;
                    }
                }
            }
        }
        else if (strcmp(buffer, "Multiple include guards may be useful for:\n") == 0)
        {
            continue;
        }
        else
        {
            if (buffer[0] != '/')
            {
                *output += buffer;
            }
        }
    }

    buildInfo->headerDependencies[tu.cFilePath] = headers;

    return pclose(pipe) == 0 ? UPDATED_CODE : ERROR_CODE;
#endif
}


int SystemInterface::linkApp(BuildInfo *buildInfo, BuildMode mode)
{
    std::string oFiles = "";
    for (const auto &tu : buildInfo->compile.translationUnits)
        oFiles += "\"" + tu.oFilePath + "\" ";

    std::string libStr = "";
    for (const auto &lib : buildInfo->output.libs)
        libStr += "\"" + lib + "\" ";

    char buffer[1024];
#if defined(_WIN32)
    HANDLE out;
    PROCESS_INFORMATION pi;
    // TODO fix this shit
    FILE *pipe = win_popen(((mode.arch == Arch::X64 ? compiler64 : compiler32) + "\\link.exe").c_str(), (char *)(std::string(mode.config == Config::DEBUG ? "/DEBUG:FASTLINK " : "") + "/out:\"" + buildInfo->output.path + "\" " + oFiles + libStr + generateLinkerFlags(&(buildInfo->output)) + (mode.arch == Arch::X64 ? systemLib64Paths : systemLib32Paths)).c_str(), &out, &pi);
#elif defined(__linux__)
    FILE *pipe = popen(("g++ -march=x86-64 " + std::string(mode.arch == Arch::X86 ? "-m32 " : "-m64 ") + "-o \"" + buildInfo->output.path + "\" " + oFiles + generateLinkerFlags(&(buildInfo->output)) + libStr + " 2>&1").c_str(), "r");
#endif
    if (!pipe)
    {
        return ERROR_CODE;
    }

#if defined(_WIN32)
    for (int i = 0; i < 3; i++)
        fgets(buffer, 1024, pipe);
#endif

    while (fgets(buffer, 1024, pipe))
        printf("%s", buffer);
#if defined(_WIN32)
    return win_pclose(&out, &pi) == 0 ? UPDATED_CODE : ERROR_CODE;
#elif defined(__linux__)
    return pclose(pipe) == 0 ? UPDATED_CODE : ERROR_CODE;
#endif
}

int SystemInterface::createLib(BuildInfo *buildInfo, BuildMode mode)
{
    char buffer[1024];
    std::string oFiles = "";
    for (const auto &tu : buildInfo->compile.translationUnits)
        oFiles += "\"" + tu.oFilePath + "\" ";
#if defined(_WIN32)
    HANDLE out;
    PROCESS_INFORMATION pi;
    FILE *pipe = win_popen(((mode.arch == Arch::X64 ? compiler64 : compiler32) + "\\lib.exe").c_str(), (char *)(" /out:\"" + buildInfo->output.path + "\" " + oFiles).c_str(), &out, &pi);
#elif defined(__linux__)
    FILE *pipe = popen(("ar rcs \"" + buildInfo->output.path + "\" " + oFiles + " 2>&1").c_str(), "r");
#endif
    if (!pipe)
    {
        return ERROR_CODE;
    }

#if defined(_WIN32)
    for (int i = 0; i < 3; i++)
        fgets(buffer, 1024, pipe);
#endif

    while (fgets(buffer, 1024, pipe))
        printf("%s", buffer);
#if defined(_WIN32)
    return win_pclose(&out, &pi) == 0 ? UPDATED_CODE : ERROR_CODE;
#elif defined(__linux__)
    return pclose(pipe) == 0 ? UPDATED_CODE : ERROR_CODE;
#endif
}

int SystemInterface::linkDll(BuildInfo *buildInfo, BuildMode mode)
{
    std::string oFiles = "";
    for (const auto &tu : buildInfo->compile.translationUnits)
        oFiles += "\"" + tu.oFilePath + "\" ";

    std::string libStr = "";
    for (const auto &lib : buildInfo->output.libs)
        libStr += "\"" + lib + "\" ";

    char buffer[1024];
#if defined(_WIN32)
    HANDLE out;
    PROCESS_INFORMATION pi;
    // TODO fix this shit
    FILE *pipe = win_popen(((mode.arch == Arch::X64 ? compiler64 : compiler32) + "\\link.exe").c_str(), (char *)(std::string(mode.config == Config::DEBUG ? "/DEBUG:FASTLINK " : "") + "/DLL /out:\"" + buildInfo->output.path + "\" " + oFiles + libStr + generateLinkerFlags(&(buildInfo->output)) + (mode.arch == Arch::X64 ? systemLib64Paths : systemLib32Paths)).c_str(), &out, &pi);
#elif defined(__linux__)
    FILE *pipe = popen(("g++ --shared -march=x86-64 " + std::string(mode.arch == Arch::X86 ? "-m32 " : "-m64 ") + "-o \"" + buildInfo->output.path + "\" " + oFiles + generateLinkerFlags(&(buildInfo->output)) + libStr + " 2>&1").c_str(), "r");
#endif
    if (!pipe)
    {
        return ERROR_CODE;
    }

#if defined(_WIN32)
    for (int i = 0; i < 3; i++)
        fgets(buffer, 1024, pipe);
#endif

    while (fgets(buffer, 1024, pipe))
        printf("%s", buffer);
#if defined(_WIN32)
    return win_pclose(&out, &pi) == 0 ? UPDATED_CODE : ERROR_CODE;
#elif defined(__linux__)
    return pclose(pipe) == 0 ? UPDATED_CODE : ERROR_CODE;
#endif
}

int SystemInterface::executeProgram(const char *prog, const char *args)
{
    char buffer[1024];
#if defined(_WIN32)
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
    ZeroMemory(&si, sizeof(STARTUPINFOA));

    int bSuccess = CreateProcessA(prog,
                                  (char *)args, // command line
                                  NULL,         // process security attributes
                                  NULL,         // primary thread security attributes
                                  TRUE,         // handles are inherited
                                  0,            // creation flags
                                  NULL,         // use parent's environment
                                  NULL,         // use parent's current directory
                                  &si,          // STARTUPINFO pointer
                                  &pi);         // receives PROCESS_INFORMATION

    if (bSuccess)
    {
        // Wait for the process to exit
        WaitForSingleObject(pi.hProcess, INFINITE);

        // Process has exited - check its exit code
        DWORD exitCode;
        GetExitCodeProcess(pi.hProcess, &exitCode);

        // At this point exitCode is set to the process' exit code

        // Handles must be closed when they are no longer needed
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);

        return exitCode;
    }
#elif defined(__linux__)
    return system(("\'" + std::string(prog) + "\' " + std::string(args == NULL ? "" : args)).c_str());
#endif
}
/**/