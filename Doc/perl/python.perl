# python.perl by Fred L. Drake, Jr. <fdrake@acm.org>		-*- perl -*-
#
# Heavily based on Guido van Rossum's myformat.perl (now obsolete).
#
# Extension to LaTeX2HTML for documents using myformat.sty.
# Subroutines of the form do_cmd_<name> here define translations
# for LaTeX commands \<name> defined in the corresponding .sty file.

package main;


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

$AUTHOR_ADDRESS = '';
$PYTHON_VERSION = '';

sub do_cmd_version{ $PYTHON_VERSION . @_[0]; }
sub do_cmd_release{
    local($_) = @_;
    s/$next_pair_pr_rx//;
    $PYTHON_VERSION = "$2";
    $_;
}

sub do_cmd_authoraddress{
    local($_) = @_;
    s/$next_pair_pr_rx//;
    $AUTHOR_ADDRESS = "$2";
    $_;
}

sub do_cmd_hackscore{
    local($_) = @_;
    s/$next_pair_pr_rx/_/;
    $_;
}

sub do_cmd_optional{
    local($_) = @_;
    s|$next_pair_pr_rx|</var><big>\[</big><var>\2</var><big>\]</big><var>|;
    $_;
}

sub do_cmd_varvars{
    local($_) = @_;
    s|$next_pair_pr_rx|<var>\2</var>|;
    $_;
}

# Logical formatting (some based on texinfo), needs to be converted to
# minimalist HTML.  The "minimalist" is primarily to reduce the size of
# output files for users that read them over the network rather than
# from local repositories.

sub do_cmd_code{
    local($_) = @_;
    s|$next_pair_pr_rx|<tt>\2</tt>|;
    $_;
}

sub use_sans_serif{
    local($_) = @_;
    s|$next_pair_pr_rx|<font face=sans-serif>\2</font>|;
    $_;
}

sub use_italics{
    local($_) = @_;
    s|$next_pair_pr_rx|<i>\2</i>|;
    $_;
}

sub do_cmd_sectcode{ &do_cmd_code(@_); }
sub do_cmd_module{ &do_cmd_code(@_); }
sub do_cmd_keyword{ &do_cmd_code(@_); }
sub do_cmd_exception{ &do_cmd_code(@_); }
sub do_cmd_class{ &do_cmd_code(@_); }
sub do_cmd_function{ &do_cmd_code(@_); }
sub do_cmd_constant{ &do_cmd_code(@_); }
sub do_cmd_member{ &do_cmd_code(@_); }
sub do_cmd_method{ &do_cmd_code(@_); }
sub do_cmd_cfunction{ &do_cmd_code(@_); }
sub do_cmd_cdata{ &do_cmd_code(@_); }
sub do_cmd_ctype{ &do_cmd_code(@_); }
sub do_cmd_regexp{ &do_cmd_code(@_); }
sub do_cmd_key{ &do_cmd_code(@_); }

sub do_cmd_character{ &do_cmd_samp(@_); }

sub do_cmd_program{ &do_cmd_strong(@_); }

sub do_cmd_email{ &use_sans_serif(@_); }
sub do_cmd_mimetype{ &use_sans_serif(@_); }

sub do_cmd_var{ &use_italics(@_); }
sub do_cmd_dfn{ &use_italics(@_); }	# make an index entry?
sub do_cmd_emph{ &use_italics(@_); }


sub do_cmd_envvar{
    local($_) = @_;
    s/$next_pair_pr_rx//;
    my($br_id,$envvar) = ($1, $2);
    my($name,$aname,$ahref) = link_info($br_id);
    add_index_entry("environment variables!$envvar@\$$envvar", $ahref);
    add_index_entry("$envvar@\$$envvar", $ahref);
    "$aname\$$envvar</a>" . $_;
}

sub do_cmd_url{
    # use the URL as both text and hyperlink
    local($_) = @_;
    s/$next_pair_pr_rx//;
    my $url = $2;
    $url =~ s/~/&#126;/g;
    "<a href=\"$url\"><font face=sans-serif>$url</font></a>" . $_;
}

