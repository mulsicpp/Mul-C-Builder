#pragma once

#include "BuildInfo.h"
#include "BuildMode.h"

struct SystemInterface
{
    std::string systemIncludePaths = "", systemLib32Paths = "", systemLib64_hs = "", compiler32 = "", compiler64 = "";

    int compile(BuildInfo::Compile::TranslationUnit tu, BuildInfo *buildInfo, BuildMode mode, std::string *output);

    int linkApp(BuildInfo *buildInfo, BuildMode mode);

    int linkApp(BuildInfo *buildInfo, BuildMode mode);

    int createLib(BuildInfo *buildInfo, BuildMode mode);

    int executeProgram(const char* prog, const char* args);
};