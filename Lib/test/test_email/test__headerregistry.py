import datetime
import textwrap
import unittest
from email import errors
from email import policy
from test.test_email import TestEmailBase
from email import _headerregistry
# Address and Group are public but I'm not sure where to put them yet.
from email._headerregistry import Address, Group


class TestHeaderRegistry(TestEmailBase):

    def test_arbitrary_name_unstructured(self):
        factory = _headerregistry.HeaderRegistry()
        h = factory('foobar', 'test')
        self.assertIsInstance(h, _headerregistry.BaseHeader)
        self.assertIsInstance(h, _headerregistry.UnstructuredHeader)

    def test_name_case_ignored(self):
        factory = _headerregistry.HeaderRegistry()
        # Whitebox check that test is valid
        self.assertNotIn('Subject', factory.registry)
        h = factory('Subject', 'test')
        self.assertIsInstance(h, _headerregistry.BaseHeader)
        self.assertIsInstance(h, _headerregistry.UniqueUnstructuredHeader)

    class FooBase:
        def __init__(self, *args, **kw):
            pass

    def test_override_default_base_class(self):
        factory = _headerregistry.HeaderRegistry(base_class=self.FooBase)
        h = factory('foobar', 'test')
        self.assertIsInstance(h, self.FooBase)
        self.assertIsInstance(h, _headerregistry.UnstructuredHeader)

    class FooDefault:
        parse = _headerregistry.UnstructuredHeader.parse

    def test_override_default_class(self):
        factory = _headerregistry.HeaderRegistry(default_class=self.FooDefault)
        h = factory('foobar', 'test')
        self.assertIsInstance(h, _headerregistry.BaseHeader)
        self.assertIsInstance(h, self.FooDefault)

    def test_override_default_class_only_overrides_default(self):
        factory = _headerregistry.HeaderRegistry(default_class=self.FooDefault)
        h = factory('subject', 'test')
        self.assertIsInstance(h, _headerregistry.BaseHeader)
        self.assertIsInstance(h, _headerregistry.UniqueUnstructuredHeader)

    def test_dont_use_default_map(self):
        factory = _headerregistry.HeaderRegistry(use_default_map=False)
        h = factory('subject', 'test')
        self.assertIsInstance(h, _headerregistry.BaseHeader)
        self.assertIsInstance(h, _headerregistry.UnstructuredHeader)

    def test_map_to_type(self):
        factory = _headerregistry.HeaderRegistry()
        h1 = factory('foobar', 'test')
        factory.map_to_type('foobar', _headerregistry.UniqueUnstructuredHeader)
        h2 = factory('foobar', 'test')
        self.assertIsInstance(h1, _headerregistry.BaseHeader)
        self.assertIsInstance(h1, _headerregistry.UnstructuredHeader)
        self.assertIsInstance(h2, _headerregistry.BaseHeader)
        self.assertIsInstance(h2, _headerregistry.UniqueUnstructuredHeader)


class TestHeaderBase(TestEmailBase):

    factory = _headerregistry.HeaderRegistry()

    def make_header(self, name, value):
        return self.factory(name, value)


class TestBaseHeaderFeatures(TestHeaderBase):

    def test_str(self):
        h = self.make_header('subject', 'this is a test')
        self.assertIsInstance(h, str)
        self.assertEqual(h, 'this is a test')
        self.assertEqual(str(h), 'this is a test')

    def test_substr(self):
        h = self.make_header('subject', 'this is a test')
        self.assertEqual(h[5:7], 'is')

    def test_has_name(self):
        h = self.make_header('subject', 'this is a test')
        self.assertEqual(h.name, 'subject')

    def _test_attr_ro(self, attr):
        h = self.make_header('subject', 'this is a test')
        with self.assertRaises(AttributeError):
            setattr(h, attr, 'foo')

    def test_name_read_only(self):
        self._test_attr_ro('name')

    def test_defects_read_only(self):
        self._test_attr_ro('defects')

    def test_defects_is_tuple(self):
        h = self.make_header('subject', 'this is a test')
        self.assertEqual(len(h.defects), 0)
        self.assertIsInstance(h.defects, tuple)
        # Make sure it is still true when there are defects.
        h = self.make_header('date', '')
        self.assertEqual(len(h.defects), 1)
        self.assertIsInstance(h.defects, tuple)

    # XXX: FIXME
    #def test_CR_in_value(self):
    #    # XXX: this also re-raises the issue of embedded headers,
    #    # need test and solution for that.
    #    value = '\r'.join(['this is', ' a test'])
    #    h = self.make_header('subject', value)
    #    self.assertEqual(h, value)
    #    self.assertDefectsEqual(h.defects, [errors.ObsoleteHeaderDefect])

    def test_RFC2047_value_decoded(self):
        value = '=?utf-8?q?this_is_a_test?='
        h = self.make_header('subject', value)
        self.assertEqual(h, 'this is a test')