sub do_cmd_manpage{
    # two parameters:  \manpage{name}{section}
    local($_) = @_;
    my $rx = "$next_pair_pr_rx$any_next_pair_pr_rx3";
    s|$rx|<i>\2</i>(\4)|;
    $_;
}

sub do_cmd_rfc{
    local($_) = @_;
    s/$next_pair_pr_rx//;
    my($br_id,$rfcnumber) = ($1, $2);

    # Save the reference
    my $nstr = &gen_index_id("RFC!RFC $rfcnumber", '');
    $index{$nstr} .= &make_half_href("$CURRENT_FILE#$br_id");
    "<a name=$br_id>RFC $rfcnumber</a>" .$_;
}

sub do_cmd_kbd{
    local($_) = @_;
    s|$next_pair_pr_rx|<kbd>\2</kbd>|;
    $_;
}

sub do_cmd_strong{
    local($_) = @_;
    s|$next_pair_pr_rx|<b>\2</b>|;
    $_;
}

sub do_cmd_deprecated{
    # two parameters:  \deprecated{version}{whattodo}
    local($_) = @_;
    my $rx = "$next_pair_pr_rx$any_next_pair_pr_rx3";
    s|$rx|<b>Deprecated since release \2.</b>\n\4<p>|;
    $_;
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
    s/$next_pair_pr_rx//;
    $INDEX_SUBITEM = $2;
    $_;
}

sub do_cmd_withsubitem{
    # We can't really do the right right thing, because LaTeX2HTML doesn't
    # do things in the right order, but we need to at least strip this stuff
    # out, and leave anything that the second argument expanded out to.
    #
    local($_) = @_;
    my $any_next_pair_pr_rx3 = "$OP(\\d+)$CP([\\s\\S]*)$OP\\3$CP";
    s/$next_pair_pr_rx$any_next_pair_pr_rx3/\4/;
    $INDEX_SUBITEM = $2;
    $_;
}

sub do_cmd_makemodindex{ @_[0]; }

# We're in the document subdirectory when this happens!
open(IDXFILE, ">index.dat") || die "\n$!\n";
open(INTLABELS, ">intlabels.pl") || die "\n$!\n";
print INTLABELS
  "%internal_labels = ();\n1;  # hack in case there are no entries\n\n";

sub gen_target_name{
    "l2h-" . @_[0];
}

sub gen_target{
    "<a name=\"" . @_[0] . "\">";
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
    print IDXFILE $ahref, "\0", $str, "\n";
}

sub link_info{
    my $name = gen_target_name(@_[0]);
    my $aname = gen_target($name);
    my $ahref = gen_link($CURRENT_FILE, $name);
    return ($name, $aname, $ahref);
}

sub do_cmd_index{
    local($_) = @_;
    s/$next_pair_pr_rx[\n]?//o;
    my($br_id,$str) = ($1, $2);
    #
    my($name,$aname,$ahref) = link_info($br_id);
    add_index_entry("$str", $ahref);
    "$aname$anchor_invisible_mark</a>" . $_;
}

sub do_cmd_indexii{
    local($_) = @_;
    s/$next_pair_pr_rx//o;
    my($br_id1,$str1) = ($1, $2);
    s/$next_pair_pr_rx[\n]?//o;
    my($br_id2,$str2) = ($1, $2);
    #
    my($name,$aname,$ahref) = link_info($br_id1);
    add_index_entry("$str1!$str2", $ahref);
    add_index_entry("$str2!$str1", $ahref);
    "$aname$anchor_invisible_mark</a>" . $_;
}

sub do_cmd_indexiii{
    local($_) = @_;
    s/$next_pair_pr_rx//o;
    my($br_id1,$str1) = ($1, $2);
    s/$next_pair_pr_rx//o;
    my($br_id2,$str2) = ($1, $2);
    s/$next_pair_pr_rx[\n]?//o;
    my($br_id3,$str3) = ($1, $2);
    #
    my($name,$aname,$ahref) = link_info($br_id1);
    add_index_entry("$str1!$str2 $str3", $ahref);
    add_index_entry("$str2!$str3, $str1", $ahref);
    add_index_entry("$str3!$str1 $str2", $ahref);
    "$aname$anchor_invisible_mark</a>" . $_;
}

