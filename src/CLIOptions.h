#pragma once

#include <string>
#include <functional>

class FlagIterator
{
private:
    int argc;
    const char **argv;

    int index;

public:
    FlagIterator(int argc, const char **argv);

    bool hasNext(void);
    const char *next(void);
    const char *current(void);
};

class Flag
{
private:
    std::function<bool(FlagIterator *)> processor;

public:
    Flag(std::vector<const char *> names, std::function<void(void)> func);
    Flag(std::vector<const char *> names, std::function<void(std::string)> func);

    bool process(FlagIterator *it);
};

struct CLIOptions
{
    bool run = false;
    bool force = false;
    std::string path = ".";

    std::string s;

    std::vector<Flag> flags = {
        Flag({"--run", "-r"}, [this](){ this->run = true; }),
        Flag({"--force", "-f"}, [this](){ this->force = true; }),
        Flag({"--path", "-p"}, [this](std::string s){ this->path = s; })
    };
};
