import W

w = W.Window((600, 400), "Ha!", minsize = (240, 200))

w.panes = W.HorizontalPanes((8, 8, -8, -20), (0.6, 0.4))

w.panes.panes = W.VerticalPanes(None, (0.3, 0.4, 0.3))
w.panes.panes.blah1 = W.EditText(None, "eehhh...")
w.panes.panes.blah2 = W.EditText(None, "nou...")
w.panes.panes.blah3 = W.List(None, ["eehhh...", 'abc', 'def'])

w.panes.group = W.Group(None)
w.panes.group.mytext = W.EditText((0, 24, 0, 0), "eehhh...")
w.panes.group.button1 = W.Button((0, 0, 80, 16), "A Button")

w.open()
