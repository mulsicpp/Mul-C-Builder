import json, sys

mode = {}
[mode['os'], mode['arch'], mode['config']] = sys.argv[2].split('/')


_vars = { '_os': mode['os'], '_arch': mode['arch'], '_config': mode['config'] }

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
    return name + '.exe' if mode['os'] == 'windows' else name

def lib(name):
    return name + '.lib' if mode['os'] == 'windows' else 'lib' + name + '.a'

def dll(name):
    return name + '.dll' if mode['os'] == 'windows' else 'lib' + name + '.so'