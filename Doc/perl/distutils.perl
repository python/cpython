# LaTeX2HTML support for distutils.sty.

package main;

sub do_cmd_command {
    return use_wrappers(@_[0], '<code>', '</code>');
}

sub do_cmd_option {
    return use_wrappers(@_[0], '<font face="sans-serif">', '</font>');
}

sub do_cmd_filevar {
    return use_wrappers(@_[0], '<font face="sans-serif"></i>', '</i></font>');
}

sub do_cmd_XXX {
    return use_wrappers(@_[0], '<b>** ', ' **</b>');
}

1;
