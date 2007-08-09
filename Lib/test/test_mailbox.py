import os
import sys
import time
import stat
import socket
import email
import email.message
import rfc822
import re
import io
from test import test_support
import unittest
import mailbox
import glob
try:
    import fcntl
except ImportError:
    pass


class TestBase(unittest.TestCase):

    def _check_sample(self, msg):
        # Inspect a mailbox.Message representation of the sample message
        self.assert_(isinstance(msg, email.message.Message))
        self.assert_(isinstance(msg, mailbox.Message))
        for key, value in _sample_headers.items():
            self.assert_(value in msg.get_all(key))
        self.assert_(msg.is_multipart())
        self.assertEqual(len(msg.get_payload()), len(_sample_payloads))
        for i, payload in enumerate(_sample_payloads):
            part = msg.get_payload(i)
            self.assert_(isinstance(part, email.message.Message))
            self.assert_(not isinstance(part, mailbox.Message))
            self.assertEqual(part.get_payload(), payload)

    def _delete_recursively(self, target):
        # Delete a file or delete a directory recursively
        if os.path.isdir(target):
            for path, dirs, files in os.walk(target, topdown=False):
                for name in files:
                    os.remove(os.path.join(path, name))
                for name in dirs:
                    os.rmdir(os.path.join(path, name))
            os.rmdir(target)
        elif os.path.exists(target):
            os.remove(target)


class TestMailbox(TestBase):

    _factory = None     # Overridden by subclasses to reuse tests
    _template = 'From: foo\n\n%s'

    def setUp(self):
        self._path = test_support.TESTFN
        self._delete_recursively(self._path)
        self._box = self._factory(self._path)

    def tearDown(self):
        self._delete_recursively(self._path)

    def test_add(self):
        # Add copies of a sample message
        keys = []
        keys.append(self._box.add(self._template % 0))
        self.assertEqual(len(self._box), 1)
        keys.append(self._box.add(mailbox.Message(_sample_message)))
        self.assertEqual(len(self._box), 2)
        keys.append(self._box.add(email.message_from_string(_sample_message)))
        self.assertEqual(len(self._box), 3)
        keys.append(self._box.add(io.StringIO(_sample_message)))
        self.assertEqual(len(self._box), 4)
        keys.append(self._box.add(_sample_message))
        self.assertEqual(len(self._box), 5)
        self.assertEqual(self._box.get_string(keys[0]), self._template % 0)
        for i in (1, 2, 3, 4):
            self._check_sample(self._box[keys[i]])

    def test_remove(self):
        # Remove messages using remove()
        self._test_remove_or_delitem(self._box.remove)

    def test_delitem(self):
        # Remove messages using __delitem__()
        self._test_remove_or_delitem(self._box.__delitem__)

    def _test_remove_or_delitem(self, method):
        # (Used by test_remove() and test_delitem().)
        key0 = self._box.add(self._template % 0)
        key1 = self._box.add(self._template % 1)
        self.assertEqual(len(self._box), 2)
        method(key0)
        l = len(self._box)
        self.assert_(l == 1, "actual l: %s" % l)
        self.assertRaises(KeyError, lambda: self._box[key0])
        self.assertRaises(KeyError, lambda: method(key0))
        self.assertEqual(self._box.get_string(key1), self._template % 1)
        key2 = self._box.add(self._template % 2)
        self.assertEqual(len(self._box), 2)
        method(key2)
        l = len(self._box)
        self.assert_(l == 1, "actual l: %s" % l)
        self.assertRaises(KeyError, lambda: self._box[key2])
        self.assertRaises(KeyError, lambda: method(key2))
        self.assertEqual(self._box.get_string(key1), self._template % 1)
        method(key1)
        self.assertEqual(len(self._box), 0)
        self.assertRaises(KeyError, lambda: self._box[key1])
        self.assertRaises(KeyError, lambda: method(key1))

    def test_discard(self, repetitions=10):
        # Discard messages
        key0 = self._box.add(self._template % 0)
        key1 = self._box.add(self._template % 1)
        self.assertEqual(len(self._box), 2)
        self._box.discard(key0)
        self.assertEqual(len(self._box), 1)
        self.assertRaises(KeyError, lambda: self._box[key0])
        self._box.discard(key0)
        self.assertEqual(len(self._box), 1)
        self.assertRaises(KeyError, lambda: self._box[key0])

    def test_get(self):
        # Retrieve messages using get()
        key0 = self._box.add(self._template % 0)
        msg = self._box.get(key0)
        self.assertEqual(msg['from'], 'foo')
        self.assertEqual(msg.get_payload(), '0')
        self.assert_(self._box.get('foo') is None)
        self.assert_(self._box.get('foo', False) is False)
        self._box.close()
        self._box = self._factory(self._path, factory=rfc822.Message)
        key1 = self._box.add(self._template % 1)
        msg = self._box.get(key1)
        self.assertEqual(msg['from'], 'foo')
        self.assertEqual(msg.fp.read(), '1')

    def test_getitem(self):
        # Retrieve message using __getitem__()
        key0 = self._box.add(self._template % 0)
        msg = self._box[key0]
        self.assertEqual(msg['from'], 'foo')
        self.assertEqual(msg.get_payload(), '0')
        self.assertRaises(KeyError, lambda: self._box['foo'])
        self._box.discard(key0)
        self.assertRaises(KeyError, lambda: self._box[key0])

    def test_get_message(self):
        # Get Message representations of messages
        key0 = self._box.add(self._template % 0)
        key1 = self._box.add(_sample_message)
        msg0 = self._box.get_message(key0)
        self.assert_(isinstance(msg0, mailbox.Message))
        self.assertEqual(msg0['from'], 'foo')
        self.assertEqual(msg0.get_payload(), '0')
        self._check_sample(self._box.get_message(key1))

    def test_get_string(self):
        # Get string representations of messages
        key0 = self._box.add(self._template % 0)
        key1 = self._box.add(_sample_message)
        self.assertEqual(self._box.get_string(key0), self._template % 0)
        self.assertEqual(self._box.get_string(key1), _sample_message)

    def test_get_file(self):
        # Get file representations of messages
        key0 = self._box.add(self._template % 0)
        key1 = self._box.add(_sample_message)
        self.assertEqual(self._box.get_file(key0).read().replace(os.linesep, '\n'),
                         self._template % 0)
        self.assertEqual(self._box.get_file(key1).read().replace(os.linesep, '\n'),
                         _sample_message)

    def test_iterkeys(self):
        # Get keys using iterkeys()
        self._check_iteration(self._box.keys, do_keys=True, do_values=False)

    def test_keys(self):
        # Get keys using keys()
        self._check_iteration(self._box.keys, do_keys=True, do_values=False)

    def test_itervalues(self):
        # Get values using itervalues()
        self._check_iteration(self._box.values, do_keys=False,
                              do_values=True)

    def test_iter(self):
        # Get values using __iter__()
        self._check_iteration(self._box.__iter__, do_keys=False,
                              do_values=True)

    def test_values(self):
        # Get values using values()
        self._check_iteration(self._box.values, do_keys=False, do_values=True)

    def test_iteritems(self):
        # Get keys and values using iteritems()
        self._check_iteration(self._box.items, do_keys=True,
                              do_values=True)

    def test_items(self):
        # Get keys and values using items()
        self._check_iteration(self._box.items, do_keys=True, do_values=True)

    def _check_iteration(self, method, do_keys, do_values, repetitions=10):
        for value in method():
            self.fail("Not empty")
        keys, values = [], []
        for i in range(repetitions):
            keys.append(self._box.add(self._template % i))
            values.append(self._template % i)
        if do_keys and not do_values:
            returned_keys = list(method())
        elif do_values and not do_keys:
            returned_values = list(method())
        else:
            returned_keys, returned_values = [], []
            for key, value in method():
                returned_keys.append(key)
                returned_values.append(value)
        if do_keys:
            self.assertEqual(len(keys), len(returned_keys))
            self.assertEqual(set(keys), set(returned_keys))
        if do_values:
            count = 0
            for value in returned_values:
                self.assertEqual(value['from'], 'foo')
                self.assert_(int(value.get_payload()) < repetitions)
                count += 1
            self.assertEqual(len(values), count)

    def test_contains(self):
        # Check existence of keys using __contains__()
        method = self._box.__contains__
        self.assert_(not method('foo'))
        key0 = self._box.add(self._template % 0)
        self.assert_(method(key0))
        self.assert_(not method('foo'))
        key1 = self._box.add(self._template % 1)
        self.assert_(method(key1))
        self.assert_(method(key0))
        self.assert_(not method('foo'))
        self._box.remove(key0)
        self.assert_(not method(key0))
        self.assert_(method(key1))
        self.assert_(not method('foo'))
        self._box.remove(key1)
        self.assert_(not method(key1))
        self.assert_(not method(key0))
        self.assert_(not method('foo'))

    def test_len(self, repetitions=10):
        # Get message count
        keys = []
        for i in range(repetitions):
            self.assertEqual(len(self._box), i)
            keys.append(self._box.add(self._template % i))
            self.assertEqual(len(self._box), i + 1)
        for i in range(repetitions):
            self.assertEqual(len(self._box), repetitions - i)
            self._box.remove(keys[i])
            self.assertEqual(len(self._box), repetitions - i - 1)

    def test_set_item(self):
        # Modify messages using __setitem__()
        key0 = self._box.add(self._template % 'original 0')
        self.assertEqual(self._box.get_string(key0),
                         self._template % 'original 0')
        key1 = self._box.add(self._template % 'original 1')
        self.assertEqual(self._box.get_string(key1),
                         self._template % 'original 1')
        self._box[key0] = self._template % 'changed 0'
        self.assertEqual(self._box.get_string(key0),
                         self._template % 'changed 0')
        self._box[key1] = self._template % 'changed 1'
        self.assertEqual(self._box.get_string(key1),
                         self._template % 'changed 1')
        self._box[key0] = _sample_message
        self._check_sample(self._box[key0])
        self._box[key1] = self._box[key0]
        self._check_sample(self._box[key1])
        self._box[key0] = self._template % 'original 0'
        self.assertEqual(self._box.get_string(key0),
                     self._template % 'original 0')
        self._check_sample(self._box[key1])
        self.assertRaises(KeyError,
                          lambda: self._box.__setitem__('foo', 'bar'))
        self.assertRaises(KeyError, lambda: self._box['foo'])
        self.assertEqual(len(self._box), 2)

    def test_clear(self, iterations=10):
        # Remove all messages using clear()
        keys = []
        for i in range(iterations):
            self._box.add(self._template % i)
        for i, key in enumerate(keys):
            self.assert_(self._box.get_string(key) == self._template % i)
        self._box.clear()
        self.assertEqual(len(self._box), 0)
        for i, key in enumerate(keys):
            self.assertRaises(KeyError, lambda: self._box.get_string(key))

    def test_pop(self):
        # Get and remove a message using pop()
        key0 = self._box.add(self._template % 0)
        self.assert_(key0 in self._box)
        key1 = self._box.add(self._template % 1)
        self.assert_(key1 in self._box)
        self.assertEqual(self._box.pop(key0).get_payload(), '0')
        self.assert_(key0 not in self._box)
        self.assert_(key1 in self._box)
        key2 = self._box.add(self._template % 2)
        self.assert_(key2 in self._box)
        self.assertEqual(self._box.pop(key2).get_payload(), '2')
        self.assert_(key2 not in self._box)
        self.assert_(key1 in self._box)
        self.assertEqual(self._box.pop(key1).get_payload(), '1')
        self.assert_(key1 not in self._box)
        self.assertEqual(len(self._box), 0)

    def test_popitem(self, iterations=10):
        # Get and remove an arbitrary (key, message) using popitem()
        keys = []
        for i in range(10):
            keys.append(self._box.add(self._template % i))
        seen = []
        for i in range(10):
            key, msg = self._box.popitem()
            self.assert_(key in keys)
            self.assert_(key not in seen)
            seen.append(key)
            self.assertEqual(int(msg.get_payload()), keys.index(key))
        self.assertEqual(len(self._box), 0)
        for key in keys:
            self.assertRaises(KeyError, lambda: self._box[key])

    def test_update(self):
        # Modify multiple messages using update()
        key0 = self._box.add(self._template % 'original 0')
        key1 = self._box.add(self._template % 'original 1')
        key2 = self._box.add(self._template % 'original 2')
        self._box.update({key0: self._template % 'changed 0',
                          key2: _sample_message})
        self.assertEqual(len(self._box), 3)
        self.assertEqual(self._box.get_string(key0),
                     self._template % 'changed 0')
        self.assertEqual(self._box.get_string(key1),
                     self._template % 'original 1')
        self._check_sample(self._box[key2])
        self._box.update([(key2, self._template % 'changed 2'),
                    (key1, self._template % 'changed 1'),
                    (key0, self._template % 'original 0')])
        self.assertEqual(len(self._box), 3)
        self.assertEqual(self._box.get_string(key0),
                     self._template % 'original 0')
        self.assertEqual(self._box.get_string(key1),
                     self._template % 'changed 1')
        self.assertEqual(self._box.get_string(key2),
                     self._template % 'changed 2')
        self.assertRaises(KeyError,
                          lambda: self._box.update({'foo': 'bar',
                                          key0: self._template % "changed 0"}))
        self.assertEqual(len(self._box), 3)
        self.assertEqual(self._box.get_string(key0),
                     self._template % "changed 0")
        self.assertEqual(self._box.get_string(key1),
                     self._template % "changed 1")
        self.assertEqual(self._box.get_string(key2),
                     self._template % "changed 2")

    def test_flush(self):
        # Write changes to disk
        self._test_flush_or_close(self._box.flush)

    def test_lock_unlock(self):
        # Lock and unlock the mailbox
        self.assert_(not os.path.exists(self._get_lock_path()))
        self._box.lock()
        self.assert_(os.path.exists(self._get_lock_path()))
        self._box.unlock()
        self.assert_(not os.path.exists(self._get_lock_path()))

    def test_close(self):
        # Close mailbox and flush changes to disk
        self._test_flush_or_close(self._box.close)

    def _test_flush_or_close(self, method):
        contents = [self._template % i for i in range(3)]
        self._box.add(contents[0])
        self._box.add(contents[1])
        self._box.add(contents[2])
        method()
        self._box = self._factory(self._path)
        keys = self._box.keys()
        self.assertEqual(len(keys), 3)
        for key in keys:
            self.assert_(self._box.get_string(key) in contents)

    def test_dump_message(self):
        # Write message representations to disk
        for input in (email.message_from_string(_sample_message),
                      _sample_message, io.StringIO(_sample_message)):
            output = io.StringIO()
            self._box._dump_message(input, output)
            self.assert_(output.getvalue() ==
                         _sample_message.replace('\n', os.linesep))
        output = io.StringIO()
        self.assertRaises(TypeError,
                          lambda: self._box._dump_message(None, output))

    def _get_lock_path(self):
        # Return the path of the dot lock file. May be overridden.
        return self._path + '.lock'


