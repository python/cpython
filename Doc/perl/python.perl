# python.perl by Fred L. Drake, Jr. <fdrake@acm.org>		-*- perl -*-
#
# Heavily based on Guido van Rossum's myformat.perl (now obsolete).
#
# Extension to LaTeX2HTML for documents using myformat.sty.
# Subroutines of the form do_cmd_<name> here define translations
# for LaTeX commands \<name> defined in the corresponding .sty file.

package main;


sub next_argument_id{
    my ($param,$br_id);
    $param = missing_braces()
        unless ((s/$next_pair_pr_rx/$br_id=$1;$param=$2;''/eo)
		||(s/$next_pair_rx/$br_id=$1;$param=$2;''/eo));
    ($param, $br_id);
}

sub next_argument{
    my ($param,$br_id) = next_argument_id();
    $param;
}

sub next_optional_argument{
    my($param,$rx) = ('', "^\\s*(\\[([^]]*)\\])?");
    s/$rx/$param=$2;''/eo;
    $param;
}

sub swallow_newline{
    s/[\n]?//o;
}

sub ArabictoRoman {
    # Written by Robert Donohue <donahue@cfassp48.harvard.edu>,
    # fixed by Fred Drake <fdrake@acm.org>,
    # kept around in case it becomes useful.
    my $a = @_;
    my($b,@c,$i);
    my(@D0,@D1,@D2);

    @D0 = ("", "I", "II", "III", "IV", "V", "VI", "VII", "VIII", "IX");
    @D1 = ("", "X", "XX", "XXX", "XL", "L", "LX", "LXX", "LXXX", "XC");
    @D2 = ("", "C", "CC", "CCC", "CD", "D", "DC", "DCC", "DCCC", "CM");

    $b = $a;
    $i = 0;
    while($b > 0) {
	$c[$i] = ($b % 10);
	$b -= ($b % 10);
	$b /= 10;
	$i++;
    }
    $D2[$c[2]] . $D1[$c[1]] . $D0[$c[0]];
}


# This is a fairly simple hack; it supports \let when it is used to create
# (or redefine) a macro to exactly be some other macro: \let\newname=\oldname.
# Many possible uses of \let aren't supported or aren't supported correctly.
#
sub do_cmd_let{
    local($_) = @_;
    my $matched = 0;
    s/[\\]([a-zA-Z]+)\s*(=\s*)?[\\]([a-zA-Z]*)/$matched=1; ''/e;
    if ($matched) {
	my($new, $old) = ($1, $3);
	eval "sub do_cmd_$new { do_cmd_$old" . '(@_); }';
	print "\ndefining handler for \\$new using \\$old\n";
    }
    else {
	s/[\\]([a-zA-Z]+)\s*(=\s*)?([^\\])/$matched=1; ''/es;
	if ($matched) {
	    my($new, $char) = ($1, $3);
	    eval "sub do_cmd_$new { \"\\$char\" . \@_[0]; }";
	    print "\ndefining handler for \\$new to insert '$char'\n";
	}
	else {
	    write_warnings("Could not interpret \\let construct...");
	}
    }
    $_;
}


# words typeset in a special way (not in HTML though)

sub do_cmd_ABC{ 'ABC' . @_[0]; }
sub do_cmd_UNIX{ 'Unix'. @_[0]; }
sub do_cmd_ASCII{ 'ASCII' . @_[0]; }
sub do_cmd_POSIX{ 'POSIX' . @_[0]; }
sub do_cmd_C{ 'C' . @_[0]; }
sub do_cmd_Cpp{ 'C++' . @_[0]; }
sub do_cmd_EOF{ 'EOF' . @_[0]; }
sub do_cmd_NULL{ '<tt>NULL</tt>' . @_[0]; }

sub do_cmd_e{ '&#92;' . @_[0]; }

$DEVELOPER_ADDRESS = '';
$PYTHON_VERSION = '';

sub do_cmd_version{ $PYTHON_VERSION . @_[0]; }
sub do_cmd_release{
    local($_) = @_;
    $PYTHON_VERSION = next_argument();
    $_;
}

sub do_cmd_authoraddress{
    local($_) = @_;
    $DEVELOPER_ADDRESS = next_argument();
    $_;
}

#sub do_cmd_developer{ do_cmd_author(@_[0]); }
#sub do_cmd_developers{ do_cmd_author(@_[0]); }
#sub do_cmd_developersaddress{ do_cmd_authoraddress(@_[0]); }

