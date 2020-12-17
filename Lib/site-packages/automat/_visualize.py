from __future__ import print_function
import argparse
import sys

import graphviz

from ._discover import findMachines


def _gvquote(s):
    return '"{}"'.format(s.replace('"', r'\"'))


def _gvhtml(s):
    return '<{}>'.format(s)


def elementMaker(name, *children, **attrs):
    """
    Construct a string from the HTML element description.
    """
    formattedAttrs = ' '.join('{}={}'.format(key, _gvquote(str(value)))
                              for key, value in sorted(attrs.items()))
    formattedChildren = ''.join(children)
    return u'<{name} {attrs}>{children}</{name}>'.format(
        name=name,
        attrs=formattedAttrs,
        children=formattedChildren)


def tableMaker(inputLabel, outputLabels, port, _E=elementMaker):
    """
    Construct an HTML table to label a state transition.
    """
    colspan = {}
    if outputLabels:
        colspan['colspan'] = str(len(outputLabels))

    inputLabelCell = _E("td",
                        _E("font",
                           inputLabel,
                           face="menlo-italic"),
                        color="purple",
                        port=port,
                        **colspan)

    pointSize = {"point-size": "9"}
    outputLabelCells = [_E("td",
                           _E("font",
                              outputLabel,
                              **pointSize),
                           color="pink")
                        for outputLabel in outputLabels]

    rows = [_E("tr", inputLabelCell)]

    if outputLabels:
        rows.append(_E("tr", *outputLabelCells))

    return _E("table", *rows)


def makeDigraph(automaton, inputAsString=repr,
                outputAsString=repr,
                stateAsString=repr):
    """
    Produce a L{graphviz.Digraph} object from an automaton.
    """
    digraph = graphviz.Digraph(graph_attr={'pack': 'true',
                                           'dpi': '100'},
                               node_attr={'fontname': 'Menlo'},
                               edge_attr={'fontname': 'Menlo'})

    for state in automaton.states():
        if state is automaton.initialState:
            stateShape = "bold"
            fontName = "Menlo-Bold"
        else:
            stateShape = ""
            fontName = "Menlo"
        digraph.node(stateAsString(state),
                     fontame=fontName,
                     shape="ellipse",
                     style=stateShape,
                     color="blue")
    for n, eachTransition in enumerate(automaton.allTransitions()):
        inState, inputSymbol, outState, outputSymbols = eachTransition
        thisTransition = "t{}".format(n)
        inputLabel = inputAsString(inputSymbol)

        port = "tableport"
        table = tableMaker(inputLabel, [outputAsString(outputSymbol)
                                        for outputSymbol in outputSymbols],
                           port=port)

        digraph.node(thisTransition,
                     label=_gvhtml(table), margin="0.2", shape="none")

        digraph.edge(stateAsString(inState),
                     '{}:{}:w'.format(thisTransition, port),
                     arrowhead="none")
        digraph.edge('{}:{}:e'.format(thisTransition, port),
                     stateAsString(outState))

    return digraph


def tool(_progname=sys.argv[0],
         _argv=sys.argv[1:],
         _syspath=sys.path,
         _findMachines=findMachines,
         _print=print):
    """
    Entry point for command line utility.
    """

    DESCRIPTION = """
    Visualize automat.MethodicalMachines as graphviz graphs.
    """
    EPILOG = """
    You must have the graphviz tool suite installed.  Please visit
    http://www.graphviz.org for more information.
    """
    if _syspath[0]:
        _syspath.insert(0, '')
    argumentParser = argparse.ArgumentParser(
        prog=_progname,
        description=DESCRIPTION,
        epilog=EPILOG)
    argumentParser.add_argument('fqpn',
                                help="A Fully Qualified Path name"
                                " representing where to find machines.")
    argumentParser.add_argument('--quiet', '-q',
                                help="suppress output",
                                default=False,
                                action="store_true")
    argumentParser.add_argument('--dot-directory', '-d',
                                help="Where to write out .dot files.",
                                default=".automat_visualize")
    argumentParser.add_argument('--image-directory', '-i',
                                help="Where to write out image files.",
                                default=".automat_visualize")
    argumentParser.add_argument('--image-type', '-t',
                                help="The image format.",
                                choices=graphviz.FORMATS,
                                default='png')
    argumentParser.add_argument('--view', '-v',
                                help="View rendered graphs with"
                                " default image viewer",
                                default=False,
                                action="store_true")
    args = argumentParser.parse_args(_argv)

    explicitlySaveDot = (args.dot_directory
                         and (not args.image_directory
                              or args.image_directory != args.dot_directory))
    if args.quiet:
        def _print(*args):
            pass

    for fqpn, machine in _findMachines(args.fqpn):
        _print(fqpn, '...discovered')

        digraph = machine.asDigraph()

        if explicitlySaveDot:
            digraph.save(filename="{}.dot".format(fqpn),
                         directory=args.dot_directory)
            _print(fqpn, "...wrote dot into", args.dot_directory)

        if args.image_directory:
            deleteDot = not args.dot_directory or explicitlySaveDot
            digraph.format = args.image_type
            digraph.render(filename="{}.dot".format(fqpn),
                           directory=args.image_directory,
                           view=args.view,
                           cleanup=deleteDot)
            if deleteDot:
                msg = "...wrote image into"
            else:
                msg = "...wrote image and dot into"
            _print(fqpn, msg, args.image_directory)