sub do_cmd_indexiv{
    local($_) = @_;
    s/$next_pair_pr_rx//o;
    my($br_id1,$str1) = ($1, $2);
    s/$next_pair_pr_rx//o;
    my($br_id2,$str2) = ($1, $2);
    s/$next_pair_pr_rx//o;
    my($br_id3,$str3) = ($1, $2);
    s/$next_pair_pr_rx[\n]?//o;
    my($br_id4,$str4) = ($1, $2);
    #
    my($name,$aname,$ahref) = link_info($br_id1);
    add_index_entry("$str1!$str2 $str3 $str4", $ahref);
    add_index_entry("$str2!$str3 $str4, $str1", $ahref);
    add_index_entry("$str3!$str4, $str1 $str2", $ahref);
    add_index_entry("$str4!$$str1 $str2 $str3", $ahref);
    "$aname$anchor_invisible_mark</a>" . $_;
}

sub do_cmd_ttindex{
    local($_) = @_;
    s/$next_pair_pr_rx[\n]?//;
    my($br_id,$str) = ($1, $2);
    &make_index_entry($br_id, $str . &get_indexsubitem) . $_;
}

sub my_typed_index_helper{
    local($word,$_) = @_;
    s/$next_pair_pr_rx[\n]?//o;
    my($br_id,$str) = ($1, $2);
    #
    my($name,$aname,$ahref) = link_info($br_id1);
    add_index_entry("$str $word", $ahref);
    add_index_entry("$word!$str", $ahref);
    "$aname$anchor_invisible_mark</a>" . $_;
}

sub do_cmd_stindex{ &my_typed_index_helper('statement', @_); }
sub do_cmd_opindex{ &my_typed_index_helper('operator', @_); }
sub do_cmd_exindex{ &my_typed_index_helper('exception', @_); }
sub do_cmd_obindex{ &my_typed_index_helper('object', @_); }

sub my_parword_index_helper{
    local($word,$_) = @_;
    s/$next_pair_pr_rx[\n]?//o;
    my($br_id,$str) = ($1, $2);
    make_index_entry($br_id, "$str ($word)") . $_;
}


# Set this to true to strip out the <tt>...</tt> from index entries;
# this is analogous to using the second definition of \idxcode{} from
# myformat.sty.
#
# It is used from &make_mod_index_entry() and &make_str_index_entry().
#
$STRIP_INDEX_TT = 0;

sub make_mod_index_entry{
    my($br_id,$str,$define) = @_;
    my($name,$aname,$ahref) = link_info($br_id);
    $str =~ s|<tt>(.*)</tt>|\1|
        if $STRIP_INDEX_TT;
    # equivalent of add_index_entry() using $define instead of ''
    $str = gen_index_id($str, $define);
    $index{$str} .= $ahref;
    print IDXFILE $ahref, "\0", $str, "\n";

    if ($define eq 'DEF') {
	# add to the module index
	my($nstr,$garbage) = split / /, $str, 2;
	$Modules{$nstr} .= $ahref;
    }
    "$aname$anchor_invisible_mark</a>";
}

$THIS_MODULE = '';
$THIS_CLASS = '';

sub my_module_index_helper{
    local($word, $_) = @_;
    s/$next_pair_pr_rx[\n]*//o;
    my($br_id, $str) = ($1, $2);
    my $section_tag = join('', @curr_sec_id);
    $word = "$word " if $word;
    $THIS_MODULE = "$str";
    make_mod_index_entry("SECTION$section_tag",
			 "<tt>$str</tt> (${word}module)", 'DEF') . $_;
}

sub ref_module_index_helper{
    local($word, $_) = @_;
    s/$next_pair_pr_rx[\n]?//o;
    my($br_id, $str) = ($1, $2);
    $word = "$word " if $word;
    make_mod_index_entry($br_id, "<tt>$str</tt> (${word}module)", 'REF') . $_;
}