class TestDateHeader(TestHeaderBase):

    datestring = 'Sun, 23 Sep 2001 20:10:55 -0700'
    utcoffset = datetime.timedelta(hours=-7)
    tz = datetime.timezone(utcoffset)
    dt = datetime.datetime(2001, 9, 23, 20, 10, 55, tzinfo=tz)

    def test_parse_date(self):
        h = self.make_header('date', self.datestring)
        self.assertEqual(h, self.datestring)
        self.assertEqual(h.datetime, self.dt)
        self.assertEqual(h.datetime.utcoffset(), self.utcoffset)
        self.assertEqual(h.defects, ())

    def test_set_from_datetime(self):
        h = self.make_header('date', self.dt)
        self.assertEqual(h, self.datestring)
        self.assertEqual(h.datetime, self.dt)
        self.assertEqual(h.defects, ())

    def test_date_header_properties(self):
        h = self.make_header('date', self.datestring)
        self.assertIsInstance(h, _headerregistry.UniqueDateHeader)
        self.assertEqual(h.max_count, 1)
        self.assertEqual(h.defects, ())

    def test_resent_date_header_properties(self):
        h = self.make_header('resent-date', self.datestring)
        self.assertIsInstance(h, _headerregistry.DateHeader)
        self.assertEqual(h.max_count, None)
        self.assertEqual(h.defects, ())

    def test_no_value_is_defect(self):
        h = self.make_header('date', '')
        self.assertEqual(len(h.defects), 1)
        self.assertIsInstance(h.defects[0], errors.HeaderMissingRequiredValue)

    def test_datetime_read_only(self):
        h = self.make_header('date', self.datestring)
        with self.assertRaises(AttributeError):
            h.datetime = 'foo'


