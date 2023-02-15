#pragma once

#include <string>
#include <vector>

struct BuildInfo
{
    struct Output
    {
        enum class Type
        {
            APP,
            LIB,
            DLL
        } type;
        std::string path;
    };

    Output output;
    std::vector<std::string> include_paths;
    std::vector<std::string> defines;
    std::string std;

    std::vector<std::string> lib_paths;
    std::vector<std::string> libs;
    std::vector<std::string> link_libs;

};