sub do_cmd_bifuncindex{
    local($_) = @_;
    s/$next_pair_pr_rx[\n]?//o;
    my($br_id,$str,$fname) = ($1, $2, "<tt>$2()</tt>");
    $fname = "$str()"
      if $STRIP_INDEX_TT;
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
    # XXX need some way for this to be called after &initialise; ???
    $anchor_mark = '';
    $icons{'anchor_mark'} = '';
    my $cmark = "(?:$comment_mark)?";
    # <<2>>...<<2>>
    $next_pair_rx = "^[\\s\n%]*$O(\\d+)$C([\\s\\S]*)$O\\1$C";
    $any_next_pair_rx3 = "[\\s\n%]*$O(\\d+)$C([\\s\\S]*)$O\\3$C";
    $any_next_pair_rx5 = "[\\s\n%]*$O(\\d+)$C([\\s\\S]*)$O\\5$C";
    $any_next_pair_rx7 = "[\\s\n%]*$O(\\d+)$C([\\s\\S]*)$O\\7$C";
    $any_next_pair_rx9 = "[\\s\n%]*$O(\\d+)$C([\\s\\S]*)$O\\9$C";
    # <#2#>...<#2#>
    $next_pair_pr_rx = "^[\\s\n%]*$OP(\\d+)$CP([\\s\\S]*)$OP\\1$CP$cmark";
    $any_next_pair_pr_rx3 = "[\\s\n%]*$OP(\\d+)$CP([\\s\\S]*)$OP\\3$CP$cmark";
    $any_next_pair_pr_rx5 = "[\\s\n%]*$OP(\\d+)$CP([\\s\\S]*)$OP\\5$CP$cmark";
    $any_next_pair_pr_rx7 = "[\\s\n%]*$OP(\\d+)$CP([\\s\\S]*)$OP\\7$CP$cmark";
    $any_next_pair_pr_rx9 = "[\\s\n%]*$OP(\\d+)$CP([\\s\\S]*)$OP\\9$CP$cmark";
}
init_myformat();

# similar to make_index_entry(), but includes the string in the result
# instead of the dummy filler.
#
sub make_str_index_entry{
    my($br_id,$str) = @_;
    my($name,$aname,$ahref) = link_info($br_id);
    $str =~ s|<tt>(.*)</tt>|\1|
        if $STRIP_INDEX_TT;
    add_index_entry($str, $ahref);
    "$aname$str</a>";
}

# Changed from the stock version to indent {verbatim} sections,
# and make them smaller, to better match the LaTeX version:

# (Used with LaTeX2HTML 96.1*)
sub replace_verbatim {
    # Modifies $_
    my($prefix,$suffix) = ("\n<p><dl><dd><pre>\n", "</pre></dl>");
    s/$verbatim_mark(verbatim)(\d+)/$prefix$verbatim{$2}$suffix/go;
    s/$verbatim_mark(rawhtml)(\d+)/$verbatim{$2}/ego;	# Raw HTML
}

# (Used with LaTeX2HTML 98.1)
# The HTML this produces is bad; the <PRE> is on the outside of the <dl>,
# but I haven't found a workaround yet.
sub replace_verbatim_hook{
    # Modifies $_
    my($prefix,$suffix) = ("\n<p><dl><dd>", "</dl>");
    s/$math_verbatim_rx/&put_comment("MATH: ".$verbatim{$1})/eg;
    s/$verbatim_mark(\w*[vV]erbatim\*?)(\d+)\#/$prefix$verbatim{$2}$suffix/go;
    # Raw HTML, but replacements may have protected characters
    s/$verbatim_mark(rawhtml)(\d+)#/&unprotect_raw_html($verbatim{$2})/eg;
    s/$verbatim_mark$keepcomments(\d+)#/$verbatim{$2}/ego; # Raw TeX
    s/$unfinished_mark$keepcomments(\d+)#/$verbatim{$2}/ego; # Raw TeX
}

