import W

w = W.Window((600, 400), "Ha!", minsize = (240, 200))

w.panes = W.HorizontalPanes((8, 8, -30, -8), (0.3, 0.3, 0.4))
w.panes.blah1 = W.EditText(None, "eehhh...")
w.panes.blah2 = W.EditText((8, 8, -8, -8), "xxx nou...")
w.panes.panes = W.VerticalPanes(None, (0.3, 0.4, 0.3))
w.panes.panes.blah1 = W.EditText(None, "eehhh...")
w.panes.panes.blah2 = W.Frame(None)
w.panes.panes.blah2.t = W.EditText((0, 0, 0, 0), "nou...")
w.panes.panes.blah3 = W.List(None, ["eehhh...", 'abc', 'def'])

w.open()
