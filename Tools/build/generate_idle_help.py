"""Copy the text part of idle.html to idlelib/help.html while stripping trailing whitespace.

Files with trailing whitespace cannot be pushed to the git cpython
repository.  help.html is regenerated, after
editing idle.rst on the master branch, with, in the Doc directory
  make idlehelp
Check build/html/library/idle.html, the help.html diff, and the text
displayed by Help => IDLE Help.  Add a blurb and create a PR.

It can be worthwhile to occasionally generate help.html without
touching idle.rst.  Changes to the master version and to the doc
build system may result in changes that should not change
the displayed text, but might break HelpParser.

As long as master and maintenance versions of idle.rst remain the
same, help.html can be backported.  The internal Python version
number is not displayed.  If maintenance idle.rst diverges from
the master version, then instead of backporting help.html from
master, repeat the procedure above to generate a maintenance
version.
"""

def copy_strip():
    src = 'build/html/library/idle.html'
    dst = '../Lib/idlelib/help.html'

    with open(src, encoding="utf-8") as inn, open(dst, 'w', encoding="utf-8") as out:
        copy = False
        for line in inn:
            if '<div class="clearer">' in line:
                break
            if '<section id="idle">' in line:
                copy = True
            if copy:
                out.write(line.strip() + '\n')

    print(f'{src} copied to {dst}')


if __name__ == '__main__':
    copy_strip()
