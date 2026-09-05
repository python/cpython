import os.path
import shutil
import tempfile
import unittest

from test.test_tools import imports_under_tool, skip_if_missing


skip_if_missing('c-analyzer')

with imports_under_tool('c-analyzer'):
    from c_parser.info import FileInfo
    from c_parser.parser import parse
    from c_parser.preprocessor import gcc


class StaticAssertTests(unittest.TestCase):
    def parse(self, text):
        return list(parse(
            (FileInfo('test.c', lno), line)
            for lno, line in enumerate(text.splitlines(), 1)
        ))

    def test_global(self):
        for keyword in ('_Static_assert', 'static_assert'):
            with self.subTest(keyword=keyword):
                items = self.parse(f'''
                    int before;
                    {keyword}(
                        256 > 0 && !(256 & (256 - 1)),
                        "buffer size; ("
                    )
                    ; int after;
                ''')
                self.assertEqual([item.name for item in items],
                                 ['before', 'after'])

    def test_struct(self):
        for keyword in ('_Static_assert', 'static_assert'):
            with self.subTest(keyword=keyword):
                items = self.parse(f'''
                    struct example {{
                        int before;
                        {keyword}(sizeof(int) > 0, "int size");
                        int after;
                    }};
                    int global_after;
                ''')
                struct = next(item for item in items
                              if item.name == 'example' and item.data)
                self.assertEqual([field.name for field in struct.data],
                                 ['before', 'after'])
                self.assertEqual(items[-1].name, 'global_after')

    @unittest.skipUnless(shutil.which('gcc'), 'requires gcc')
    def test_preprocess(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            filename = os.path.join(tmpdir, 'test.c')
            with open(filename, 'w', encoding='utf-8') as source:
                source.write('''
                    #define _GNU_SOURCE 1
                    #include <assert.h>
                    static_assert(256 > 0 && !(256 & 255), "buffer size");
                    int global_after;
                ''')
            lines = gcc.preprocess(filename, samefiles=(), cwd=tmpdir)
            items = list(parse((line.file, line.data) for line in lines
                               if line.kind == 'source'))
        self.assertEqual([item.name for item in items], ['global_after'])


if __name__ == '__main__':
    unittest.main()
