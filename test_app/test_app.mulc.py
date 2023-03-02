from mulC import *

config = {
    "mode": mode,
    "compile": {
        "sources": [
            "src1",
            "src2"
        ],
        "sourceBlackList": [
            "src1"
        ]
    }
}

output(config)