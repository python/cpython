from test.test_json import PyTest
import pickle
import sys
import unittest

kepler_dict = {
    "orbital_period": {
        "mercury": 88,
        "venus": 225,
        "earth": 365,
        "mars": 687,
        "jupiter": 4331,
        "saturn": 10_756,
        "uranus": 30_687,
        "neptune": 60_190,
    },
    "dist_from_sun": {
        "mercury": 58,
        "venus": 108,
        "earth": 150,
        "mars": 228,
        "jupiter": 778,
        "saturn": 1_400,
        "uranus": 2_900,
        "neptune": 4_500,
    }
}

class TestAttrDict(PyTest):

    def test_dict_subclass(self):
        self.assertTrue(issubclass(self.AttrDict, dict))

    def test_slots(self):
        d = self.AttrDict(x=1, y=2)
        with self.assertRaises(TypeError):
            vars(d)

    def test_constructor_signatures(self):
        AttrDict = self.AttrDict
        target = dict(x=1, y=2)
        self.assertEqual(AttrDict(x=1, y=2), target)                   # kwargs
        self.assertEqual(AttrDict(dict(x=1, y=2)), target)             # mapping
        self.assertEqual(AttrDict(dict(x=1, y=0), y=2), target)        # mapping, kwargs
        self.assertEqual(AttrDict([('x', 1), ('y', 2)]), target)       # iterable
        self.assertEqual(AttrDict([('x', 1), ('y', 0)], y=2), target)  # iterable, kwargs

    def test_getattr(self):
        d = self.AttrDict(x=1, y=2)
        self.assertEqual(d.x, 1)
        with self.assertRaises(AttributeError):
            d.z

    def test_setattr(self):
        d = self.AttrDict(x=1, y=2)
        d.x = 3
        d.z = 5
        self.assertEqual(d, dict(x=3, y=2, z=5))

    def test_delattr(self):
        d = self.AttrDict(x=1, y=2)
        del d.x
        self.assertEqual(d, dict(y=2))
        with self.assertRaises(AttributeError):
            del d.z

    def test_dir(self):
        d = self.AttrDict(x=1, y=2)
        self.assertTrue(set(dir(d)), set(dir(dict)).union({'x', 'y'}))

    def test_repr(self):
        # This repr is doesn't round-trip.  It matches a regular dict.
        # That seems to be the norm for AttrDict recipes being used
        # in the wild.  Also it supports the design concept that an
        # AttrDict is just like a regular dict but has optional
        # attribute style lookup.
        self.assertEqual(repr(self.AttrDict(x=1, y=2)),
                         repr(dict(x=1, y=2)))

    def test_overlapping_keys_and_methods(self):
        d = self.AttrDict(items=50)
        self.assertEqual(d['items'], 50)
        self.assertEqual(d.items(), dict(d).items())

    def test_invalid_attribute_names(self):
        d = self.AttrDict({
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
        AttrDict = self.AttrDict
        json_string = self.dumps(kepler_dict)
        kepler_ad = self.loads(json_string, object_hook=AttrDict)

        self.assertEqual(kepler_ad, kepler_dict)     # Match regular dict
        self.assertIsInstance(kepler_ad, AttrDict)   # Verify conversion
        self.assertIsInstance(kepler_ad.orbital_period, AttrDict)  # Nested

        # Exercise dotted lookups
        self.assertEqual(kepler_ad.orbital_period, kepler_dict['orbital_period'])
        self.assertEqual(kepler_ad.orbital_period.earth,
                         kepler_dict['orbital_period']['earth'])
        self.assertEqual(kepler_ad['orbital_period'].earth,
                         kepler_dict['orbital_period']['earth'])

        # Dict style error handling and Attribute style error handling
        with self.assertRaises(KeyError):
            kepler_ad.orbital_period['pluto']
        with self.assertRaises(AttributeError):
            kepler_ad.orbital_period.Pluto

        # Order preservation
        self.assertEqual(list(kepler_ad.items()), list(kepler_dict.items()))
        self.assertEqual(list(kepler_ad.orbital_period.items()),
                             list(kepler_dict['orbital_period'].items()))

        # Round trip
        self.assertEqual(self.dumps(kepler_ad), json_string)

    def test_pickle(self):
        AttrDict = self.AttrDict
        json_string = self.dumps(kepler_dict)
        kepler_ad = self.loads(json_string, object_hook=AttrDict)

        # Pickling requires the cached module to be the real module
        cached_module = sys.modules.get('json')
        sys.modules['json'] = self.json
        try:
            for protocol in range(6):
                kepler_ad2 = pickle.loads(pickle.dumps(kepler_ad, protocol))
                self.assertEqual(kepler_ad2, kepler_ad)
                self.assertEqual(type(kepler_ad2), AttrDict)
        finally:
            sys.modules['json'] = cached_module


if __name__ == "__main__":
    unittest.main()
