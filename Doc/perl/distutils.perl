# LaTeX2HTML support for distutils.sty.

package main;

sub do_cmd_command {
    return use_wrappers(@_[0], '<code class="du-command">', '</code>');
}

sub do_cmd_option {
    return use_wrappers(@_[0], '<span class="du-option">', '</span>');
}

sub do_cmd_filevar {
    return use_wrappers(@_[0], '<span class="du-filevar">', '</span>');
}

sub do_cmd_XXX {
    return use_wrappers(@_[0], '<b class="du-xxx">', '</b>');
}

1;  # Bad Perl.
