# Lame substitute for a fine script to generate the table from ast.txt

from compiler import astgen

AST_DEF = '../compiler/ast.txt'

def sort(l):
    l = l[:]
    l.sort(lambda a, b: cmp(a.name, b.name))
    return l

def main():
    nodes = astgen.parse_spec(AST_DEF)
    print "\\begin{longtableiii}{lll}{class}{Node type}{Attribute}{Value}"
    print
    for node in sort(nodes):
        if node.argnames:
            print "\\lineiii{%s}{%s}{}" % (node.name, node.argnames[0])
        else:
            print "\\lineiii{%s}{}{}" % node.name
            
        for arg in node.argnames[1:]:
            print "\\lineiii{}{\\member{%s}}{}" % arg
        print "\\hline", "\n"
    print "\\end{longtableiii}"


if __name__ == "__main__":
    main()
    