sub do_cmd_hackscore{
    local($_) = @_;
    next_argument();
    '_' . $_;
}

sub use_wrappers{
    local($_,$before,$after) = @_;
    my $stuff = next_argument();
    $before . $stuff . $after . $_;
}

sub use_current{ use_wrappers(@_[0], '', ''); }
sub use_sans_serif{ use_wrappers(@_[0], '<font face=sans-serif>', '</font>'); }
sub use_italics{ use_wrappers(@_[0], '<i>', '</i>'); }

sub do_cmd_optional{
    use_wrappers(@_[0], "</var><big>\[</big><var>",
		 "</var><big>\]</big><var>");
}

# Logical formatting (some based on texinfo), needs to be converted to
# minimalist HTML.  The "minimalist" is primarily to reduce the size of
# output files for users that read them over the network rather than
# from local repositories.

sub do_cmd_pytype{ use_current(@_); }
sub do_cmd_makevar{ use_current(@_); }
sub do_cmd_code{ use_wrappers(@_[0], '<tt>', '</tt>'); }
sub do_cmd_module{ do_cmd_code(@_); }
sub do_cmd_keyword{ do_cmd_code(@_); }
sub do_cmd_exception{ do_cmd_code(@_); }
sub do_cmd_class{ do_cmd_code(@_); }
sub do_cmd_function{ do_cmd_code(@_); }
sub do_cmd_constant{ do_cmd_code(@_); }
sub do_cmd_member{ do_cmd_code(@_); }
sub do_cmd_method{ do_cmd_code(@_); }
sub do_cmd_cfunction{ do_cmd_code(@_); }
sub do_cmd_cdata{ do_cmd_code(@_); }
sub do_cmd_ctype{ do_cmd_code(@_); }
sub do_cmd_regexp{ do_cmd_code(@_); }
sub do_cmd_character{ do_cmd_samp(@_); }
sub do_cmd_program{ do_cmd_strong(@_); }
sub do_cmd_email{ use_sans_serif(@_); }
sub do_cmd_mimetype{ use_sans_serif(@_); }
sub do_cmd_var{ use_italics(@_); }
sub do_cmd_dfn{ use_italics(@_); }	# make an index entry?
sub do_cmd_emph{ use_italics(@_); }
sub do_cmd_file{ use_wrappers(@_[0], '"<tt>', '</tt>"'); }
sub do_cmd_samp{ use_wrappers(@_[0], '"<tt>', '</tt>"'); }
sub do_cmd_kbd{ use_wrappers(@_[0], '<kbd>', '</kbd>'); }
sub do_cmd_strong{ use_wrappers(@_[0], '<b>', '</b>'); }

sub do_cmd_newsgroup{
    local($_) = @_;
    my $newsgroup = next_argument();
    my $stuff = "<a href=\"news:$newsgroup\"><font face=sans-serif>"
      . "$newsgroup</font></a>";
    $stuff . $_;
}

sub do_cmd_envvar{
    local($_) = @_;
    my($envvar,$br_id) = next_argument_id();
    my($name,$aname,$ahref) = link_info($br_id);
    # The <tt> here is really to keep buildindex.py from making
    # the variable name case-insensitive.
    add_index_entry("environment variables!$envvar@<tt>\$$envvar</tt>",
		    $ahref);
    add_index_entry("$envvar@\$$envvar", $ahref);
    "$aname\$$envvar</a>" . $_;
}


sub do_cmd_url{
    # use the URL as both text and hyperlink
    local($_) = @_;
    my $url = next_argument();
    $url =~ s/~/&#126;/g;
    "<a href=\"$url\"><font face=sans-serif>$url</font></a>" . $_;
}

sub do_cmd_manpage{
    # two parameters:  \manpage{name}{section}
    local($_) = @_;
    my $page = next_argument();
    my $section = next_argument();
    "<i>$page</i>($section)" . $_;
}

sub do_cmd_rfc{
    local($_) = @_;
    my($rfcnumber,$br_id) = next_argument();

    # Save the reference
    my $nstr = gen_index_id("RFC!RFC $rfcnumber", '');
    $index{$nstr} .= make_half_href("$CURRENT_FILE#$br_id");
    "<a name=$br_id>RFC $rfcnumber</a>" .$_;
}

