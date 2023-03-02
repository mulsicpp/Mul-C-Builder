#pragma once

#include "../json/single_include/nlohmann/json.hpp"

template <class T>
static T jsonRequire(const nlohmann::json &data, std::string key)
{
    if (!data.contains(key))
        error("The object \'%s\' was not found in the JSON object, but is required", key.c_str());
    return data[key].get<T>();
}

template <class T>
static void jsonOn(const nlohmann::json &data, std::string key, std::function<void(const T)> function)
{
    if (!data.contains(key))
        error("The object \'%s\' was not found in the JSON object, but is required", key.c_str());
    function(data[key].get<T>());
}

template <class T>
static void jsonOnEach(const nlohmann::json &data, std::string key, std::function<void(const T)> function)
{
    if (!data.contains(key))
        error("The object \'%s\' was not found in the JSON object, but is required", key.c_str());
    for(const nlohmann::json el : data[key])
        function(el.get<T>());
}

template <class T>
static void jsonTryOn(const nlohmann::json &data, std::string key, std::function<void(const T)> function)
{
    if (!data.contains(key))
        return;
    function(data[key].get<T>());
}

template <class T>
static void jsonTryOnEach(const nlohmann::json &data, std::string key, std::function<void(const T)> function)
{
    if (!data.contains(key))
        return;
    for(const nlohmann::json el : data[key])
        function(el.get<T>());
}