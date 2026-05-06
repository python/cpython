"Test zzdummy with user config, coverage 100%."

from idlelib import zzdummy
import unittest
from test.support import requires
from tkinter import Tk, Text
from idlelib import config

from idlelib.idle_test.test_zzdummy import (
    ZZDummyMixin, DummyEditwin, code_sample,
)


real_usercfg = zzdummy.idleConf.userCfg
test_usercfg = {
    'main': config.IdleUserConfParser(''),
    'highlight': config.IdleUserConfParser(''),
    'keys': config.IdleUserConfParser(''),
    'extensions': config.IdleUserConfParser(''),
}
test_usercfg["extensions"].read_dict({
    "ZzDummy": {'enable': 'True', 'enable_shell': 'False',
                'enable_editor': 'True', 'z-text': 'Z'},
    "ZzDummy_cfgBindings": {
        'z-in': '<Control-Shift-KeyRelease-Insert>'},
    "ZzDummy_bindings": {
        'z-out': '<Control-Shift-KeyRelease-Delete>'},
})
real_defaultcfg = zzdummy.idleConf.defaultCfg
test_defaultcfg = {
    'main': config.IdleUserConfParser(''),
    'highlight': config.IdleUserConfParser(''),
    'keys': config.IdleUserConfParser(''),
    'extensions': config.IdleUserConfParser(''),
}
test_defaultcfg["extensions"].read_dict({
    "AutoComplete": {'popupwait': '2000'},
    "CodeContext": {'maxlines': '15'},
    "FormatParagraph": {'max-width': '72'},
    "ParenMatch": {'style': 'expression',
                   'flash-delay': '500', 'bell': 'True'},
})
test_defaultcfg["main"].read_dict({
    "Theme": {"default": 1, "name": "IDLE Classic", "name2": ""},
    "Keys": {"default": 1, "name": "IDLE Classic", "name2": ""},
})
for key in ("keys",):
    real_default = real_defaultcfg[key]
    value = {name: dict(real_default[name]) for name in real_default}
    test_defaultcfg[key].read_dict(value)


class ZZDummyTest(ZZDummyMixin, unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        requires('gui')
        root = cls.root = Tk()
        root.withdraw()
        text = cls.text = Text(cls.root)
        cls.editor = DummyEditwin(root, text)
        zzdummy.idleConf.userCfg = test_usercfg
        zzdummy.idleConf.defaultCfg = test_defaultcfg

    @classmethod
    def tearDownClass(cls):
        zzdummy.idleConf.defaultCfg = real_defaultcfg
        zzdummy.idleConf.userCfg = real_usercfg
        del cls.editor, cls.text
        cls.root.update_idletasks()
        for id in cls.root.tk.call('after', 'info'):
            cls.root.after_cancel(id)  # Need for EditorWindow.
        cls.root.destroy()
        del cls.root

    def setUp(self):
        text = self.text
        text.insert('1.0', code_sample)
        text.undo_block_start.reset_mock()
        text.undo_block_stop.reset_mock()
        zz = self.zz = zzdummy.ZzDummy(self.editor)
        zzdummy.ZzDummy.ztext = '# ignore #'

    def tearDown(self):
        self.text.delete('1.0', 'end')
        del self.zz

    def test_exists(self):
        self.assertEqual(
            zzdummy.idleConf.GetSectionList('user', 'extensions'),
            ['ZzDummy', 'ZzDummy_cfgBindings', 'ZzDummy_bindings'])
        self.assertEqual(
            zzdummy.idleConf.GetSectionList('default', 'extensions'),
            ['AutoComplete', 'CodeContext', 'FormatParagraph',
             'ParenMatch'])
        self.assertIn("ZzDummy",
                       zzdummy.idleConf.GetExtensions())
        self.assertEqual(
            zzdummy.idleConf.GetExtensionKeys("ZzDummy"),
            {'<<z-in>>': ['<Control-Shift-KeyRelease-Insert>']})
        self.assertEqual(
            zzdummy.idleConf.GetExtensionBindings("ZzDummy"),
            {'<<z-in>>': ['<Control-Shift-KeyRelease-Insert>'],
             '<<z-out>>': ['<Control-Shift-KeyRelease-Delete>']})


if __name__ == '__main__':
    unittest.main(verbosity=2)