sub do_cmd_deprecated{
    # two parameters:  \deprecated{version}{whattodo}
    local($_) = @_;
    my $release = next_argument();
    my $reason = next_argument();
    "<b>Deprecated since release $release.</b>\n$reason<p>" . $_;
}

sub do_cmd_versionadded{
    # one parameter:  \versionadded{version}
    local($_) = @_;
    my $release = next_argument();
    "<b>New in version $release.</b><p>" . $_;
}

sub do_cmd_versionchanged{
    # one parameter:  \versionchanged{version}
    local($_) = @_;
    my $release = next_argument();
    "<b>Changed in version $release.</b><p>" . $_;
}

# file and samp are at the end of this file since they screw up fontlock.

# index commands

$INDEX_SUBITEM = "";

sub get_indexsubitem{
    #$INDEX_SUBITEM ? " $INDEX_SUBITEM" : '';
    '';
}

sub do_cmd_setindexsubitem{
    local($_) = @_;
    $INDEX_SUBITEM = next_argument();
    $_;
}

sub do_cmd_withsubitem{
    # We can't really do the right right thing, because LaTeX2HTML doesn't
    # do things in the right order, but we need to at least strip this stuff
    # out, and leave anything that the second argument expanded out to.
    #
    local($_) = @_;
    next_argument();
    my $stuff = next_argument();
    $stuff . $_;
}

# This is the prologue macro which is required to start writing the
# mod\jobname.idx file; we can just ignore it.
#
sub do_cmd_makemodindex{ @_[0]; }

# We're in the document subdirectory when this happens!
#
open(IDXFILE, '>index.dat') || die "\n$!\n";
open(INTLABELS, '>intlabels.pl') || die "\n$!\n";
print INTLABELS "%internal_labels = ();\n";
print INTLABELS "1;		# hack in case there are no entries\n\n";

# Using \0 for this is bad because we can't use common tools to work with the
# resulting files.  Things like grep can be useful with this stuff!
#
$IDXFILE_FIELD_SEP = "\1";


sub gen_target_name{
    "l2h-" . @_[0];
}

sub gen_target{
    '<a name="' . @_[0] . '">';
}

sub gen_link{
    my($node,$target) = @_;
    print INTLABELS "\$internal_labels{\"$target\"} = \"$URL/$node\";\n";
    "<a href=\"$node#$target\">";
}

sub make_index_entry{
    my($br_id,$str) = @_;
    my($name,$aname,$ahref) = link_info($br_id);
    add_index_entry($str, $ahref);
    "$aname$anchor_invisible_mark</a>";
}

sub add_index_entry{
    # add an entry to the index structures; ignore the return value
    my($str,$ahref) = @_;
    $str = gen_index_id($str, '');
    $index{$str} .= $ahref;
    print IDXFILE $ahref, $IDXFILE_FIELD_SEP, $str, "\n";
}

sub link_info{
    my $name = gen_target_name(@_[0]);
    my $aname = gen_target($name);
    my $ahref = gen_link($CURRENT_FILE, $name);
    return ($name, $aname, $ahref);
}

sub do_cmd_index{
    local($_) = @_;
    my($str,$br_id) = next_argument_id();
    swallow_newline();
    #
    my($name,$aname,$ahref) = link_info($br_id);
    add_index_entry("$str", $ahref);
    "$aname$anchor_invisible_mark</a>" . $_;
}

sub do_cmd_kwindex{
    local($_) = @_;
    my($str,$br_id) = next_argument_id();
    #
    my($name,$aname,$ahref) = link_info($br_id);
    add_index_entry("<tt>$str</tt>!keyword", $ahref);
    add_index_entry("keyword!<tt>$str</tt>", $ahref);
    "$aname$anchor_invisible_mark</a>" . $_;
}

sub do_cmd_indexii{
    local($_) = @_;
    my($str1,$br_id) = next_argument_id();
    my $str2 = next_argument();
    #
    my($name,$aname,$ahref) = link_info($br_id);
    add_index_entry("$str1!$str2", $ahref);
    add_index_entry("$str2!$str1", $ahref);
    "$aname$anchor_invisible_mark</a>" . $_;
}

sub do_cmd_indexiii{
    local($_) = @_;
    my($str1,$br_id) = next_argument();
    my $str2 = next_argument();
    my $str3 = next_argument();
    #
    my($name,$aname,$ahref) = link_info($br_id);
    add_index_entry("$str1!$str2 $str3", $ahref);
    add_index_entry("$str2!$str3, $str1", $ahref);
    add_index_entry("$str3!$str1 $str2", $ahref);
    "$aname$anchor_invisible_mark</a>" . $_;
}