sub do_env_cfuncdesc{
    local($_) = @_;
    my($return_type,$function_name,$arg_list,$idx) = ('', '', '', '');
    my $any_next_pair_rx3 = "$O(\\d+)$C([\\s\\S]*)$O\\3$C";
    my $any_next_pair_rx5 = "$O(\\d+)$C([\\s\\S]*)$O\\5$C";
    my $cfuncdesc_rx = "$next_pair_rx$any_next_pair_rx3$any_next_pair_rx5";
    if (/$cfuncdesc_rx/o) {
	$return_type = "$2";
	$function_name = "$4";
	$arg_list = "$6";
	$idx = make_str_index_entry($3,
			"<tt>$function_name()</tt>" . &get_indexsubitem);
	$idx =~ s/ \(.*\)//;
	$idx =~ s/\(\)//;
    }
    "<dl><dt>$return_type <b>$idx</b>"
      . "(<var>$arg_list</var>)\n<dd>$'</dl>"
}

sub do_env_ctypedesc{
    local($_) = @_;
    my $type_name = ('');
    my $cfuncdesc_rx = "$next_pair_rx";
    if (/$cfuncdesc_rx/o) {
	$type_name = "$2";
	$idx = make_str_index_entry($1,
			"<tt>$type_name</tt>" . &get_indexsubitem);
	$idx =~ s/ \(.*\)//;
    }
    "<dl><dt><b>$idx</b>\n<dd>$'</dl>"
}

sub do_env_cvardesc{
    local($_) = @_;
    my($var_type,$var_name,$idx) = ('', '', '');
    my $cfuncdesc_rx = "$next_pair_rx$any_next_pair_rx3";
    if (/$cfuncdesc_rx/o) {
	$var_type = "$2";
	$var_name = "$4";
	$idx = make_str_index_entry($3,
			"<tt>$var_name</tt>" . &get_indexsubitem);
	$idx =~ s/ \(.*\)//;
    }
    "<dl><dt>$var_type <b>$idx</b>\n"
      . "<dd>$'</dl>";
}

sub do_env_funcdesc{
    local($_) = @_;
    my($function_name,$arg_list,$idx) = ('', '', '');
    my $funcdesc_rx = "$next_pair_rx$any_next_pair_rx3";
    if (/$funcdesc_rx/o) {
	$function_name = "$2";
	$arg_list = "$4";
	$idx = make_str_index_entry($3, "<tt>$function_name()</tt>"
				     . &get_indexsubitem);
	$idx =~ s/ \(.*\)//;
	$idx =~ s/\(\)//;
    }
    "<dl><dt><b>$idx</b> (<var>$arg_list</var>)\n<dd>$'</dl>";
}

sub do_env_funcdescni{
    local($_) = @_;
    my($function_name,$arg_list,$idx) = ('', '', '');
    my $funcdesc_rx = "$next_pair_rx$any_next_pair_rx3";
    if (/$funcdesc_rx/o) {
	$function_name = "$2";
	$arg_list = "$4";
	if ($STRIP_INDEX_TT) {
	    $idx = "$function_name"; }
	else {
	    $idx = "<tt>$function_name</tt>"; }
    }
    "<dl><dt><b>$idx</b> (<var>$arg_list</var>)\n<dd>$'</dl>";
}

sub do_cmd_funcline{
    local($_) = @_;
    my $any_next_pair_pr_rx3 = "$OP(\\d+)$CP([\\s\\S]*)$OP\\3$CP";

    s/$next_pair_pr_rx//o;
    my $function_name = $2;
    s/$next_pair_pr_rx//o;
    my($br_id,$arg_list) = ($1, $2);
    my $idx = make_str_index_entry($br_id, "<tt>$function_name()</tt>"
				   . &get_indexsubitem);
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
    my($opcode_name,$arg_list,$stuff,$idx) = ('', '', '', '');
    my $opcodedesc_rx = "$next_pair_rx$any_next_pair_rx3";
    if (/$opcodedesc_rx/o) {
	$opcode_name = "$2";
	$arg_list = "$4";
	if ($INDEX_OPCODES) {
	    $idx = make_str_index_entry($3,
			"<tt>$opcode_name</tt> (byte code instruction)");
	    $idx =~ s/ \(byte code instruction\)//;
	}
	else {
	    $idx = "<tt>$opcode_name</tt>";
	}
      }
    $stuff = "<dl><dt><b>$idx</b>";
    if ($arg_list) {
	$stuff .= "&nbsp;&nbsp;&nbsp;&nbsp;<var>$arg_list</var>";
    }
    $stuff . "\n<dd>$'</dl>";
}

