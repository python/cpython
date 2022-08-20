import unittest
import json
from json import AttrDict

kepler_dict = {
    "orbital_period": {
        "Mercury": 88,
        "Venus": 225,
        "Earth": 365,
        "Mars": 687,
        "Jupiter": 4331,
        "Saturn": 10_756,
        "Uranus": 30_687,
        "Neptune": 60_190
    },
    "dist_from_sun": {
        "Mercury": 58,
        "Venus": 108,
        "Earth": 150,
        "Mars": 228,
        "Jupiter": 778,
        "Saturn": 1_400,
        "Uranus": 2_900,
        "Neptune": 4_500
    }
}

class TestAttrDict(unittest.TestCase):

    def test_dict_subclass(self):
        self.assertTrue(issubclass(AttrDict, dict))

    def test_getattr(self):
        d = AttrDict(x=1, y=2)
        self.assertEqual(d.x, 1)
        with self.assertRaises(AttributeError):
            d.z

    def test_setattr(self):
        d = AttrDict(x=1, y=2)
        d.x = 3
        d.z = 5
        self.assertEqual(d, dict(x=3, y=2, z=5))

    def test_delattr(self):
        d = AttrDict(x=1, y=2)
        del d.x
        self.assertEqual(d, dict(y=2))
        with self.assertRaises(AttributeError):
            del d.z

    def test_dir(self):
        d = AttrDict(x=1, y=2)
        self.assertTrue(set(dir(d)), set(dir(dict)).union({'x', 'y'}))

    def test_repr(self):
        # This repr is doesn't round-trip.  It matches a regular dict.
        # That seems to be the norm for AttrDict recipes being used
        # in the wild.  Also it supports the design concept that an
        # AttrDict is just like a regular dict but has optional
        # attribute style lookup.
        self.assertEqual(repr(AttrDict(x=1, y=2)),
                         repr(dict(x=1, y=2)))

    def test_overlapping_keys_and_methods(self):
        d = AttrDict(items=50)
        self.assertEqual(d['items'], 50)
        self.assertEqual(d.items(), dict(d).items())

    def test_invalid_attribute_names(self):
        d = AttrDict({
            'control': 'normal case',
            'class': 'keyword',
            'two words': 'contains space',
            'hypen-ate': 'contains a hyphen'
        })
        self.assertEqual(d.control, dict(d)['control'])
        self.assertEqual(d['class'], dict(d)['class'])
        self.assertEqual(d['two words'], dict(d)['two words'])
        self.assertEqual(d['hypen-ate'], dict(d)['hypen-ate'])

    def test_object_hook_use_case(self):
        json_string = json.dumps(kepler_dict)
        kepler_ad = json.loads(json_string, object_hook=AttrDict)

        self.assertEqual(kepler_ad, kepler_dict)     # Match regular dict
        self.assertIsInstance(kepler_ad, AttrDict)   # Verify conversion
        self.assertIsInstance(kepler_ad.orbital_period, AttrDict)  # Nested

        # Exercise dotted lookups
        self.assertEqual(kepler_ad.orbital_period, kepler_dict['orbital_period'])
        self.assertEqual(kepler_ad.orbital_period.Earth,
                         kepler_dict['orbital_period']['Earth'])
        self.assertEqual(kepler_ad['orbital_period'].Earth,
                         kepler_dict['orbital_period']['Earth'])

        # Dict style error handling and Attribute style error handling
        with self.assertRaises(KeyError):
            kepler_ad.orbital_period['Pluto']
        with self.assertRaises(AttributeError):
            kepler_ad.orbital_period.Pluto

        # Order preservation
        self.assertEqual(list(kepler_ad.items()), list(kepler_dict.items()))
        self.assertEqual(list(kepler_ad.orbital_period.items()),
                             list(kepler_dict['orbital_period'].items()))


if __name__ == "__main__":
    unittest.main()
