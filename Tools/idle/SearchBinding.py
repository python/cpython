import tkSimpleDialog

###$ event <<find>>
###$ win <Control-f>
###$ unix <Control-u><Control-u><Control-s>

###$ event <<find-again>>
###$ win <Control-g>
###$ win <F3>
###$ unix <Control-u><Control-s>

###$ event <<find-selection>>
###$ win <Control-F3>
###$ unix <Control-s>

###$ event <<find-in-files>>
###$ win <Alt-F3>

###$ event <<replace>>
###$ win <Control-h>

###$ event <<goto-line>>
###$ win <Alt-g>
###$ unix <Alt-g>

class SearchBinding:

    windows_keydefs = {
        '<<find-again>>': ['<Control-g>', '<F3>'],
        '<<find-in-files>>': ['<Alt-F3>'],
        '<<find-selection>>': ['<Control-F3>'],
        '<<find>>': ['<Control-f>'],
        '<<replace>>': ['<Control-h>'],
        '<<goto-line>>': ['<Alt-g>'],
    }

    unix_keydefs = {
        '<<find-again>>': ['<Control-u><Control-s>'],
        '<<find-in-files>>': ['<Alt-s>', '<Meta-s>'],
        '<<find-selection>>': ['<Control-s>'],
        '<<find>>': ['<Control-u><Control-u><Control-s>'],
        '<<replace>>': ['<Control-r>'],
        '<<goto-line>>': ['<Alt-g>', '<Meta-g>'],
    }

    menudefs = [
        ('edit', [
            None,
            ('_Find...', '<<find>>'),
            ('Find a_gain', '<<find-again>>'),
            ('Find _selection', '<<find-selection>>'),
            ('Find in Files...', '<<find-in-files>>'),
            ('R_eplace...', '<<replace>>'),
            ('Go to _line', '<<goto-line>>'),
         ]),
    ]

    def __init__(self, editwin):
        self.editwin = editwin

    def find_event(self, event):
        import SearchDialog
        SearchDialog.find(self.editwin.text)
        return "break"

    def find_again_event(self, event):
        import SearchDialog
        SearchDialog.find_again(self.editwin.text)
        return "break"

    def find_selection_event(self, event):
        import SearchDialog
        SearchDialog.find_selection(self.editwin.text)
        return "break"

    def find_in_files_event(self, event):
        import GrepDialog
        GrepDialog.grep(self.editwin.text, self.editwin.io, self.editwin.flist)
        return "break"

    def replace_event(self, event):
        import ReplaceDialog
        ReplaceDialog.replace(self.editwin.text)
        return "break"

    def goto_line_event(self, event):
        text = self.editwin.text
        lineno = tkSimpleDialog.askinteger("Goto",
                                           "Go to line number:",
                                           parent=text)
        if lineno is None:
            return "break"
        if lineno <= 0:
            text.bell()
            return "break"
        text.mark_set("insert", "%d.0" % lineno)
        text.see("insert")
