#pragma once

#include "CLIOptions.h"
#include "BuildMode.h"
#include "BuildInfo.h"

class Builder {
private:
    CLIOptions cliOptions;
    BuildMode mode;
    BuildInfo info;
public:
    void passArgs(int argc, char *argv[]);
    void generateBuildInfo(void);
    void build(void);
};