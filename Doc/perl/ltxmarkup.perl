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

sub ltx_process_params{
    # Handle processing of \p and \op for parameter specifications for
    # envdesc and macrodesc.  It's done this way to avoid defining do_cmd_p()
    # and do_cmd_op() functions, which would be interpreted outside the context
    # in which these commands are legal, and cause LaTeX2HTML to think they're
    # defined.  This way, other uses of \p and \op are properly flagged as
    # unknown macros.
    my $s = @_[0];
    $s =~ s%\\op<<(\d+)>>(.+)<<\1>>%<tt>[</tt><var>$2</var><tt>]</tt>%;
    while ($s =~ /\\p<<(\d+)>>(.+)<<\1>>/) {
	$s =~ s%\\p<<(\d+)>>(.+)<<\1>>%<tt>{</tt><var>$2</var><tt>}</tt>%;
    }
    return $s;
}

sub do_env_macrodesc{
    local($_) = @_;
    my $macro = ltx_next_argument();
    my $params = ltx_process_params(ltx_next_argument());
    return "\n<dl class='macrodesc'>"
         . "\n<dt><b><tt class='macro'>&#92;$macro</tt></b>"
         . "\n    $params</dt>"
	 . "\n<dd>"
	 . $_
	 . '</dd></dl>';
}

sub do_env_envdesc{
    local($_) = @_;
    my $env = ltx_next_argument();
    my $params = ltx_process_params(ltx_next_argument());
    return "\n<dl class='envdesc'>"
         . "\n<dt><tt>&#92;begin{<b class='environment'>$env</b>}</tt>"
         . "\n    $params"
         . "\n<br /><tt>&#92;end{<b class='environment'>$env</b>}</tt></dt>"
	 . "\n<dd>"
	 . $_
	 . '</dd></dl>';
}

1;				# Must end with this, because Perl is bogus.
