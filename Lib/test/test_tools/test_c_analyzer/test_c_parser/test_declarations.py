import textwrap
import unittest

from .. import tool_imports_for_tests
with tool_imports_for_tests():
    from c_parser.declarations import (
        iter_global_declarations, iter_local_statements,
        parse_func, parse_var, parse_compound,
        iter_variables,
        )


class TestCaseBase(unittest.TestCase):

    maxDiff = None

    @property
    def calls(self):
        try:
            return self._calls
        except AttributeError:
            self._calls = []
            return self._calls


class IterGlobalDeclarationsTests(TestCaseBase):

    def test_functions(self):
        tests = [
            (textwrap.dedent('''
                void func1() {
                    return;
                }
                '''),
             textwrap.dedent('''
                void func1() {
                return;
                }
                ''').strip(),
             ),
            (textwrap.dedent('''
                static unsigned int * _func1(
                    const char *arg1,
                    int *arg2
                    long long arg3
                    )
                {
                    return _do_something(arg1, arg2, arg3);
                }
                '''),
             textwrap.dedent('''
                static unsigned int * _func1( const char *arg1, int *arg2 long long arg3 ) {
                return _do_something(arg1, arg2, arg3);
                }
                ''').strip(),
             ),
            (textwrap.dedent('''
                static PyObject *
                _func1(const char *arg1, PyObject *arg2)
                {
                    static int initialized = 0;
                    if (!initialized) {
                        initialized = 1;
                        _init(arg1);
                    }

                    PyObject *result = _do_something(arg1, arg2);
                    Py_INCREF(result);
                    return result;
                }
                '''),
             textwrap.dedent('''
                static PyObject * _func1(const char *arg1, PyObject *arg2) {
                static int initialized = 0;
                if (!initialized) {
                initialized = 1;
                _init(arg1);
                }
                PyObject *result = _do_something(arg1, arg2);
                Py_INCREF(result);
                return result;
                }
                ''').strip(),
             ),
            ]
        for lines, expected in tests:
            body = textwrap.dedent(
                    expected.partition('{')[2].rpartition('}')[0]
                    ).strip()
            expected = (expected, body)
            with self.subTest(lines):
                lines = lines.splitlines()

                stmts = list(iter_global_declarations(lines))

                self.assertEqual(stmts, [expected])

    @unittest.expectedFailure
    def test_declarations(self):
        tests = [
            'int spam;',
            'long long spam;',
            'static const int const *spam;',
            'int spam;',
            'typedef int myint;',
            'typedef PyObject * (*unaryfunc)(PyObject *);',
            # typedef struct
            # inline struct
            # enum
            # inline enum
            ]
        for text in tests:
            expected = (text,
                        ' '.join(l.strip() for l in text.splitlines()))
            with self.subTest(lines):
                lines = lines.splitlines()

                stmts = list(iter_global_declarations(lines))

                self.assertEqual(stmts, [expected])

    @unittest.expectedFailure
    def test_declaration_multiple_vars(self):
        lines = ['static const int const *spam, *ham=NULL, eggs = 3;']

        stmts = list(iter_global_declarations(lines))

        self.assertEqual(stmts, [
            ('static const int const *spam;', None),
            ('static const int *ham=NULL;', None),
            ('static const int eggs = 3;', None),
            ])

    def test_mixed(self):
        lines = textwrap.dedent('''
           int spam;
           static const char const *eggs;

           PyObject * start(void) {
               static int initialized = 0;
               if (initialized) {
                   initialized = 1;
                   init();
               }
               return _start();
           }

           char* ham;

           static int stop(char *reason) {
               ham = reason;
               return _stop();
           }
           ''').splitlines()
        expected = [
            (textwrap.dedent('''
                PyObject * start(void) {
                static int initialized = 0;
                if (initialized) {
                initialized = 1;
                init();
                }
                return _start();
                }
                ''').strip(),
             textwrap.dedent('''
                static int initialized = 0;
                if (initialized) {
                initialized = 1;
                init();
                }
                return _start();
                ''').strip(),
             ),
            (textwrap.dedent('''
                static int stop(char *reason) {
                ham = reason;
                return _stop();
                }
                ''').strip(),
             textwrap.dedent('''
                ham = reason;
                return _stop();
                ''').strip(),
             ),
            ]

        stmts = list(iter_global_declarations(lines))

        self.assertEqual(stmts, expected)
        #self.assertEqual([stmt for stmt, _ in stmts],
        #                 [stmt for stmt, _ in expected])
        #self.assertEqual([body for _, body in stmts],
        #                 [body for _, body in expected])

    def test_no_statements(self):
        lines = []

        stmts = list(iter_global_declarations(lines))

        self.assertEqual(stmts, [])

    def test_bogus(self):
        tests = [
                (textwrap.dedent('''
                    int spam;
                    static const char const *eggs;

                    PyObject * start(void) {
                        static int initialized = 0;
                        if (initialized) {
                            initialized = 1;
                            init();
                        }
                        return _start();
                    }

                    char* ham;

                    static int _stop(void) {
                    // missing closing bracket

                    static int stop(char *reason) {
                        ham = reason;
                        return _stop();
                    }
                    '''),
                 [(textwrap.dedent('''
                    PyObject * start(void) {
                    static int initialized = 0;
                    if (initialized) {
                    initialized = 1;
                    init();
                    }
                    return _start();
                    }
                    ''').strip(),
                   textwrap.dedent('''
                    static int initialized = 0;
                    if (initialized) {
                    initialized = 1;
                    init();
                    }
                    return _start();
                    ''').strip(),
                   ),
                   # Neither "stop()" nor "_stop()" are here.
                  ],
                 ),
                ]
        for lines, expected in tests:
            with self.subTest(lines):
                lines = lines.splitlines()

                stmts = list(iter_global_declarations(lines))

                self.assertEqual(stmts, expected)
                #self.assertEqual([stmt for stmt, _ in stmts],
                #                 [stmt for stmt, _ in expected])
                #self.assertEqual([body for _, body in stmts],
                #                 [body for _, body in expected])

    def test_ignore_comments(self):
        tests = [
            ('// msg', None),
            ('// int stmt;', None),
            ('    // ...    ', None),
            ('// /*', None),
            ('/* int stmt; */', None),
            ("""
             /**
              * ...
              * int stmt;
              */
             """, None),
            ]
        for lines, expected in tests:
            with self.subTest(lines):
                lines = lines.splitlines()

                stmts = list(iter_global_declarations(lines))

                self.assertEqual(stmts, [expected] if expected else [])


