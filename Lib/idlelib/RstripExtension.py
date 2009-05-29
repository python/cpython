'Provides "Strip trailing whitespace" under the "Format" menu.'

__author__ = "Roger D. Serwy <roger.serwy at gmail.com>"

class RstripExtension:

    menudefs = [
        ('format', [None,
               ('Strip trailing whitespace', '<<do-rstrip>>'),
       ]),]

    def __init__(self, editwin):
        self.editwin = editwin
        self.editwin.text.bind("<<do-rstrip>>", self.do_rstrip)

    def do_rstrip(self, event=None):

        text = self.editwin.text
        undo = self.editwin.undo

        undo.undo_block_start()

        end_line = int(float(text.index('end'))) + 1
        for cur in range(1, end_line):
            txt = text.get('%i.0' % cur, '%i.0 lineend' % cur)
            cut = len(txt.rstrip())
            text.delete('%i.%i' % (cur, cut), '%i.0 lineend' % cur)

        undo.undo_block_stop()