class TestMailboxSuperclass(TestBase):

    def test_notimplemented(self):
        # Test that all Mailbox methods raise NotImplementedException.
        box = mailbox.Mailbox('path')
        self.assertRaises(NotImplementedError, lambda: box.add(''))
        self.assertRaises(NotImplementedError, lambda: box.remove(''))
        self.assertRaises(NotImplementedError, lambda: box.__delitem__(''))
        self.assertRaises(NotImplementedError, lambda: box.discard(''))
        self.assertRaises(NotImplementedError, lambda: box.__setitem__('', ''))
        self.assertRaises(NotImplementedError, lambda: box.keys())
        self.assertRaises(NotImplementedError, lambda: box.keys())
        self.assertRaises(NotImplementedError, lambda: box.values().__next__())
        self.assertRaises(NotImplementedError, lambda: box.__iter__().__next__())
        self.assertRaises(NotImplementedError, lambda: box.values())
        self.assertRaises(NotImplementedError, lambda: box.items().next())
        self.assertRaises(NotImplementedError, lambda: box.items())
        self.assertRaises(NotImplementedError, lambda: box.get(''))
        self.assertRaises(NotImplementedError, lambda: box.__getitem__(''))
        self.assertRaises(NotImplementedError, lambda: box.get_message(''))
        self.assertRaises(NotImplementedError, lambda: box.get_string(''))
        self.assertRaises(NotImplementedError, lambda: box.get_file(''))
        self.assertRaises(NotImplementedError, lambda: '' in box)
        self.assertRaises(NotImplementedError, lambda: box.__contains__(''))
        self.assertRaises(NotImplementedError, lambda: box.__len__())
        self.assertRaises(NotImplementedError, lambda: box.clear())
        self.assertRaises(NotImplementedError, lambda: box.pop(''))
        self.assertRaises(NotImplementedError, lambda: box.popitem())
        self.assertRaises(NotImplementedError, lambda: box.update((('', ''),)))
        self.assertRaises(NotImplementedError, lambda: box.flush())
        self.assertRaises(NotImplementedError, lambda: box.lock())
        self.assertRaises(NotImplementedError, lambda: box.unlock())
        self.assertRaises(NotImplementedError, lambda: box.close())


