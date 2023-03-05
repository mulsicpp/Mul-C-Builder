#pragma once

#include <string>
#include <vector>
#include <unordered_map>

struct BuildInfo
{
    std::string group = "default";

    struct Compile {
        struct TranslationUnit {
            std::string cFilePath;
            std::string oFilePath;
        };

        std::vector<TranslationUnit> translationUnits;

        std::string std;
        std::vector<std::string> includePaths;
        std::vector<std::string> defines;

        std::vector<std::string> additionalFlags;
    } compile;

    struct Output
    {
        enum class Type
        {
            APP,
            LIB,
            DLL
        } type;
        std::string path;

        std::vector<std::string> libPaths;
        std::vector<std::string> libs;
        std::vector<std::string> namedLibs;

        std::vector<std::string> additionalFlags;
    } output;

    std::vector<std::string> requirements;

    struct Export
    {
        enum class Type {
            FILE,
            HEADERS
        } type;
        std::string srcPath;
        std::string dstPath;
    };

    std::vector<Export> exports;

    struct Command {
        std::string appPath;
        std::string args;
    };

    std::vector<Command> preBuildCommands;
    std::vector<Command> postBuildCommands;

    std::string exportSettings = "";

    std::unordered_map<std::string, std::vector<std::string>> headerDependencies;
};