class TestAddressHeader(TestHeaderBase):

    examples = {

        'empty':
            ('<>',
             [errors.InvalidHeaderDefect],
             '<>',
             '',
             '<>',
             '',
             '',
             None),

        'address_only':
            ('zippy@pinhead.com',
             [],
             'zippy@pinhead.com',
             '',
             'zippy@pinhead.com',
             'zippy',
             'pinhead.com',
             None),

        'name_and_address':
            ('Zaphrod Beblebrux <zippy@pinhead.com>',
             [],
             'Zaphrod Beblebrux <zippy@pinhead.com>',
             'Zaphrod Beblebrux',
             'zippy@pinhead.com',
             'zippy',
             'pinhead.com',
             None),

        'quoted_local_part':
            ('Zaphrod Beblebrux <"foo bar"@pinhead.com>',
             [],
             'Zaphrod Beblebrux <"foo bar"@pinhead.com>',
             'Zaphrod Beblebrux',
             '"foo bar"@pinhead.com',
             'foo bar',
             'pinhead.com',
             None),

        'quoted_parens_in_name':
            (r'"A \(Special\) Person" <person@dom.ain>',
             [],
             '"A (Special) Person" <person@dom.ain>',
             'A (Special) Person',
             'person@dom.ain',
             'person',
             'dom.ain',
             None),

        'quoted_backslashes_in_name':
            (r'"Arthur \\Backslash\\ Foobar" <person@dom.ain>',
             [],
             r'"Arthur \\Backslash\\ Foobar" <person@dom.ain>',
             r'Arthur \Backslash\ Foobar',
             'person@dom.ain',
             'person',
             'dom.ain',
             None),

        'name_with_dot':
            ('John X. Doe <jxd@example.com>',
             [errors.ObsoleteHeaderDefect],
             '"John X. Doe" <jxd@example.com>',
             'John X. Doe',
             'jxd@example.com',
             'jxd',
             'example.com',
             None),

        'quoted_strings_in_local_part':
            ('""example" example"@example.com',
             [errors.InvalidHeaderDefect]*3,
             '"example example"@example.com',
             '',
             '"example example"@example.com',
             'example example',
             'example.com',
             None),

        'escaped_quoted_strings_in_local_part':
            (r'"\"example\" example"@example.com',
             [],
             r'"\"example\" example"@example.com',
             '',
             r'"\"example\" example"@example.com',
             r'"example" example',
             'example.com',
            None),

        'escaped_escapes_in_local_part':
            (r'"\\"example\\" example"@example.com',
             [errors.InvalidHeaderDefect]*5,
             r'"\\example\\\\ example"@example.com',
             '',
             r'"\\example\\\\ example"@example.com',
             r'\example\\ example',
             'example.com',
            None),

        'spaces_in_unquoted_local_part_collapsed':
            ('merwok  wok  @example.com',
             [errors.InvalidHeaderDefect]*2,
             '"merwok wok"@example.com',
             '',
             '"merwok wok"@example.com',
             'merwok wok',
             'example.com',
             None),

        'spaces_around_dots_in_local_part_removed':
            ('merwok. wok .  wok@example.com',
             [errors.ObsoleteHeaderDefect],
             'merwok.wok.wok@example.com',
             '',
             'merwok.wok.wok@example.com',
             'merwok.wok.wok',
             'example.com',
             None),

        }

        # XXX: Need many more examples, and in particular some with names in
        # trailing comments, which aren't currently handled.  comments in
        # general are not handled yet.

    def _test_single_addr(self, source, defects, decoded, display_name,
                          addr_spec, username, domain, comment):
        h = self.make_header('sender', source)
        self.assertEqual(h, decoded)
        self.assertDefectsEqual(h.defects, defects)
        a = h.address
        self.assertEqual(str(a), decoded)
        self.assertEqual(len(h.groups), 1)
        self.assertEqual([a], list(h.groups[0].addresses))
        self.assertEqual([a], list(h.addresses))
        self.assertEqual(a.display_name, display_name)
        self.assertEqual(a.addr_spec, addr_spec)
        self.assertEqual(a.username, username)
        self.assertEqual(a.domain, domain)
        # XXX: we have no comment support yet.
        #self.assertEqual(a.comment, comment)

    for name in examples:
        locals()['test_'+name] = (
            lambda self, name=name:
                self._test_single_addr(*self.examples[name]))

    def _test_group_single_addr(self, source, defects, decoded, display_name,
                                addr_spec, username, domain, comment):
        source = 'foo: {};'.format(source)
        gdecoded = 'foo: {};'.format(decoded) if decoded else 'foo:;'
        h = self.make_header('to', source)
        self.assertEqual(h, gdecoded)
        self.assertDefectsEqual(h.defects, defects)
        self.assertEqual(h.groups[0].addresses, h.addresses)
        self.assertEqual(len(h.groups), 1)
        self.assertEqual(len(h.addresses), 1)
        a = h.addresses[0]
        self.assertEqual(str(a), decoded)
        self.assertEqual(a.display_name, display_name)
        self.assertEqual(a.addr_spec, addr_spec)
        self.assertEqual(a.username, username)
        self.assertEqual(a.domain, domain)

    for name in examples:
        locals()['test_group_'+name] = (
            lambda self, name=name:
                self._test_group_single_addr(*self.examples[name]))

    def test_simple_address_list(self):
        value = ('Fred <dinsdale@python.org>, foo@example.com, '
                    '"Harry W. Hastings" <hasty@example.com>')
        h = self.make_header('to', value)
        self.assertEqual(h, value)
        self.assertEqual(len(h.groups), 3)
        self.assertEqual(len(h.addresses), 3)
        for i in range(3):
            self.assertEqual(h.groups[i].addresses[0], h.addresses[i])
        self.assertEqual(str(h.addresses[0]), 'Fred <dinsdale@python.org>')
        self.assertEqual(str(h.addresses[1]), 'foo@example.com')
        self.assertEqual(str(h.addresses[2]),
            '"Harry W. Hastings" <hasty@example.com>')
        self.assertEqual(h.addresses[2].display_name,
            'Harry W. Hastings')

    def test_complex_address_list(self):
        examples = list(self.examples.values())
        source = ('dummy list:;, another: (empty);,' +
                 ', '.join([x[0] for x in examples[:4]]) + ', ' +
                 r'"A \"list\"": ' +
                    ', '.join([x[0] for x in examples[4:6]]) + ';,' +
                 ', '.join([x[0] for x in examples[6:]])
            )
        # XXX: the fact that (empty) disappears here is a potential API design
        # bug.  We don't currently have a way to preserve comments.
        expected = ('dummy list:;, another:;, ' +
                 ', '.join([x[2] for x in examples[:4]]) + ', ' +
                 r'"A \"list\"": ' +
                    ', '.join([x[2] for x in examples[4:6]]) + ';, ' +
                 ', '.join([x[2] for x in examples[6:]])
            )

        h = self.make_header('to', source)
        self.assertEqual(h.split(','), expected.split(','))
        self.assertEqual(h, expected)
        self.assertEqual(len(h.groups), 7 + len(examples) - 6)
        self.assertEqual(h.groups[0].display_name, 'dummy list')
        self.assertEqual(h.groups[1].display_name, 'another')
        self.assertEqual(h.groups[6].display_name, 'A "list"')
        self.assertEqual(len(h.addresses), len(examples))
        for i in range(4):
            self.assertIsNone(h.groups[i+2].display_name)
            self.assertEqual(str(h.groups[i+2].addresses[0]), examples[i][2])
        for i in range(7, 7 + len(examples) - 6):
            self.assertIsNone(h.groups[i].display_name)
            self.assertEqual(str(h.groups[i].addresses[0]), examples[i-1][2])
        for i in range(len(examples)):
            self.assertEqual(str(h.addresses[i]), examples[i][2])
            self.assertEqual(h.addresses[i].addr_spec, examples[i][4])

    def test_address_read_only(self):
        h = self.make_header('sender', 'abc@xyz.com')
        with self.assertRaises(AttributeError):
            h.address = 'foo'

    def test_addresses_read_only(self):
        h = self.make_header('sender', 'abc@xyz.com')
        with self.assertRaises(AttributeError):
            h.addresses = 'foo'

    def test_groups_read_only(self):
        h = self.make_header('sender', 'abc@xyz.com')
        with self.assertRaises(AttributeError):
            h.groups = 'foo'

    def test_addresses_types(self):
        source = 'me <who@example.com>'
        h = self.make_header('to', source)
        self.assertIsInstance(h.addresses, tuple)
        self.assertIsInstance(h.addresses[0], Address)

    def test_groups_types(self):
        source = 'me <who@example.com>'
        h = self.make_header('to', source)
        self.assertIsInstance(h.groups, tuple)
        self.assertIsInstance(h.groups[0], Group)

    def test_set_from_Address(self):
        h = self.make_header('to', Address('me', 'foo', 'example.com'))
        self.assertEqual(h, 'me <foo@example.com>')

    def test_set_from_Address_list(self):
        h = self.make_header('to', [Address('me', 'foo', 'example.com'),
                                    Address('you', 'bar', 'example.com')])
        self.assertEqual(h, 'me <foo@example.com>, you <bar@example.com>')

    def test_set_from_Address_and_Group_list(self):
        h = self.make_header('to', [Address('me', 'foo', 'example.com'),
                                    Group('bing', [Address('fiz', 'z', 'b.com'),
                                                   Address('zif', 'f', 'c.com')]),
                                    Address('you', 'bar', 'example.com')])
        self.assertEqual(h, 'me <foo@example.com>, bing: fiz <z@b.com>, '
                            'zif <f@c.com>;, you <bar@example.com>')
        self.assertEqual(h.fold(policy=policy.default.clone(max_line_length=40)),
                        'to: me <foo@example.com>,\n'
                        ' bing: fiz <z@b.com>, zif <f@c.com>;,\n'
                        ' you <bar@example.com>\n')

    def test_set_from_Group_list(self):
        h = self.make_header('to', [Group('bing', [Address('fiz', 'z', 'b.com'),
                                                   Address('zif', 'f', 'c.com')])])
        self.assertEqual(h, 'bing: fiz <z@b.com>, zif <f@c.com>;')