sub do_cmd_indexiv{
    local($_) = @_;
    my($str1,$br_id) = next_argument();
    my $str2 = next_argument();
    my $str3 = next_argument();
    my $str4 = next_argument();
    #
    my($name,$aname,$ahref) = link_info($br_id);
    add_index_entry("$str1!$str2 $str3 $str4", $ahref);
    add_index_entry("$str2!$str3 $str4, $str1", $ahref);
    add_index_entry("$str3!$str4, $str1 $str2", $ahref);
    add_index_entry("$str4!$$str1 $str2 $str3", $ahref);
    "$aname$anchor_invisible_mark</a>" . $_;
}

sub do_cmd_ttindex{
    local($_) = @_;
    my($str,$br_id) = next_argument_id();
    swallow_newline();
    make_index_entry($br_id, $str . get_indexsubitem()) . $_;
}

sub my_typed_index_helper{
    local($word,$_) = @_;
    my($str,$br_id) = next_argument();
    swallow_newline();
    #
    my($name,$aname,$ahref) = link_info($br_id1);
    add_index_entry("$str $word", $ahref);
    add_index_entry("$word!$str", $ahref);
    "$aname$anchor_invisible_mark</a>" . $_;
}

sub do_cmd_stindex{ my_typed_index_helper('statement', @_); }
sub do_cmd_opindex{ my_typed_index_helper('operator', @_); }
sub do_cmd_exindex{ my_typed_index_helper('exception', @_); }
sub do_cmd_obindex{ my_typed_index_helper('object', @_); }

sub my_parword_index_helper{
    local($word,$_) = @_;
    my($str,$br_id) = next_argument_id();
    swallow_newline();
    make_index_entry($br_id, "$str ($word)") . $_;
}


sub make_mod_index_entry{
    my($br_id,$str,$define) = @_;
    my($name,$aname,$ahref) = link_info($br_id);
    # equivalent of add_index_entry() using $define instead of ''
    $str = gen_index_id($str, $define);
    $index{$str} .= $ahref;
    print IDXFILE $ahref, $IDXFILE_FIELD_SEP, $str, "\n";

    if ($define eq 'DEF') {
	# add to the module index
	my($nstr,$garbage) = split / /, $str, 2;
	$Modules{$nstr} .= $ahref;
    }
    "$aname$anchor_invisible_mark</a>";
}

$THIS_MODULE = '';
$THIS_CLASS = '';

sub define_module{
    my($word,$name) = @_;
    my $section_tag = join('', @curr_sec_id);
    $word = "$word " if $word;
    $THIS_MODULE = "$name";
    make_mod_index_entry("SECTION$section_tag",
			 "<tt>$name</tt> (${word}module)", 'DEF');
}

sub my_module_index_helper{
    local($word, $_) = @_;
    my $name = next_argument();
    swallow_newline();
    define_module($word, $name) . $_;
}

sub ref_module_index_helper{
    local($word, $_) = @_;
    my($str,$br_id) = next_argument();
    swallow_newline();
    $word = "$word " if $word;
    make_mod_index_entry($br_id, "<tt>$str</tt> (${word}module)", 'REF') . $_;
}

sub do_cmd_bifuncindex{
    local($_) = @_;
    my($str,$br_id) = next_argument();
    swallow_newline();
    my $fname = "<tt>$str()</tt>";
    make_index_entry($br_id, "$fname (built-in function)") . $_;
}

sub do_cmd_modindex{ my_module_index_helper('', @_); }
sub do_cmd_bimodindex{ my_module_index_helper('built-in', @_); }
sub do_cmd_exmodindex{ my_module_index_helper('extension', @_); }
sub do_cmd_stmodindex{ my_module_index_helper('standard', @_); }

# these should be adjusted a bit....
sub do_cmd_refmodindex{ ref_module_index_helper('', @_); }
sub do_cmd_refbimodindex{ ref_module_index_helper('built-in', @_); }
sub do_cmd_refexmodindex{ ref_module_index_helper('extension', @_); }
sub do_cmd_refstmodindex{ ref_module_index_helper('standard', @_); }

sub do_cmd_nodename{ do_cmd_label(@_); }

sub init_myformat{
#    $anchor_invisible_mark = '';
    $anchor_mark = '';
    $icons{'anchor_mark'} = '';
}
init_myformat();

