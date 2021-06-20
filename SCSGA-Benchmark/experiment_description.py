import json
import copy


class ExperimentDescription:
    def _parse_input(path):
        with open(path, 'r') as fp:
            data = json.load(fp)
        return data

    def __init__(self, path):
        self.desc = ExperimentDescription._parse_input(path)
        self.read_paths = set()

    def get_keys(self, path):
        keys = []
        for key in path.split('/'):
            # Heltal antas vara index i en lista, annars
            # en nyckel i en dict.
            try:
                keys.append(int(key))
            except ValueError:
                keys.append(key)
        return keys

    def get(self, path, default_value=None):
        keys = self.get_keys(path)
        e = self.desc
        for key in keys[:-1]:
            e = e[key]

        self.read_paths.add(path)

        key = keys[-1]

        if key in e:
            value = e[key]
        else:
            if default_value is None:
                print(e)
                raise Exception('Could not find path {} and no default set'.format(path))
            else:
                e[key] = default_value
                value = default_value

        return value

    def set(self, path, value):
        if path in self.read_paths:
            raise Exception('Tried to set value to path {} when it had already been read once'.format(path))

        keys = self.get_keys(path)

        e = self.desc

        for key in keys[:-1]:
            if type(key) != int and key not in e:
                e[key] = {}

            e = e[key]

        key = keys[-1]

        e[key] = value

    def __contains__(self, path):
        keys = self.get_keys(path)
        e = self.desc
        for key in keys:
            if key in e:
                e = e[key]
            else:
                return False
        return True

    def save(self, name):
        with open(name + '.json', 'w') as fp:
            json.dump(self.desc, fp, indent=2, sort_keys=True)

    def copy(self):
        return copy.deepcopy(self)
