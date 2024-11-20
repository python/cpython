
class GrammarSnippetDirective(SphinxDirective):
    """Transform a grammar-snippet directive to a Sphinx productionlist

    That is, turn something like:

        .. grammar-snippet:: file
           :group: python-grammar
           :generated-by: Tools/peg_generator/docs_generator.py

           file: (NEWLINE | statement)*

    into something like:

        .. productionlist:: python-grammar
           file: (NEWLINE | statement)*

    The custom directive is needed because Sphinx's `productionlist` does
    not support options.
    """
    has_content = True
    option_spec = {
        'group': directives.unchanged,
        'generated-by': directives.unchanged,
        'diagrams': directives.unchanged,
    }

    # Arguments are used by the tool that generates grammar-snippet,
    # this Directive ignores them.
    required_arguments = 1
    optional_arguments = 0
    final_argument_whitespace = True

    def run(self):
        group_name = self.options['group']

        rawsource = '''
        # Docutils elements have a `rawsource` attribute that is supposed to be
        # set to the original ReST source.
        # Sphinx does the following with it:
        # - if it's empty, set it to `self.astext()`
        # - if it matches `self.astext()` when generating the output,
        #   apply syntax highlighting (which is based on the plain-text content
        #   and thus discards internal formatting, like references).
        # To get around this, we set it to this fake (and very non-empty)
        # string!
        '''

        literal = nodes.literal_block(
            rawsource,
            '',
            # TODO: Use a dedicated CSS class here and for strings,
            # and add it to the theme too
            classes=['highlight'],
        )

        grammar_re = re.compile(
            """
                (?P<rule_name>^[a-zA-Z0-9_]+)     # identifier at start of line
                (?=:)                             # ... followed by a colon
            |
                [`](?P<rule_ref>[a-zA-Z0-9_]+)[`] # identifier in backquotes
            |
                (?P<single_quoted>'[^']*')        # string in 'quotes'
            |
                (?P<double_quoted>"[^"]*")        # string in "quotes"
            """,
            re.VERBOSE,
        )

        for line in self.content:
            last_pos = 0
            for match in grammar_re.finditer(line):
                # Handle text between matches
                if match.start() > last_pos:
                    literal += nodes.Text(line[last_pos:match.start()])
                last_pos = match.end()

                # Handle matches
                groupdict = {
                    name: content
                    for name, content in match.groupdict().items()
                    if content is not None
                }
                match groupdict:
                    case {'rule_name': name}:
                        name_node = addnodes.literal_strong()

                        # Cargo-culted magic to make `name_node` a link target
                        # similar to Sphinx `production`:
                        domain = self.env.domains['std']
                        obj_name = f"{group_name}:{name}"
                        prefix = f'grammar-token-{group_name}'
                        node_id = make_id(self.env, self.state.document, prefix, name)
                        name_node['ids'].append(node_id)
                        self.state.document.note_implicit_target(name_node, name_node)
                        domain.note_object('token', obj_name, node_id, location=name_node)

                        text_node = nodes.Text(name)
                        name_node += text_node
                        literal += name_node
                    case {'rule_ref': name}:
                        ref_node = addnodes.pending_xref(
                            name,
                            reftype="token",
                            refdomain="std",
                            reftarget=f"{group_name}:{name}",
                        )
                        ref_node += nodes.Text(name)
                        literal += ref_node
                    case {'single_quoted': name} | {'double_quoted': name}:
                        string_node = nodes.inline(classes=['nb'])
                        string_node += nodes.Text(name)
                        literal += string_node
                    case _:
                        raise ValueError('unhandled match')
            literal += nodes.Text(line[last_pos:] + '\n')


        node = nodes.paragraph(
            '', '',
            literal,
        )

        content = StringList()
        for rule_name in self.options['diagrams'].split():
            content.append('', source=__file__)
            content.append(f'``{rule_name}``:', source=__file__)
            content.append('', source=__file__)
            content.append(f'.. image:: diagrams/{rule_name}.svg', source=__file__)

        self.state.nested_parse(content, 0, node)

        return [node]
