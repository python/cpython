# Generate custlib.tex, which is a site-specific library document.

# Phase I: list all the things that can be imported

import glob, os, sys, string
modules={}

for modname in sys.builtin_module_names:
    modules[modname]=modname
    
for dir in sys.path:
    # Look for *.py files
    filelist=glob.glob(os.path.join(dir, '*.py'))
    for file in filelist: 
        path, file = os.path.split(file)
        base, ext=os.path.splitext(file)
        modules[string.lower(base)]=base

    # Look for shared library files
    filelist=(glob.glob(os.path.join(dir, '*.so')) + 
              glob.glob(os.path.join(dir, '*.sl')) +
              glob.glob(os.path.join(dir, '*.o')) )
    for file in filelist: 
        path, file = os.path.split(file)
        base, ext=os.path.splitext(file)
        if base[-6:]=='module': base=base[:-6]
        modules[string.lower(base)]=base

# Minor oddity: the types module is documented in libtypes2.tex
if modules.has_key('types'):
    del modules['types'] ; modules['types2']=None

# Phase II: find all documentation files (lib*.tex)
#           and eliminate modules that don't have one.

docs={}
filelist=glob.glob('lib*.tex')
for file in filelist:
    modname=file[3:-4]
    docs[modname]=modname

mlist=modules.keys()
mlist=filter(lambda x, docs=docs: docs.has_key(x), mlist)
mlist.sort()
mlist=map(lambda x, docs=docs: docs[x], mlist)

modules=mlist

# Phase III: write custlib.tex

# Write the boilerplate
# XXX should be fancied up.  
print """\documentstyle[twoside,11pt,myformat]{report}
\\title{Python Library Reference}
\\input{boilerplate}
\\makeindex                     % tell \\index to actually write the .idx file
\\begin{document}
\\pagenumbering{roman}
\\maketitle
\\input{copyright}
\\begin{abstract}
\\noindent This is a customized version of the Python Library Reference.
\\end{abstract}
\\pagebreak
{\\parskip = 0mm \\tableofcontents}
\\pagebreak\\pagenumbering{arabic}"""
    
for modname in mlist: 
    print "\\input{lib%s}" % (modname,)
    
# Write the end
print """\\input{custlib.ind}                   % Index
\\end{document}"""