class TestMaildir(TestMailbox):

    _factory = lambda self, path, factory=None: mailbox.Maildir(path, factory)

    def setUp(self):
        TestMailbox.setUp(self)
        if os.name in ('nt', 'os2') or sys.platform == 'cygwin':
            self._box.colon = '!'

    def test_add_MM(self):
        # Add a MaildirMessage instance
        msg = mailbox.MaildirMessage(self._template % 0)
        msg.set_subdir('cur')
        msg.set_info('foo')
        key = self._box.add(msg)
        self.assert_(os.path.exists(os.path.join(self._path, 'cur', '%s%sfoo' %
                                                 (key, self._box.colon))))

    def test_get_MM(self):
        # Get a MaildirMessage instance
        msg = mailbox.MaildirMessage(self._template % 0)
        msg.set_subdir('cur')
        msg.set_flags('RF')
        key = self._box.add(msg)
        msg_returned = self._box.get_message(key)
        self.assert_(isinstance(msg_returned, mailbox.MaildirMessage))
        self.assertEqual(msg_returned.get_subdir(), 'cur')
        self.assertEqual(msg_returned.get_flags(), 'FR')

    def test_set_MM(self):
        # Set with a MaildirMessage instance
        msg0 = mailbox.MaildirMessage(self._template % 0)
        msg0.set_flags('TP')
        key = self._box.add(msg0)
        msg_returned = self._box.get_message(key)
        self.assertEqual(msg_returned.get_subdir(), 'new')
        self.assertEqual(msg_returned.get_flags(), 'PT')
        msg1 = mailbox.MaildirMessage(self._template % 1)
        self._box[key] = msg1
        msg_returned = self._box.get_message(key)
        self.assertEqual(msg_returned.get_subdir(), 'new')
        self.assertEqual(msg_returned.get_flags(), '')
        self.assertEqual(msg_returned.get_payload(), '1')
        msg2 = mailbox.MaildirMessage(self._template % 2)
        msg2.set_info('2,S')
        self._box[key] = msg2
        self._box[key] = self._template % 3
        msg_returned = self._box.get_message(key)
        self.assertEqual(msg_returned.get_subdir(), 'new')
        self.assertEqual(msg_returned.get_flags(), 'S')
        self.assertEqual(msg_returned.get_payload(), '3')

    def test_initialize_new(self):
        # Initialize a non-existent mailbox
        self.tearDown()
        self._box = mailbox.Maildir(self._path)
        self._check_basics(factory=rfc822.Message)
        self._delete_recursively(self._path)
        self._box = self._factory(self._path, factory=None)
        self._check_basics()

    def test_initialize_existing(self):
        # Initialize an existing mailbox
        self.tearDown()
        for subdir in '', 'tmp', 'new', 'cur':
            os.mkdir(os.path.normpath(os.path.join(self._path, subdir)))
        self._box = mailbox.Maildir(self._path)
        self._check_basics(factory=rfc822.Message)
        self._box = mailbox.Maildir(self._path, factory=None)
        self._check_basics()

    def _check_basics(self, factory=None):
        # (Used by test_open_new() and test_open_existing().)
        self.assertEqual(self._box._path, os.path.abspath(self._path))
        self.assertEqual(self._box._factory, factory)
        for subdir in '', 'tmp', 'new', 'cur':
            path = os.path.join(self._path, subdir)
            mode = os.stat(path)[stat.ST_MODE]
            self.assert_(stat.S_ISDIR(mode), "Not a directory: '%s'" % path)

    def test_list_folders(self):
        # List folders
        self._box.add_folder('one')
        self._box.add_folder('two')
        self._box.add_folder('three')
        self.assertEqual(len(self._box.list_folders()), 3)
        self.assertEqual(set(self._box.list_folders()),
                     set(('one', 'two', 'three')))

    def test_get_folder(self):
        # Open folders
        self._box.add_folder('foo.bar')
        folder0 = self._box.get_folder('foo.bar')
        folder0.add(self._template % 'bar')
        self.assert_(os.path.isdir(os.path.join(self._path, '.foo.bar')))
        folder1 = self._box.get_folder('foo.bar')
        self.assertEqual(folder1.get_string(folder1.keys()[0]),
                         self._template % 'bar')

    def test_add_and_remove_folders(self):
        # Delete folders
        self._box.add_folder('one')
        self._box.add_folder('two')
        self.assertEqual(len(self._box.list_folders()), 2)
        self.assert_(set(self._box.list_folders()) == set(('one', 'two')))
        self._box.remove_folder('one')
        self.assertEqual(len(self._box.list_folders()), 1)
        self.assertEqual(set(self._box.list_folders()), set(('two',)))
        self._box.add_folder('three')
        self.assertEqual(len(self._box.list_folders()), 2)
        self.assertEqual(set(self._box.list_folders()), set(('two', 'three')))
        self._box.remove_folder('three')
        self.assertEqual(len(self._box.list_folders()), 1)
        self.assertEqual(set(self._box.list_folders()), set(('two',)))
        self._box.remove_folder('two')
        self.assertEqual(len(self._box.list_folders()), 0)
        self.assertEqual(self._box.list_folders(), [])

    def test_clean(self):
        # Remove old files from 'tmp'
        foo_path = os.path.join(self._path, 'tmp', 'foo')
        bar_path = os.path.join(self._path, 'tmp', 'bar')
        f = open(foo_path, 'w')
        f.write("@")
        f.close()
        f = open(bar_path, 'w')
        f.write("@")
        f.close()
        self._box.clean()
        self.assert_(os.path.exists(foo_path))
        self.assert_(os.path.exists(bar_path))
        foo_stat = os.stat(foo_path)
        os.utime(foo_path, (time.time() - 129600 - 2,
                            foo_stat.st_mtime))
        self._box.clean()
        self.assert_(not os.path.exists(foo_path))
        self.assert_(os.path.exists(bar_path))

    def test_create_tmp(self, repetitions=10):
        # Create files in tmp directory
        hostname = socket.gethostname()
        if '/' in hostname:
            hostname = hostname.replace('/', r'\057')
        if ':' in hostname:
            hostname = hostname.replace(':', r'\072')
        pid = os.getpid()
        pattern = re.compile(r"(?P<time>\d+)\.M(?P<M>\d{1,6})P(?P<P>\d+)"
                             r"Q(?P<Q>\d+)\.(?P<host>[^:/]+)")
        previous_groups = None
        for x in range(repetitions):
            tmp_file = self._box._create_tmp()
            head, tail = os.path.split(tmp_file.name)
            self.assertEqual(head, os.path.abspath(os.path.join(self._path,
                                                                "tmp")),
                             "File in wrong location: '%s'" % head)
            match = pattern.match(tail)
            self.assert_(match != None, "Invalid file name: '%s'" % tail)
            groups = match.groups()
            if previous_groups != None:
                self.assert_(int(groups[0] >= previous_groups[0]),
                             "Non-monotonic seconds: '%s' before '%s'" %
                             (previous_groups[0], groups[0]))
                self.assert_(int(groups[1] >= previous_groups[1]) or
                             groups[0] != groups[1],
                             "Non-monotonic milliseconds: '%s' before '%s'" %
                             (previous_groups[1], groups[1]))
                self.assertEqual(int(groups[2]), pid,
                             "Process ID mismatch: '%s' should be '%s'" %
                             (groups[2], pid))
                self.assertEqual(int(groups[3]), int(previous_groups[3]) + 1,
                             "Non-sequential counter: '%s' before '%s'" %
                             (previous_groups[3], groups[3]))
                self.assertEqual(groups[4], hostname,
                             "Host name mismatch: '%s' should be '%s'" %
                             (groups[4], hostname))
            previous_groups = groups
            tmp_file.write(_sample_message)
            tmp_file.seek(0)
            self.assertEqual(tmp_file.read(), _sample_message)
            tmp_file.close()
        file_count = len(os.listdir(os.path.join(self._path, "tmp")))
        self.assertEqual(file_count, repetitions,
                     "Wrong file count: '%s' should be '%s'" %
                     (file_count, repetitions))

    def test_refresh(self):
        # Update the table of contents
        self.assertEqual(self._box._toc, {})
        key0 = self._box.add(self._template % 0)
        key1 = self._box.add(self._template % 1)
        self.assertEqual(self._box._toc, {})
        self._box._refresh()
        self.assertEqual(self._box._toc, {key0: os.path.join('new', key0),
                                          key1: os.path.join('new', key1)})
        key2 = self._box.add(self._template % 2)
        self.assertEqual(self._box._toc, {key0: os.path.join('new', key0),
                                          key1: os.path.join('new', key1)})
        self._box._refresh()
        self.assertEqual(self._box._toc, {key0: os.path.join('new', key0),
                                          key1: os.path.join('new', key1),
                                          key2: os.path.join('new', key2)})

    def test_lookup(self):
        # Look up message subpaths in the TOC
        self.assertRaises(KeyError, lambda: self._box._lookup('foo'))
        key0 = self._box.add(self._template % 0)
        self.assertEqual(self._box._lookup(key0), os.path.join('new', key0))
        os.remove(os.path.join(self._path, 'new', key0))
        self.assertEqual(self._box._toc, {key0: os.path.join('new', key0)})
        self.assertRaises(KeyError, lambda: self._box._lookup(key0))
        self.assertEqual(self._box._toc, {})

    def test_lock_unlock(self):
        # Lock and unlock the mailbox. For Maildir, this does nothing.
        self._box.lock()
        self._box.unlock()

    def test_folder (self):
        # Test for bug #1569790: verify that folders returned by .get_folder()
        # use the same factory function.
        def dummy_factory (s):
            return None
        box = self._factory(self._path, factory=dummy_factory)
        folder = box.add_folder('folder1')
        self.assert_(folder._factory is dummy_factory)

        folder1_alias = box.get_folder('folder1')
        self.assert_(folder1_alias._factory is dummy_factory)

    def test_directory_in_folder (self):
        # Test that mailboxes still work if there's a stray extra directory
        # in a folder.
        for i in range(10):
            self._box.add(mailbox.Message(_sample_message))

        # Create a stray directory
        os.mkdir(os.path.join(self._path, 'cur', 'stray-dir'))

        # Check that looping still works with the directory present.
        for msg in self._box:
            pass