sub do_env_datadesc{
    local($_) = @_;
    my $idx = '';
    if (/$next_pair_rx/o) {
	$idx = make_str_index_entry($1, "<tt>$2</tt>" . &get_indexsubitem);
	$idx =~ s/ \(.*\)//;
    }
    "<dl><dt><b>$idx</b>\n<dd>$'</dl>"
}

sub do_env_datadescni{
    local($_) = @_;
    my $idx = '';
    if (/$next_pair_rx/o) {
	if ($STRING_INDEX_TT) {
	    $idx = "$2"; }
	else {
	    $idx = "<tt>$2</tt>"; }
    }
    "<dl><dt><b>$idx</b>\n<dd>$'</dl>"
}

sub do_cmd_dataline{
    local($_) = @_;

    s/$next_pair_pr_rx//o;
    my($br_id, $data_name) = ($1, $2);
    my $idx = make_str_index_entry($br_id, "<tt>$data_name</tt>"
				   . &get_indexsubitem);
    $idx =~ s/ \(.*\)//;

    "<dt><b>$idx</b><dd>" . $_;
}

sub do_env_excdesc{
    local($_) = @_;
    /$next_pair_rx/o;
    my($br_id,$excname,$rest) = ($1, $2, $');
    my $idx = make_str_index_entry($br_id,
			"<tt>$excname</tt> (exception in $THIS_MODULE)");
    $idx =~ s/ \(.*\)//;
    "<dl><dt><b>$idx</b>\n<dd>$rest</dl>"
}

sub do_env_fulllineitems{ do_env_itemize(@_); }


sub do_env_classdesc{
    local($_) = @_;
    my($class_name,$arg_list,$idx) = ('', '', '');
    my $funcdesc_rx = "$next_pair_rx$any_next_pair_rx3";
    if (/$funcdesc_rx/o) {
	$class_name = "$2";
	$arg_list = "$4";
	$THIS_CLASS = $class_name;
	$idx = make_str_index_entry($3,
			"<tt>$class_name</tt> (class in $THIS_MODULE)" );
	$idx =~ s/ \(.*\)//;
    }
    "<dl><dt><b>$idx</b> (<var>$arg_list</var>)\n<dd>$'</dl>";
}


sub do_env_methoddesc{
    local($_) = @_;
    my($class_name,$arg_list,$idx,$extra);
    # Predefined $opt_arg_rx & $optional_arg_rx don't work because they
    # require the argument to be there.
    my $opt_arg_rx = "^\\s*(\\[([^]]*)\\])?";
    my $funcdesc_rx = "$opt_arg_rx$any_next_pair_rx3$any_next_pair_rx5";
    if (/$funcdesc_rx/o) {
	$class_name = $2;
	$class_name = $THIS_CLASS
	    unless $class_name;
	$method_name = $4;
	$arg_list = $6;
	if ($class_name) {
	    $extra = " ($class_name method)";
	}
	$idx = make_str_index_entry($3, "<tt>$method_name()</tt>$extra");
	$idx =~ s/ \(.*\)//;
	$idx =~ s/\(\)//;
    }
    "<dl><dt><b>$idx</b> (<var>$arg_list</var>)\n<dd>$'</dl>";
}


sub do_env_methoddescni{
    local($_) = @_;
    # Predefined $opt_arg_rx & $optional_arg_rx don't work because they
    # require the argument to be there.
    my $opt_arg_rx = "^\\s*(\\[([^]]*)\\])?";
    my $funcdesc_rx = "$opt_arg_rx$any_next_pair_rx3$any_next_pair_rx5";
    /$funcdesc_rx/o;
    my $method = $4;
    my $arg_list = $6;
    "<dl><dt><b>$method</b> (<var>$arg_list</var>)\n<dd>$'</dl>";
}


sub do_env_memberdesc{
    local($_) = @_;
    # Predefined $opt_arg_rx & $optional_arg_rx don't work because they
    # require the argument to be there.
    my $opt_arg_rx = "^\\s*(\\[([^]]*)\\])?";
    my $funcdesc_rx = "$opt_arg_rx$any_next_pair_rx3$any_next_pair_rx5";
    /$funcdesc_rx/o;
    my($class,$member,$arg_list) = ($2, $4, $6);
    $class = $THIS_CLASS
        unless $class;
    $extra = " ($class_name attribute)"
        if $class;
    my $idx = make_str_index_entry($3, "<tt>$member()</tt>$extra");
    $idx =~ s/ \(.*\)//;
    $idx =~ s/\(\)//;
    "<dl><dt><b>$idx</b>\n<dd>$'</dl>";
}


sub do_env_memberdescni{
    local($_) = @_;
    # Predefined $opt_arg_rx & $optional_arg_rx don't work because they
    # require the argument to be there.
    my $opt_arg_rx = "^\\s*(\\[([^]]*)\\])?";
    my $funcdesc_rx = "$opt_arg_rx$any_next_pair_rx3";
    /$funcdesc_rx/o;
    my $member = $4;
    "<dl><dt><b>$member</b>\n<dd>$'</dl>";
}


@col_aligns = ("<td>", "<td>", "<td>");

sub setup_column_alignments{
    local($_) = @_;
    my($j1,$a1,$a2,$a3,$j4) = split(/[|]/,$_);
    my($th1,$th2,$th3) = ('<th>', '<th>', '<th>');
    $col_aligns[0] = (($a1 eq "c") ? "<td align=center>" : "<td>");
    $col_aligns[1] = (($a2 eq "c") ? "<td align=center>" : "<td>");
    $col_aligns[2] = (($a3 eq "c") ? "<td align=center>" : "<td>");
    # return the aligned header start tags; only used for \begin{tableiii?}
    $th1 = (($a1 eq "l") ? "<th align=left>"
	    : ($a1 eq "r" ? "<th align=right>" : "<th>"));
    $th2 = (($a2 eq "l") ? "<th align=left>"
	    : ($a2 eq "r" ? "<th align=right>" : "<th>"));
    $th3 = (($a3 eq "l") ? "<th align=left>"
	    : ($a3 eq "r" ? "<th align=right>" : "<th>"));
    ($th1, $th2, $th3);
}

sub do_env_tableii{
    local($_) = @_;
    my($font,$h1,$h2) = ('', '', '');
    my $tableiii_rx =
      "$next_pair_rx$any_next_pair_rx3$any_next_pair_rx5$any_next_pair_rx7";
    if (/$tableiii_rx/o) {
	$font = $4;
	$h1 = $6;
	$h2 = $8;
    }
    my($th1,$th2,$th3) = setup_column_alignments($2);
    $globals{"lineifont"} = $font;
    "<table border align=center>"
      . "\n  <tr>$th1<b>$h1</b></th>"
      . "\n      $th2<b>$h2</b></th>$'"
      . "\n</table>";
}

sub do_cmd_lineii{
    local($_) = @_;
    s/$next_pair_pr_rx//o;
    my $c1 = $2;
    s/$next_pair_pr_rx//o;
    my $c2 = $2;
    my $font = $globals{"lineifont"};
    my($c1align,$c2align) = @col_aligns[0,1];
    "<tr>$c1align<$font>$c1</$font></td>\n"
      . "      $c2align$c2</td>$'";
}

sub do_env_tableiii{
    local($_) = @_;
    my($font,$h1,$h2,$h3) = ('', '', '', '');
  
    my $tableiii_rx =
      "$next_pair_rx$any_next_pair_rx3$any_next_pair_rx5$any_next_pair_rx7"
	. "$any_next_pair_rx9";
    if (/$tableiii_rx/o) {
	$font = $4;
	$h1 = $6;
	$h2 = $8;
	$h3 = $10;
    }
    my($th1,$th2,$th3) = setup_column_alignments($2);
    $globals{"lineifont"} = $font;
    "<table border align=center>"
      . "\n  <tr>$th1<b>$h1</b></th>"
      . "\n      $th2<b>$h2</b></th>"
      . "\n      $th3<b>$h3</b></th>$'"
      . "\n</table>";
}

sub do_cmd_lineiii{
    local($_) = @_;
    s/$next_pair_pr_rx//o;
    my $c1 = $2;
    s/$next_pair_pr_rx//o;
    my $c2 = $2;
    s/$next_pair_pr_rx//o;
    my $c3 = $2;
    my $font = $globals{"lineifont"};
    my($c1align, $c2align, $c3align) = @col_aligns;
    "<tr>$c1align<$font>$c1</$font></td>\n"
      . "      $c2align$c2</td>\n"
      . "      $c3align$c3</td>$'";
}

sub do_env_seealso{
    "<p><b>See Also:</b></p>\n" . @_[0];
}

sub do_cmd_seemodule{
    # Insert the right magic to jump to the module definition.  This should
    # work most of the time, at least for repeat builds....
    local($_) = @_;
    my $any_next_pair_pr_rx3 = "$OP(\\d+)$CP([\\s\\S]*)$OP\\3$CP";
    my $any_next_pair_pr_rx5 = "$OP(\\d+)$CP([\\s\\S]*)$OP\\5$CP";
    # Predefined $opt_arg_rx & $optional_arg_rx don't work because they
    # require the argument to be there.
    my $opt_arg_rx = "^\\s*(\\[([^]]*)\\])?";
    s/$opt_arg_rx$any_next_pair_pr_rx3$any_next_pair_pr_rx5//;
    my($key,$module,$text) = ($2, $4, $6);
    $key = $module unless $key;
    "<p>Module <tt><b><a href=\"module-$key.html\">$module</a></b></tt>"
      . "&nbsp;&nbsp;&nbsp;($text)</p>"
      . $_;
}

sub do_cmd_seetext{
    "<p>" . @_[0];
}


sub do_cmd_maketitle {
    local($_) = @_;
    my $the_title = '';
    if ($t_title) {
	$the_title .= "<h1 align=center>$t_title</h1>";
    } else { &write_warnings("\nThis document has no title."); }
    $the_title .= "\n<center>";
    if ($t_author) {
	if ($t_authorURL) {
	    my $href = &translate_commands($t_authorURL);
	    $href = &make_named_href('author', $href, "<strong>${t_author}</strong>");
	    $the_title .= "\n<p>$href</p>";
	} else {
	    $the_title .= "\n<p><strong>$t_author</strong></p>";
	}
    } else { &write_warnings("\nThere is no author for this document."); }
    if ($t_institute) {
        $the_title .= "\n<p>$t_institute</p>";}
    if ($AUTHOR_ADDRESS) {
        $the_title .= "\n<p>$AUTHOR_ADDRESS</p>";}
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


# sub do_cmd_indexlabel{
#     "genindex" . @_[0];
# }


# These are located down here since they screw up fontlock.  -- used to.

sub do_cmd_file{
    # This uses a weird HTML construct to adjust the font to be
    # reasonable match that used in the printed form as much as
    # possible.  The expected behavior is that a browser that doesn't
    # understand "<font face=...>" markup will use courier (or whatever
    # the font is for <tt>).
    local($_) = @_;
    s|$next_pair_pr_rx|\"<tt>\2</tt>\"|;
    $_;
}

sub do_cmd_samp{
    local($_) = @_;
    s|$next_pair_pr_rx|\"<tt>\2</tt>\"|;
    $_;
}

1;				# This must be the last line
