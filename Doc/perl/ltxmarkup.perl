# LaTeX2HTML support for the ltxmarkup package.  Doesn't do indexing.

package main;


sub ltx_next_argument{
    my $param;
    $param = missing_braces()
      unless ((s/$next_pair_pr_rx/$param=$2;''/eo)
	      ||(s/$next_pair_rx/$param=$2;''/eo));
    return $param;
}


sub do_cmd_macro{
    local($_) = @_;
    my $macro = ltx_next_argument();
    return "<tt class='macro'>&#92;$macro</tt>" . $_;
}

sub do_cmd_env{
    local($_) = @_;
    my $env = ltx_next_argument();
    return "<tt class='environment'>&#92;$env</tt>" . $_;
}

sub do_env_macrodesc{
    local($_) = @_;
    my $macro = ltx_next_argument();
    my $params = ltx_next_argument();
    return "\n<dl class='macrodesc'>"
         . "\n<dt><b><tt class='macro'>&#92;$macro</tt></b>"
         . "\n                         $params"
	 . "\n<dd>"
	 . $_
	 . "</dl>";
}

sub do_env_envdesc{
    local($_) = @_;
    my $env = ltx_next_argument();
    my $params = ltx_next_argument();
    return "\n<dl class='envdesc'>"
         . "\n<dt><b><tt class='environment'>&#92;$env</tt></b>"
         . "\n                               $params"
	 . "\n<dd>"
	 . $_
	 . "</dl>";
}

1;				# Must end with this, because Perl is bogus.
