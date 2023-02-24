import json, sys


[platform, arch, config] = sys.argv[2].split('/')


_vars = {}

for var_str in sys.argv[3].split(';'):
    if(var_str != ''):
        var = var_str.split('=')
        _vars[var[0]] = var[1]

def output(config):
    json_object = json.dumps(config, indent = 4)
    file = open(sys.argv[1], 'w')
    file.write(json_object)
    file.close()

def var(name):
    try:
        return _vars[name]
    except KeyError:
        return ''

def app(name):
    return name if platform == 'linux' else name + '.exe'