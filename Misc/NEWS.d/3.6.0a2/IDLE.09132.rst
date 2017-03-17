Issue #27156: Remove obsolete code not used by IDLE.  Replacements:
1. help.txt, replaced by help.html, is out-of-date and should not be used.
Its dedicated viewer has be replaced by the html viewer in help.py.
2. ``import idlever; I = idlever.IDLE_VERSION`` is the same as
``import sys; I = version[:version.index(' ')]``
3. After ``ob = stackviewer.VariablesTreeItem(*args)``,
``ob.keys() == list(ob.object.keys)``.
4. In macosc, runningAsOSXAPP == isAquaTk; idCarbonAquaTk == isCarbonTk