'Provides "Strip trailing whitespace" under the "Format" menu.'

class Rstrip:

    def __init__(self, editwin):
        self.editwin = editwin

    def do_rstrip(self, event=None):

        text = self.editwin.text
        undo = self.editwin.undo

        undo.undo_block_start()

        end_line = int(float(text.index('end')))
        for cur in range(1, end_line):
            txt = text.get('%i.0' % cur, '%i.end' % cur)
            raw = len(txt)
            cut = len(txt.rstrip())
            # Since text.delete() marks file as changed, even if not,
            # only call it when needed to actually delete something.
            if cut < raw:
                text.delete('%i.%i' % (cur, cut), '%i.end' % cur)

        undo.undo_block_stop()

if __name__ == "__main__":
    from unittest import main
    main('idlelib.idle_test.test_rstrip', verbosity=2,)