class _TestMboxMMDF(TestMailbox):

    def tearDown(self):
        self._delete_recursively(self._path)
        for lock_remnant in glob.glob(self._path + '.*'):
            test_support.unlink(lock_remnant)

    def test_add_from_string(self):
        # Add a string starting with 'From ' to the mailbox
        key = self._box.add('From foo@bar blah\nFrom: foo\n\n0')
        self.assertEqual(self._box[key].get_from(), 'foo@bar blah')
        self.assertEqual(self._box[key].get_payload(), '0')

    def test_add_mbox_or_mmdf_message(self):
        # Add an mboxMessage or MMDFMessage
        for class_ in (mailbox.mboxMessage, mailbox.MMDFMessage):
            msg = class_('From foo@bar blah\nFrom: foo\n\n0')
            key = self._box.add(msg)

    def test_open_close_open(self):
        # Open and inspect previously-created mailbox
        values = [self._template % i for i in range(3)]
        for value in values:
            self._box.add(value)
        self._box.close()
        mtime = os.path.getmtime(self._path)
        self._box = self._factory(self._path)
        self.assertEqual(len(self._box), 3)
        for key in self._box.keys():
            self.assert_(self._box.get_string(key) in values)
        self._box.close()
        self.assertEqual(mtime, os.path.getmtime(self._path))

    def test_add_and_close(self):
        # Verifying that closing a mailbox doesn't change added items
        self._box.add(_sample_message)
        for i in range(3):
            self._box.add(self._template % i)
        self._box.add(_sample_message)
        self._box._file.flush()
        self._box._file.seek(0)
        contents = self._box._file.read()
        self._box.close()
        self.assertEqual(contents, open(self._path, 'r').read())
        self._box = self._factory(self._path)

    def test_lock_conflict(self):
        # Fork off a subprocess that will lock the file for 2 seconds,
        # unlock it, and then exit.
        if not hasattr(os, 'fork'):
            return
        pid = os.fork()
        if pid == 0:
            # In the child, lock the mailbox.
            try:
                self._box.lock()
                time.sleep(2)
                self._box.unlock()
            finally:
                os._exit(0)

        # In the parent, sleep a bit to give the child time to acquire
        # the lock.
        time.sleep(0.5)
        try:
            self.assertRaises(mailbox.ExternalClashError,
                              self._box.lock)
        finally:
            # Wait for child to exit.  Locking should now succeed.
            exited_pid, status = os.waitpid(pid, 0)

        self._box.lock()
        self._box.unlock()

    def test_relock(self):
        # Test case for bug #1575506: the mailbox class was locking the
        # wrong file object in its flush() method.
        msg = "Subject: sub\n\nbody\n"
        key1 = self._box.add(msg)
        self._box.flush()
        self._box.close()

        self._box = self._factory(self._path)
        self._box.lock()
        key2 = self._box.add(msg)
        self._box.flush()
        self.assert_(self._box._locked)
        self._box.close()



class TestMbox(_TestMboxMMDF):

    _factory = lambda self, path, factory=None: mailbox.mbox(path, factory)


class TestMMDF(_TestMboxMMDF):

    _factory = lambda self, path, factory=None: mailbox.MMDF(path, factory)


class TestMH(TestMailbox):

    _factory = lambda self, path, factory=None: mailbox.MH(path, factory)

    def test_list_folders(self):
        # List folders
        self._box.add_folder('one')
        self._box.add_folder('two')
        self._box.add_folder('three')
        self.assertEqual(len(self._box.list_folders()), 3)
        self.assertEqual(set(self._box.list_folders()),
                     set(('one', 'two', 'three')))

    def test_get_folder(self):
        # Open folders
        def dummy_factory (s):
            return None
        self._box = self._factory(self._path, dummy_factory)

        new_folder = self._box.add_folder('foo.bar')
        folder0 = self._box.get_folder('foo.bar')
        folder0.add(self._template % 'bar')
        self.assert_(os.path.isdir(os.path.join(self._path, 'foo.bar')))
        folder1 = self._box.get_folder('foo.bar')
        self.assertEqual(folder1.get_string(folder1.keys()[0]),
                         self._template % 'bar')

        # Test for bug #1569790: verify that folders returned by .get_folder()
        # use the same factory function.
        self.assert_(new_folder._factory is self._box._factory)
        self.assert_(folder0._factory is self._box._factory)

    def test_add_and_remove_folders(self):
        # Delete folders
        self._box.add_folder('one')
        self._box.add_folder('two')
        self.assertEqual(len(self._box.list_folders()), 2)
        self.assertEqual(set(self._box.list_folders()), set(('one', 'two')))
        self._box.remove_folder('one')
        self.assertEqual(len(self._box.list_folders()), 1)
        self.assertEqual(set(self._box.list_folders()), set(('two',)))
        self._box.add_folder('three')
        self.assertEqual(len(self._box.list_folders()), 2)
        self.assertEqual(set(self._box.list_folders()), set(('two', 'three')))
        self._box.remove_folder('three')
        self.assertEqual(len(self._box.list_folders()), 1)
        self.assertEqual(set(self._box.list_folders()), set(('two',)))
        self._box.remove_folder('two')
        self.assertEqual(len(self._box.list_folders()), 0)
        self.assertEqual(self._box.list_folders(), [])

    def test_sequences(self):
        # Get and set sequences
        self.assertEqual(self._box.get_sequences(), {})
        msg0 = mailbox.MHMessage(self._template % 0)
        msg0.add_sequence('foo')
        key0 = self._box.add(msg0)
        self.assertEqual(self._box.get_sequences(), {'foo':[key0]})
        msg1 = mailbox.MHMessage(self._template % 1)
        msg1.set_sequences(['bar', 'replied', 'foo'])
        key1 = self._box.add(msg1)
        self.assertEqual(self._box.get_sequences(),
                     {'foo':[key0, key1], 'bar':[key1], 'replied':[key1]})
        msg0.set_sequences(['flagged'])
        self._box[key0] = msg0
        self.assertEqual(self._box.get_sequences(),
                     {'foo':[key1], 'bar':[key1], 'replied':[key1],
                      'flagged':[key0]})
        self._box.remove(key1)
        self.assertEqual(self._box.get_sequences(), {'flagged':[key0]})

    def test_pack(self):
        # Pack the contents of the mailbox
        msg0 = mailbox.MHMessage(self._template % 0)
        msg1 = mailbox.MHMessage(self._template % 1)
        msg2 = mailbox.MHMessage(self._template % 2)
        msg3 = mailbox.MHMessage(self._template % 3)
        msg0.set_sequences(['foo', 'unseen'])
        msg1.set_sequences(['foo'])
        msg2.set_sequences(['foo', 'flagged'])
        msg3.set_sequences(['foo', 'bar', 'replied'])
        key0 = self._box.add(msg0)
        key1 = self._box.add(msg1)
        key2 = self._box.add(msg2)
        key3 = self._box.add(msg3)
        self.assertEqual(self._box.get_sequences(),
                     {'foo':[key0,key1,key2,key3], 'unseen':[key0],
                      'flagged':[key2], 'bar':[key3], 'replied':[key3]})
        self._box.remove(key2)
        self.assertEqual(self._box.get_sequences(),
                     {'foo':[key0,key1,key3], 'unseen':[key0], 'bar':[key3],
                      'replied':[key3]})
        self._box.pack()
        self.assert_(self._box.keys() == [1, 2, 3])
        key0 = key0
        key1 = key0 + 1
        key2 = key1 + 1
        self.assertEqual(self._box.get_sequences(),
                     {'foo':[1, 2, 3], 'unseen':[1], 'bar':[3], 'replied':[3]})

        # Test case for packing while holding the mailbox locked.
        key0 = self._box.add(msg1)
        key1 = self._box.add(msg1)
        key2 = self._box.add(msg1)
        key3 = self._box.add(msg1)

        self._box.remove(key0)
        self._box.remove(key2)
        self._box.lock()
        self._box.pack()
        self._box.unlock()
        self.assertEqual(self._box.get_sequences(),
                     {'foo':[1, 2, 3, 4, 5],
                      'unseen':[1], 'bar':[3], 'replied':[3]})

    def _get_lock_path(self):
        return os.path.join(self._path, '.mh_sequences.lock')


class TestBabyl(TestMailbox):

    _factory = lambda self, path, factory=None: mailbox.Babyl(path, factory)

    def tearDown(self):
        self._delete_recursively(self._path)
        for lock_remnant in glob.glob(self._path + '.*'):
            test_support.unlink(lock_remnant)

    def test_labels(self):
        # Get labels from the mailbox
        self.assertEqual(self._box.get_labels(), [])
        msg0 = mailbox.BabylMessage(self._template % 0)
        msg0.add_label('foo')
        key0 = self._box.add(msg0)
        self.assertEqual(self._box.get_labels(), ['foo'])
        msg1 = mailbox.BabylMessage(self._template % 1)
        msg1.set_labels(['bar', 'answered', 'foo'])
        key1 = self._box.add(msg1)
        self.assertEqual(set(self._box.get_labels()), set(['foo', 'bar']))
        msg0.set_labels(['blah', 'filed'])
        self._box[key0] = msg0
        self.assertEqual(set(self._box.get_labels()),
                     set(['foo', 'bar', 'blah']))
        self._box.remove(key1)
        self.assertEqual(set(self._box.get_labels()), set(['blah']))


class TestMessage(TestBase):

    _factory = mailbox.Message      # Overridden by subclasses to reuse tests

    def setUp(self):
        self._path = test_support.TESTFN

    def tearDown(self):
        self._delete_recursively(self._path)

    def test_initialize_with_eMM(self):
        # Initialize based on email.message.Message instance
        eMM = email.message_from_string(_sample_message)
        msg = self._factory(eMM)
        self._post_initialize_hook(msg)
        self._check_sample(msg)

    def test_initialize_with_string(self):
        # Initialize based on string
        msg = self._factory(_sample_message)
        self._post_initialize_hook(msg)
        self._check_sample(msg)

    def test_initialize_with_file(self):
        # Initialize based on contents of file
        f = open(self._path, 'w+')
        f.write(_sample_message)
        f.seek(0)
        msg = self._factory(f)
        self._post_initialize_hook(msg)
        self._check_sample(msg)
        f.close()

    def test_initialize_with_nothing(self):
        # Initialize without arguments
        msg = self._factory()
        self._post_initialize_hook(msg)
        self.assert_(isinstance(msg, email.message.Message))
        self.assert_(isinstance(msg, mailbox.Message))
        self.assert_(isinstance(msg, self._factory))
        self.assertEqual(msg.keys(), [])
        self.assert_(not msg.is_multipart())
        self.assertEqual(msg.get_payload(), None)

    def test_initialize_incorrectly(self):
        # Initialize with invalid argument
        self.assertRaises(TypeError, lambda: self._factory(object()))

    def test_become_message(self):
        # Take on the state of another message
        eMM = email.message_from_string(_sample_message)
        msg = self._factory()
        msg._become_message(eMM)
        self._check_sample(msg)

    def test_explain_to(self):
        # Copy self's format-specific data to other message formats.
        # This test is superficial; better ones are in TestMessageConversion.
        msg = self._factory()
        for class_ in (mailbox.Message, mailbox.MaildirMessage,
                       mailbox.mboxMessage, mailbox.MHMessage,
                       mailbox.BabylMessage, mailbox.MMDFMessage):
            other_msg = class_()
            msg._explain_to(other_msg)
        other_msg = email.message.Message()
        self.assertRaises(TypeError, lambda: msg._explain_to(other_msg))

    def _post_initialize_hook(self, msg):
        # Overridden by subclasses to check extra things after initialization
        pass