class TestAddressAndGroup(TestEmailBase):

    def _test_attr_ro(self, obj, attr):
        with self.assertRaises(AttributeError):
            setattr(obj, attr, 'foo')

    def test_address_display_name_ro(self):
        self._test_attr_ro(Address('foo', 'bar', 'baz'), 'display_name')

    def test_address_username_ro(self):
        self._test_attr_ro(Address('foo', 'bar', 'baz'), 'username')

    def test_address_domain_ro(self):
        self._test_attr_ro(Address('foo', 'bar', 'baz'), 'domain')

    def test_group_display_name_ro(self):
        self._test_attr_ro(Group('foo'), 'display_name')

    def test_group_addresses_ro(self):
        self._test_attr_ro(Group('foo'), 'addresses')

    def test_address_from_username_domain(self):
        a = Address('foo', 'bar', 'baz')
        self.assertEqual(a.display_name, 'foo')
        self.assertEqual(a.username, 'bar')
        self.assertEqual(a.domain, 'baz')
        self.assertEqual(a.addr_spec, 'bar@baz')
        self.assertEqual(str(a), 'foo <bar@baz>')

    def test_address_from_addr_spec(self):
        a = Address('foo', addr_spec='bar@baz')
        self.assertEqual(a.display_name, 'foo')
        self.assertEqual(a.username, 'bar')
        self.assertEqual(a.domain, 'baz')
        self.assertEqual(a.addr_spec, 'bar@baz')
        self.assertEqual(str(a), 'foo <bar@baz>')

    def test_address_with_no_display_name(self):
        a = Address(addr_spec='bar@baz')
        self.assertEqual(a.display_name, '')
        self.assertEqual(a.username, 'bar')
        self.assertEqual(a.domain, 'baz')
        self.assertEqual(a.addr_spec, 'bar@baz')
        self.assertEqual(str(a), 'bar@baz')

    def test_null_address(self):
        a = Address()
        self.assertEqual(a.display_name, '')
        self.assertEqual(a.username, '')
        self.assertEqual(a.domain, '')
        self.assertEqual(a.addr_spec, '<>')
        self.assertEqual(str(a), '<>')

    def test_domain_only(self):
        # This isn't really a valid address.
        a = Address(domain='buzz')
        self.assertEqual(a.display_name, '')
        self.assertEqual(a.username, '')
        self.assertEqual(a.domain, 'buzz')
        self.assertEqual(a.addr_spec, '@buzz')
        self.assertEqual(str(a), '@buzz')

    def test_username_only(self):
        # This isn't really a valid address.
        a = Address(username='buzz')
        self.assertEqual(a.display_name, '')
        self.assertEqual(a.username, 'buzz')
        self.assertEqual(a.domain, '')
        self.assertEqual(a.addr_spec, 'buzz')
        self.assertEqual(str(a), 'buzz')

    def test_display_name_only(self):
        a = Address('buzz')
        self.assertEqual(a.display_name, 'buzz')
        self.assertEqual(a.username, '')
        self.assertEqual(a.domain, '')
        self.assertEqual(a.addr_spec, '<>')
        self.assertEqual(str(a), 'buzz <>')

    def test_quoting(self):
        # Ideally we'd check every special individually, but I'm not up for
        # writing that many tests.
        a = Address('Sara J.', 'bad name', 'example.com')
        self.assertEqual(a.display_name, 'Sara J.')
        self.assertEqual(a.username, 'bad name')
        self.assertEqual(a.domain, 'example.com')
        self.assertEqual(a.addr_spec, '"bad name"@example.com')
        self.assertEqual(str(a), '"Sara J." <"bad name"@example.com>')

    def test_il8n(self):
        a = Address('Éric', 'wok', 'exàmple.com')
        self.assertEqual(a.display_name, 'Éric')
        self.assertEqual(a.username, 'wok')
        self.assertEqual(a.domain, 'exàmple.com')
        self.assertEqual(a.addr_spec, 'wok@exàmple.com')
        self.assertEqual(str(a), 'Éric <wok@exàmple.com>')

    # XXX: there is an API design issue that needs to be solved here.
    #def test_non_ascii_username_raises(self):
    #    with self.assertRaises(ValueError):
    #        Address('foo', 'wők', 'example.com')

    def test_non_ascii_username_in_addr_spec_raises(self):
        with self.assertRaises(ValueError):
            Address('foo', addr_spec='wők@example.com')

    def test_address_addr_spec_and_username_raises(self):
        with self.assertRaises(TypeError):
            Address('foo', username='bing', addr_spec='bar@baz')

    def test_address_addr_spec_and_domain_raises(self):
        with self.assertRaises(TypeError):
            Address('foo', domain='bing', addr_spec='bar@baz')

    def test_address_addr_spec_and_username_and_domain_raises(self):
        with self.assertRaises(TypeError):
            Address('foo', username='bong', domain='bing', addr_spec='bar@baz')

    def test_space_in_addr_spec_username_raises(self):
        with self.assertRaises(ValueError):
            Address('foo', addr_spec="bad name@example.com")

    def test_bad_addr_sepc_raises(self):
        with self.assertRaises(ValueError):
            Address('foo', addr_spec="name@ex[]ample.com")

    def test_empty_group(self):
        g = Group('foo')
        self.assertEqual(g.display_name, 'foo')
        self.assertEqual(g.addresses, tuple())
        self.assertEqual(str(g), 'foo:;')

    def test_empty_group_list(self):
        g = Group('foo', addresses=[])
        self.assertEqual(g.display_name, 'foo')
        self.assertEqual(g.addresses, tuple())
        self.assertEqual(str(g), 'foo:;')

    def test_null_group(self):
        g = Group()
        self.assertIsNone(g.display_name)
        self.assertEqual(g.addresses, tuple())
        self.assertEqual(str(g), 'None:;')

    def test_group_with_addresses(self):
        addrs = [Address('b', 'b', 'c'), Address('a', 'b','c')]
        g = Group('foo', addrs)
        self.assertEqual(g.display_name, 'foo')
        self.assertEqual(g.addresses, tuple(addrs))
        self.assertEqual(str(g), 'foo: b <b@c>, a <b@c>;')

    def test_group_with_addresses_no_display_name(self):
        addrs = [Address('b', 'b', 'c'), Address('a', 'b','c')]
        g = Group(addresses=addrs)
        self.assertIsNone(g.display_name)
        self.assertEqual(g.addresses, tuple(addrs))
        self.assertEqual(str(g), 'None: b <b@c>, a <b@c>;')

    def test_group_with_one_address_no_display_name(self):
        addrs = [Address('b', 'b', 'c')]
        g = Group(addresses=addrs)
        self.assertIsNone(g.display_name)
        self.assertEqual(g.addresses, tuple(addrs))
        self.assertEqual(str(g), 'b <b@c>')

    def test_display_name_quoting(self):
        g = Group('foo.bar')
        self.assertEqual(g.display_name, 'foo.bar')
        self.assertEqual(g.addresses, tuple())
        self.assertEqual(str(g), '"foo.bar":;')

    def test_display_name_blanks_not_quoted(self):
        g = Group('foo bar')
        self.assertEqual(g.display_name, 'foo bar')
        self.assertEqual(g.addresses, tuple())
        self.assertEqual(str(g), 'foo bar:;')