# similar to make_index_entry(), but includes the string in the result
# instead of the dummy filler.
#
sub make_str_index_entry{
    my($br_id,$str) = @_;
    my($name,$aname,$ahref) = link_info($br_id);
    add_index_entry($str, $ahref);
    "$aname$str</a>";
}

sub do_env_cfuncdesc{
    local($_) = @_;
    my $return_type = next_argument();
    my($function_name,$br_id) = next_argument_id();
    my $arg_list = next_argument();
    my $idx = make_str_index_entry($br_id,
			"<tt>$function_name()</tt>" . get_indexsubitem());
    $idx =~ s/ \(.*\)//;
    $idx =~ s/\(\)//;		# ????
    "<dl><dt>$return_type <b>$idx</b>"
      . "(<var>$arg_list</var>)\n<dd>"
      . $_
      . '</dl>';
}

sub do_env_ctypedesc{
    local($_) = @_;
    my($type_name,$br_id) = next_argument_id();
    my $idx = make_str_index_entry($br_id,
				   "<tt>$type_name</tt>" . get_indexsubitem());
    $idx =~ s/ \(.*\)//;
    "<dl><dt><b>$idx</b>\n<dd>"
      . $_
      . '</dl>'
}

sub do_env_cvardesc{
    local($_) = @_;
    my $var_type = next_argument();
    my($var_name,$br_id) = next_argument_id();
    my $idx = make_str_index_entry($br_id,
				   "<tt>$var_name</tt>" . get_indexsubitem());
    $idx =~ s/ \(.*\)//;
    "<dl><dt>$var_type <b>$idx</b>\n"
      . '<dd>'
      . $_
      . '</dl>';
}

sub do_env_funcdesc{
    local($_) = @_;
    my $function_name = next_argument();
    my($arg_list,$br_id) = next_argument_id();
    my $idx = make_str_index_entry($br_id, "<tt>$function_name()</tt>"
				   . get_indexsubitem());
    $idx =~ s/ \(.*\)//;
    $idx =~ s/\(\)//;
    "<dl><dt><b>$idx</b> (<var>$arg_list</var>)\n<dd>" . $_ . '</dl>';
}

sub do_env_funcdescni{
    local($_) = @_;
    my $function_name = next_argument();
    my $arg_list = next_argument();
    "<dl><dt><b><tt>$function_name</tt></b> (<var>$arg_list</var>)\n<dd>"
      . $_ . '</dl>';
}

sub do_cmd_funcline{
    local($_) = @_;
    my $function_name = next_argument();
    my($arg_list,$br_id) = next_argument_id();
    my $idx = make_str_index_entry($br_id, "<tt>$function_name()</tt>"
				   . get_indexsubitem());
    $idx =~ s/\(\)//;
    "<dt><b>$idx</b> (<var>$arg_list</var>)\n<dd>" . $_;
}

# Change this flag to index the opcode entries.  I don't think it's very
# useful to index them, since they're only presented to describe the dis
# module.
#
$INDEX_OPCODES = 0;

sub do_env_opcodedesc{
    local($_) = @_;
    my $opcode_name = next_argument();
    my($arg_list,$br_id) = next_argument_id();
    my $idx;
    if ($INDEX_OPCODES) {
	$idx = make_str_index_entry($br_id,
			"<tt>$opcode_name</tt> (byte code instruction)");
	$idx =~ s/ \(byte code instruction\)//;
    }
    else {
	$idx = "<tt>$opcode_name</tt>";
    }
    my $stuff = "<dl><dt><b>$idx</b>";
    if ($arg_list) {
	$stuff .= "&nbsp;&nbsp;&nbsp;&nbsp;<var>$arg_list</var>";
    }
    $stuff . "\n<dd>" . $_ . '</dl>';
}

sub do_env_datadesc{
    local($_) = @_;
    my($dataname,$br_id) = next_argument_id();
    my $idx = make_str_index_entry($br_id,
				   "<tt>$dataname</tt>" . get_indexsubitem());
    $idx =~ s/ \(.*\)//;
    "<dl><dt><b>$idx</b>\n<dd>"
      . $_
      . '</dl>';
}

sub do_env_datadescni{
    local($_) = @_;
    my $idx = next_argument();
    if (! $STRING_INDEX_TT) {
	$idx = "<tt>$idx</tt>";
    }
    "<dl><dt><b>$idx</b>\n<dd>" . $_ . '</dl>';
}