class TestMaildirMessage(TestMessage):

    _factory = mailbox.MaildirMessage

    def _post_initialize_hook(self, msg):
        self.assertEqual(msg._subdir, 'new')
        self.assertEqual(msg._info, '')

    def test_subdir(self):
        # Use get_subdir() and set_subdir()
        msg = mailbox.MaildirMessage(_sample_message)
        self.assertEqual(msg.get_subdir(), 'new')
        msg.set_subdir('cur')
        self.assertEqual(msg.get_subdir(), 'cur')
        msg.set_subdir('new')
        self.assertEqual(msg.get_subdir(), 'new')
        self.assertRaises(ValueError, lambda: msg.set_subdir('tmp'))
        self.assertEqual(msg.get_subdir(), 'new')
        msg.set_subdir('new')
        self.assertEqual(msg.get_subdir(), 'new')
        self._check_sample(msg)

    def test_flags(self):
        # Use get_flags(), set_flags(), add_flag(), remove_flag()
        msg = mailbox.MaildirMessage(_sample_message)
        self.assertEqual(msg.get_flags(), '')
        self.assertEqual(msg.get_subdir(), 'new')
        msg.set_flags('F')
        self.assertEqual(msg.get_subdir(), 'new')
        self.assertEqual(msg.get_flags(), 'F')
        msg.set_flags('SDTP')
        self.assertEqual(msg.get_flags(), 'DPST')
        msg.add_flag('FT')
        self.assertEqual(msg.get_flags(), 'DFPST')
        msg.remove_flag('TDRP')
        self.assertEqual(msg.get_flags(), 'FS')
        self.assertEqual(msg.get_subdir(), 'new')
        self._check_sample(msg)

    def test_date(self):
        # Use get_date() and set_date()
        msg = mailbox.MaildirMessage(_sample_message)
        self.assert_(abs(msg.get_date() - time.time()) < 60)
        msg.set_date(0.0)
        self.assertEqual(msg.get_date(), 0.0)

    def test_info(self):
        # Use get_info() and set_info()
        msg = mailbox.MaildirMessage(_sample_message)
        self.assertEqual(msg.get_info(), '')
        msg.set_info('1,foo=bar')
        self.assertEqual(msg.get_info(), '1,foo=bar')
        self.assertRaises(TypeError, lambda: msg.set_info(None))
        self._check_sample(msg)

    def test_info_and_flags(self):
        # Test interaction of info and flag methods
        msg = mailbox.MaildirMessage(_sample_message)
        self.assertEqual(msg.get_info(), '')
        msg.set_flags('SF')
        self.assertEqual(msg.get_flags(), 'FS')
        self.assertEqual(msg.get_info(), '2,FS')
        msg.set_info('1,')
        self.assertEqual(msg.get_flags(), '')
        self.assertEqual(msg.get_info(), '1,')
        msg.remove_flag('RPT')
        self.assertEqual(msg.get_flags(), '')
        self.assertEqual(msg.get_info(), '1,')
        msg.add_flag('D')
        self.assertEqual(msg.get_flags(), 'D')
        self.assertEqual(msg.get_info(), '2,D')
        self._check_sample(msg)


class _TestMboxMMDFMessage(TestMessage):

    _factory = mailbox._mboxMMDFMessage

    def _post_initialize_hook(self, msg):
        self._check_from(msg)

    def test_initialize_with_unixfrom(self):
        # Initialize with a message that already has a _unixfrom attribute
        msg = mailbox.Message(_sample_message)
        msg.set_unixfrom('From foo@bar blah')
        msg = mailbox.mboxMessage(msg)
        self.assertEqual(msg.get_from(), 'foo@bar blah', msg.get_from())

    def test_from(self):
        # Get and set "From " line
        msg = mailbox.mboxMessage(_sample_message)
        self._check_from(msg)
        msg.set_from('foo bar')
        self.assertEqual(msg.get_from(), 'foo bar')
        msg.set_from('foo@bar', True)
        self._check_from(msg, 'foo@bar')
        msg.set_from('blah@temp', time.localtime())
        self._check_from(msg, 'blah@temp')

    def test_flags(self):
        # Use get_flags(), set_flags(), add_flag(), remove_flag()
        msg = mailbox.mboxMessage(_sample_message)
        self.assertEqual(msg.get_flags(), '')
        msg.set_flags('F')
        self.assertEqual(msg.get_flags(), 'F')
        msg.set_flags('XODR')
        self.assertEqual(msg.get_flags(), 'RODX')
        msg.add_flag('FA')
        self.assertEqual(msg.get_flags(), 'RODFAX')
        msg.remove_flag('FDXA')
        self.assertEqual(msg.get_flags(), 'RO')
        self._check_sample(msg)

    def _check_from(self, msg, sender=None):
        # Check contents of "From " line
        if sender is None:
            sender = "MAILER-DAEMON"
        self.assert_(re.match(sender + r" \w{3} \w{3} [\d ]\d [\d ]\d:\d{2}:"
                              r"\d{2} \d{4}", msg.get_from()) is not None)


class TestMboxMessage(_TestMboxMMDFMessage):

    _factory = mailbox.mboxMessage


class TestMHMessage(TestMessage):

    _factory = mailbox.MHMessage

    def _post_initialize_hook(self, msg):
        self.assertEqual(msg._sequences, [])

    def test_sequences(self):
        # Get, set, join, and leave sequences
        msg = mailbox.MHMessage(_sample_message)
        self.assertEqual(msg.get_sequences(), [])
        msg.set_sequences(['foobar'])
        self.assertEqual(msg.get_sequences(), ['foobar'])
        msg.set_sequences([])
        self.assertEqual(msg.get_sequences(), [])
        msg.add_sequence('unseen')
        self.assertEqual(msg.get_sequences(), ['unseen'])
        msg.add_sequence('flagged')
        self.assertEqual(msg.get_sequences(), ['unseen', 'flagged'])
        msg.add_sequence('flagged')
        self.assertEqual(msg.get_sequences(), ['unseen', 'flagged'])
        msg.remove_sequence('unseen')
        self.assertEqual(msg.get_sequences(), ['flagged'])
        msg.add_sequence('foobar')
        self.assertEqual(msg.get_sequences(), ['flagged', 'foobar'])
        msg.remove_sequence('replied')
        self.assertEqual(msg.get_sequences(), ['flagged', 'foobar'])
        msg.set_sequences(['foobar', 'replied'])
        self.assertEqual(msg.get_sequences(), ['foobar', 'replied'])


class TestBabylMessage(TestMessage):

    _factory = mailbox.BabylMessage

    def _post_initialize_hook(self, msg):
        self.assertEqual(msg._labels, [])

    def test_labels(self):
        # Get, set, join, and leave labels
        msg = mailbox.BabylMessage(_sample_message)
        self.assertEqual(msg.get_labels(), [])
        msg.set_labels(['foobar'])
        self.assertEqual(msg.get_labels(), ['foobar'])
        msg.set_labels([])
        self.assertEqual(msg.get_labels(), [])
        msg.add_label('filed')
        self.assertEqual(msg.get_labels(), ['filed'])
        msg.add_label('resent')
        self.assertEqual(msg.get_labels(), ['filed', 'resent'])
        msg.add_label('resent')
        self.assertEqual(msg.get_labels(), ['filed', 'resent'])
        msg.remove_label('filed')
        self.assertEqual(msg.get_labels(), ['resent'])
        msg.add_label('foobar')
        self.assertEqual(msg.get_labels(), ['resent', 'foobar'])
        msg.remove_label('unseen')
        self.assertEqual(msg.get_labels(), ['resent', 'foobar'])
        msg.set_labels(['foobar', 'answered'])
        self.assertEqual(msg.get_labels(), ['foobar', 'answered'])

    def test_visible(self):
        # Get, set, and update visible headers
        msg = mailbox.BabylMessage(_sample_message)
        visible = msg.get_visible()
        self.assertEqual(visible.keys(), [])
        self.assert_(visible.get_payload() is None)
        visible['User-Agent'] = 'FooBar 1.0'
        visible['X-Whatever'] = 'Blah'
        self.assertEqual(msg.get_visible().keys(), [])
        msg.set_visible(visible)
        visible = msg.get_visible()
        self.assert_(visible.keys() == ['User-Agent', 'X-Whatever'])
        self.assert_(visible['User-Agent'] == 'FooBar 1.0')
        self.assertEqual(visible['X-Whatever'], 'Blah')
        self.assert_(visible.get_payload() is None)
        msg.update_visible()
        self.assertEqual(visible.keys(), ['User-Agent', 'X-Whatever'])
        self.assert_(visible.get_payload() is None)
        visible = msg.get_visible()
        self.assertEqual(visible.keys(), ['User-Agent', 'Date', 'From', 'To',
                                          'Subject'])
        for header in ('User-Agent', 'Date', 'From', 'To', 'Subject'):
            self.assertEqual(visible[header], msg[header])