class IterLocalStatementsTests(TestCaseBase):

    def test_vars(self):
        tests = [
            # POTS
            'int spam;',
            'unsigned int spam;',
            'char spam;',
            'float spam;',

            # typedefs
            'uint spam;',
            'MyType spam;',

            # complex
            'struct myspam spam;',
            'union choice spam;',
            # inline struct
            # inline union
            # enum?
            ]
        # pointers
        tests.extend([
            # POTS
            'int * spam;',
            'unsigned int * spam;',
            'char *spam;',
            'char const *spam = "spamspamspam...";',
            # typedefs
            'MyType *spam;',
            # complex
            'struct myspam *spam;',
            'union choice *spam;',
            # packed with details
            'const char const *spam;',
            # void pointer
            'void *data = NULL;',
            # function pointers
            'int (* func)(char *arg1);',
            'char * (* func)(void);',
            ])
        # storage class
        tests.extend([
            'static int spam;',
            'extern int spam;',
            'static unsigned int spam;',
            'static struct myspam spam;',
            ])
        # type qualifier
        tests.extend([
            'const int spam;',
            'const unsigned int spam;',
            'const struct myspam spam;',
            ])
        # combined
        tests.extend([
            'const char *spam = eggs;',
            'static const char const *spam = "spamspamspam...";',
            'extern const char const *spam;',
            'static void *data = NULL;',
            'static int (const * func)(char *arg1) = func1;',
            'static char * (* func)(void);',
            ])
        for line in tests:
            expected = line
            with self.subTest(line):
                stmts = list(iter_local_statements([line]))

                self.assertEqual(stmts, [(expected, None)])

    @unittest.expectedFailure
    def test_vars_multiline_var(self):
        lines = textwrap.dedent('''
            PyObject *
            spam
            = NULL;
            ''').splitlines()
        expected = 'PyObject * spam = NULL;'

        stmts = list(iter_local_statements(lines))

        self.assertEqual(stmts, [(expected, None)])

    @unittest.expectedFailure
    def test_declaration_multiple_vars(self):
        lines = ['static const int const *spam, *ham=NULL, ham2[]={1, 2, 3}, ham3[2]={1, 2}, eggs = 3;']

        stmts = list(iter_global_declarations(lines))

        self.assertEqual(stmts, [
            ('static const int const *spam;', None),
            ('static const int *ham=NULL;', None),
            ('static const int ham[]={1, 2, 3};', None),
            ('static const int ham[2]={1, 2};', None),
            ('static const int eggs = 3;', None),
            ])

    @unittest.expectedFailure
    def test_other_simple(self):
        raise NotImplementedError

    @unittest.expectedFailure
    def test_compound(self):
        raise NotImplementedError

    @unittest.expectedFailure
    def test_mixed(self):
        raise NotImplementedError

    def test_no_statements(self):
        lines = []

        stmts = list(iter_local_statements(lines))

        self.assertEqual(stmts, [])

    @unittest.expectedFailure
    def test_bogus(self):
        raise NotImplementedError

    def test_ignore_comments(self):
        tests = [
            ('// msg', None),
            ('// int stmt;', None),
            ('    // ...    ', None),
            ('// /*', None),
            ('/* int stmt; */', None),
            ("""
             /**
              * ...
              * int stmt;
              */
             """, None),
            # mixed with statements
            ('int stmt; // ...', ('int stmt;', None)),
            ( 'int stmt; /* ...  */', ('int stmt;', None)),
            ( '/* ...  */ int stmt;', ('int stmt;', None)),
            ]
        for lines, expected in tests:
            with self.subTest(lines):
                lines = lines.splitlines()

                stmts = list(iter_local_statements(lines))

                self.assertEqual(stmts, [expected] if expected else [])