class TestFolding(TestHeaderBase):

    def test_short_unstructured(self):
        h = self.make_header('subject', 'this is a test')
        self.assertEqual(h.fold(policy=self.policy),
                         'subject: this is a test\n')

    def test_long_unstructured(self):
        h = self.make_header('Subject', 'This is a long header '
            'line that will need to be folded into two lines '
            'and will demonstrate basic folding')
        self.assertEqual(h.fold(policy=self.policy),
                        'Subject: This is a long header line that will '
                            'need to be folded into two lines\n'
                        ' and will demonstrate basic folding\n')

    def test_unstructured_short_max_line_length(self):
        h = self.make_header('Subject', 'this is a short header '
            'that will be folded anyway')
        self.assertEqual(
            h.fold(policy=policy.default.clone(max_line_length=20)),
            textwrap.dedent("""\
                Subject: this is a
                 short header that
                 will be folded
                 anyway
                """))

    def test_fold_unstructured_single_word(self):
        h = self.make_header('Subject', 'test')
        self.assertEqual(h.fold(policy=self.policy), 'Subject: test\n')

    def test_fold_unstructured_short(self):
        h = self.make_header('Subject', 'test test test')
        self.assertEqual(h.fold(policy=self.policy),
                        'Subject: test test test\n')

    def test_fold_unstructured_with_overlong_word(self):
        h = self.make_header('Subject', 'thisisaverylonglineconsistingofa'
            'singlewordthatwontfit')
        self.assertEqual(
            h.fold(policy=policy.default.clone(max_line_length=20)),
            'Subject: thisisaverylonglineconsistingofasinglewordthatwontfit\n')

    def test_fold_unstructured_with_two_overlong_words(self):
        h = self.make_header('Subject', 'thisisaverylonglineconsistingofa'
            'singlewordthatwontfit plusanotherverylongwordthatwontfit')
        self.assertEqual(
            h.fold(policy=policy.default.clone(max_line_length=20)),
            'Subject: thisisaverylonglineconsistingofasinglewordthatwontfit\n'
                ' plusanotherverylongwordthatwontfit\n')

    def test_fold_unstructured_with_slightly_long_word(self):
        h = self.make_header('Subject', 'thislongwordislessthanmaxlinelen')
        self.assertEqual(
            h.fold(policy=policy.default.clone(max_line_length=35)),
            'Subject:\n thislongwordislessthanmaxlinelen\n')

    def test_fold_unstructured_with_commas(self):
        # The old wrapper would fold this at the commas.
        h = self.make_header('Subject', "This header is intended to "
            "demonstrate, in a fairly susinct way, that we now do "
            "not give a , special treatment in unstructured headers.")
        self.assertEqual(
            h.fold(policy=policy.default.clone(max_line_length=60)),
            textwrap.dedent("""\
                Subject: This header is intended to demonstrate, in a fairly
                 susinct way, that we now do not give a , special treatment
                 in unstructured headers.
                 """))

    def test_fold_address_list(self):
        h = self.make_header('To', '"Theodore H. Perfect" <yes@man.com>, '
            '"My address is very long because my name is long" <foo@bar.com>, '
            '"Only A. Friend" <no@yes.com>')
        self.assertEqual(h.fold(policy=self.policy), textwrap.dedent("""\
            To: "Theodore H. Perfect" <yes@man.com>,
             "My address is very long because my name is long" <foo@bar.com>,
             "Only A. Friend" <no@yes.com>
             """))

    def test_fold_date_header(self):
        h = self.make_header('Date', 'Sat, 2 Feb 2002 17:00:06 -0800')
        self.assertEqual(h.fold(policy=self.policy),
                        'Date: Sat, 02 Feb 2002 17:00:06 -0800\n')


if __name__ == '__main__':
    unittest.main()
