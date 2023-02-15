#pragma once

#if defined(_WIN32)
    #define OS WINDOWS
#elif defined(__linux__)
    #define OS LINUX_UNIX
#else
    #define OS UNKNOWN
#endif

struct BuildMode {
    enum class OperatinSystem {
        UNKNOWN,
        WINDOWS,
        LINUX_UNIX
    } os;

    enum class Architecture {
        UNKNOWN,
        X86,
        X64
    } arch;

    enum class Configuration {
        RELEASE,
        DEBUG
    } config;
};