sub do_cmd_dataline{
    local($_) = @_;
    my($data_name,$br_id) = next_argument_id();
    my $idx = make_str_index_entry($br_id, "<tt>$data_name</tt>"
				   . get_indexsubitem());
    $idx =~ s/ \(.*\)//;
    "<dt><b>$idx</b><dd>" . $_;
}

sub do_env_excdesc{
    local($_) = @_;
    my($excname,$br_id) = next_argument_id();
    my $idx = make_str_index_entry($br_id, "<tt>$excname</tt>");
    "<dl><dt><b>$idx</b>\n<dd>" . $_ . '</dl>'
}

sub do_env_fulllineitems{ do_env_itemize(@_); }


sub do_env_classdesc{
    local($_) = @_;
    $THIS_CLASS = next_argument();
    my($arg_list,$br_id) = next_argument_id();
    $idx = make_str_index_entry($br_id,
			"<tt>$THIS_CLASS</tt> (class in $THIS_MODULE)" );
    $idx =~ s/ \(.*\)//;
    "<dl><dt><b>$idx</b> (<var>$arg_list</var>)\n<dd>" . $_ . '</dl>';
}


sub do_env_methoddesc{
    local($_) = @_;
    my $class_name = next_optional_argument();
    $class_name = $THIS_CLASS
        unless $class_name;
    my($method_name,$br_id) = next_argument_id();
    my $arg_list = next_argument();
    my $extra = '';
    if ($class_name) {
	$extra = " ($class_name method)";
    }
    my $idx = make_str_index_entry($br_id, "<tt>$method_name()</tt>$extra");
    $idx =~ s/ \(.*\)//;
    $idx =~ s/\(\)//;
    "<dl><dt><b>$idx</b> (<var>$arg_list</var>)\n<dd>" . $_ . '</dl>';
}


sub do_env_methoddescni{
    local($_) = @_;
    next_optional_argument();
    my $method = next_argument();
    my $arg_list = next_argument();
    "<dl><dt><b>$method</b> (<var>$arg_list</var>)\n<dd>" . $_ . '</dl>';
}


sub do_env_memberdesc{
    local($_) = @_;
    my $class = next_optional_argument();
    my($member,$br_id) = next_argument();
    $class = $THIS_CLASS
        unless $class;
    my $extra = '';
    $extra = " ($class_name attribute)"
        if (!($class eq ''));
    my $idx = make_str_index_entry($br_id, "<tt>$member</tt>$extra");
    $idx =~ s/ \(.*\)//;
    $idx =~ s/\(\)//;
    "<dl><dt><b>$idx</b>\n<dd>" . $_ . '</dl>';
}


sub do_cmd_memberline{
    local($_) = @_;
    my $class = next_optional_argument();
    my($member,$br_id) = next_argument_id();
    $class = $THIS_CLASS
        unless $class;
    my $extra = '';
    $extra = " ($class_name attribute)"
        if (!($class eq ''));
    my $idx = make_str_index_entry($br_id, "<tt>$member</tt>$extra");
    $idx =~ s/ \(.*\)//;
    $idx =~ s/\(\)//;
    "<dt><b>$idx</b><dd>" . $_;
}

sub do_env_memberdescni{
    local($_) = @_;
    next_optional_argument();
    my $member = next_argument();
    "<dl><dt><b>$member</b>\n<dd>" . $_ . '</dl>';
}


sub do_cmd_memberlineni{
    local($_) = @_;
    next_optional_argument();
    my $member = next_argument();
    "<dt><b>$member</b><dd>" . $_;
}

@col_aligns = ("<td>", "<td>", "<td>", "<td>");

sub setup_column_alignments{
    local($_) = @_;
    my($a1,$a2,$a3,$a4) = split(/[|]/,$_);
    my($th1,$th2,$th3,$th4) = ('<th>', '<th>', '<th>', '<th>');
    $col_aligns[0] = (($a1 eq 'c') ? '<td align=center>' : '<td>');
    $col_aligns[1] = (($a2 eq 'c') ? '<td align=center>' : '<td>');
    $col_aligns[2] = (($a3 eq 'c') ? '<td align=center>' : '<td>');
    $col_aligns[3] = (($a4 eq 'c') ? '<td align=center>' : '<td>');
    # return the aligned header start tags; only used for \begin{tableiii?}
    $th1 = (($a1 eq 'l') ? '<th align=left>'
	    : ($a1 eq 'r' ? '<th align=right>' : '<th>'));
    $th2 = (($a2 eq 'l') ? '<th align=left>'
	    : ($a2 eq 'r' ? '<th align=right>' : '<th>'));
    $th3 = (($a3 eq 'l') ? '<th align=left>'
	    : ($a3 eq 'r' ? '<th align=right>' : '<th>'));
    $th4 = (($a4 eq 'l') ? '<th align=left>'
	    : ($a4 eq 'r' ? '<th align=right>' : '<th>'));
    ($th1, $th2, $th3, $th4);
}

