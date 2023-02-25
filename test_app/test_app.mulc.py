from mulC import *

config = {
    "compile": {
        "os": platform
    },
    "link": {
        "config": config
    },
    "export": {
        "arch": arch,
        "name": var("name"),
        "x": 1
    }
}

output(config)