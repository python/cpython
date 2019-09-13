import itertools
import textwrap
import unittest
import sys

from ..util import wrapped_arg_combos, StrProxy
from .. import tool_imports_for_tests
with tool_imports_for_tests():
    from c_parser.preprocessor import (
        iter_lines,
        # directives
        parse_directive, PreprocessorDirective,
        Constant, Macro, IfDirective, Include, OtherDirective,
        )


class TestCaseBase(unittest.TestCase):

    maxDiff = None

    def reset(self):
        self._calls = []
        self.errors = None

    @property
    def calls(self):
        try:
            return self._calls
        except AttributeError:
            self._calls = []
            return self._calls

    errors = None

    def try_next_exc(self):
        if not self.errors:
            return
        if exc := self.errors.pop(0):
            raise exc

    def check_calls(self, *expected):
        self.assertEqual(self.calls, list(expected))
        self.assertEqual(self.errors or [], [])


class IterLinesTests(TestCaseBase):

    parsed = None

    def check_calls(self, *expected):
        super().check_calls(*expected)
        self.assertEqual(self.parsed or [], [])

    def _parse_directive(self, line):
        self.calls.append(
                ('_parse_directive', line))
        self.try_next_exc()
        return self.parsed.pop(0)

    def test_no_lines(self):
        lines = []

        results = list(
                iter_lines(lines, _parse_directive=self._parse_directive))

        self.assertEqual(results, [])
        self.check_calls()

    def test_no_directives(self):
        lines = textwrap.dedent('''

            // xyz
            typedef enum {
                SPAM
                EGGS
            } kind;

            struct info {
                kind kind;
                int status;
            };

            typedef struct spam {
                struct info info;
            } myspam;

            static int spam = 0;

            /**
             * ...
             */
            static char *
            get_name(int arg,
                     char *default,
                     )
            {
                return default
            }

            int check(void) {
                return 0;
            }

            ''')[1:-1].splitlines()
        expected = [(lno, line, None, ())
                    for lno, line in enumerate(lines, 1)]
        expected[1] = (2, ' ', None, ())
        expected[20] = (21, ' ', None, ())
        del expected[19]
        del expected[18]

        results = list(
                iter_lines(lines, _parse_directive=self._parse_directive))

        self.assertEqual(results, expected)
        self.check_calls()

    def test_single_directives(self):
        tests = [
            ('#include <stdio>', Include('<stdio>')),
            ('#define SPAM 1', Constant('SPAM', '1')),
            ('#define SPAM() 1', Macro('SPAM', (), '1')),
            ('#define SPAM(a, b) a = b;', Macro('SPAM', ('a', 'b'), 'a = b;')),
            ('#if defined(SPAM)', IfDirective('if', 'defined(SPAM)')),
            ('#ifdef SPAM', IfDirective('ifdef', 'SPAM')),
            ('#ifndef SPAM', IfDirective('ifndef', 'SPAM')),
            ('#elseif defined(SPAM)', IfDirective('elseif', 'defined(SPAM)')),
            ('#else', OtherDirective('else', None)),
            ('#endif', OtherDirective('endif', None)),
            ('#error ...', OtherDirective('error', '...')),
            ('#warning ...', OtherDirective('warning', '...')),
            ('#__FILE__ ...', OtherDirective('__FILE__', '...')),
            ('#__LINE__ ...', OtherDirective('__LINE__', '...')),
            ('#__DATE__ ...', OtherDirective('__DATE__', '...')),
            ('#__TIME__ ...', OtherDirective('__TIME__', '...')),
            ('#__TIMESTAMP__ ...', OtherDirective('__TIMESTAMP__', '...')),
            ]
        for line, directive in tests:
            with self.subTest(line):
                self.reset()
                self.parsed = [
                    directive,
                    ]
                text = textwrap.dedent('''
                    static int spam = 0;
                    {}
                    static char buffer[256];
                    ''').strip().format(line)
                lines = text.strip().splitlines()

                results = list(
                        iter_lines(lines, _parse_directive=self._parse_directive))

                self.assertEqual(results, [
                    (1, 'static int spam = 0;', None, ()),
                    (2, line, directive, ()),
                    ((3, 'static char buffer[256];', None, ('defined(SPAM)',))
                     if directive.kind in ('if', 'ifdef', 'elseif')
                     else (3, 'static char buffer[256];', None, ('! defined(SPAM)',))
                     if directive.kind == 'ifndef'
                     else (3, 'static char buffer[256];', None, ())),
                    ])
                self.check_calls(
                        ('_parse_directive', line),
                        )

    def test_directive_whitespace(self):
        line = ' # define  eggs  (  a  ,  b  )  {  a  =  b  ;  }  '
        directive = Macro('eggs', ('a', 'b'), '{ a = b; }')
        self.parsed = [
            directive,
            ]
        lines = [line]

        results = list(
                iter_lines(lines, _parse_directive=self._parse_directive))

        self.assertEqual(results, [
            (1, line, directive, ()),
            ])
        self.check_calls(
                ('_parse_directive', '#define eggs ( a , b ) { a = b ; }'),
                )

    @unittest.skipIf(sys.platform == 'win32', 'needs fix under Windows')
    def test_split_lines(self):
        directive = Macro('eggs', ('a', 'b'), '{ a = b; }')
        self.parsed = [
            directive,
            ]
        text = textwrap.dedent(r'''
            static int spam = 0;
            #define eggs(a, b) \
                { \
                    a = b; \
                }
            static char buffer[256];
            ''').strip()
        lines = [line + '\n' for line in text.splitlines()]
        lines[-1] = lines[-1][:-1]

        results = list(
                iter_lines(lines, _parse_directive=self._parse_directive))

        self.assertEqual(results, [
            (1, 'static int spam = 0;\n', None, ()),
            (5, '#define eggs(a, b)      {          a = b;      }\n', directive, ()),
            (6, 'static char buffer[256];', None, ()),
            ])
        self.check_calls(
                ('_parse_directive', '#define eggs(a, b) { a = b; }'),
                )

    def test_nested_conditions(self):
        directives = [
            IfDirective('ifdef', 'SPAM'),
            IfDirective('if', 'SPAM == 1'),
            IfDirective('elseif', 'SPAM == 2'),
            OtherDirective('else', None),
            OtherDirective('endif', None),
            OtherDirective('endif', None),
            ]
        self.parsed = list(directives)
        text = textwrap.dedent(r'''
            static int spam = 0;

            #ifdef SPAM
            static int start = 0;
            #  if SPAM == 1
            static char buffer[10];
            #  elif SPAM == 2
            static char buffer[100];
            #  else
            static char buffer[256];
            #  endif
            static int end = 0;
            #endif

            static int eggs = 0;
            ''').strip()
        lines = [line for line in text.splitlines() if line.strip()]

        results = list(
                iter_lines(lines, _parse_directive=self._parse_directive))

        self.assertEqual(results, [
            (1, 'static int spam = 0;', None, ()),
            (2, '#ifdef SPAM', directives[0], ()),
            (3, 'static int start = 0;', None, ('defined(SPAM)',)),
            (4, '#  if SPAM == 1', directives[1], ('defined(SPAM)',)),
            (5, 'static char buffer[10];', None, ('defined(SPAM)', 'SPAM == 1')),
            (6, '#  elif SPAM == 2', directives[2], ('defined(SPAM)', 'SPAM == 1')),
            (7, 'static char buffer[100];', None, ('defined(SPAM)', '! (SPAM == 1)', 'SPAM == 2')),
            (8, '#  else', directives[3], ('defined(SPAM)', '! (SPAM == 1)', 'SPAM == 2')),
            (9, 'static char buffer[256];', None, ('defined(SPAM)', '! (SPAM == 1)', '! (SPAM == 2)')),
            (10, '#  endif', directives[4], ('defined(SPAM)', '! (SPAM == 1)', '! (SPAM == 2)')),
            (11, 'static int end = 0;', None, ('defined(SPAM)',)),
            (12, '#endif', directives[5], ('defined(SPAM)',)),
            (13, 'static int eggs = 0;', None, ()),
            ])
        self.check_calls(
                ('_parse_directive', '#ifdef SPAM'),
                ('_parse_directive', '#if SPAM == 1'),
                ('_parse_directive', '#elif SPAM == 2'),
                ('_parse_directive', '#else'),
                ('_parse_directive', '#endif'),
                ('_parse_directive', '#endif'),
                )

    def test_split_blocks(self):
        directives = [
            IfDirective('ifdef', 'SPAM'),
            OtherDirective('else', None),
            OtherDirective('endif', None),
            ]
        self.parsed = list(directives)
        text = textwrap.dedent(r'''
            void str_copy(char *buffer, *orig);

            int init(char *name) {
                static int initialized = 0;
                if (initialized) {
                    return 0;
                }
            #ifdef SPAM
                static char buffer[10];
                str_copy(buffer, char);
            }

            void copy(char *buffer, *orig) {
                strncpy(buffer, orig, 9);
                buffer[9] = 0;
            }

            #else
                static char buffer[256];
                str_copy(buffer, char);
            }

            void copy(char *buffer, *orig) {
                strcpy(buffer, orig);
            }

            #endif
            ''').strip()
        lines = [line for line in text.splitlines() if line.strip()]

        results = list(
                iter_lines(lines, _parse_directive=self._parse_directive))

        self.assertEqual(results, [
            (1, 'void str_copy(char *buffer, *orig);', None, ()),
            (2, 'int init(char *name) {', None, ()),
            (3, '    static int initialized = 0;', None, ()),
            (4, '    if (initialized) {', None, ()),
            (5, '        return 0;', None, ()),
            (6, '    }', None, ()),

            (7, '#ifdef SPAM', directives[0], ()),

            (8, '    static char buffer[10];', None, ('defined(SPAM)',)),
            (9, '    str_copy(buffer, char);', None, ('defined(SPAM)',)),
            (10, '}', None, ('defined(SPAM)',)),
            (11, 'void copy(char *buffer, *orig) {', None, ('defined(SPAM)',)),
            (12, '    strncpy(buffer, orig, 9);', None, ('defined(SPAM)',)),
            (13, '    buffer[9] = 0;', None, ('defined(SPAM)',)),
            (14, '}', None, ('defined(SPAM)',)),

            (15, '#else', directives[1], ('defined(SPAM)',)),

            (16, '    static char buffer[256];', None, ('! (defined(SPAM))',)),
            (17, '    str_copy(buffer, char);', None, ('! (defined(SPAM))',)),
            (18, '}', None, ('! (defined(SPAM))',)),
            (19, 'void copy(char *buffer, *orig) {', None, ('! (defined(SPAM))',)),
            (20, '    strcpy(buffer, orig);', None, ('! (defined(SPAM))',)),
            (21, '}', None, ('! (defined(SPAM))',)),

            (22, '#endif', directives[2], ('! (defined(SPAM))',)),
            ])
        self.check_calls(
                ('_parse_directive', '#ifdef SPAM'),
                ('_parse_directive', '#else'),
                ('_parse_directive', '#endif'),
                )

    @unittest.skipIf(sys.platform == 'win32', 'needs fix under Windows')
    def test_basic(self):
        directives = [
            Include('<stdio.h>'),
            IfDirective('ifdef', 'SPAM'),
            IfDirective('if', '! defined(HAM) || !HAM'),
            Constant('HAM', '0'),
            IfDirective('elseif', 'HAM < 0'),
            Constant('HAM', '-1'),
            OtherDirective('else', None),
            OtherDirective('endif', None),
            OtherDirective('endif', None),
            IfDirective('if', 'defined(HAM) && (HAM < 0 || ! HAM)'),
            OtherDirective('undef', 'HAM'),
            OtherDirective('endif', None),
            IfDirective('ifndef', 'HAM'),
            OtherDirective('endif', None),
            ]
        self.parsed = list(directives)
        text = textwrap.dedent(r'''
            #include <stdio.h>
            print("begin");
            #ifdef SPAM
               print("spam");
               #if ! defined(HAM) || !HAM
            #      DEFINE HAM 0
               #elseif HAM < 0
            #      DEFINE HAM -1
               #else
                   print("ham HAM");
               #endif
            #endif

            #if defined(HAM) && \
                (HAM < 0 || ! HAM)
              print("ham?");
              #undef HAM
            # endif

            #ifndef HAM
               print("no ham");
            #endif
            print("end");
            ''')[1:-1]
        lines = [line + '\n' for line in text.splitlines()]
        lines[-1] = lines[-1][:-1]

        results = list(
                iter_lines(lines, _parse_directive=self._parse_directive))

        self.assertEqual(results, [
            (1, '#include <stdio.h>\n', Include('<stdio.h>'), ()),
            (2, 'print("begin");\n', None, ()),
            #
            (3, '#ifdef SPAM\n',
                IfDirective('ifdef', 'SPAM'),
                ()),
            (4, '   print("spam");\n',
                None,
                ('defined(SPAM)',)),
            (5, '   #if ! defined(HAM) || !HAM\n',
                IfDirective('if', '! defined(HAM) || !HAM'),
                ('defined(SPAM)',)),
            (6, '#      DEFINE HAM 0\n',
                Constant('HAM', '0'),
                ('defined(SPAM)', '! defined(HAM) || !HAM')),
            (7, '   #elseif HAM < 0\n',
                IfDirective('elseif', 'HAM < 0'),
                ('defined(SPAM)', '! defined(HAM) || !HAM')),
            (8, '#      DEFINE HAM -1\n',
                Constant('HAM', '-1'),
                ('defined(SPAM)', '! (! defined(HAM) || !HAM)', 'HAM < 0')),
            (9, '   #else\n',
                OtherDirective('else', None),
                ('defined(SPAM)', '! (! defined(HAM) || !HAM)', 'HAM < 0')),
            (10, '       print("ham HAM");\n',
                None,
                ('defined(SPAM)', '! (! defined(HAM) || !HAM)', '! (HAM < 0)')),
            (11, '   #endif\n',
                OtherDirective('endif', None),
                ('defined(SPAM)', '! (! defined(HAM) || !HAM)', '! (HAM < 0)')),
            (12, '#endif\n',
                OtherDirective('endif', None),
                ('defined(SPAM)',)),
            #
            (13, '\n', None, ()),
            #
            (15, '#if defined(HAM) &&      (HAM < 0 || ! HAM)\n',
                IfDirective('if', 'defined(HAM) && (HAM < 0 || ! HAM)'),
                ()),
            (16, '  print("ham?");\n',
                None,
                ('defined(HAM) && (HAM < 0 || ! HAM)',)),
            (17, '  #undef HAM\n',
                OtherDirective('undef', 'HAM'),
                ('defined(HAM) && (HAM < 0 || ! HAM)',)),
            (18, '# endif\n',
                OtherDirective('endif', None),
                ('defined(HAM) && (HAM < 0 || ! HAM)',)),
            #
            (19, '\n', None, ()),
            #
            (20, '#ifndef HAM\n',
                IfDirective('ifndef', 'HAM'),
                ()),
            (21, '   print("no ham");\n',
                None,
                ('! defined(HAM)',)),
            (22, '#endif\n',
                OtherDirective('endif', None),
                ('! defined(HAM)',)),
            #
            (23, 'print("end");', None, ()),
            ])

    @unittest.skipIf(sys.platform == 'win32', 'needs fix under Windows')
    def test_typical(self):
        # We use Include/compile.h from commit 66c4f3f38b86.  It has
        # a good enough mix of code without being too large.
        directives = [
            IfDirective('ifndef', 'Py_COMPILE_H'),
            Constant('Py_COMPILE_H', None),

            IfDirective('ifndef', 'Py_LIMITED_API'),

            Include('"code.h"'),

            IfDirective('ifdef', '__cplusplus'),
            OtherDirective('endif', None),

            Constant('PyCF_MASK', '(CO_FUTURE_DIVISION | CO_FUTURE_ABSOLUTE_IMPORT | CO_FUTURE_WITH_STATEMENT | CO_FUTURE_PRINT_FUNCTION | CO_FUTURE_UNICODE_LITERALS | CO_FUTURE_BARRY_AS_BDFL | CO_FUTURE_GENERATOR_STOP | CO_FUTURE_ANNOTATIONS)'),
            Constant('PyCF_MASK_OBSOLETE', '(CO_NESTED)'),
            Constant('PyCF_SOURCE_IS_UTF8', ' 0x0100'),
            Constant('PyCF_DONT_IMPLY_DEDENT', '0x0200'),
            Constant('PyCF_ONLY_AST', '0x0400'),
            Constant('PyCF_IGNORE_COOKIE', '0x0800'),
            Constant('PyCF_TYPE_COMMENTS', '0x1000'),
            Constant('PyCF_ALLOW_TOP_LEVEL_AWAIT', '0x2000'),

            IfDirective('ifndef', 'Py_LIMITED_API'),
            OtherDirective('endif', None),

            Constant('FUTURE_NESTED_SCOPES', '"nested_scopes"'),
            Constant('FUTURE_GENERATORS', '"generators"'),
            Constant('FUTURE_DIVISION', '"division"'),
            Constant('FUTURE_ABSOLUTE_IMPORT', '"absolute_import"'),
            Constant('FUTURE_WITH_STATEMENT', '"with_statement"'),
            Constant('FUTURE_PRINT_FUNCTION', '"print_function"'),
            Constant('FUTURE_UNICODE_LITERALS', '"unicode_literals"'),
            Constant('FUTURE_BARRY_AS_BDFL', '"barry_as_FLUFL"'),
            Constant('FUTURE_GENERATOR_STOP', '"generator_stop"'),
            Constant('FUTURE_ANNOTATIONS', '"annotations"'),

            Macro('PyAST_Compile', ('mod', 's', 'f', 'ar'), 'PyAST_CompileEx(mod, s, f, -1, ar)'),

            Constant('PY_INVALID_STACK_EFFECT', 'INT_MAX'),

            IfDirective('ifdef', '__cplusplus'),
            OtherDirective('endif', None),

            OtherDirective('endif', None),  # ifndef Py_LIMITED_API

            Constant('Py_single_input', '256'),
            Constant('Py_file_input', '257'),
            Constant('Py_eval_input', '258'),
            Constant('Py_func_type_input', '345'),

            OtherDirective('endif', None),  # ifndef Py_COMPILE_H
            ]
        self.parsed = list(directives)
        text = textwrap.dedent(r'''
            #ifndef Py_COMPILE_H
            #define Py_COMPILE_H

            #ifndef Py_LIMITED_API
            #include "code.h"

            #ifdef __cplusplus
            extern "C" {
            #endif

            /* Public interface */
            struct _node; /* Declare the existence of this type */
            PyAPI_FUNC(PyCodeObject *) PyNode_Compile(struct _node *, const char *);
            /* XXX (ncoghlan): Unprefixed type name in a public API! */

            #define PyCF_MASK (CO_FUTURE_DIVISION | CO_FUTURE_ABSOLUTE_IMPORT | \
                               CO_FUTURE_WITH_STATEMENT | CO_FUTURE_PRINT_FUNCTION | \
                               CO_FUTURE_UNICODE_LITERALS | CO_FUTURE_BARRY_AS_BDFL | \
                               CO_FUTURE_GENERATOR_STOP | CO_FUTURE_ANNOTATIONS)
            #define PyCF_MASK_OBSOLETE (CO_NESTED)
            #define PyCF_SOURCE_IS_UTF8  0x0100
            #define PyCF_DONT_IMPLY_DEDENT 0x0200
            #define PyCF_ONLY_AST 0x0400
            #define PyCF_IGNORE_COOKIE 0x0800
            #define PyCF_TYPE_COMMENTS 0x1000
            #define PyCF_ALLOW_TOP_LEVEL_AWAIT 0x2000

            #ifndef Py_LIMITED_API
            typedef struct {
                int cf_flags;  /* bitmask of CO_xxx flags relevant to future */
                int cf_feature_version;  /* minor Python version (PyCF_ONLY_AST) */
            } PyCompilerFlags;
            #endif

            /* Future feature support */

            typedef struct {
                int ff_features;      /* flags set by future statements */
                int ff_lineno;        /* line number of last future statement */
            } PyFutureFeatures;

            #define FUTURE_NESTED_SCOPES "nested_scopes"
            #define FUTURE_GENERATORS "generators"
            #define FUTURE_DIVISION "division"
            #define FUTURE_ABSOLUTE_IMPORT "absolute_import"
            #define FUTURE_WITH_STATEMENT "with_statement"
            #define FUTURE_PRINT_FUNCTION "print_function"
            #define FUTURE_UNICODE_LITERALS "unicode_literals"
            #define FUTURE_BARRY_AS_BDFL "barry_as_FLUFL"
            #define FUTURE_GENERATOR_STOP "generator_stop"
            #define FUTURE_ANNOTATIONS "annotations"

            struct _mod; /* Declare the existence of this type */
            #define PyAST_Compile(mod, s, f, ar) PyAST_CompileEx(mod, s, f, -1, ar)
            PyAPI_FUNC(PyCodeObject *) PyAST_CompileEx(
                struct _mod *mod,
                const char *filename,       /* decoded from the filesystem encoding */
                PyCompilerFlags *flags,
                int optimize,
                PyArena *arena);
            PyAPI_FUNC(PyCodeObject *) PyAST_CompileObject(
                struct _mod *mod,
                PyObject *filename,
                PyCompilerFlags *flags,
                int optimize,
                PyArena *arena);
            PyAPI_FUNC(PyFutureFeatures *) PyFuture_FromAST(
                struct _mod * mod,
                const char *filename        /* decoded from the filesystem encoding */
                );
            PyAPI_FUNC(PyFutureFeatures *) PyFuture_FromASTObject(
                struct _mod * mod,
                PyObject *filename
                );

            /* _Py_Mangle is defined in compile.c */
            PyAPI_FUNC(PyObject*) _Py_Mangle(PyObject *p, PyObject *name);

            #define PY_INVALID_STACK_EFFECT INT_MAX
            PyAPI_FUNC(int) PyCompile_OpcodeStackEffect(int opcode, int oparg);
            PyAPI_FUNC(int) PyCompile_OpcodeStackEffectWithJump(int opcode, int oparg, int jump);

            PyAPI_FUNC(int) _PyAST_Optimize(struct _mod *, PyArena *arena, int optimize);

            #ifdef __cplusplus
            }
            #endif

            #endif /* !Py_LIMITED_API */

            /* These definitions must match corresponding definitions in graminit.h. */
            #define Py_single_input 256
            #define Py_file_input 257
            #define Py_eval_input 258
            #define Py_func_type_input 345

            #endif /* !Py_COMPILE_H */
            ''').strip()
        lines = [line + '\n' for line in text.splitlines()]
        lines[-1] = lines[-1][:-1]

        results = list(
                iter_lines(lines, _parse_directive=self._parse_directive))

        self.assertEqual(results, [
            (1, '#ifndef Py_COMPILE_H\n',
                IfDirective('ifndef', 'Py_COMPILE_H'),
                ()),
            (2, '#define Py_COMPILE_H\n',
                Constant('Py_COMPILE_H', None),
                ('! defined(Py_COMPILE_H)',)),
            (3, '\n',
                None,
                ('! defined(Py_COMPILE_H)',)),
            (4, '#ifndef Py_LIMITED_API\n',
                IfDirective('ifndef', 'Py_LIMITED_API'),
                ('! defined(Py_COMPILE_H)',)),
            (5, '#include "code.h"\n',
                Include('"code.h"'),
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (6, '\n',
                None,
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (7, '#ifdef __cplusplus\n',
                IfDirective('ifdef', '__cplusplus'),
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (8, 'extern "C" {\n',
                None,
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)', 'defined(__cplusplus)')),
            (9, '#endif\n',
                OtherDirective('endif', None),
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)', 'defined(__cplusplus)')),
            (10, '\n',
                None,
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (11, ' \n',
                None,
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (12, 'struct _node;  \n',
                None,
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (13, 'PyAPI_FUNC(PyCodeObject *) PyNode_Compile(struct _node *, const char *);\n',
                None,
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (14, ' \n',
                None,
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (15, '\n',
                None,
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (19, '#define PyCF_MASK (CO_FUTURE_DIVISION | CO_FUTURE_ABSOLUTE_IMPORT |                     CO_FUTURE_WITH_STATEMENT | CO_FUTURE_PRINT_FUNCTION |                     CO_FUTURE_UNICODE_LITERALS | CO_FUTURE_BARRY_AS_BDFL |                     CO_FUTURE_GENERATOR_STOP | CO_FUTURE_ANNOTATIONS)\n',
                Constant('PyCF_MASK', '(CO_FUTURE_DIVISION | CO_FUTURE_ABSOLUTE_IMPORT | CO_FUTURE_WITH_STATEMENT | CO_FUTURE_PRINT_FUNCTION | CO_FUTURE_UNICODE_LITERALS | CO_FUTURE_BARRY_AS_BDFL | CO_FUTURE_GENERATOR_STOP | CO_FUTURE_ANNOTATIONS)'),
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (20, '#define PyCF_MASK_OBSOLETE (CO_NESTED)\n',
                Constant('PyCF_MASK_OBSOLETE', '(CO_NESTED)'),
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (21, '#define PyCF_SOURCE_IS_UTF8  0x0100\n',
                Constant('PyCF_SOURCE_IS_UTF8', ' 0x0100'),
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (22, '#define PyCF_DONT_IMPLY_DEDENT 0x0200\n',
                Constant('PyCF_DONT_IMPLY_DEDENT', '0x0200'),
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (23, '#define PyCF_ONLY_AST 0x0400\n',
                Constant('PyCF_ONLY_AST', '0x0400'),
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (24, '#define PyCF_IGNORE_COOKIE 0x0800\n',
                Constant('PyCF_IGNORE_COOKIE', '0x0800'),
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (25, '#define PyCF_TYPE_COMMENTS 0x1000\n',
                Constant('PyCF_TYPE_COMMENTS', '0x1000'),
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (26, '#define PyCF_ALLOW_TOP_LEVEL_AWAIT 0x2000\n',
                Constant('PyCF_ALLOW_TOP_LEVEL_AWAIT', '0x2000'),
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (27, '\n',
                None,
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (28, '#ifndef Py_LIMITED_API\n',
                IfDirective('ifndef', 'Py_LIMITED_API'),
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (29, 'typedef struct {\n',
                None,
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)', '! defined(Py_LIMITED_API)')),
            (30, '    int cf_flags;   \n',
                None,
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)', '! defined(Py_LIMITED_API)')),
            (31, '    int cf_feature_version;   \n',
                None,
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)', '! defined(Py_LIMITED_API)')),
            (32, '} PyCompilerFlags;\n',
                None,
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)', '! defined(Py_LIMITED_API)')),
            (33, '#endif\n',
                OtherDirective('endif', None),
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)', '! defined(Py_LIMITED_API)')),
            (34, '\n',
                None,
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (35, ' \n',
                None,
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (36, '\n',
                None,
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (37, 'typedef struct {\n',
                None,
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (38, '    int ff_features;       \n',
                None,
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (39, '    int ff_lineno;         \n',
                None,
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (40, '} PyFutureFeatures;\n',
                None,
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (41, '\n',
                None,
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (42, '#define FUTURE_NESTED_SCOPES "nested_scopes"\n',
                Constant('FUTURE_NESTED_SCOPES', '"nested_scopes"'),
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (43, '#define FUTURE_GENERATORS "generators"\n',
                Constant('FUTURE_GENERATORS', '"generators"'),
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (44, '#define FUTURE_DIVISION "division"\n',
                Constant('FUTURE_DIVISION', '"division"'),
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (45, '#define FUTURE_ABSOLUTE_IMPORT "absolute_import"\n',
                Constant('FUTURE_ABSOLUTE_IMPORT', '"absolute_import"'),
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (46, '#define FUTURE_WITH_STATEMENT "with_statement"\n',
                Constant('FUTURE_WITH_STATEMENT', '"with_statement"'),
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (47, '#define FUTURE_PRINT_FUNCTION "print_function"\n',
                Constant('FUTURE_PRINT_FUNCTION', '"print_function"'),
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (48, '#define FUTURE_UNICODE_LITERALS "unicode_literals"\n',
                Constant('FUTURE_UNICODE_LITERALS', '"unicode_literals"'),
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (49, '#define FUTURE_BARRY_AS_BDFL "barry_as_FLUFL"\n',
                Constant('FUTURE_BARRY_AS_BDFL', '"barry_as_FLUFL"'),
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (50, '#define FUTURE_GENERATOR_STOP "generator_stop"\n',
                Constant('FUTURE_GENERATOR_STOP', '"generator_stop"'),
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (51, '#define FUTURE_ANNOTATIONS "annotations"\n',
                Constant('FUTURE_ANNOTATIONS', '"annotations"'),
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (52, '\n',
                None,
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (53, 'struct _mod;  \n',
                None,
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (54, '#define PyAST_Compile(mod, s, f, ar) PyAST_CompileEx(mod, s, f, -1, ar)\n',
                Macro('PyAST_Compile', ('mod', 's', 'f', 'ar'), 'PyAST_CompileEx(mod, s, f, -1, ar)'),
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (55, 'PyAPI_FUNC(PyCodeObject *) PyAST_CompileEx(\n',
                None,
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (56, '    struct _mod *mod,\n',
                None,
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (57, '    const char *filename,        \n',
                None,
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (58, '    PyCompilerFlags *flags,\n',
                None,
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (59, '    int optimize,\n',
                None,
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (60, '    PyArena *arena);\n',
                None,
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (61, 'PyAPI_FUNC(PyCodeObject *) PyAST_CompileObject(\n',
                None,
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (62, '    struct _mod *mod,\n',
                None,
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (63, '    PyObject *filename,\n',
                None,
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (64, '    PyCompilerFlags *flags,\n',
                None,
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (65, '    int optimize,\n',
                None,
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (66, '    PyArena *arena);\n',
                None,
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (67, 'PyAPI_FUNC(PyFutureFeatures *) PyFuture_FromAST(\n',
                None,
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (68, '    struct _mod * mod,\n',
                None,
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (69, '    const char *filename         \n',
                None,
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (70, '    );\n',
                None,
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (71, 'PyAPI_FUNC(PyFutureFeatures *) PyFuture_FromASTObject(\n',
                None,
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (72, '    struct _mod * mod,\n',
                None,
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (73, '    PyObject *filename\n',
                None,
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (74, '    );\n',
                None,
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (75, '\n',
                None,
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (76, ' \n',
                None,
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (77, 'PyAPI_FUNC(PyObject*) _Py_Mangle(PyObject *p, PyObject *name);\n',
                None,
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (78, '\n',
                None,
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (79, '#define PY_INVALID_STACK_EFFECT INT_MAX\n',
                Constant('PY_INVALID_STACK_EFFECT', 'INT_MAX'),
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (80, 'PyAPI_FUNC(int) PyCompile_OpcodeStackEffect(int opcode, int oparg);\n',
                None,
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (81, 'PyAPI_FUNC(int) PyCompile_OpcodeStackEffectWithJump(int opcode, int oparg, int jump);\n',
                None,
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (82, '\n',
                None,
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (83, 'PyAPI_FUNC(int) _PyAST_Optimize(struct _mod *, PyArena *arena, int optimize);\n',
                None,
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (84, '\n',
                None,
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (85, '#ifdef __cplusplus\n',
                IfDirective('ifdef', '__cplusplus'),
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (86, '}\n',
                None,
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)', 'defined(__cplusplus)')),
            (87, '#endif\n',
                OtherDirective('endif', None),
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)', 'defined(__cplusplus)')),
            (88, '\n',
                None,
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (89, '#endif  \n',
                OtherDirective('endif', None),
                ('! defined(Py_COMPILE_H)', '! defined(Py_LIMITED_API)')),
            (90, '\n',
                None,
                ('! defined(Py_COMPILE_H)',)),
            (91, ' \n',
                None,
                ('! defined(Py_COMPILE_H)',)),
            (92, '#define Py_single_input 256\n',
                Constant('Py_single_input', '256'),
                ('! defined(Py_COMPILE_H)',)),
            (93, '#define Py_file_input 257\n',
                Constant('Py_file_input', '257'),
                ('! defined(Py_COMPILE_H)',)),
            (94, '#define Py_eval_input 258\n',
                Constant('Py_eval_input', '258'),
                ('! defined(Py_COMPILE_H)',)),
            (95, '#define Py_func_type_input 345\n',
                Constant('Py_func_type_input', '345'),
                ('! defined(Py_COMPILE_H)',)),
            (96, '\n',
                None,
                ('! defined(Py_COMPILE_H)',)),
            (97, '#endif  ',
                OtherDirective('endif', None),
                ('! defined(Py_COMPILE_H)',)),
            ])
        self.check_calls(
                ('_parse_directive', '#ifndef Py_COMPILE_H'),
                ('_parse_directive', '#define Py_COMPILE_H'),
                ('_parse_directive', '#ifndef Py_LIMITED_API'),
                ('_parse_directive', '#include "code.h"'),
                ('_parse_directive', '#ifdef __cplusplus'),
                ('_parse_directive', '#endif'),
                ('_parse_directive', '#define PyCF_MASK (CO_FUTURE_DIVISION | CO_FUTURE_ABSOLUTE_IMPORT | CO_FUTURE_WITH_STATEMENT | CO_FUTURE_PRINT_FUNCTION | CO_FUTURE_UNICODE_LITERALS | CO_FUTURE_BARRY_AS_BDFL | CO_FUTURE_GENERATOR_STOP | CO_FUTURE_ANNOTATIONS)'),
                ('_parse_directive', '#define PyCF_MASK_OBSOLETE (CO_NESTED)'),
                ('_parse_directive', '#define PyCF_SOURCE_IS_UTF8 0x0100'),
                ('_parse_directive', '#define PyCF_DONT_IMPLY_DEDENT 0x0200'),
                ('_parse_directive', '#define PyCF_ONLY_AST 0x0400'),
                ('_parse_directive', '#define PyCF_IGNORE_COOKIE 0x0800'),
                ('_parse_directive', '#define PyCF_TYPE_COMMENTS 0x1000'),
                ('_parse_directive', '#define PyCF_ALLOW_TOP_LEVEL_AWAIT 0x2000'),
                ('_parse_directive', '#ifndef Py_LIMITED_API'),
                ('_parse_directive', '#endif'),
                ('_parse_directive', '#define FUTURE_NESTED_SCOPES "nested_scopes"'),
                ('_parse_directive', '#define FUTURE_GENERATORS "generators"'),
                ('_parse_directive', '#define FUTURE_DIVISION "division"'),
                ('_parse_directive', '#define FUTURE_ABSOLUTE_IMPORT "absolute_import"'),
                ('_parse_directive', '#define FUTURE_WITH_STATEMENT "with_statement"'),
                ('_parse_directive', '#define FUTURE_PRINT_FUNCTION "print_function"'),
                ('_parse_directive', '#define FUTURE_UNICODE_LITERALS "unicode_literals"'),
                ('_parse_directive', '#define FUTURE_BARRY_AS_BDFL "barry_as_FLUFL"'),
                ('_parse_directive', '#define FUTURE_GENERATOR_STOP "generator_stop"'),
                ('_parse_directive', '#define FUTURE_ANNOTATIONS "annotations"'),
                ('_parse_directive', '#define PyAST_Compile(mod, s, f, ar) PyAST_CompileEx(mod, s, f, -1, ar)'),
                ('_parse_directive', '#define PY_INVALID_STACK_EFFECT INT_MAX'),
                ('_parse_directive', '#ifdef __cplusplus'),
                ('_parse_directive', '#endif'),
                ('_parse_directive', '#endif'),
                ('_parse_directive', '#define Py_single_input 256'),
                ('_parse_directive', '#define Py_file_input 257'),
                ('_parse_directive', '#define Py_eval_input 258'),
                ('_parse_directive', '#define Py_func_type_input 345'),
                ('_parse_directive', '#endif'),
                )


class ParseDirectiveTests(unittest.TestCase):

    def test_directives(self):
        tests = [
            # includes
            ('#include "internal/pycore_pystate.h"', Include('"internal/pycore_pystate.h"')),
            ('#include <stdio>', Include('<stdio>')),

            # defines
            ('#define SPAM int', Constant('SPAM', 'int')),
            ('#define SPAM', Constant('SPAM', '')),
            ('#define SPAM(x, y) run(x, y)', Macro('SPAM', ('x', 'y'), 'run(x, y)')),
            ('#undef SPAM', None),

            # conditionals
            ('#if SPAM', IfDirective('if', 'SPAM')),
            # XXX complex conditionls
            ('#ifdef SPAM', IfDirective('ifdef', 'SPAM')),
            ('#ifndef SPAM', IfDirective('ifndef', 'SPAM')),
            ('#elseif SPAM', IfDirective('elseif', 'SPAM')),
            # XXX complex conditionls
            ('#else', OtherDirective('else', '')),
            ('#endif', OtherDirective('endif', '')),

            # other
            ('#error oops!', None),
            ('#warning oops!', None),
            ('#pragma ...', None),
            ('#__FILE__ ...', None),
            ('#__LINE__ ...', None),
            ('#__DATE__ ...', None),
            ('#__TIME__ ...', None),
            ('#__TIMESTAMP__ ...', None),

            # extra whitespace
            (' # include  <stdio> ', Include('<stdio>')),
            ('#else  ', OtherDirective('else', '')),
            ('#endif  ', OtherDirective('endif', '')),
            ('#define SPAM int  ', Constant('SPAM', 'int')),
            ('#define SPAM  ', Constant('SPAM', '')),
            ]
        for line, expected in tests:
            if expected is None:
                kind, _, text = line[1:].partition(' ')
                expected = OtherDirective(kind, text)
            with self.subTest(line):
                directive = parse_directive(line)

                self.assertEqual(directive, expected)

    def test_bad_directives(self):
        tests = [
            # valid directives with bad text
            '#define 123',
            '#else spam',
            '#endif spam',
            ]
        for kind in PreprocessorDirective.KINDS:
            # missing leading "#"
            tests.append(kind)
            if kind in ('else', 'endif'):
                continue
            # valid directives with missing text
            tests.append('#' + kind)
            tests.append('#' + kind + ' ')
        for line in tests:
            with self.subTest(line):
                with self.assertRaises(ValueError):
                    parse_directive(line)

    def test_not_directives(self):
        tests = [
            '',
            ' ',
            'directive',
            'directive?',
            '???',
            ]
        for line in tests:
            with self.subTest(line):
                with self.assertRaises(ValueError):
                    parse_directive(line)


class ConstantTests(unittest.TestCase):

    def test_type(self):
        directive = Constant('SPAM', '123')

        self.assertIs(type(directive), Constant)
        self.assertIsInstance(directive, PreprocessorDirective)

    def test_attrs(self):
        d = Constant('SPAM', '123')
        kind, name, value = d.kind, d.name, d.value

        self.assertEqual(kind, 'define')
        self.assertEqual(name, 'SPAM')
        self.assertEqual(value, '123')

    def test_text(self):
        tests = [
            (('SPAM', '123'), 'SPAM 123'),
            (('SPAM',), 'SPAM'),
            ]
        for args, expected in tests:
            with self.subTest(args):
                d = Constant(*args)
                text = d.text

                self.assertEqual(text, expected)

    def test_iter(self):
        kind, name, value = Constant('SPAM', '123')

        self.assertEqual(kind, 'define')
        self.assertEqual(name, 'SPAM')
        self.assertEqual(value, '123')

    def test_defaults(self):
        kind, name, value = Constant('SPAM')

        self.assertEqual(kind, 'define')
        self.assertEqual(name, 'SPAM')
        self.assertIs(value, None)

    def test_coerce(self):
        tests = []
        # coerced name, value
        for args in wrapped_arg_combos('SPAM', '123'):
            tests.append((args, ('SPAM', '123')))
        # missing name, value
        for name in ('', ' ', None, StrProxy(' '), ()):
            for value in ('', ' ', None, StrProxy(' '), ()):
                tests.append(
                        ((name, value), (None, None)))
        # whitespace
        tests.extend([
            ((' SPAM ', ' 123 '), ('SPAM', '123')),
            ])

        for args, expected in tests:
            with self.subTest(args):
                d = Constant(*args)

                self.assertEqual(d[1:], expected)
                for i, exp in enumerate(expected, start=1):
                    if exp is not None:
                        self.assertIs(type(d[i]), str)

    def test_valid(self):
        tests = [
            ('SPAM', '123'),
            # unusual name
            ('_SPAM_', '123'),
            ('X_1', '123'),
            # unusual value
            ('SPAM', None),
            ]
        for args in tests:
            with self.subTest(args):
                directive = Constant(*args)

                directive.validate()

    def test_invalid(self):
        tests = [
            # invalid name
            ((None, '123'), TypeError),
            (('_', '123'), ValueError),
            (('1', '123'), ValueError),
            (('_1_', '123'), ValueError),
            # There is no invalid value (including None).
            ]
        for args, exctype in tests:
            with self.subTest(args):
                directive = Constant(*args)

                with self.assertRaises(exctype):
                    directive.validate()


class MacroTests(unittest.TestCase):

    def test_type(self):
        directive = Macro('SPAM', ('x', 'y'), '123')

        self.assertIs(type(directive), Macro)
        self.assertIsInstance(directive, PreprocessorDirective)

    def test_attrs(self):
        d = Macro('SPAM', ('x', 'y'), '123')
        kind, name, args, body = d.kind, d.name, d.args, d.body

        self.assertEqual(kind, 'define')
        self.assertEqual(name, 'SPAM')
        self.assertEqual(args, ('x', 'y'))
        self.assertEqual(body, '123')

    def test_text(self):
        tests = [
            (('SPAM', ('x', 'y'), '123'), 'SPAM(x, y) 123'),
            (('SPAM', ('x', 'y'),), 'SPAM(x, y)'),
            ]
        for args, expected in tests:
            with self.subTest(args):
                d = Macro(*args)
                text = d.text

                self.assertEqual(text, expected)

    def test_iter(self):
        kind, name, args, body = Macro('SPAM', ('x', 'y'), '123')

        self.assertEqual(kind, 'define')
        self.assertEqual(name, 'SPAM')
        self.assertEqual(args, ('x', 'y'))
        self.assertEqual(body, '123')

    def test_defaults(self):
        kind, name, args, body = Macro('SPAM', ('x', 'y'))

        self.assertEqual(kind, 'define')
        self.assertEqual(name, 'SPAM')
        self.assertEqual(args, ('x', 'y'))
        self.assertIs(body, None)

    def test_coerce(self):
        tests = []
        # coerce name and body
        for args in wrapped_arg_combos('SPAM', ('x', 'y'), '123'):
            tests.append(
                    (args, ('SPAM', ('x', 'y'), '123')))
        # coerce args
        tests.extend([
            (('SPAM', 'x', '123'),
             ('SPAM', ('x',), '123')),
            (('SPAM', 'x,y', '123'),
             ('SPAM', ('x', 'y'), '123')),
            ])
        # coerce arg names
        for argnames in wrapped_arg_combos('x', 'y'):
            tests.append(
                    (('SPAM', argnames, '123'),
                     ('SPAM', ('x', 'y'), '123')))
        # missing name, body
        for name in ('', ' ', None, StrProxy(' '), ()):
            for argnames in (None, ()):
                for body in ('', ' ', None, StrProxy(' '), ()):
                    tests.append(
                            ((name, argnames, body),
                             (None, (), None)))
        # missing args
        tests.extend([
            (('SPAM', None, '123'),
             ('SPAM', (), '123')),
            (('SPAM', (), '123'),
             ('SPAM', (), '123')),
            ])
        # missing arg names
        for arg in ('', ' ', None, StrProxy(' '), ()):
            tests.append(
                    (('SPAM', (arg,), '123'),
                     ('SPAM', (None,), '123')))
        tests.extend([
            (('SPAM', ('x', '', 'z'), '123'),
             ('SPAM', ('x', None, 'z'), '123')),
            ])
        # whitespace
        tests.extend([
            ((' SPAM ', (' x ', ' y '), ' 123 '),
             ('SPAM', ('x', 'y'), '123')),
            (('SPAM', 'x, y', '123'),
             ('SPAM', ('x', 'y'), '123')),
            ])

        for args, expected in tests:
            with self.subTest(args):
                d = Macro(*args)

                self.assertEqual(d[1:], expected)
                for i, exp in enumerate(expected, start=1):
                    if i == 2:
                        self.assertIs(type(d[i]), tuple)
                    elif exp is not None:
                        self.assertIs(type(d[i]), str)

    def test_init_bad_args(self):
        tests = [
            ('SPAM', StrProxy('x'), '123'),
            ('SPAM', object(), '123'),
            ]
        for args in tests:
            with self.subTest(args):
                with self.assertRaises(TypeError):
                    Macro(*args)

    def test_valid(self):
        tests = [
            # unusual name
            ('SPAM', ('x', 'y'), 'run(x, y)'),
            ('_SPAM_', ('x', 'y'), 'run(x, y)'),
            ('X_1', ('x', 'y'), 'run(x, y)'),
            # unusual args
            ('SPAM', (), 'run(x, y)'),
            ('SPAM', ('_x_', 'y_1'), 'run(x, y)'),
            ('SPAM', 'x', 'run(x, y)'),
            ('SPAM', 'x, y', 'run(x, y)'),
            # unusual body
            ('SPAM', ('x', 'y'), None),
            ]
        for args in tests:
            with self.subTest(args):
                directive = Macro(*args)

                directive.validate()

    def test_invalid(self):
        tests = [
            # invalid name
            ((None, ('x', 'y'), '123'), TypeError),
            (('_', ('x', 'y'), '123'), ValueError),
            (('1', ('x', 'y'), '123'), ValueError),
            (('_1', ('x', 'y'), '123'), ValueError),
            # invalid args
            (('SPAM', (None, 'y'), '123'), ValueError),
            (('SPAM', ('x', '_'), '123'), ValueError),
            (('SPAM', ('x', '1'), '123'), ValueError),
            (('SPAM', ('x', '_1_'), '123'), ValueError),
            # There is no invalid body (including None).
            ]
        for args, exctype in tests:
            with self.subTest(args):
                directive = Macro(*args)

                with self.assertRaises(exctype):
                    directive.validate()


class IfDirectiveTests(unittest.TestCase):

    def test_type(self):
        directive = IfDirective('if', '1')

        self.assertIs(type(directive), IfDirective)
        self.assertIsInstance(directive, PreprocessorDirective)

    def test_attrs(self):
        d = IfDirective('if', '1')
        kind, condition = d.kind, d.condition

        self.assertEqual(kind, 'if')
        self.assertEqual(condition, '1')
        #self.assertEqual(condition, (ArithmeticCondition('1'),))

    def test_text(self):
        tests = [
            (('if', 'defined(SPAM) && 1 || (EGGS > 3 && defined(HAM))'),
             'defined(SPAM) && 1 || (EGGS > 3 && defined(HAM))'),
            ]
        for kind in IfDirective.KINDS:
            tests.append(
                    ((kind, 'SPAM'), 'SPAM'))
        for args, expected in tests:
            with self.subTest(args):
                d = IfDirective(*args)
                text = d.text

                self.assertEqual(text, expected)

    def test_iter(self):
        kind, condition = IfDirective('if', '1')

        self.assertEqual(kind, 'if')
        self.assertEqual(condition, '1')
        #self.assertEqual(condition, (ArithmeticCondition('1'),))

    #def test_complex_conditions(self):
    #    ...

    def test_coerce(self):
        tests = []
        for kind in IfDirective.KINDS:
            if kind == 'ifdef':
                cond = 'defined(SPAM)'
            elif kind == 'ifndef':
                cond = '! defined(SPAM)'
            else:
                cond = 'SPAM'
            for args in wrapped_arg_combos(kind, 'SPAM'):
                tests.append((args, (kind, cond)))
            tests.extend([
                ((' ' + kind + ' ', ' SPAM '), (kind, cond)),
                ])
            for raw in ('', ' ', None, StrProxy(' '), ()):
                tests.append(((kind, raw), (kind, None)))
        for kind in ('', ' ', None, StrProxy(' '), ()):
            tests.append(((kind, 'SPAM'), (None, 'SPAM')))
        for args, expected in tests:
            with self.subTest(args):
                d = IfDirective(*args)

                self.assertEqual(tuple(d), expected)
                for i, exp in enumerate(expected):
                    if exp is not None:
                        self.assertIs(type(d[i]), str)

    def test_valid(self):
        tests = []
        for kind in IfDirective.KINDS:
            tests.extend([
                (kind, 'SPAM'),
                (kind, '_SPAM_'),
                (kind, 'X_1'),
                (kind, '()'),
                (kind, '--'),
                (kind, '???'),
                ])
        for args in tests:
            with self.subTest(args):
                directive = IfDirective(*args)

                directive.validate()

    def test_invalid(self):
        tests = []
        # kind
        tests.extend([
            ((None, 'SPAM'), TypeError),
            (('_', 'SPAM'), ValueError),
            (('-', 'SPAM'), ValueError),
            (('spam', 'SPAM'), ValueError),
            ])
        for kind in PreprocessorDirective.KINDS:
            if kind in IfDirective.KINDS:
                continue
            tests.append(
                ((kind, 'SPAM'), ValueError))
        # condition
        for kind in IfDirective.KINDS:
            tests.extend([
                ((kind, None), TypeError),
                # Any other condition is valid.
                ])
        for args, exctype in tests:
            with self.subTest(args):
                directive = IfDirective(*args)

                with self.assertRaises(exctype):
                    directive.validate()


class IncludeTests(unittest.TestCase):

    def test_type(self):
        directive = Include('<stdio>')

        self.assertIs(type(directive), Include)
        self.assertIsInstance(directive, PreprocessorDirective)

    def test_attrs(self):
        d = Include('<stdio>')
        kind, file, text = d.kind, d.file, d.text

        self.assertEqual(kind, 'include')
        self.assertEqual(file, '<stdio>')
        self.assertEqual(text, '<stdio>')

    def test_iter(self):
        kind, file = Include('<stdio>')

        self.assertEqual(kind, 'include')
        self.assertEqual(file, '<stdio>')

    def test_coerce(self):
        tests = []
        for arg, in wrapped_arg_combos('<stdio>'):
            tests.append((arg, '<stdio>'))
        tests.extend([
            (' <stdio> ', '<stdio>'),
            ])
        for arg in ('', ' ', None, StrProxy(' '), ()):
            tests.append((arg, None ))
        for arg, expected in tests:
            with self.subTest(arg):
                _, file = Include(arg)

                self.assertEqual(file, expected)
                if expected is not None:
                    self.assertIs(type(file), str)

    def test_valid(self):
        tests = [
            '<stdio>',
            '"spam.h"',
            '"internal/pycore_pystate.h"',
            ]
        for arg in tests:
            with self.subTest(arg):
                directive = Include(arg)

                directive.validate()

    def test_invalid(self):
        tests = [
            (None, TypeError),
            # We currently don't check the file.
            ]
        for arg, exctype in tests:
            with self.subTest(arg):
                directive = Include(arg)

                with self.assertRaises(exctype):
                    directive.validate()


class OtherDirectiveTests(unittest.TestCase):

    def test_type(self):
        directive = OtherDirective('undef', 'SPAM')

        self.assertIs(type(directive), OtherDirective)
        self.assertIsInstance(directive, PreprocessorDirective)

    def test_attrs(self):
        d = OtherDirective('undef', 'SPAM')
        kind, text = d.kind, d.text

        self.assertEqual(kind, 'undef')
        self.assertEqual(text, 'SPAM')

    def test_iter(self):
        kind, text = OtherDirective('undef', 'SPAM')

        self.assertEqual(kind, 'undef')
        self.assertEqual(text, 'SPAM')

    def test_coerce(self):
        tests = []
        for kind in OtherDirective.KINDS:
            if kind in ('else', 'endif'):
                continue
            for args in wrapped_arg_combos(kind, '...'):
                tests.append((args, (kind, '...')))
            tests.extend([
                ((' ' + kind + ' ', ' ... '), (kind, '...')),
                ])
            for raw in ('', ' ', None, StrProxy(' '), ()):
                tests.append(((kind, raw), (kind, None)))
        for kind in ('else', 'endif'):
            for args in wrapped_arg_combos(kind, None):
                tests.append((args, (kind, None)))
            tests.extend([
                ((' ' + kind + ' ', None), (kind, None)),
                ])
        for kind in ('', ' ', None, StrProxy(' '), ()):
            tests.append(((kind, '...'), (None, '...')))
        for args, expected in tests:
            with self.subTest(args):
                d = OtherDirective(*args)

                self.assertEqual(tuple(d), expected)
                for i, exp in enumerate(expected):
                    if exp is not None:
                        self.assertIs(type(d[i]), str)

    def test_valid(self):
        tests = []
        for kind in OtherDirective.KINDS:
            if kind in ('else', 'endif'):
                continue
            tests.extend([
                (kind, '...'),
                (kind, '???'),
                (kind, 'SPAM'),
                (kind, '1 + 1'),
                ])
        for kind in ('else', 'endif'):
            tests.append((kind, None))
        for args in tests:
            with self.subTest(args):
                directive = OtherDirective(*args)

                directive.validate()

    def test_invalid(self):
        tests = []
        # kind
        tests.extend([
            ((None, '...'), TypeError),
            (('_', '...'), ValueError),
            (('-', '...'), ValueError),
            (('spam', '...'), ValueError),
            ])
        for kind in PreprocessorDirective.KINDS:
            if kind in OtherDirective.KINDS:
                continue
            tests.append(
                ((kind, None), ValueError))
        # text
        for kind in OtherDirective.KINDS:
            if kind in ('else', 'endif'):
                tests.extend([
                    # Any text is invalid.
                    ((kind, 'SPAM'), ValueError),
                    ((kind, '...'), ValueError),
                    ])
            else:
                tests.extend([
                    ((kind, None), TypeError),
                    # Any other text is valid.
                    ])
        for args, exctype in tests:
            with self.subTest(args):
                directive = OtherDirective(*args)

                with self.assertRaises(exctype):
                    directive.validate()