sub do_env_tableii{
    local($_) = @_;
    my($th1,$th2,$th3,$th4) = setup_column_alignments(next_argument());
    my $font = next_argument();
    my $h1 = next_argument();
    my $h2 = next_argument();
    $font = ''
        if ($font eq 'textrm');
    $globals{'lineifont'} = $font;
    '<table border align=center>'
      . "\n  <tr>$th1<b>$h1</b></th>"
      . "\n      $th2<b>$h2</b></th>"
      . $_
      . "\n</table>";
}

sub do_cmd_lineii{
    local($_) = @_;
    my $c1 = next_argument();
    my $c2 = next_argument();
    my($font,$sfont,$efont) = ($globals{'lineifont'}, '', '');
    if ($font) {
	$sfont = "<$font>";
	$efont = "</$font>";
    }
    my($c1align,$c2align) = @col_aligns[0,1];
    "<tr>$c1align$sfont$c1$efont</td>\n"
      . "      $c2align$c2</td>"
      . $_;
}

sub do_env_tableiii{
    local($_) = @_;
    my($th1,$th2,$th3,$th4) = setup_column_alignments(next_argument());
    my $font = next_argument();
    my $h1 = next_argument();
    my $h2 = next_argument();
    my $h3 = next_argument();
    $font = ''
        if ($font eq 'textrm');
    $globals{'lineifont'} = $font;
    '<table border align=center>'
      . "\n  <tr>$th1<b>$h1</b></th>"
      . "\n      $th2<b>$h2</b></th>"
      . "\n      $th3<b>$h3</b></th>"
      . $_
      . "\n</table>";
}

sub do_cmd_lineiii{
    local($_) = @_;
    my $c1 = next_argument();
    my $c2 = next_argument(); 
    my $c3 = next_argument();
    my($font,$sfont,$efont) = ($globals{'lineifont'}, '', '');
    if ($font) {
	$sfont = "<$font>";
	$efont = "</$font>";
    }
    my($c1align,$c2align,$c3align) = @col_aligns[0,1,2];
    "<tr>$c1align$sfont$c1$efont</td>\n"
      . "      $c2align$c2</td>\n"
      . "      $c3align$c3</td>"
      . $_;
}

sub do_env_tableiv{
    local($_) = @_;
    my($th1,$th2,$th3,$th4) = setup_column_alignments(next_argument());
    my $font = next_argument();
    my $h1 = next_argument();
    my $h2 = next_argument();
    my $h3 = next_argument();
    my $h4 = next_argument();
    $font = ''
        if ($font eq 'textrm');
    $globals{'lineifont'} = $font;
    '<table border align=center>'
      . "\n  <tr>$th1<b>$h1</b></th>"
      . "\n      $th2<b>$h2</b></th>"
      . "\n      $th3<b>$h3</b></th>"
      . "\n      $th4<b>$h4</b></th>"
      . $_
      . "\n</table>";
}

sub do_cmd_lineiv{
    local($_) = @_;
    my $c1 = next_argument();
    my $c2 = next_argument(); 
    my $c3 = next_argument();
    my $c4 = next_argument();
    my($font,$sfont,$efont) = ($globals{'lineifont'}, '', '');
    if ($font) {
	$sfont = "<$font>";
	$efont = "</$font>";
    }
    my($c1align,$c2align,$c3align,$c4align) = @col_aligns;
    "<tr>$c1align$sfont$c1$efont</td>\n"
      . "      $c2align$c2</td>\n"
      . "      $c3align$c3</td>\n"
      . "      $c4align$c4</td>"
      . $_;
}

