"""Fix incompatible imports and module references."""
# Author: Collin Winter

# Local imports
from .. import fixer_base
from ..fixer_util import Name, attr_chain, any, set

MAPPING = {'StringIO':  'io',
           'cStringIO': 'io',
           'cPickle': 'pickle',
           '__builtin__' : 'builtins',
           'copy_reg': 'copyreg',
           'Queue': 'queue',
           'SocketServer': 'socketserver',
           'ConfigParser': 'configparser',
           'repr': 'reprlib',
           'FileDialog': 'tkinter.filedialog',
           'tkFileDialog': 'tkinter.filedialog',
           'SimpleDialog': 'tkinter.simpledialog',
           'tkSimpleDialog': 'tkinter.simpledialog',
           'tkColorChooser': 'tkinter.colorchooser',
           'tkCommonDialog': 'tkinter.commondialog',
           'Dialog': 'tkinter.dialog',
           'Tkdnd': 'tkinter.dnd',
           'tkFont': 'tkinter.font',
           'tkMessageBox': 'tkinter.messagebox',
           'ScrolledText': 'tkinter.scrolledtext',
           'turtle': 'tkinter.turtle',
           'Tkconstants': 'tkinter.constants',
           'Tix': 'tkinter.tix',
           'Tkinter': 'tkinter',
           'markupbase': '_markupbase',
           '_winreg': 'winreg',
           'thread': '_thread',
           'dummy_thread': '_dummy_thread',
           # anydbm and whichdb are handled by fix_imports2
           'dbhash': 'dbm.bsd',
           'dumbdbm': 'dbm.dumb',
           'dbm': 'dbm.ndbm',
           'gdbm': 'dbm.gnu',
           'xmlrpclib': 'xmlrpc.client',
           'DocXMLRPCServer': 'xmlrpc.server',
           'SimpleXMLRPCServer': 'xmlrpc.server',
           'httplib': 'http.client',
           'Cookie': 'http.cookies',
           'cookielib': 'http.cookiejar',
           'BaseHTTPServer': 'http.server',
           'SimpleHTTPServer': 'http.server',
           'CGIHTTPServer': 'http.server',
           #'test.test_support': 'test.support',
           'commands': 'subprocess',
           'UserString' : 'collections',
           'UserList' : 'collections',
           'urlparse' : 'urllib.parse',
           'robotparser' : 'urllib.robotparser',
}


def alternates(members):
    return "(" + "|".join(map(repr, members)) + ")"


def build_pattern(mapping=MAPPING):
    mod_list = ' | '.join(["module='" + key + "'" for key in mapping.keys()])
    mod_name_list = ' | '.join(["module_name='" + key + "'" for key in mapping.keys()])
    yield """import_name< 'import' ((%s)
                          | dotted_as_names< any* (%s) any* >) >
          """ % (mod_list, mod_list)
    yield """import_from< 'from' (%s) 'import'
              ( any | import_as_name< any 'as' any > |
                import_as_names< any* >) >
          """ % mod_name_list
    yield """import_name< 'import'
                          dotted_as_name< (%s) 'as' any > >
          """ % mod_name_list
    # Find usages of module members in code e.g. urllib.foo(bar)
    yield """power< (%s)
             trailer<'.' any > any* >
          """ % mod_name_list
    yield """bare_name=%s""" % alternates(mapping.keys())

class FixImports(fixer_base.BaseFix):
    PATTERN = "|".join(build_pattern())
    order = "pre" # Pre-order tree traversal

    mapping = MAPPING

    # Don't match the node if it's within another match
    def match(self, node):
        match = super(FixImports, self).match
        results = match(node)
        if results:
            if any([match(obj) for obj in attr_chain(node, "parent")]):
                return False
            return results
        return False

    def start_tree(self, tree, filename):
        super(FixImports, self).start_tree(tree, filename)
        self.replace = {}

    def transform(self, node, results):
        import_mod = results.get("module")
        mod_name = results.get("module_name")
        bare_name = results.get("bare_name")

        if import_mod or mod_name:
            new_name = self.mapping[(import_mod or mod_name).value]

        if import_mod:
            self.replace[import_mod.value] = new_name
            import_mod.replace(Name(new_name, prefix=import_mod.get_prefix()))
        elif mod_name:
            mod_name.replace(Name(new_name, prefix=mod_name.get_prefix()))
        elif bare_name:
            bare_name = bare_name[0]
            new_name = self.replace.get(bare_name.value)
            if new_name:
                bare_name.replace(Name(new_name, prefix=bare_name.get_prefix()))