class TestMMDFMessage(_TestMboxMMDFMessage):

    _factory = mailbox.MMDFMessage


class TestMessageConversion(TestBase):

    def test_plain_to_x(self):
        # Convert Message to all formats
        for class_ in (mailbox.Message, mailbox.MaildirMessage,
                       mailbox.mboxMessage, mailbox.MHMessage,
                       mailbox.BabylMessage, mailbox.MMDFMessage):
            msg_plain = mailbox.Message(_sample_message)
            msg = class_(msg_plain)
            self._check_sample(msg)

    def test_x_to_plain(self):
        # Convert all formats to Message
        for class_ in (mailbox.Message, mailbox.MaildirMessage,
                       mailbox.mboxMessage, mailbox.MHMessage,
                       mailbox.BabylMessage, mailbox.MMDFMessage):
            msg = class_(_sample_message)
            msg_plain = mailbox.Message(msg)
            self._check_sample(msg_plain)

    def test_x_to_invalid(self):
        # Convert all formats to an invalid format
        for class_ in (mailbox.Message, mailbox.MaildirMessage,
                       mailbox.mboxMessage, mailbox.MHMessage,
                       mailbox.BabylMessage, mailbox.MMDFMessage):
            self.assertRaises(TypeError, lambda: class_(False))

    def test_maildir_to_maildir(self):
        # Convert MaildirMessage to MaildirMessage
        msg_maildir = mailbox.MaildirMessage(_sample_message)
        msg_maildir.set_flags('DFPRST')
        msg_maildir.set_subdir('cur')
        date = msg_maildir.get_date()
        msg = mailbox.MaildirMessage(msg_maildir)
        self._check_sample(msg)
        self.assertEqual(msg.get_flags(), 'DFPRST')
        self.assertEqual(msg.get_subdir(), 'cur')
        self.assertEqual(msg.get_date(), date)

    def test_maildir_to_mboxmmdf(self):
        # Convert MaildirMessage to mboxmessage and MMDFMessage
        pairs = (('D', ''), ('F', 'F'), ('P', ''), ('R', 'A'), ('S', 'R'),
                 ('T', 'D'), ('DFPRST', 'RDFA'))
        for class_ in (mailbox.mboxMessage, mailbox.MMDFMessage):
            msg_maildir = mailbox.MaildirMessage(_sample_message)
            msg_maildir.set_date(0.0)
            for setting, result in pairs:
                msg_maildir.set_flags(setting)
                msg = class_(msg_maildir)
                self.assertEqual(msg.get_flags(), result)
                self.assertEqual(msg.get_from(), 'MAILER-DAEMON %s' %
                             time.asctime(time.gmtime(0.0)))
            msg_maildir.set_subdir('cur')
            self.assertEqual(class_(msg_maildir).get_flags(), 'RODFA')

    def test_maildir_to_mh(self):
        # Convert MaildirMessage to MHMessage
        msg_maildir = mailbox.MaildirMessage(_sample_message)
        pairs = (('D', ['unseen']), ('F', ['unseen', 'flagged']),
                 ('P', ['unseen']), ('R', ['unseen', 'replied']), ('S', []),
                 ('T', ['unseen']), ('DFPRST', ['replied', 'flagged']))
        for setting, result in pairs:
            msg_maildir.set_flags(setting)
            self.assertEqual(mailbox.MHMessage(msg_maildir).get_sequences(),
                             result)

    def test_maildir_to_babyl(self):
        # Convert MaildirMessage to Babyl
        msg_maildir = mailbox.MaildirMessage(_sample_message)
        pairs = (('D', ['unseen']), ('F', ['unseen']),
                 ('P', ['unseen', 'forwarded']), ('R', ['unseen', 'answered']),
                 ('S', []), ('T', ['unseen', 'deleted']),
                 ('DFPRST', ['deleted', 'answered', 'forwarded']))
        for setting, result in pairs:
            msg_maildir.set_flags(setting)
            self.assertEqual(mailbox.BabylMessage(msg_maildir).get_labels(),
                             result)

    def test_mboxmmdf_to_maildir(self):
        # Convert mboxMessage and MMDFMessage to MaildirMessage
        for class_ in (mailbox.mboxMessage, mailbox.MMDFMessage):
            msg_mboxMMDF = class_(_sample_message)
            msg_mboxMMDF.set_from('foo@bar', time.gmtime(0.0))
            pairs = (('R', 'S'), ('O', ''), ('D', 'T'), ('F', 'F'), ('A', 'R'),
                     ('RODFA', 'FRST'))
            for setting, result in pairs:
                msg_mboxMMDF.set_flags(setting)
                msg = mailbox.MaildirMessage(msg_mboxMMDF)
                self.assertEqual(msg.get_flags(), result)
                self.assert_(msg.get_date() == 0.0, msg.get_date())
            msg_mboxMMDF.set_flags('O')
            self.assertEqual(mailbox.MaildirMessage(msg_mboxMMDF).get_subdir(),
                             'cur')

    def test_mboxmmdf_to_mboxmmdf(self):
        # Convert mboxMessage and MMDFMessage to mboxMessage and MMDFMessage
        for class_ in (mailbox.mboxMessage, mailbox.MMDFMessage):
            msg_mboxMMDF = class_(_sample_message)
            msg_mboxMMDF.set_flags('RODFA')
            msg_mboxMMDF.set_from('foo@bar')
            for class2_ in (mailbox.mboxMessage, mailbox.MMDFMessage):
                msg2 = class2_(msg_mboxMMDF)
                self.assertEqual(msg2.get_flags(), 'RODFA')
                self.assertEqual(msg2.get_from(), 'foo@bar')

    def test_mboxmmdf_to_mh(self):
        # Convert mboxMessage and MMDFMessage to MHMessage
        for class_ in (mailbox.mboxMessage, mailbox.MMDFMessage):
            msg_mboxMMDF = class_(_sample_message)
            pairs = (('R', []), ('O', ['unseen']), ('D', ['unseen']),
                     ('F', ['unseen', 'flagged']),
                     ('A', ['unseen', 'replied']),
                     ('RODFA', ['replied', 'flagged']))
            for setting, result in pairs:
                msg_mboxMMDF.set_flags(setting)
                self.assertEqual(mailbox.MHMessage(msg_mboxMMDF).get_sequences(),
                                 result)

    def test_mboxmmdf_to_babyl(self):
        # Convert mboxMessage and MMDFMessage to BabylMessage
        for class_ in (mailbox.mboxMessage, mailbox.MMDFMessage):
            msg = class_(_sample_message)
            pairs = (('R', []), ('O', ['unseen']),
                     ('D', ['unseen', 'deleted']), ('F', ['unseen']),
                     ('A', ['unseen', 'answered']),
                     ('RODFA', ['deleted', 'answered']))
            for setting, result in pairs:
                msg.set_flags(setting)
                self.assertEqual(mailbox.BabylMessage(msg).get_labels(), result)

    def test_mh_to_maildir(self):
        # Convert MHMessage to MaildirMessage
        pairs = (('unseen', ''), ('replied', 'RS'), ('flagged', 'FS'))
        for setting, result in pairs:
            msg = mailbox.MHMessage(_sample_message)
            msg.add_sequence(setting)
            self.assertEqual(mailbox.MaildirMessage(msg).get_flags(), result)
            self.assertEqual(mailbox.MaildirMessage(msg).get_subdir(), 'cur')
        msg = mailbox.MHMessage(_sample_message)
        msg.add_sequence('unseen')
        msg.add_sequence('replied')
        msg.add_sequence('flagged')
        self.assertEqual(mailbox.MaildirMessage(msg).get_flags(), 'FR')
        self.assertEqual(mailbox.MaildirMessage(msg).get_subdir(), 'cur')

    def test_mh_to_mboxmmdf(self):
        # Convert MHMessage to mboxMessage and MMDFMessage
        pairs = (('unseen', 'O'), ('replied', 'ROA'), ('flagged', 'ROF'))
        for setting, result in pairs:
            msg = mailbox.MHMessage(_sample_message)
            msg.add_sequence(setting)
            for class_ in (mailbox.mboxMessage, mailbox.MMDFMessage):
                self.assertEqual(class_(msg).get_flags(), result)
        msg = mailbox.MHMessage(_sample_message)
        msg.add_sequence('unseen')
        msg.add_sequence('replied')
        msg.add_sequence('flagged')
        for class_ in (mailbox.mboxMessage, mailbox.MMDFMessage):
            self.assertEqual(class_(msg).get_flags(), 'OFA')

    def test_mh_to_mh(self):
        # Convert MHMessage to MHMessage
        msg = mailbox.MHMessage(_sample_message)
        msg.add_sequence('unseen')
        msg.add_sequence('replied')
        msg.add_sequence('flagged')
        self.assertEqual(mailbox.MHMessage(msg).get_sequences(),
                         ['unseen', 'replied', 'flagged'])

    def test_mh_to_babyl(self):
        # Convert MHMessage to BabylMessage
        pairs = (('unseen', ['unseen']), ('replied', ['answered']),
                 ('flagged', []))
        for setting, result in pairs:
            msg = mailbox.MHMessage(_sample_message)
            msg.add_sequence(setting)
            self.assertEqual(mailbox.BabylMessage(msg).get_labels(), result)
        msg = mailbox.MHMessage(_sample_message)
        msg.add_sequence('unseen')
        msg.add_sequence('replied')
        msg.add_sequence('flagged')
        self.assertEqual(mailbox.BabylMessage(msg).get_labels(),
                         ['unseen', 'answered'])

    def test_babyl_to_maildir(self):
        # Convert BabylMessage to MaildirMessage
        pairs = (('unseen', ''), ('deleted', 'ST'), ('filed', 'S'),
                 ('answered', 'RS'), ('forwarded', 'PS'), ('edited', 'S'),
                 ('resent', 'PS'))
        for setting, result in pairs:
            msg = mailbox.BabylMessage(_sample_message)
            msg.add_label(setting)
            self.assertEqual(mailbox.MaildirMessage(msg).get_flags(), result)
            self.assertEqual(mailbox.MaildirMessage(msg).get_subdir(), 'cur')
        msg = mailbox.BabylMessage(_sample_message)
        for label in ('unseen', 'deleted', 'filed', 'answered', 'forwarded',
                      'edited', 'resent'):
            msg.add_label(label)
        self.assertEqual(mailbox.MaildirMessage(msg).get_flags(), 'PRT')
        self.assertEqual(mailbox.MaildirMessage(msg).get_subdir(), 'cur')

    def test_babyl_to_mboxmmdf(self):
        # Convert BabylMessage to mboxMessage and MMDFMessage
        pairs = (('unseen', 'O'), ('deleted', 'ROD'), ('filed', 'RO'),
                 ('answered', 'ROA'), ('forwarded', 'RO'), ('edited', 'RO'),
                 ('resent', 'RO'))
        for setting, result in pairs:
            for class_ in (mailbox.mboxMessage, mailbox.MMDFMessage):
                msg = mailbox.BabylMessage(_sample_message)
                msg.add_label(setting)
                self.assertEqual(class_(msg).get_flags(), result)
        msg = mailbox.BabylMessage(_sample_message)
        for label in ('unseen', 'deleted', 'filed', 'answered', 'forwarded',
                      'edited', 'resent'):
            msg.add_label(label)
        for class_ in (mailbox.mboxMessage, mailbox.MMDFMessage):
            self.assertEqual(class_(msg).get_flags(), 'ODA')

    def test_babyl_to_mh(self):
        # Convert BabylMessage to MHMessage
        pairs = (('unseen', ['unseen']), ('deleted', []), ('filed', []),
                 ('answered', ['replied']), ('forwarded', []), ('edited', []),
                 ('resent', []))
        for setting, result in pairs:
            msg = mailbox.BabylMessage(_sample_message)
            msg.add_label(setting)
            self.assertEqual(mailbox.MHMessage(msg).get_sequences(), result)
        msg = mailbox.BabylMessage(_sample_message)
        for label in ('unseen', 'deleted', 'filed', 'answered', 'forwarded',
                      'edited', 'resent'):
            msg.add_label(label)
        self.assertEqual(mailbox.MHMessage(msg).get_sequences(),
                         ['unseen', 'replied'])

    def test_babyl_to_babyl(self):
        # Convert BabylMessage to BabylMessage
        msg = mailbox.BabylMessage(_sample_message)
        msg.update_visible()
        for label in ('unseen', 'deleted', 'filed', 'answered', 'forwarded',
                      'edited', 'resent'):
            msg.add_label(label)
        msg2 = mailbox.BabylMessage(msg)
        self.assertEqual(msg2.get_labels(), ['unseen', 'deleted', 'filed',
                                             'answered', 'forwarded', 'edited',
                                             'resent'])
        self.assertEqual(msg.get_visible().keys(), msg2.get_visible().keys())
        for key in msg.get_visible().keys():
            self.assertEqual(msg.get_visible()[key], msg2.get_visible()[key])


