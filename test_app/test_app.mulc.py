from mulC import *

config = {
    "mode": mode,
    "compile": {
        "sources": [
            "src",
            "src_bla"
        ],
        "sourceBlackList": [
            "src/src_nested"
        ]
    },
    "output": {
        "type": "app",
        "path": F"out/{app('test')}"
    }
}

output(config)