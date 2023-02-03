#pragma once

#include <string>
#include <vector>
#include <unordered_set>

struct BuildInfo {
    struct Output {
        enum Type {
            NONE,
            APP,
            LIB,
            DLL
        } type;
        std::string path;
    } output;
    std::unordered_set<std::string> include_paths;
    std::unordered_set<std::string> defines;
    std::string std;

    std::unordered_set<std::string> lib_paths;
    std::unordered_set<std::string> libs;
    std::unordered_set<std::string> link_libs;


};