sub do_cmd_maketitle {
    local($_) = @_;
    my $the_title = '';
    if ($t_title) {
	$the_title .= "<h1 align=center>$t_title</h1>";
    } else { write_warnings("\nThis document has no title."); }
    $the_title .= "\n<center>";
    if ($t_author) {
	if ($t_authorURL) {
	    my $href = translate_commands($t_authorURL);
	    $href = make_named_href('author', $href, "<strong>${t_author}</strong>");
	    $the_title .= "\n<p>$href</p>";
	} else {
	    $the_title .= "\n<p><strong>$t_author</strong></p>";
	}
    } else { write_warnings("\nThere is no author for this document."); }
    if ($t_institute) {
        $the_title .= "\n<p>$t_institute</p>";}
    if ($DEVELOPER_ADDRESS) {
        $the_title .= "\n<p>$DEVELOPER_ADDRESS</p>";}
    if ($t_affil) {
	$the_title .= "\n<p><i>$t_affil</i></p>";}
    if ($t_date) {
	$the_title .= "\n<p><strong>$t_date</strong>";
	if ($PYTHON_VERSION) {
	    $the_title .= "<br><strong>Release $PYTHON_VERSION</strong>";}
	$the_title .= "</p>"
    }
    $the_title .= "\n</center>";
    if ($t_address) {
	$the_title .= "\n<p>$t_address</p>";
    } else { $the_title .= "\n<p>"}
    if ($t_email) {
	$the_title .= "\n<p>$t_email</p>";
    }# else { $the_title .= "</p>" }
    $the_title . "<hr>\n" . $_ ;
}


#
#  Module synopsis support
#

require SynopsisTable;

sub get_chapter_id(){
    my $id = do_cmd_thechapter('');
    $id =~ s/<SPAN CLASS="arabic">(\d+)<\/SPAN>\./\1/;
    return $id;
}

%ModuleSynopses = ('chapter' => 'SynopsisTable instance');

sub get_synopsis_table($){
    my($chap) = @_;
    my $st = $ModuleSynopses{$chap};
    my $key;
    foreach $key (keys %ModuleSynopses) {
	if ($key eq $chap) {
	    return $ModuleSynopses{$chap};
	}
    }
    $st = SynopsisTable->new();
    $ModuleSynopses{$chap} = $st;
    return $st;
}

sub do_cmd_declaremodule{
    local($_) = @_;
    my $key = next_optional_argument();
    my $type = next_argument();
    my $name = next_argument();
    my $st = get_synopsis_table(get_chapter_id());
    #
    $key = $name unless $key;
    $type = 'built-in' if $type eq 'builtin';
    $st->declare($name, $key, $type);
    define_module($type, $name);
    anchor_label("module-$key",$CURRENT_FILE,$_)
}

sub do_cmd_modulesynopsis{
    local($_) = @_;
    my $st = get_synopsis_table(get_chapter_id());
    $st->set_synopsis($THIS_MODULE, next_argument());
    swallow_newline();
    $_;
}

sub do_cmd_localmoduletable{
    local($_) = @_;
    my $chap = get_chapter_id();
    "<tex2htmllocalmoduletable><$chap>\\tableofchildlinks[off]" . $_;
}

sub process_all_localmoduletables{
    while (/<tex2htmllocalmoduletable><(\d+)>/) {
	my $match = $&;
	my $chap = $1;
	my $st = get_synopsis_table($chap);
	my $data = $st->tohtml();
	s/$match/$data/;
    }
}


#
#  "See also:" -- references placed at the end of a \section
#

sub do_env_seealso{
    "<p><b>See Also:</b></p>\n" . @_[0];
}

sub do_cmd_seemodule{
    # Insert the right magic to jump to the module definition.  This should
    # work most of the time, at least for repeat builds....
    local($_) = @_;
    my $key = next_optional_argument();
    my $module = next_argument();
    my $text = next_argument();
    $key = $module
        unless $key;
    "<p>Module <tt><b><a href=\"module-$key.html\">$module</a></b></tt>"
      . "&nbsp;&nbsp;&nbsp;($text)</p>"
      . $_;
}

sub do_cmd_seetext{
    '<p>' . @_[0];
}


#
#  Definition list support.
#

sub do_env_definitions{
    local($_) = @_;
    swallow_newline();
    "<dl>$_</dl>\n";
}

sub do_cmd_term{
    local($_) = @_;
    my($term, $id) = next_argument_id();
    my($name,$aname,$ahref) = link_info($id);
    swallow_newline();
    # could easily add an index entry here...
    "<dt><b>$aname" . $term . "</a></b>\n<dd>" . $_;
}


1;				# This must be the last line