class ParseFuncTests(TestCaseBase):

    def test_typical(self):
        tests = [
            ('PyObject *\nspam(char *a)\n{\nreturn _spam(a);\n}',
             'return _spam(a);',
             ('spam', 'PyObject * spam(char *a)'),
             ),
            ]
        for stmt, body, expected in tests:
            with self.subTest(stmt):
                name, signature = parse_func(stmt, body)

                self.assertEqual((name, signature), expected)


class ParseVarTests(TestCaseBase):

    def test_typical(self):
        tests = [
            # POTS
            ('int spam;', ('spam', 'int')),
            ('unsigned int spam;', ('spam', 'unsigned int')),
            ('char spam;', ('spam', 'char')),
            ('float spam;', ('spam', 'float')),

            # typedefs
            ('uint spam;', ('spam', 'uint')),
            ('MyType spam;', ('spam', 'MyType')),

            # complex
            ('struct myspam spam;', ('spam', 'struct myspam')),
            ('union choice spam;', ('spam', 'union choice')),
            # inline struct
            # inline union
            # enum?
            ]
        # pointers
        tests.extend([
            # POTS
            ('int * spam;', ('spam', 'int *')),
            ('unsigned int * spam;', ('spam', 'unsigned int *')),
            ('char *spam;', ('spam', 'char *')),
            ('char const *spam = "spamspamspam...";', ('spam', 'char const *')),
            # typedefs
            ('MyType *spam;', ('spam', 'MyType *')),
            # complex
            ('struct myspam *spam;', ('spam', 'struct myspam *')),
            ('union choice *spam;', ('spam', 'union choice *')),
            # packed with details
            ('const char const *spam;', ('spam', 'const char const *')),
            # void pointer
            ('void *data = NULL;', ('data', 'void *')),
            # function pointers
            ('int (* func)(char *);', ('func', 'int (*)(char *)')),
            ('char * (* func)(void);', ('func', 'char * (*)(void)')),
            ])
        # storage class
        tests.extend([
            ('static int spam;', ('spam', 'static int')),
            ('extern int spam;', ('spam', 'extern int')),
            ('static unsigned int spam;', ('spam', 'static unsigned int')),
            ('static struct myspam spam;', ('spam', 'static struct myspam')),
            ])
        # type qualifier
        tests.extend([
            ('const int spam;', ('spam', 'const int')),
            ('const unsigned int spam;', ('spam', 'const unsigned int')),
            ('const struct myspam spam;', ('spam', 'const struct myspam')),
            ])
        # combined
        tests.extend([
            ('const char *spam = eggs;', ('spam', 'const char *')),
            ('static const char const *spam = "spamspamspam...";',
             ('spam', 'static const char const *')),
            ('extern const char const *spam;',
             ('spam', 'extern const char const *')),
            ('static void *data = NULL;', ('data', 'static void *')),
            ('static int (const * func)(char *) = func1;',
             ('func', 'static int (const *)(char *)')),
            ('static char * (* func)(void);',
             ('func', 'static char * (*)(void)')),
            ])
        for stmt, expected in tests:
            with self.subTest(stmt):
                name, vartype = parse_var(stmt)

                self.assertEqual((name, vartype), expected)