class TestProxyFileBase(TestBase):

    def _test_read(self, proxy):
        # Read by byte
        proxy.seek(0)
        self.assertEqual(proxy.read(), 'bar')
        proxy.seek(1)
        self.assertEqual(proxy.read(), 'ar')
        proxy.seek(0)
        self.assertEqual(proxy.read(2), 'ba')
        proxy.seek(1)
        self.assertEqual(proxy.read(-1), 'ar')
        proxy.seek(2)
        self.assertEqual(proxy.read(1000), 'r')

    def _test_readline(self, proxy):
        # Read by line
        proxy.seek(0)
        self.assertEqual(proxy.readline(), 'foo' + os.linesep)
        self.assertEqual(proxy.readline(), 'bar' + os.linesep)
        self.assertEqual(proxy.readline(), 'fred' + os.linesep)
        self.assertEqual(proxy.readline(), 'bob')
        proxy.seek(2)
        self.assertEqual(proxy.readline(), 'o' + os.linesep)
        proxy.seek(6 + 2 * len(os.linesep))
        self.assertEqual(proxy.readline(), 'fred' + os.linesep)
        proxy.seek(6 + 2 * len(os.linesep))
        self.assertEqual(proxy.readline(2), 'fr')
        self.assertEqual(proxy.readline(-10), 'ed' + os.linesep)

    def _test_readlines(self, proxy):
        # Read multiple lines
        proxy.seek(0)
        self.assertEqual(proxy.readlines(), ['foo' + os.linesep,
                                           'bar' + os.linesep,
                                           'fred' + os.linesep, 'bob'])
        proxy.seek(0)
        self.assertEqual(proxy.readlines(2), ['foo' + os.linesep])
        proxy.seek(3 + len(os.linesep))
        self.assertEqual(proxy.readlines(4 + len(os.linesep)),
                     ['bar' + os.linesep, 'fred' + os.linesep])
        proxy.seek(3)
        self.assertEqual(proxy.readlines(1000), [os.linesep, 'bar' + os.linesep,
                                               'fred' + os.linesep, 'bob'])

    def _test_iteration(self, proxy):
        # Iterate by line
        proxy.seek(0)
        iterator = iter(proxy)
        self.assertEqual(next(iterator), 'foo' + os.linesep)
        self.assertEqual(next(iterator), 'bar' + os.linesep)
        self.assertEqual(next(iterator), 'fred' + os.linesep)
        self.assertEqual(next(iterator), 'bob')
        self.assertRaises(StopIteration, next, iterator)

    def _test_seek_and_tell(self, proxy):
        # Seek and use tell to check position
        proxy.seek(3)
        self.assertEqual(proxy.tell(), 3)
        self.assertEqual(proxy.read(len(os.linesep)), os.linesep)
        proxy.seek(2, 1)
        self.assertEqual(proxy.read(1 + len(os.linesep)), 'r' + os.linesep)
        proxy.seek(-3 - len(os.linesep), 2)
        self.assertEqual(proxy.read(3), 'bar')
        proxy.seek(2, 0)
        self.assertEqual(proxy.read(), 'o' + os.linesep + 'bar' + os.linesep)
        proxy.seek(100)
        self.assertEqual(proxy.read(), '')

    def _test_close(self, proxy):
        # Close a file
        proxy.close()
        self.assertRaises(AttributeError, lambda: proxy.close())


class TestProxyFile(TestProxyFileBase):

    def setUp(self):
        self._path = test_support.TESTFN
        self._file = open(self._path, 'wb+')

    def tearDown(self):
        self._file.close()
        self._delete_recursively(self._path)

    def test_initialize(self):
        # Initialize and check position
        self._file.write('foo')
        pos = self._file.tell()
        proxy0 = mailbox._ProxyFile(self._file)
        self.assertEqual(proxy0.tell(), pos)
        self.assertEqual(self._file.tell(), pos)
        proxy1 = mailbox._ProxyFile(self._file, 0)
        self.assertEqual(proxy1.tell(), 0)
        self.assertEqual(self._file.tell(), pos)

    def test_read(self):
        self._file.write('bar')
        self._test_read(mailbox._ProxyFile(self._file))

    def test_readline(self):
        self._file.write('foo%sbar%sfred%sbob' % (os.linesep, os.linesep,
                                                  os.linesep))
        self._test_readline(mailbox._ProxyFile(self._file))

    def test_readlines(self):
        self._file.write('foo%sbar%sfred%sbob' % (os.linesep, os.linesep,
                                                  os.linesep))
        self._test_readlines(mailbox._ProxyFile(self._file))

    def test_iteration(self):
        self._file.write('foo%sbar%sfred%sbob' % (os.linesep, os.linesep,
                                                  os.linesep))
        self._test_iteration(mailbox._ProxyFile(self._file))

    def test_seek_and_tell(self):
        self._file.write('foo%sbar%s' % (os.linesep, os.linesep))
        self._test_seek_and_tell(mailbox._ProxyFile(self._file))

    def test_close(self):
        self._file.write('foo%sbar%s' % (os.linesep, os.linesep))
        self._test_close(mailbox._ProxyFile(self._file))


class TestPartialFile(TestProxyFileBase):

    def setUp(self):
        self._path = test_support.TESTFN
        self._file = open(self._path, 'wb+')

    def tearDown(self):
        self._file.close()
        self._delete_recursively(self._path)

    def test_initialize(self):
        # Initialize and check position
        self._file.write('foo' + os.linesep + 'bar')
        pos = self._file.tell()
        proxy = mailbox._PartialFile(self._file, 2, 5)
        self.assertEqual(proxy.tell(), 0)
        self.assertEqual(self._file.tell(), pos)

    def test_read(self):
        self._file.write('***bar***')
        self._test_read(mailbox._PartialFile(self._file, 3, 6))

    def test_readline(self):
        self._file.write('!!!!!foo%sbar%sfred%sbob!!!!!' %
                         (os.linesep, os.linesep, os.linesep))
        self._test_readline(mailbox._PartialFile(self._file, 5,
                                                 18 + 3 * len(os.linesep)))

    def test_readlines(self):
        self._file.write('foo%sbar%sfred%sbob?????' %
                         (os.linesep, os.linesep, os.linesep))
        self._test_readlines(mailbox._PartialFile(self._file, 0,
                                                  13 + 3 * len(os.linesep)))

    def test_iteration(self):
        self._file.write('____foo%sbar%sfred%sbob####' %
                         (os.linesep, os.linesep, os.linesep))
        self._test_iteration(mailbox._PartialFile(self._file, 4,
                                                  17 + 3 * len(os.linesep)))

    def test_seek_and_tell(self):
        self._file.write('(((foo%sbar%s$$$' % (os.linesep, os.linesep))
        self._test_seek_and_tell(mailbox._PartialFile(self._file, 3,
                                                      9 + 2 * len(os.linesep)))

    def test_close(self):
        self._file.write('&foo%sbar%s^' % (os.linesep, os.linesep))
        self._test_close(mailbox._PartialFile(self._file, 1,
                                              6 + 3 * len(os.linesep)))


