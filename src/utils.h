#pragma once

#define F_RESET "\033[0m"

#define F_BLACK "\033[0;30m"
#define F_RED "\033[0;31m"
#define F_GREEN "\033[0;32m"
#define F_YELLOW "\033[0;33m"
#define F_BLUE "\033[0;34m"
#define F_PURPLE "\033[0;35m"
#define F_CYAN "\033[0;36m"
#define F_WHITE "\033[0;37m"
#define F_BOLD "\033[1m"

#include <stdio.h>
#include <stdlib.h>

template <class... Ts>
void error(const char *str, Ts... args)
{
    printf(F_RED "ERROR: ");
    printf(str, args...);
    printf("\n" F_RESET);
    exit(-1);
}

template <class... Ts>
void syntaxError(const char *str, Ts... args)
{
    printf(F_RED "SYNTAX ERROR: ");
    printf(str, args...);
    printf("\n" F_RESET);
    exit(-1);
}

template <class... Ts>
void warning(const char *str, Ts... args)
{
    printf(F_YELLOW "WARNING: ");
    printf(str, args...);
    printf("\n" F_RESET);
}