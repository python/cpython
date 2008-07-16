"""Fix changes imports of urllib which are now incompatible.
   This is rather similar to fix_imports, but because of the more
   complex nature of the fixing for urllib, it has its own fixer.
"""
# Author: Nick Edds

# Local imports
from .fix_imports import alternates, FixImports
from .. import fixer_base
from ..fixer_util import Name, Comma, FromImport, Newline, attr_chain, any, set

MAPPING = {'urllib':  [
                ('urllib.request',
                    ['URLOpener', 'FancyURLOpener', 'urlretrieve',
                     '_urlopener', 'urlcleanup']),
                ('urllib.parse',
                    ['quote', 'quote_plus', 'unquote', 'unquote_plus',
                     'urlencode', 'pahtname2url', 'url2pathname']),
                ('urllib.error',
                    ['ContentTooShortError'])],
           'urllib2' : [
                ('urllib.request',
                    ['urlopen', 'install_opener', 'build_opener',
                     'Request', 'OpenerDirector', 'BaseHandler',
                     'HTTPDefaultErrorHandler', 'HTTPRedirectHandler',
                     'HTTPCookieProcessor', 'ProxyHandler',
                     'HTTPPasswordMgr',
                     'HTTPPasswordMgrWithDefaultRealm',
                     'AbstractBasicAuthHandler',
                     'HTTPBasicAuthHandler', 'ProxyBasicAuthHandler',
                     'AbstractDigestAuthHandler',
                     'HTTPDigestAuthHander', 'ProxyDigestAuthHandler',
                     'HTTPHandler', 'HTTPSHandler', 'FileHandler',
                     'FTPHandler', 'CacheFTPHandler',
                     'UnknownHandler']),
                ('urllib.error',
                    ['URLError', 'HTTPError'])],
}


# def alternates(members):
#     return "(" + "|".join(map(repr, members)) + ")"


def build_pattern():
    bare = set()
    for old_module, changes in MAPPING.items():
        for change in changes:
            new_module, members = change
            members = alternates(members)
            yield """import_name< 'import' (module=%r
                                  | dotted_as_names< any* module=%r any* >) >
                  """ % (old_module, old_module)
            yield """import_from< 'from' mod_member=%r 'import'
                       ( member=%s | import_as_name< member=%s 'as' any > |
                         import_as_names< members=any*  >) >
                  """ % (old_module, members, members)
            yield """import_from< 'from' module_star=%r 'import' star='*' >
                  """ % old_module
            yield """import_name< 'import'
                                  dotted_as_name< module_as=%r 'as' any > >
                  """ % old_module
            yield """power< module_dot=%r trailer< '.' member=%s > any* >
                  """ % (old_module, members)


class FixUrllib(FixImports):
    PATTERN = "|".join(build_pattern())

    def transform_import(self, node, results):
        """Transform for the basic import case. Replaces the old
           import name with a comma separated list of its
           replacements.
        """
        import_mod = results.get('module')
        pref = import_mod.get_prefix()

        names = []

        # create a Node list of the replacement modules
        for name in MAPPING[import_mod.value][:-1]:
            names.extend([Name(name[0], prefix=pref), Comma()])
        names.append(Name(MAPPING[import_mod.value][-1][0], prefix=pref))
        import_mod.replace(names)

    def transform_member(self, node, results):
        """Transform for imports of specific module elements. Replaces
           the module to be imported from with the appropriate new
           module.
        """
        mod_member = results.get('mod_member')
        pref = mod_member.get_prefix()
        member = results.get('member')

        # Simple case with only a single member being imported
        if member:
            # this may be a list of length one, or just a node
            if isinstance(member, list):
                member = member[0]
            new_name = None
            for change in MAPPING[mod_member.value]:
                if member.value in change[1]:
                    new_name = change[0]
                    break
            if new_name:
                mod_member.replace(Name(new_name, prefix=pref))
            else:
                self.cannot_convert(node,
                                    'This is an invalid module element')

        # Multiple members being imported
        else:
            # a dictionary for replacements, order matters
            modules = []
            mod_dict = {}
            members = results.get('members')
            for member in members:
                member = member.value
                # we only care about the actual members
                if member != ',':
                    for change in MAPPING[mod_member.value]:
                        if member in change[1]:
                            if change[0] in mod_dict:
                                mod_dict[change[0]].append(member)
                            else:
                                mod_dict[change[0]] = [member]
                                modules.append(change[0])

            new_nodes = []
            for module in modules:
                elts = mod_dict[module]
                names = []
                for elt in elts[:-1]:
                    names.extend([Name(elt, prefix=pref), Comma()])
                names.append(Name(elts[-1], prefix=pref))
                new_nodes.append(FromImport(module, names))
            if new_nodes:
                nodes = []
                for new_node in new_nodes[:-1]:
                    nodes.extend([new_node, Newline()])
                nodes.append(new_nodes[-1])
                node.replace(nodes)
            else:
                self.cannot_convert(node, 'All module elements are invalid')

    def transform_dot(self, node, results):
        """Transform for calls to module members in code."""
        module_dot = results.get('module_dot')
        member = results.get('member')
        # this may be a list of length one, or just a node
        if isinstance(member, list):
            member = member[0]
        new_name = None
        for change in MAPPING[module_dot.value]:
            if member.value in change[1]:
                new_name = change[0]
                break
        if new_name:
            module_dot.replace(Name(new_name,
                                    prefix=module_dot.get_prefix()))
        else:
            self.cannot_convert(node, 'This is an invalid module element')

    def transform(self, node, results):
        if results.get('module'):
            self.transform_import(node, results)
        elif results.get('mod_member'):
            self.transform_member(node, results)
        elif results.get('module_dot'):
            self.transform_dot(node, results)
        # Renaming and star imports are not supported for these modules.
        elif results.get('module_star'):
            self.cannot_convert(node, 'Cannot handle star imports.')
        elif results.get('module_as'):
            self.cannot_convert(node, 'This module is now multiple modules')