## Start: tests from the original module (for backward compatibility).

FROM_ = "From some.body@dummy.domain  Sat Jul 24 13:43:35 2004\n"
DUMMY_MESSAGE = """\
From: some.body@dummy.domain
To: me@my.domain
Subject: Simple Test

This is a dummy message.
"""

class MaildirTestCase(unittest.TestCase):

    def setUp(self):
        # create a new maildir mailbox to work with:
        self._dir = test_support.TESTFN
        os.mkdir(self._dir)
        os.mkdir(os.path.join(self._dir, "cur"))
        os.mkdir(os.path.join(self._dir, "tmp"))
        os.mkdir(os.path.join(self._dir, "new"))
        self._counter = 1
        self._msgfiles = []

    def tearDown(self):
        list(map(os.unlink, self._msgfiles))
        os.rmdir(os.path.join(self._dir, "cur"))
        os.rmdir(os.path.join(self._dir, "tmp"))
        os.rmdir(os.path.join(self._dir, "new"))
        os.rmdir(self._dir)

    def createMessage(self, dir, mbox=False):
        t = int(time.time() % 1000000)
        pid = self._counter
        self._counter += 1
        filename = os.extsep.join((str(t), str(pid), "myhostname", "mydomain"))
        tmpname = os.path.join(self._dir, "tmp", filename)
        newname = os.path.join(self._dir, dir, filename)
        fp = open(tmpname, "w")
        self._msgfiles.append(tmpname)
        if mbox:
            fp.write(FROM_)
        fp.write(DUMMY_MESSAGE)
        fp.close()
        if hasattr(os, "link"):
            os.link(tmpname, newname)
        else:
            fp = open(newname, "w")
            fp.write(DUMMY_MESSAGE)
            fp.close()
        self._msgfiles.append(newname)
        return tmpname

    def test_empty_maildir(self):
        """Test an empty maildir mailbox"""
        # Test for regression on bug #117490:
        # Make sure the boxes attribute actually gets set.
        self.mbox = mailbox.Maildir(test_support.TESTFN)
        #self.assert_(hasattr(self.mbox, "boxes"))
        #self.assertEqual(len(self.mbox.boxes), 0)
        self.assert_(self.mbox.next() is None)
        self.assert_(self.mbox.next() is None)

    def test_nonempty_maildir_cur(self):
        self.createMessage("cur")
        self.mbox = mailbox.Maildir(test_support.TESTFN)
        #self.assertEqual(len(self.mbox.boxes), 1)
        self.assert_(self.mbox.next() is not None)
        self.assert_(self.mbox.next() is None)
        self.assert_(self.mbox.next() is None)

    def test_nonempty_maildir_new(self):
        self.createMessage("new")
        self.mbox = mailbox.Maildir(test_support.TESTFN)
        #self.assertEqual(len(self.mbox.boxes), 1)
        self.assert_(self.mbox.next() is not None)
        self.assert_(self.mbox.next() is None)
        self.assert_(self.mbox.next() is None)

    def test_nonempty_maildir_both(self):
        self.createMessage("cur")
        self.createMessage("new")
        self.mbox = mailbox.Maildir(test_support.TESTFN)
        #self.assertEqual(len(self.mbox.boxes), 2)
        self.assert_(self.mbox.next() is not None)
        self.assert_(self.mbox.next() is not None)
        self.assert_(self.mbox.next() is None)
        self.assert_(self.mbox.next() is None)

    def test_unix_mbox(self):
        ### should be better!
        import email.parser
        fname = self.createMessage("cur", True)
        n = 0
        for msg in mailbox.PortableUnixMailbox(open(fname),
                                               email.parser.Parser().parse):
            n += 1
            self.assertEqual(msg["subject"], "Simple Test")
            self.assertEqual(len(str(msg)), len(FROM_)+len(DUMMY_MESSAGE))
        self.assertEqual(n, 1)

## End: classes from the original module (for backward compatibility).


_sample_message = """\
Return-Path: <gkj@gregorykjohnson.com>
X-Original-To: gkj+person@localhost
Delivered-To: gkj+person@localhost
Received: from localhost (localhost [127.0.0.1])
        by andy.gregorykjohnson.com (Postfix) with ESMTP id 356ED9DD17
        for <gkj+person@localhost>; Wed, 13 Jul 2005 17:23:16 -0400 (EDT)
Delivered-To: gkj@sundance.gregorykjohnson.com
Received: from localhost [127.0.0.1]
        by localhost with POP3 (fetchmail-6.2.5)
        for gkj+person@localhost (single-drop); Wed, 13 Jul 2005 17:23:16 -0400 (EDT)
Received: from andy.gregorykjohnson.com (andy.gregorykjohnson.com [64.32.235.228])
        by sundance.gregorykjohnson.com (Postfix) with ESMTP id 5B056316746
        for <gkj@gregorykjohnson.com>; Wed, 13 Jul 2005 17:23:11 -0400 (EDT)
Received: by andy.gregorykjohnson.com (Postfix, from userid 1000)
        id 490CD9DD17; Wed, 13 Jul 2005 17:23:11 -0400 (EDT)
Date: Wed, 13 Jul 2005 17:23:11 -0400
From: "Gregory K. Johnson" <gkj@gregorykjohnson.com>
To: gkj@gregorykjohnson.com
Subject: Sample message
Message-ID: <20050713212311.GC4701@andy.gregorykjohnson.com>
Mime-Version: 1.0
Content-Type: multipart/mixed; boundary="NMuMz9nt05w80d4+"
Content-Disposition: inline
User-Agent: Mutt/1.5.9i


--NMuMz9nt05w80d4+
Content-Type: text/plain; charset=us-ascii
Content-Disposition: inline

This is a sample message.

--
Gregory K. Johnson

--NMuMz9nt05w80d4+
Content-Type: application/octet-stream
Content-Disposition: attachment; filename="text.gz"
Content-Transfer-Encoding: base64

H4sICM2D1UIAA3RleHQAC8nILFYAokSFktSKEoW0zJxUPa7wzJIMhZLyfIWczLzUYj0uAHTs
3FYlAAAA

--NMuMz9nt05w80d4+--
"""

_sample_headers = {
    "Return-Path":"<gkj@gregorykjohnson.com>",
    "X-Original-To":"gkj+person@localhost",
    "Delivered-To":"gkj+person@localhost",
    "Received":"""from localhost (localhost [127.0.0.1])
        by andy.gregorykjohnson.com (Postfix) with ESMTP id 356ED9DD17
        for <gkj+person@localhost>; Wed, 13 Jul 2005 17:23:16 -0400 (EDT)""",
    "Delivered-To":"gkj@sundance.gregorykjohnson.com",
    "Received":"""from localhost [127.0.0.1]
        by localhost with POP3 (fetchmail-6.2.5)
        for gkj+person@localhost (single-drop); Wed, 13 Jul 2005 17:23:16 -0400 (EDT)""",
    "Received":"""from andy.gregorykjohnson.com (andy.gregorykjohnson.com [64.32.235.228])
        by sundance.gregorykjohnson.com (Postfix) with ESMTP id 5B056316746
        for <gkj@gregorykjohnson.com>; Wed, 13 Jul 2005 17:23:11 -0400 (EDT)""",
    "Received":"""by andy.gregorykjohnson.com (Postfix, from userid 1000)
        id 490CD9DD17; Wed, 13 Jul 2005 17:23:11 -0400 (EDT)""",
    "Date":"Wed, 13 Jul 2005 17:23:11 -0400",
    "From":""""Gregory K. Johnson" <gkj@gregorykjohnson.com>""",
    "To":"gkj@gregorykjohnson.com",
    "Subject":"Sample message",
    "Mime-Version":"1.0",
    "Content-Type":"""multipart/mixed; boundary="NMuMz9nt05w80d4+\"""",
    "Content-Disposition":"inline",
    "User-Agent": "Mutt/1.5.9i" }

_sample_payloads = ("""This is a sample message.

--
Gregory K. Johnson
""",
"""H4sICM2D1UIAA3RleHQAC8nILFYAokSFktSKEoW0zJxUPa7wzJIMhZLyfIWczLzUYj0uAHTs
3FYlAAAA
""")


def test_main():
    tests = (TestMailboxSuperclass, TestMaildir, TestMbox, TestMMDF, TestMH,
             TestBabyl, TestMessage, TestMaildirMessage, TestMboxMessage,
             TestMHMessage, TestBabylMessage, TestMMDFMessage,
             TestMessageConversion, TestProxyFile, TestPartialFile,
             MaildirTestCase)
    test_support.run_unittest(*tests)
    test_support.reap_children()


if __name__ == '__main__':
    test_main()
