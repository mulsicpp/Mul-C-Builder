#pragma once

#include "CLIOptions.h"
#include "BuildMode.h"
#include "BuildInfo.h"

#include <filesystem>

class Builder {
private:
    CLIOptions cliOptions;
    BuildMode mode;
    BuildInfo info;

    std::filesystem::path builderPath = "";
    std::filesystem::path initialPath = "";
    std::filesystem::path buildFilePath = "";
public:
    void run(int argc, char *argv[]);

private:
    void initPaths(void);

    void passArgs(int argc, char *argv[]);
    void generateBuildInfo(void);

    void build(void);
    void setup(void);
    void clear(void);
};