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
        "x": var("x")
    }
}

output(config)