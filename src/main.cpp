#include <stdio.h>

#include "../json/single_include/nlohmann/json.hpp"

using json = nlohmann::json;

int main(void) {
    printf("Hello World!\n");

    json j = R"(
        {
            "a": 1,
            "b": "x",
            "c": [
                1,
                2,
                3
            ],
            "d": {
                "d1": "hello",
                "d2": "you"
            }
        }
    )"_json;

    printf("%s\n", j.dump(4).c_str());

    return 0;
}