@unittest.skip('not finished')
class ParseCompoundTests(TestCaseBase):

    def test_typical(self):
        headers, bodies = parse_compound(stmt, blocks)
        ...


class IterVariablesTests(TestCaseBase):

    _return_iter_source_lines = None
    _return_iter_global = None
    _return_iter_local = None
    _return_parse_func = None
    _return_parse_var = None
    _return_parse_compound = None

    def _iter_source_lines(self, filename):
        self.calls.append(
                ('_iter_source_lines', (filename,)))
        return self._return_iter_source_lines.splitlines()

    def _iter_global(self, lines):
        self.calls.append(
                ('_iter_global', (lines,)))
        try:
            return self._return_iter_global.pop(0)
        except IndexError:
            return ('???', None)

    def _iter_local(self, lines):
        self.calls.append(
                ('_iter_local', (lines,)))
        try:
            return self._return_iter_local.pop(0)
        except IndexError:
            return ('???', None)

    def _parse_func(self, stmt, body):
        self.calls.append(
                ('_parse_func', (stmt, body)))
        try:
            return self._return_parse_func.pop(0)
        except IndexError:
            return ('???', '???')

    def _parse_var(self, lines):
        self.calls.append(
                ('_parse_var', (lines,)))
        try:
            return self._return_parse_var.pop(0)
        except IndexError:
            return ('???', '???')

    def _parse_compound(self, stmt, blocks):
        self.calls.append(
                ('_parse_compound', (stmt, blocks)))
        try:
            return self._return_parse_compound.pop(0)
        except IndexError:
            return (['???'], ['???'])

    def test_empty_file(self):
        self._return_iter_source_lines = ''
        self._return_iter_global = [
            [],
            ]
        self._return_parse_func = None
        self._return_parse_var = None
        self._return_parse_compound = None

        srcvars = list(iter_variables('spam.c',
                                      _iter_source_lines=self._iter_source_lines,
                                      _iter_global=self._iter_global,
                                      _iter_local=self._iter_local,
                                      _parse_func=self._parse_func,
                                      _parse_var=self._parse_var,
                                      _parse_compound=self._parse_compound,
                                      ))

        self.assertEqual(srcvars, [])
        self.assertEqual(self.calls, [
            ('_iter_source_lines', ('spam.c',)),
            ('_iter_global', ([],)),
            ])

    def test_no_statements(self):
        content = textwrap.dedent('''
        ...
        ''')
        self._return_iter_source_lines = content
        self._return_iter_global = [
            [],
            ]
        self._return_parse_func = None
        self._return_parse_var = None
        self._return_parse_compound = None

        srcvars = list(iter_variables('spam.c',
                                      _iter_source_lines=self._iter_source_lines,
                                      _iter_global=self._iter_global,
                                      _iter_local=self._iter_local,
                                      _parse_func=self._parse_func,
                                      _parse_var=self._parse_var,
                                      _parse_compound=self._parse_compound,
                                      ))

        self.assertEqual(srcvars, [])
        self.assertEqual(self.calls, [
            ('_iter_source_lines', ('spam.c',)),
            ('_iter_global', (content.splitlines(),)),
            ])

    def test_typical(self):
        content = textwrap.dedent('''
        ...
        ''')
        self._return_iter_source_lines = content
        self._return_iter_global = [
            [('<lines 1>', None),  # var1
             ('<lines 2>', None),  # non-var
             ('<lines 3>', None),  # var2
             ('<lines 4>', '<body 1>'),  # func1
             ('<lines 9>', None),  # var4
             ],
            ]
        self._return_iter_local = [
            # func1
            [('<lines 5>', None),  # var3
             ('<lines 6>', [('<header 1>', '<block 1>')]),  # if
             ('<lines 8>', None),  # non-var
             ],
            # if
            [('<lines 7>', None),  # var2 ("collision" with global var)
             ],
            ]
        self._return_parse_func = [
            ('func1', '<sig 1>'),
            ]
        self._return_parse_var = [
            ('var1', '<vartype 1>'),
            (None, None),
            ('var2', '<vartype 2>'),
            ('var3', '<vartype 3>'),
            ('var2', '<vartype 2b>'),
            ('var4', '<vartype 4>'),
            (None, None),
            (None, None),
            (None, None),
            ('var5', '<vartype 5>'),
            ]
        self._return_parse_compound = [
            ([[
                'if (',
                '<simple>',
                ')',
                ],
              ],
             ['<block 1>']),
            ]

        srcvars = list(iter_variables('spam.c',
                                      _iter_source_lines=self._iter_source_lines,
                                      _iter_global=self._iter_global,
                                      _iter_local=self._iter_local,
                                      _parse_func=self._parse_func,
                                      _parse_var=self._parse_var,
                                      _parse_compound=self._parse_compound,
                                      ))

        self.assertEqual(srcvars, [
            (None, 'var1', '<vartype 1>'),
            (None, 'var2', '<vartype 2>'),
            ('func1', 'var3', '<vartype 3>'),
            ('func1', 'var2', '<vartype 2b>'),
            ('func1', 'var4', '<vartype 4>'),
            (None, 'var5', '<vartype 5>'),
            ])
        self.assertEqual(self.calls, [
            ('_iter_source_lines', ('spam.c',)),
            ('_iter_global', (content.splitlines(),)),
            ('_parse_var', ('<lines 1>',)),
            ('_parse_var', ('<lines 2>',)),
            ('_parse_var', ('<lines 3>',)),
            ('_parse_func', ('<lines 4>', '<body 1>')),
            ('_iter_local', (['<body 1>'],)),
            ('_parse_var', ('<lines 5>',)),
            ('_parse_compound', ('<lines 6>', [('<header 1>', '<block 1>')])),
            ('_parse_var', ('if (',)),
            ('_parse_var', ('<simple>',)),
            ('_parse_var', (')',)),
            ('_parse_var', ('<lines 8>',)),
            ('_iter_local', (['<block 1>'],)),
            ('_parse_var', ('<lines 7>',)),
            ('_parse_var', ('<lines 9>',)),
            ])

    def test_no_locals(self):
        content = textwrap.dedent('''
        ...
        ''')
        self._return_iter_source_lines = content
        self._return_iter_global = [
            [('<lines 1>', None),  # var1
             ('<lines 2>', None),  # non-var
             ('<lines 3>', None),  # var2
             ('<lines 4>', '<body 1>'),  # func1
             ],
            ]
        self._return_iter_local = [
            # func1
            [('<lines 5>', None),  # non-var
             ('<lines 6>', [('<header 1>', '<block 1>')]),  # if
             ('<lines 8>', None),  # non-var
             ],
            # if
            [('<lines 7>', None),  # non-var
             ],
            ]
        self._return_parse_func = [
            ('func1', '<sig 1>'),
            ]
        self._return_parse_var = [
            ('var1', '<vartype 1>'),
            (None, None),
            ('var2', '<vartype 2>'),
            (None, None),
            (None, None),
            (None, None),
            (None, None),
            (None, None),
            (None, None),
            ]
        self._return_parse_compound = [
            ([[
                'if (',
                '<simple>',
                ')',
                ],
              ],
             ['<block 1>']),
            ]

        srcvars = list(iter_variables('spam.c',
                                      _iter_source_lines=self._iter_source_lines,
                                      _iter_global=self._iter_global,
                                      _iter_local=self._iter_local,
                                      _parse_func=self._parse_func,
                                      _parse_var=self._parse_var,
                                      _parse_compound=self._parse_compound,
                                      ))

        self.assertEqual(srcvars, [
            (None, 'var1', '<vartype 1>'),
            (None, 'var2', '<vartype 2>'),
            ])
        self.assertEqual(self.calls, [
            ('_iter_source_lines', ('spam.c',)),
            ('_iter_global', (content.splitlines(),)),
            ('_parse_var', ('<lines 1>',)),
            ('_parse_var', ('<lines 2>',)),
            ('_parse_var', ('<lines 3>',)),
            ('_parse_func', ('<lines 4>', '<body 1>')),
            ('_iter_local', (['<body 1>'],)),
            ('_parse_var', ('<lines 5>',)),
            ('_parse_compound', ('<lines 6>', [('<header 1>', '<block 1>')])),
            ('_parse_var', ('if (',)),
            ('_parse_var', ('<simple>',)),
            ('_parse_var', (')',)),
            ('_parse_var', ('<lines 8>',)),
            ('_iter_local', (['<block 1>'],)),
            ('_parse_var', ('<lines 7>',)),
            ])
