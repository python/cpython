'Provides "Trim trailing whitespace and blank line" option'


class TrimExtension:

    def __init__(self, editwin):
        self.editwin = editwin
        self.editwin.text.bind('<<do-trim>>', self.do_trim)

    def do_trim(self, event=None):
        text = self.editwin.text
        undo = self.editwin.undo

        undo.undo_block_start()

        last = 0
        end_line = int(float(text.index('end')))
        for cur in range(1, end_line):
            txt = text.get('%i.0' % cur, '%i.end' % cur)
            raw = len(txt)
            cut = len(txt.rstrip())

            # Get the last non-blank line of code
            if cut:
                last = cur

            # Since text.delete() marks file as changed, even if not,
            # only call it when needed to actually delete something.
            if cut < raw:
                text.delete('%i.%i' % (cur, cut), '%i.end' % cur)

        # Trim trailing blank line
        text.delete('end-%ic linestart' % (end_line - last - 1), 'end')

        undo.undo_block_stop()


if __name__ == "__main__":
    import unittest
    unittest.main('idlelib.idle_test.test_trim', verbosity=2, exit=False)
