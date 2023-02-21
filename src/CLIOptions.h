#pragma once

#include <string>
#include <functional>

class FlagIterator
{
private:
    int argc;
    char **argv;

    int index;

public:
    FlagIterator(int argc, char **argv);

    bool hasNext(void);
    char *next(void);
    char *current(void);
};

class Flag
{
private:
    std::function<bool(FlagIterator *)> processor;

public:
    Flag(std::vector<const char *> names, std::function<void(void)> func);
    Flag(std::vector<const char *> names, std::function<void(char *)> func);

    bool process(FlagIterator *it);
};

struct CLIOptions
{
    bool run = false;
    bool force = false;
    std::string path = ".";

    enum class Action {
        BUILD,
        SETUP,
        CLEAR
    } action = Action::BUILD;

    std::unordered_map<std::string, std::string> vars;

    std::vector<Flag> flags = {
        Flag({"--run", "-r"}, [this](){ this->run = true; }),
        Flag({"--force", "-f"}, [this](){ this->force = true; }),
        Flag({"--path", "-p"}, [this](char* s){ this->path = s; }),
        Flag({"--var", "-v"}, [this](char* name){
            char *value = strchr(name, '=');
            *value++ = 0;
            vars[name] = value;
        })
    };
};
