# python.perl by Fred L. Drake, Jr. <fdrake@acm.org>		-*- perl -*-
#
# Heavily based on Guido van Rossum's myformat.perl (now obsolete).
#
# Extension to LaTeX2HTML for documents using myformat.sty.
# Subroutines of the form do_cmd_<name> here define translations
# for LaTeX commands \<name> defined in the corresponding .sty file.

package main;

use File::Basename;


sub next_argument{
    my $param;
    $param = missing_braces()
      unless ((s/$next_pair_pr_rx/$param=$2;''/eo)
	      ||(s/$next_pair_rx/$param=$2;''/eo));
    return $param;
}

sub next_optional_argument{
    my($param,$rx) = ('', "^\\s*(\\[([^]]*)\\])?");
    s/$rx/$param=$2;''/eo;
    return $param;
}

sub make_icon_filename($){
    my($myname, $mydir, $myext) = fileparse(@_[0], '\..*');
    chop $mydir;
    if ($mydir eq '.') {
        $mydir = $ICONSERVER;
    }
    $myext = ".$IMAGE_TYPE"
      unless $myext;
    return "$mydir$dd$myname$myext";
}

sub get_link_icon($){
    my $url = @_[0];
    if ($OFF_SITE_LINK_ICON && ($url =~ /^[-a-zA-Z0-9.]+:/)) {
        # absolute URL; assume it points off-site
        my $icon = make_icon_filename($OFF_SITE_LINK_ICON);
        return (" <img src='$icon'\n"
                . "  border='0' class='offsitelink'"
                . ($OFF_SITE_LINK_ICON_HEIGHT
                   ? " height='$OFF_SITE_LINK_ICON_HEIGHT'"
                   : '')
                . ($OFF_SITE_LINK_ICON_WIDTH
                   ? " width='$OFF_SITE_LINK_ICON_WIDTH'"
                   : '')
                . " alt='[off-site link]'\n"
                . "  >");
    }
    return '';
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
    return $_;
}


# the older version of LaTeX2HTML we use doesn't support this, but we use it:

sub do_cmd_textasciitilde{ '~' . @_[0]; }


# words typeset in a special way (not in HTML though)

sub do_cmd_ABC{ 'ABC' . @_[0]; }
sub do_cmd_UNIX{ 'Unix'. @_[0]; }
sub do_cmd_ASCII{ 'ASCII' . @_[0]; }
sub do_cmd_POSIX{ 'POSIX' . @_[0]; }
sub do_cmd_C{ 'C' . @_[0]; }
sub do_cmd_Cpp{ 'C++' . @_[0]; }
sub do_cmd_EOF{ 'EOF' . @_[0]; }
sub do_cmd_NULL{ '<tt class="constant">NULL</tt>' . @_[0]; }

sub do_cmd_e{ '&#92;' . @_[0]; }

$DEVELOPER_ADDRESS = '';
$SHORT_VERSION = '';
$PACKAGE_VERSION = '';

sub do_cmd_version{ $PACKAGE_VERSION . @_[0]; }
sub do_cmd_shortversion{ $SHORT_VERSION . @_[0]; }
sub do_cmd_release{
    local($_) = @_;
    $PACKAGE_VERSION = next_argument();
    return $_;
}

sub do_cmd_setshortversion{
    local($_) = @_;
    $SHORT_VERSION = next_argument();
    return $_;
}

sub do_cmd_authoraddress{
    local($_) = @_;
    $DEVELOPER_ADDRESS = next_argument();
    return $_;
}

#sub do_cmd_developer{ do_cmd_author(@_[0]); }
#sub do_cmd_developers{ do_cmd_author(@_[0]); }
#sub do_cmd_developersaddress{ do_cmd_authoraddress(@_[0]); }

sub do_cmd_hackscore{
    local($_) = @_;
    next_argument();
    return '_' . $_;
}

sub use_wrappers{
    local($_,$before,$after) = @_;
    my $stuff = next_argument();
    return $before . $stuff . $after . $_;
}

$IN_DESC_HANDLER = 0;
sub do_cmd_optional{
    if ($IN_DESC_HANDLER) {
        return use_wrappers(@_[0], "</var><big>\[</big><var>",
                            "</var><big>\]</big><var>");
    }
    else {
        return use_wrappers(@_[0], "<big>\[</big>", "<big>\]</big>");
    }
}

# Logical formatting (some based on texinfo), needs to be converted to
# minimalist HTML.  The "minimalist" is primarily to reduce the size of
# output files for users that read them over the network rather than
# from local repositories.

# \file and \samp are at the end of this file since they screw up fontlock.

sub do_cmd_pytype{ return @_[0]; }
sub do_cmd_makevar{
    return use_wrappers(@_[0], '<span class="makevar">', '</span>'); }
sub do_cmd_code{
    return use_wrappers(@_[0], '<code>', '</code>'); }
sub do_cmd_module{
    return use_wrappers(@_[0], '<tt class="module">', '</tt>'); }
sub do_cmd_keyword{
    return use_wrappers(@_[0], '<tt class="keyword">', '</tt>'); }
sub do_cmd_exception{
    return use_wrappers(@_[0], '<tt class="exception">', '</tt>'); }
sub do_cmd_class{
    return use_wrappers(@_[0], '<tt class="class">', '</tt>'); }
sub do_cmd_function{
    return use_wrappers(@_[0], '<tt class="function">', '</tt>'); }
sub do_cmd_constant{
    return use_wrappers(@_[0], '<tt class="constant">', '</tt>'); }
sub do_cmd_member{
    return use_wrappers(@_[0], '<tt class="member">', '</tt>'); }
sub do_cmd_method{
    return use_wrappers(@_[0], '<tt class="method">', '</tt>'); }
sub do_cmd_cfunction{
    return use_wrappers(@_[0], '<tt class="cfunction">', '</tt>'); }
sub do_cmd_cdata{
    return use_wrappers(@_[0], '<tt class="cdata">', '</tt>'); }
sub do_cmd_ctype{
    return use_wrappers(@_[0], '<tt class="ctype">', '</tt>'); }
sub do_cmd_regexp{
    return use_wrappers(@_[0], '<tt class="regexp">', '</tt>'); }
sub do_cmd_character{
    return use_wrappers(@_[0], '"<tt class="character">', '</tt>"'); }
sub do_cmd_program{
    return use_wrappers(@_[0], '<b class="program">', '</b>'); }
sub do_cmd_programopt{
    return use_wrappers(@_[0], '<b class="programopt">', '</b>'); }
sub do_cmd_longprogramopt{
    # note that the --- will be later converted to -- by LaTeX2HTML
    return use_wrappers(@_[0], '<b class="programopt">---', '</b>'); }
sub do_cmd_email{
    return use_wrappers(@_[0], '<span class="email">', '</span>'); }
sub do_cmd_mimetype{
    return use_wrappers(@_[0], '<span class="mimetype">', '</span>'); }
sub do_cmd_var{
    return use_wrappers(@_[0], "<var>", "</var>"); }
sub do_cmd_dfn{
    return use_wrappers(@_[0], '<i class="dfn">', '</i>'); }
sub do_cmd_emph{
    return use_wrappers(@_[0], '<i>', '</i>'); }
sub do_cmd_file{
    return use_wrappers(@_[0], '<span class="file">', '</span>'); }
sub do_cmd_filenq{
    return do_cmd_file(@_[0]); }
sub do_cmd_samp{
    return use_wrappers(@_[0], '"<tt class="samp">', '</tt>"'); }
sub do_cmd_kbd{
    return use_wrappers(@_[0], '<kbd>', '</kbd>'); }
sub do_cmd_strong{
    return use_wrappers(@_[0], '<b>', '</b>'); }
sub do_cmd_textbf{
    return use_wrappers(@_[0], '<b>', '</b>'); }
sub do_cmd_textit{
    return use_wrappers(@_[0], '<i>', '</i>'); }

sub do_cmd_moreargs{
    return '...' . @_[0]; }
sub do_cmd_unspecified{
    return '...' . @_[0]; }


sub do_cmd_refmodule{
    # Insert the right magic to jump to the module definition.
    local($_) = @_;
    my $key = next_optional_argument();
    my $module = next_argument();
    $key = $module
        unless $key;
    return "<tt class='module'><a href='module-$key.html'>$module</a></tt>"
      . $_;
}

sub do_cmd_newsgroup{
    local($_) = @_;
    my $newsgroup = next_argument();
    my $icon = get_link_icon("news:$newsgroup");
    my $stuff = "<a class='newsgroup' href='news:$newsgroup'>"
      . "$newsgroup$icon</a>";
    return $stuff . $_;
}

sub do_cmd_envvar{
    local($_) = @_;
    my $envvar = next_argument();
    my($name,$aname,$ahref) = new_link_info();
    # The <tt> here is really to keep buildindex.py from making
    # the variable name case-insensitive.
    add_index_entry("environment variables!$envvar@<tt>$envvar</tt>",
		    $ahref);
    add_index_entry("$envvar (environment variable)", $ahref);
    $aname =~ s/<a/<a class="envvar"/;
    return "$aname$envvar</a>" . $_;
}

sub do_cmd_url{
    # use the URL as both text and hyperlink
    local($_) = @_;
    my $url = next_argument();
    my $icon = get_link_icon($url);
    $url =~ s/~/&#126;/g;
    return "<a class=\"url\" href=\"$url\">$url$icon</a>" . $_;
}

sub do_cmd_manpage{
    # two parameters:  \manpage{name}{section}
    local($_) = @_;
    my $page = next_argument();
    my $section = next_argument();
    return "<span class='manpage'><i>$page</i>($section)</span>" . $_;
}

$PEP_FORMAT = "http://python.sourceforge.net/peps/pep-XXXX.html";
$RFC_FORMAT = "http://www.ietf.org/rfc/rfcXXXX.txt";

sub get_rfc_url($$){
    my($rfcnum, $format) = @_;
    $rfcnum = sprintf("%04d", $rfcnum);
    $format = "$format";
    $format =~ s/XXXX/$rfcnum/;
    return $format;
}

sub do_cmd_pep{
    local($_) = @_;
    my $rfcnumber = next_argument();
    my $id = "rfcref-" . ++$global{'max_id'};
    my $href = get_rfc_url($rfcnumber, $PEP_FORMAT);
    my $icon = get_link_icon($href);
    # Save the reference
    my $nstr = gen_index_id("Python Enhancement Proposals!PEP $rfcnumber", '');
    $index{$nstr} .= make_half_href("$CURRENT_FILE#$id");
    return ("<a class=\"rfc\" name=\"$id\"\nhref=\"$href\">PEP $rfcnumber"
            . "$icon</a>" . $_);
}

sub do_cmd_rfc{
    local($_) = @_;
    my $rfcnumber = next_argument();
    my $id = "rfcref-" . ++$global{'max_id'};
    my $href = get_rfc_url($rfcnumber, $RFC_FORMAT);
    my $icon = get_link_icon($href);
    # Save the reference
    my $nstr = gen_index_id("RFC!RFC $rfcnumber", '');
    $index{$nstr} .= make_half_href("$CURRENT_FILE#$id");
    return ("<a class=\"rfc\" name=\"$id\"\nhref=\"$href\">RFC $rfcnumber"
            . "$icon</a>" . $_);
}

sub do_cmd_citetitle{
    local($_) = @_;
    my $url = next_optional_argument();
    my $title = next_argument();
    my $icon = get_link_icon($url);
    my $repl = '';
    if ($url) {
        $repl = ("<em class='citetitle'><a\n"
                 . " href='$url'\n"
                 . " title='$title'\n"
                 . " >$title$icon</a></em>");
    }
    else {
        $repl = "<em class='citetitle'\n >$title</em>";
    }
    return $repl . $_;
}

sub do_cmd_deprecated{
    # two parameters:  \deprecated{version}{whattodo}
    local($_) = @_;
    my $release = next_argument();
    my $reason = next_argument();
    return ('<div class="versionnote">'
            . "<b>Deprecated since release $release.</b>"
            . "\n$reason</div><p>"
            . $_);
}

sub do_cmd_versionadded{
    # one parameter:  \versionadded{version}
    local($_) = @_;
    my $release = next_argument();
    return ("\n<span class='versionnote'>New in version $release.</span>\n"
            . $_);
}

sub do_cmd_versionchanged{
    # one parameter:  \versionchanged{version}
    local($_) = @_;
    my $explanation = next_optional_argument();
    my $release = next_argument();
    my $text = "Changed in version $release.";
    if ($explanation) {
        $text = "Changed in version $release:\n$explanation.";
    }
    return "\n<span class='versionnote'>$text</span>\n" . $_;
}

#
# These function handle platform dependency tracking.
#
sub do_cmd_platform{
    local($_) = @_;
    my $platform = next_argument();
    $ModulePlatforms{"<tt class='module'>$THIS_MODULE</tt>"} = $platform;
    $platform = "Macintosh"
      if $platform eq 'Mac';
    return "\n<p class='availability'>Availability: <span"
      . "\n class='platform'>$platform</span>.</p>\n" . $_;
}

$IGNORE_PLATFORM_ANNOTATION = '';
sub do_cmd_ignorePlatformAnnotation{
    local($_) = @_;
    $IGNORE_PLATFORM_ANNOTATION = next_argument();
    return $_;
}


# index commands

$INDEX_SUBITEM = "";

sub get_indexsubitem{
    return $INDEX_SUBITEM ? " $INDEX_SUBITEM" : '';
}

sub do_cmd_setindexsubitem{
    local($_) = @_;
    $INDEX_SUBITEM = next_argument();
    return $_;
}

sub do_cmd_withsubitem{
    # We can't really do the right thing, because LaTeX2HTML doesn't
    # do things in the right order, but we need to at least strip this stuff
    # out, and leave anything that the second argument expanded out to.
    #
    local($_) = @_;
    my $oldsubitem = $INDEX_SUBITEM;
    $INDEX_SUBITEM = next_argument();
    my $stuff = next_argument();
    my $br_id = ++$globals{'max_id'};
    my $marker = "$O$br_id$C";
    return
      $stuff
      . "\\setindexsubitem$marker$oldsubitem$marker"
      . $_;
}

# This is the prologue macro which is required to start writing the
# mod\jobname.idx file; we can just ignore it.  (Defining this suppresses
# a warning that \makemodindex is unknown.)
#
sub do_cmd_makemodindex{ return @_[0]; }

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

sub write_idxfile{
    my ($ahref, $str) = @_;
    print IDXFILE $ahref, $IDXFILE_FIELD_SEP, $str, "\n";
}


sub gen_link{
    my($node,$target) = @_;
    print INTLABELS "\$internal_labels{\"$target\"} = \"$URL/$node\";\n";
    return "<a href='$node#$target'>";
}

sub add_index_entry{
    # add an entry to the index structures; ignore the return value
    my($str,$ahref) = @_;
    $str = gen_index_id($str, '');
    $index{$str} .= $ahref;
    write_idxfile($ahref, $str);
}

sub new_link_info{
    my $name = "l2h-" . ++$globals{'max_id'};
    my $aname = "<a name='$name'>";
    my $ahref = gen_link($CURRENT_FILE, $name);
    return ($name, $aname, $ahref);
}

$IndexMacroPattern = '';
sub define_indexing_macro{
    my $count = @_;
    my $i = 0;
    for (; $i < $count; ++$i) {
	my $name = @_[$i];
	my $cmd = "idx_cmd_$name";
	die "\nNo function $cmd() defined!\n"
	  if (!defined &$cmd);
	eval ("sub do_cmd_$name { return process_index_macros("
	      . "\@_[0], '$name'); }");
	if (length($IndexMacroPattern) == 0) {
	    $IndexMacroPattern = "$name";
	}
	else {
	    $IndexMacroPattern .= "|$name";
	}
    }
}

$DEBUG_INDEXING = 0;
sub process_index_macros{
    local($_) = @_;
    my $cmdname = @_[1];	# This is what triggered us in the first place;
				# we know it's real, so just process it.
    my($name,$aname,$ahref) = new_link_info();
    my $cmd = "idx_cmd_$cmdname";
    print "\nIndexing: \\$cmdname"
      if $DEBUG_INDEXING;
    &$cmd($ahref);		# modifies $_ and adds index entries
    while (/^[\s\n]*\\($IndexMacroPattern)</) {
	$cmdname = "$1";
	print " \\$cmdname"
	  if $DEBUG_INDEXING;
	$cmd = "idx_cmd_$cmdname";
	if (!defined &$cmd) {
	    last;
	}
	else {
	    s/^[\s\n]*\\$cmdname//;
	    &$cmd($ahref);
	}
    }
    if (/^[ \t\r\n]/) {
        $_ = substr($_, 1);
    }
    return "$aname$anchor_invisible_mark</a>" . $_;
}

define_indexing_macro('index');
sub idx_cmd_index{
    my $str = next_argument();
    add_index_entry("$str", @_[0]);
}

define_indexing_macro('kwindex');
sub idx_cmd_kwindex{
    my $str = next_argument();
    add_index_entry("<tt>$str</tt>!keyword", @_[0]);
    add_index_entry("keyword!<tt>$str</tt>", @_[0]);
}

define_indexing_macro('indexii');
sub idx_cmd_indexii{
    my $str1 = next_argument();
    my $str2 = next_argument();
    add_index_entry("$str1!$str2", @_[0]);
    add_index_entry("$str2!$str1", @_[0]);
}

define_indexing_macro('indexiii');
sub idx_cmd_indexiii{
    my $str1 = next_argument();
    my $str2 = next_argument();
    my $str3 = next_argument();
    add_index_entry("$str1!$str2 $str3", @_[0]);
    add_index_entry("$str2!$str3, $str1", @_[0]);
    add_index_entry("$str3!$str1 $str2", @_[0]);
}

define_indexing_macro('indexiv');
sub idx_cmd_indexiv{
    my $str1 = next_argument();
    my $str2 = next_argument();
    my $str3 = next_argument();
    my $str4 = next_argument();
    add_index_entry("$str1!$str2 $str3 $str4", @_[0]);
    add_index_entry("$str2!$str3 $str4, $str1", @_[0]);
    add_index_entry("$str3!$str4, $str1 $str2", @_[0]);
    add_index_entry("$str4!$$str1 $str2 $str3", @_[0]);
}

define_indexing_macro('ttindex');
sub idx_cmd_ttindex{
    my $str = next_argument();
    my $entry = $str . get_indexsubitem();
    add_index_entry($entry, @_[0]);
}

sub my_typed_index_helper{
    my($word,$ahref) = @_;
    my $str = next_argument();
    add_index_entry("$str $word", $ahref);
    add_index_entry("$word!$str", $ahref);
}

define_indexing_macro('stindex', 'opindex', 'exindex', 'obindex');
sub idx_cmd_stindex{ my_typed_index_helper('statement', @_[0]); }
sub idx_cmd_opindex{ my_typed_index_helper('operator', @_[0]); }
sub idx_cmd_exindex{ my_typed_index_helper('exception', @_[0]); }
sub idx_cmd_obindex{ my_typed_index_helper('object', @_[0]); }

define_indexing_macro('bifuncindex');
sub idx_cmd_bifuncindex{
    my $str = next_argument();
    add_index_entry("<tt class='function'>$str()</tt> (built-in function)",
                    @_[0]);
}


sub make_mod_index_entry{
    my($str,$define) = @_;
    my($name,$aname,$ahref) = new_link_info();
    # equivalent of add_index_entry() using $define instead of ''
    $ahref =~ s/\#[-_a-zA-Z0-9]*\"/\"/
      if ($define eq 'DEF');
    $str = gen_index_id($str, $define);
    $index{$str} .= $ahref;
    write_idxfile($ahref, $str);

    if ($define eq 'DEF') {
	# add to the module index
        $str =~ /(<tt.*<\/tt>)/;
        my $nstr = $1;
	$Modules{$nstr} .= $ahref;
    }
    return "$aname$anchor_invisible_mark2</a>";
}


$THIS_MODULE = '';
$THIS_CLASS = '';

sub define_module{
    my($word,$name) = @_;
    my $section_tag = join('', @curr_sec_id);
    if ($word ne "built-in" && $word ne "extension"
	&& $word ne "standard" && $word ne "") {
	write_warnings("Bad module type '$word'"
		       . " for \\declaremodule (module $name)");
	$word = "";
    }
    $word = "$word " if $word;
    $THIS_MODULE = "$name";
    $INDEX_SUBITEM = "(in module $name)";
    print "[$name]";
    return make_mod_index_entry(
        "<tt class='module'>$name</tt> (${word}module)", 'DEF');
}

sub my_module_index_helper{
    local($word, $_) = @_;
    my $name = next_argument();
    return define_module($word, $name) . $_;
}

sub do_cmd_modindex{ return my_module_index_helper('', @_); }
sub do_cmd_bimodindex{ return my_module_index_helper('built-in', @_); }
sub do_cmd_exmodindex{ return my_module_index_helper('extension', @_); }
sub do_cmd_stmodindex{ return my_module_index_helper('standard', @_); }

sub ref_module_index_helper{
    my($word, $ahref) = @_;
    my $str = next_argument();
    $word = "$word " if $word;
    $str = "<tt class='module'>$str</tt> (${word}module)";
    # can't use add_index_entry() since the 2nd arg to gen_index_id() is used;
    # just inline it all here
    $str = gen_index_id($str, 'REF');
    $index{$str} .= $ahref;
    write_idxfile($ahref, $str);
}

# these should be adjusted a bit....
define_indexing_macro('refmodindex', 'refbimodindex',
		      'refexmodindex', 'refstmodindex');
sub idx_cmd_refmodindex{ return ref_module_index_helper('', @_); }
sub idx_cmd_refbimodindex{ return ref_module_index_helper('built-in', @_); }
sub idx_cmd_refexmodindex{ return ref_module_index_helper('extension', @_); }
sub idx_cmd_refstmodindex{ return ref_module_index_helper('standard', @_); }

sub do_cmd_nodename{ return do_cmd_label(@_); }

sub init_myformat{
    $anchor_invisible_mark = '&nbsp;';
    $anchor_invisible_mark2 = '';
    $anchor_mark = '';
    $icons{'anchor_mark'} = '';
}
init_myformat();

# Create an index entry, but include the string in the target anchor
# instead of the dummy filler.
#
sub make_str_index_entry{
    my($str) = @_;
    my($name,$aname,$ahref) = new_link_info();
    add_index_entry($str, $ahref);
    return "$aname$str</a>";
}

$REFCOUNTS_LOADED = 0;

sub load_refcounts{
    $REFCOUNTS_LOADED = 1;

    my $myname, $mydir, $myext;
    ($myname, $mydir, $myext) = fileparse(__FILE__, '\..*');
    chop $mydir;			# remove trailing '/'
    ($myname, $mydir, $myext) = fileparse($mydir, '\..*');
    chop $mydir;			# remove trailing '/'
    $mydir = getcwd() . "$dd$mydir"
      unless $mydir =~ s|^/|/|;
    local $_;
    my $filename = "$mydir${dd}api${dd}refcounts.dat";
    open(REFCOUNT_FILE, "<$filename") || die "\n$!\n";
    print "[loading API refcount data]";
    while (<REFCOUNT_FILE>) {
        if (/([a-zA-Z0-9_]+):PyObject\*:([a-zA-Z0-9_]*):(0|[-+]1|null):(.*)$/) {
            my($func, $param, $count, $comment) = ($1, $2, $3, $4);
            #print "\n$func($param) --> $count";
            $REFCOUNTS{"$func:$param"} = $count;
        }
    }
}

sub get_refcount{
    my ($func, $param) = @_;
    load_refcounts()
        unless $REFCOUNTS_LOADED;
    return $REFCOUNTS{"$func:$param"};
}

sub do_env_cfuncdesc{
    local($_) = @_;
    my $return_type = next_argument();
    my $function_name = next_argument();
    my $arg_list = next_argument();
    my $idx = make_str_index_entry(
        "<tt class='cfunction'>$function_name()</tt>" . get_indexsubitem());
    $idx =~ s/ \(.*\)//;
    $idx =~ s/\(\)//;		# ???? - why both of these?
    my $result_rc = get_refcount($function_name, '');
    my $rcinfo = '';
    if ($result_rc eq '+1') {
        $rcinfo = 'New reference';
    }
    elsif ($result_rc eq '0') {
        $rcinfo = 'Borrowed reference';
    }
    elsif ($result_rc eq 'null') {
        $rcinfo = 'Always <tt class="constant">NULL</tt>';
    }
    if ($rcinfo ne '') {
        $rcinfo = (  "\n<div class=\"refcount-info\">"
                   . "\n  <span class=\"label\">Return value:</span>"
                   . "\n  <span class=\"value\">$rcinfo.</span>"
                   . "\n</div>");
    }
    return "<dl><dt>$return_type <b>$idx</b>(<var>$arg_list</var>)\n<dd>"
           . $rcinfo
           . $_
           . '</dl>';
}

sub do_env_csimplemacrodesc{
    local($_) = @_;
    my $name = next_argument();
    my $idx = make_str_index_entry("<tt class='macro'>$name</tt>");
    return "<dl><dt><b>$idx</b>\n<dd>"
           . $_
           . '</dl>'
}

sub do_env_ctypedesc{
    local($_) = @_;
    my $index_name = next_optional_argument();
    my $type_name = next_argument();
    $index_name = $type_name
      unless $index_name;
    my($name,$aname,$ahref) = new_link_info();
    add_index_entry("<tt class='ctype'>$index_name</tt> (C type)", $ahref);
    return "<dl><dt><b><tt class='ctype'>$aname$type_name</a></tt></b>\n<dd>"
           . $_
           . '</dl>'
}

sub do_env_cvardesc{
    local($_) = @_;
    my $var_type = next_argument();
    my $var_name = next_argument();
    my $idx = make_str_index_entry("<tt class='cdata'>$var_name</tt>"
				   . get_indexsubitem());
    $idx =~ s/ \(.*\)//;
    return "<dl><dt>$var_type <b>$idx</b>\n"
           . '<dd>'
           . $_
           . '</dl>';
}

sub convert_args($){
    local($IN_DESC_HANDLER) = 1;
    local($_) = @_;
    return translate_commands($_);
}

sub do_env_funcdesc{
    local($_) = @_;
    my $function_name = next_argument();
    my $arg_list = convert_args(next_argument());
    my $idx = make_str_index_entry("<tt class='function'>$function_name()</tt>"
				   . get_indexsubitem());
    $idx =~ s/ \(.*\)//;
    $idx =~ s/\(\)<\/tt>/<\/tt>/;
    return "<dl><dt><b>$idx</b>(<var>$arg_list</var>)\n<dd>" . $_ . '</dl>';
}

sub do_env_funcdescni{
    local($_) = @_;
    my $function_name = next_argument();
    my $arg_list = convert_args(next_argument());
    return "<dl><dt><b><tt class='function'>$function_name</tt></b>"
      . "(<var>$arg_list</var>)\n"
      . '<dd>'
      . $_
      . '</dl>';
}

sub do_cmd_funcline{
    local($_) = @_;
    my $function_name = next_argument();
    my $arg_list = convert_args(next_argument());
    my $prefix = "<tt class='function'>$function_name()</tt>";
    my $idx = make_str_index_entry($prefix . get_indexsubitem());
    $prefix =~ s/\(\)//;

    return "<dt><b>$prefix</b>(<var>$arg_list</var>)\n<dd>" . $_;
}

sub do_cmd_funclineni{
    local($_) = @_;
    my $function_name = next_argument();
    my $arg_list = convert_args(next_argument());
    my $prefix = "<tt class='function'>$function_name</tt>";

    return "<dt><b>$prefix</b>(<var>$arg_list</var>)\n<dd>" . $_;
}

# Change this flag to index the opcode entries.  I don't think it's very
# useful to index them, since they're only presented to describe the dis
# module.
#
$INDEX_OPCODES = 0;

sub do_env_opcodedesc{
    local($_) = @_;
    my $opcode_name = next_argument();
    my $arg_list = next_argument();
    my $idx;
    if ($INDEX_OPCODES) {
	$idx = make_str_index_entry("<tt class='opcode'>$opcode_name</tt>"
                                    . " (byte code instruction)");
	$idx =~ s/ \(byte code instruction\)//;
    }
    else {
	$idx = "<tt class='opcode'>$opcode_name</tt>";
    }
    my $stuff = "<dl><dt><b>$idx</b>";
    if ($arg_list) {
	$stuff .= "&nbsp;&nbsp;&nbsp;&nbsp;<var>$arg_list</var>";
    }
    return $stuff . "\n<dd>" . $_ . '</dl>';
}

sub do_env_datadesc{
    local($_) = @_;
    my $dataname = next_argument();
    my $idx = make_str_index_entry("<tt>$dataname</tt>" . get_indexsubitem());
    $idx =~ s/ \(.*\)//;
    return "<dl><dt><b>$idx</b>\n<dd>"
           . $_
	   . '</dl>';
}

sub do_env_datadescni{
    local($_) = @_;
    my $idx = next_argument();
    if (! $STRING_INDEX_TT) {
	$idx = "<tt>$idx</tt>";
    }
    return "<dl><dt><b>$idx</b>\n<dd>" . $_ . '</dl>';
}

sub do_cmd_dataline{
    local($_) = @_;
    my $data_name = next_argument();
    my $idx = make_str_index_entry("<tt>$data_name</tt>" . get_indexsubitem());
    $idx =~ s/ \(.*\)//;
    return "<dt><b>$idx</b><dd>" . $_;
}

sub do_cmd_datalineni{
    local($_) = @_;
    my $data_name = next_argument();
    return "<dt><b><tt>$data_name</tt></b><dd>" . $_;
}

sub do_env_excdesc{
    local($_) = @_;
    my $excname = next_argument();
    my $idx = make_str_index_entry("<tt class='exception'>$excname</tt>");
    return "<dl><dt><b>exception $idx</b>\n<dd>" . $_ . '</dl>'
}

sub do_env_fulllineitems{ return do_env_itemize(@_); }


sub handle_classlike_descriptor{
    local($_, $what) = @_;
    $THIS_CLASS = next_argument();
    my $arg_list = convert_args(next_argument());
    $idx = make_str_index_entry(
		"<tt class='$what'>$THIS_CLASS</tt> ($what in $THIS_MODULE)" );
    $idx =~ s/ \(.*\)//;
    return ("<dl><dt><b>$what $idx</b>(<var>$arg_list</var>)\n<dd>"
            . $_
            . '</dl>');
}

sub do_env_classdesc{
    return handle_classlike_descriptor(@_[0], "class");
}

sub do_env_excclassdesc{
    return handle_classlike_descriptor(@_[0], "exception");
}


sub do_env_methoddesc{
    local($_) = @_;
    my $class_name = next_optional_argument();
    $class_name = $THIS_CLASS
        unless $class_name;
    my $method = next_argument();
    my $arg_list = convert_args(next_argument());
    my $extra = '';
    if ($class_name) {
	$extra = " ($class_name method)";
    }
    my $idx = make_str_index_entry("<tt class='method'>$method()</tt>$extra");
    $idx =~ s/ \(.*\)//;
    $idx =~ s/\(\)//;
    return "<dl><dt><b>$idx</b>(<var>$arg_list</var>)\n<dd>" . $_ . '</dl>';
}


sub do_cmd_methodline{
    local($_) = @_;
    my $class_name = next_optional_argument();
    $class_name = $THIS_CLASS
        unless $class_name;
    my $method = next_argument();
    my $arg_list = convert_args(next_argument());
    my $extra = '';
    if ($class_name) {
	$extra = " ($class_name method)";
    }
    my $idx = make_str_index_entry("<tt class='method'>$method()</tt>$extra");
    $idx =~ s/ \(.*\)//;
    $idx =~ s/\(\)//;
    return "<dt><b>$idx</b>(<var>$arg_list</var>)\n<dd>"
           . $_;
}


sub do_cmd_methodlineni{
    local($_) = @_;
    next_optional_argument();
    my $method = next_argument();
    my $arg_list = convert_args(next_argument());
    return "<dt><b>$method</b>(<var>$arg_list</var>)\n<dd>"
           . $_;
}

sub do_env_methoddescni{
    local($_) = @_;
    next_optional_argument();
    my $method = next_argument();
    my $arg_list = convert_args(next_argument());
    return "<dl><dt><b>$method</b>(<var>$arg_list</var>)\n<dd>"
           . $_
	   . '</dl>';
}


sub do_env_memberdesc{
    local($_) = @_;
    my $class = next_optional_argument();
    my $member = next_argument();
    $class = $THIS_CLASS
        unless $class;
    my $extra = '';
    $extra = " ($class attribute)"
        if ($class ne '');
    my $idx = make_str_index_entry("<tt class='member'>$member</tt>$extra");
    $idx =~ s/ \(.*\)//;
    $idx =~ s/\(\)//;
    return "<dl><dt><b>$idx</b>\n<dd>" . $_ . '</dl>';
}


sub do_cmd_memberline{
    local($_) = @_;
    my $class = next_optional_argument();
    my $member = next_argument();
    $class = $THIS_CLASS
        unless $class;
    my $extra = '';
    $extra = " ($class attribute)"
        if ($class ne '');
    my $idx = make_str_index_entry("<tt class='member'>$member</tt>$extra");
    $idx =~ s/ \(.*\)//;
    $idx =~ s/\(\)//;
    return "<dt><b>$idx</b><dd>" . $_;
}

sub do_env_memberdescni{
    local($_) = @_;
    next_optional_argument();
    my $member = next_argument();
    return "<dl><dt><b><tt class='member'>$member</tt></b>\n<dd>"
           . $_
           . '</dl>';
}


sub do_cmd_memberlineni{
    local($_) = @_;
    next_optional_argument();
    my $member = next_argument();
    return "<dt><b><tt class='member'>$member</tt></b><dd>" . $_;
}

@col_aligns = ('<td>', '<td>', '<td>', '<td>');

sub fix_font{
    # do a little magic on a font name to get the right behavior in the first
    # column of the output table
    my $font = @_[0];
    if ($font eq 'textrm') {
	$font = '';
    }
    elsif ($font eq 'file' || $font eq 'filenq') {
	$font = 'tt class="file"';
    }
    elsif ($font eq 'member') {
        $font = 'tt class="member"';
    }
    elsif ($font eq 'class') {
        $font = 'tt class="class"';
    }
    elsif ($font eq 'constant') {
        $font = 'tt class="constant"';
    }
    elsif ($font eq 'kbd') {
        $font = 'kbd';
    }
    elsif ($font eq 'programopt') {
        $font = 'b';
    }
    elsif ($font eq 'exception') {
        $font = 'tt class="exception"';
    }
    return $font;
}

sub figure_column_alignment{
    my $a = @_[0];
    my $mark = substr($a, 0, 1);
    my $r = '';
    if ($mark eq 'c')
      { $r = ' align="center"'; }
    elsif ($mark eq 'r')
      { $r = ' align="right"'; }
    elsif ($mark eq 'l')
      { $r = ' align="left"'; }
    elsif ($mark eq 'p')
      { $r = ' align="left"'; }
    return $r;
}

sub setup_column_alignments{
    local($_) = @_;
    my($s1,$s2,$s3,$s4) = split(/[|]/,$_);
    my $a1 = figure_column_alignment($s1);
    my $a2 = figure_column_alignment($s2);
    my $a3 = figure_column_alignment($s3);
    my $a4 = figure_column_alignment($s4);
    $col_aligns[0] = "<td$a1 valign=\"baseline\">";
    $col_aligns[1] = "<td$a2>";
    $col_aligns[2] = "<td$a3>";
    $col_aligns[3] = "<td$a4>";
    # return the aligned header start tags
    return ("<th$a1>", "<th$a2>", "<th$a3>", "<th$a4>");
}

sub get_table_col1_fonts{
    my $font = $globals{'lineifont'};
    my ($sfont,$efont) = ('', '');
    if ($font) {
        $sfont = "<$font>";
        $efont = "</$font>";
        $efont =~ s/ .*>/>/;
    }
    return ($sfont, $efont);
}

sub do_env_tableii{
    local($_) = @_;
    my($th1,$th2,$th3,$th4) = setup_column_alignments(next_argument());
    my $font = fix_font(next_argument());
    my $h1 = next_argument();
    my $h2 = next_argument();
    s/[\s\n]+//;
    $globals{'lineifont'} = $font;
    my $a1 = $col_aligns[0];
    my $a2 = $col_aligns[1];
    s/\\lineii</\\lineii[$a1|$a2]</g;
    return '<table border align="center" style="border-collapse: collapse">'
	   . "\n  <thead>"
	   . "\n    <tr class=\"tableheader\">"
	   . "\n      $th1<b>$h1</b>\&nbsp;</th>"
	   . "\n      $th2<b>$h2</b>\&nbsp;</th>"
	   . "\n      </tr>"
	   . "\n    </thead>"
	   . "\n  <tbody valign='baseline'>"
	   . $_
	   . "\n    </tbody>"
	   . "\n</table>";
}

sub do_env_longtableii{
    return do_env_tableii(@_);
}

sub do_cmd_lineii{
    local($_) = @_;
    my $aligns = next_optional_argument();
    my $c1 = next_argument();
    my $c2 = next_argument();
    s/[\s\n]+//;
    my($sfont,$efont) = get_table_col1_fonts();
    $c2 = '&nbsp;' if ($c2 eq '');
    my($c1align,$c2align) = split('\|', $aligns);
    my $padding = '';
    if ($c1align =~ /align="right"/ || $c1 eq '') {
        $padding = '&nbsp;';
    }
    return "\n    <tr>$c1align$sfont$c1$efont$padding</td>\n"
           . "        $c2align$c2</td>"
	   . $_;
}

sub do_env_tableiii{
    local($_) = @_;
    my($th1,$th2,$th3,$th4) = setup_column_alignments(next_argument());
    my $font = fix_font(next_argument());
    my $h1 = next_argument();
    my $h2 = next_argument();
    my $h3 = next_argument();
    s/[\s\n]+//;
    $globals{'lineifont'} = $font;
    my $a1 = $col_aligns[0];
    my $a2 = $col_aligns[1];
    my $a3 = $col_aligns[2];
    s/\\lineiii</\\lineiii[$a1|$a2|$a3]</g;
    return '<table border align="center" style="border-collapse: collapse">'
	   . "\n  <thead>"
	   . "\n    <tr class=\"tableheader\">"
	   . "\n      $th1<b>$h1</b>\&nbsp;</th>"
	   . "\n      $th2<b>$h2</b>\&nbsp;</th>"
	   . "\n      $th3<b>$h3</b>\&nbsp;</th>"
	   . "\n      </tr>"
	   . "\n    </thead>"
	   . "\n  <tbody valign='baseline'>"
	   . $_
	   . "\n    </tbody>"
	   . "\n</table>";
}

sub do_env_longtableiii{
    return do_env_tableiii(@_);
}

sub do_cmd_lineiii{
    local($_) = @_;
    my $aligns = next_optional_argument();
    my $c1 = next_argument();
    my $c2 = next_argument(); 
    my $c3 = next_argument();
    s/[\s\n]+//;
    my($sfont,$efont) = get_table_col1_fonts();
    $c3 = '&nbsp;' if ($c3 eq '');
    my($c1align,$c2align,$c3align) = split('\|', $aligns);
    my $padding = '';
    if ($c1align =~ /align="right"/ || $c1 eq '') {
        $padding = '&nbsp;';
    }
    return "\n    <tr>$c1align$sfont$c1$efont$padding</td>\n"
           . "        $c2align$c2</td>\n"
	   . "        $c3align$c3</td>"
	   . $_;
}

sub do_env_tableiv{
    local($_) = @_;
    my($th1,$th2,$th3,$th4) = setup_column_alignments(next_argument());
    my $font = fix_font(next_argument());
    my $h1 = next_argument();
    my $h2 = next_argument();
    my $h3 = next_argument();
    my $h4 = next_argument();
    s/[\s\n]+//;
    $globals{'lineifont'} = $font;
    my $a1 = $col_aligns[0];
    my $a2 = $col_aligns[1];
    my $a3 = $col_aligns[2];
    my $a4 = $col_aligns[3];
    s/\\lineiv</\\lineiv[$a1|$a2|$a3|$a4]</g;
    return '<table border align="center" style="border-collapse: collapse">'
	   . "\n  <thead>"
	   . "\n    <tr class=\"tableheader\">"
	   . "\n      $th1<b>$h1</b>\&nbsp;</th>"
	   . "\n      $th2<b>$h2</b>\&nbsp;</th>"
	   . "\n      $th3<b>$h3</b>\&nbsp;</th>"
	   . "\n      $th4<b>$h4</b>\&nbsp;</th>"
	   . "\n      </tr>"
	   . "\n    </thead>"
	   . "\n  <tbody valign='baseline'>"
	   . $_
	   . "\n    </tbody>"
	   . "\n</table>";
}

sub do_env_longtableiv{
    return do_env_tableiv(@_);
}

sub do_cmd_lineiv{
    local($_) = @_;
    my $aligns = next_optional_argument();
    my $c1 = next_argument();
    my $c2 = next_argument(); 
    my $c3 = next_argument();
    my $c4 = next_argument();
    s/[\s\n]+//;
    my($sfont,$efont) = get_table_col1_fonts();
    $c4 = '&nbsp;' if ($c4 eq '');
    my($c1align,$c2align,$c3align,$c4align) = split('\|', $aligns);
    my $padding = '';
    if ($c1align =~ /align="right"/ || $c1 eq '') {
        $padding = '&nbsp;';
    }
    return "\n    <tr>$c1align$sfont$c1$efont$padding</td>\n"
           . "        $c2align$c2</td>\n"
	   . "        $c3align$c3</td>\n"
	   . "        $c4align$c4</td>"
	   . $_;
}


# These can be used to control the title page appearance;
# they need a little bit of documentation.
#
# If $TITLE_PAGE_GRAPHIC is set, it should be the name of a file in the
# $ICONSERVER directory, or include path information (other than "./").  The
# default image type will be assumed if an extension is not provided.
#
# If specified, the "title page" will contain two colums: one containing the
# title/author/etc., and the other containing the graphic.  Use the other
# four variables listed here to control specific details of the layout; all
# are optional.
#
# $TITLE_PAGE_GRAPHIC = "my-company-logo";
# $TITLE_PAGE_GRAPHIC_COLWIDTH = "30%";
# $TITLE_PAGE_GRAPHIC_WIDTH = 150;
# $TITLE_PAGE_GRAPHIC_HEIGHT = 150;
# $TITLE_PAGE_GRAPHIC_ON_RIGHT = 0;

sub make_my_titlepage() {
    my $the_title = "";
    if ($t_title) {
	$the_title .= "\n<h1>$t_title</h1>";
    }
    else {
        write_warnings("\nThis document has no title.");
    }
    if ($t_author) {
	if ($t_authorURL) {
	    my $href = translate_commands($t_authorURL);
	    $href = make_named_href('author', $href,
				    "<b><font size='+2'>$t_author</font></b>");
	    $the_title .= "\n<p>$href</p>";
	}
        else {
	    $the_title .= ("\n<p><b><font size='+2'>$t_author</font></b></p>");
	}
    }
    else {
        write_warnings("\nThere is no author for this document.");
    }
    if ($t_institute) {
        $the_title .= "\n<p>$t_institute</p>";
    }
    if ($DEVELOPER_ADDRESS) {
        $the_title .= "\n<p>$DEVELOPER_ADDRESS</p>";
    }
    if ($t_affil) {
	$the_title .= "\n<p><i>$t_affil</i></p>";
    }
    if ($t_date) {
	$the_title .= "\n<p>";
	if ($PACKAGE_VERSION) {
	    $the_title .= "<strong>Release $PACKAGE_VERSION</strong><br>\n";
        }
	$the_title .= "<strong>$t_date</strong></p>"
    }
    if ($t_address) {
	$the_title .= "\n<p>$t_address</p>";
    }
    else {
        $the_title .= "\n<p>";
    }
    if ($t_email) {
	$the_title .= "\n<p>$t_email</p>";
    }
    return $the_title;
}

sub make_my_titlegraphic() {
    my $filename = make_icon_filename($TITLE_PAGE_GRAPHIC);
    my $graphic = "<td class=\"titlegraphic\"";
    $graphic .= " width=\"$TITLE_PAGE_GRAPHIC_COLWIDTH\""
      if ($TITLE_PAGE_GRAPHIC_COLWIDTH);
    $graphic .= "><img";
    $graphic .= " width=\"$TITLE_PAGE_GRAPHIC_WIDTH\""
      if ($TITLE_PAGE_GRAPHIC_WIDTH);
    $graphic .= " height=\"$TITLE_PAGE_GRAPHIC_HEIGHT\""
      if ($TITLE_PAGE_GRAPHIC_HEIGHT);
    $graphic .= "\n  src=\"$filename\"></td>\n";
    return $graphic;
}

sub do_cmd_maketitle {
    local($_) = @_;
    my $the_title = "\n<div class=\"titlepage\">";
    if ($TITLE_PAGE_GRAPHIC) {
        if ($TITLE_PAGE_GRAPHIC_ON_RIGHT) {
            $the_title .= ("\n<table border=\"0\" width=\"100%\">"
                           . "<tr align=\"right\">\n<td>"
                           . make_my_titlepage()
                           . "</td>\n"
                           . make_my_titlegraphic()
                           . "</tr>\n</table>");
        }
        else {
            $the_title .= ("\n<table border=\"0\" width=\"100%\"><tr>\n"
                           . make_my_titlegraphic()
                           . "<td>"
                           . make_my_titlepage()
                           . "</td></tr>\n</table>");
        }
    }
    else {
        $the_title .= ("\n<center>"
                       . make_my_titlepage()
                       . "\n</center>");
    }
    $the_title .= "\n</div>";
    return $the_title . $_;
    $the_title .= "\n</center></div>";
    return $the_title . $_ ;
}


#
#  Module synopsis support
#

require SynopsisTable;

sub get_chapter_id(){
    my $id = do_cmd_thechapter('');
    $id =~ s/<SPAN CLASS="arabic">(\d+)<\/SPAN>/\1/;
    $id =~ s/\.//;
    return $id;
}

# 'chapter' => 'SynopsisTable instance'
%ModuleSynopses = ();

sub get_synopsis_table($){
    my($chap) = @_;
    my $key;
    foreach $key (keys %ModuleSynopses) {
	if ($key eq $chap) {
	    return $ModuleSynopses{$chap};
	}
    }
    my $st = SynopsisTable->new();
    $ModuleSynopses{$chap} = $st;
    return $st;
}

sub do_cmd_moduleauthor{
    local($_) = @_;
    next_argument();
    next_argument();
    return $_;
}

sub do_cmd_sectionauthor{
    local($_) = @_;
    next_argument();
    next_argument();
    return $_;
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
    return anchor_label("module-$key",$CURRENT_FILE,$_)
}

sub do_cmd_modulesynopsis{
    local($_) = @_;
    my $st = get_synopsis_table(get_chapter_id());
    $st->set_synopsis($THIS_MODULE, translate_commands(next_argument()));
    return $_;
}

sub do_cmd_localmoduletable{
    local($_) = @_;
    my $chap = get_chapter_id();
    my $st = get_synopsis_table($chap);
    $st->set_file("$CURRENT_FILE");
    return "<tex2html-localmoduletable><$chap>\\tableofchildlinks[off]" . $_;
}

sub process_all_localmoduletables{
    my $key;
    my $st, $file;
    foreach $key (keys %ModuleSynopses) {
        $st = $ModuleSynopses{$key};
        $file = $st->get_file();
        if ($file) {
            process_localmoduletables_in_file($file);
        }
        else {
            print "\nsynopsis table $key has no file association";
        }
    }
}

sub process_localmoduletables_in_file{
    my $file = @_[0];
    open(MYFILE, "<$file");
    local($_);
    sysread(MYFILE, $_, 1024*1024);
    close(MYFILE);
    # need to get contents of file in $_
    while (/<tex2html-localmoduletable><(\d+)>/) {
	my $match = $&;
	my $chap = $1;
	my $st = get_synopsis_table($chap);
	my $data = $st->tohtml();
	s/$match/$data/;
    }
    open(MYFILE,">$file");
    print MYFILE $_;
    close(MYFILE);
}
sub process_python_state{
    process_all_localmoduletables();
}


#
#  "See also:" -- references placed at the end of a \section
#

sub do_env_seealso{
    return "<div class='seealso'>\n  "
      . "<p class='heading'><b>See Also:</b></p>\n"
      . @_[0]
      . '</div>';
}

sub do_cmd_seemodule{
    # Insert the right magic to jump to the module definition.  This should
    # work most of the time, at least for repeat builds....
    local($_) = @_;
    my $key = next_optional_argument();
    my $module = next_argument();
    my $text = next_argument();
    my $period = '.';
    $key = $module
        unless $key;
    if ($text =~ /\.$/) {
	$period = '';
    }
    return '<dl compact class="seemodule">'
      . "\n    <dt>Module <b><tt class='module'><a href='module-$key.html'>"
      . "$module</a></tt>:</b>"
      . "\n    <dd>$text$period\n  </dl>"
      . $_;
}

sub strip_html_markup($){
    my $str = @_[0];
    my $s = "$str";
    $s =~ s/<[a-zA-Z0-9]+(\s+[a-zA-Z0-9]+(\s*=\s*(\'[^\']*\'|\"[^\"]*\"|[a-zA-Z0-9]+))?)*\s*>//g;
    $s =~ s/<\/[a-zA-Z0-9]+>//g;
    return $s;
}

sub handle_rfclike_reference{
    local($_, $what, $format) = @_;
    my $rfcnum = next_argument();
    my $title = next_argument();
    my $text = next_argument();
    my $url = get_rfc_url($rfcnum, $format);
    my $icon = get_link_icon($url);
    my $attrtitle = strip_html_markup($title);
    return '<dl compact class="seerfc">'
      . "\n    <dt><a href=\"$url\""
      . "\n        title=\"$attrtitle\""
      . "\n        >$what $rfcnum, <em>$title</em>$icon</a>"
      . "\n    <dd>$text\n  </dl>"
      . $_;
}

sub do_cmd_seepep{
    return handle_rfclike_reference(@_[0], "PEP", $PEP_FORMAT);
}

sub do_cmd_seerfc{
    return handle_rfclike_reference(@_[0], "RFC", $RFC_FORMAT);
}

sub do_cmd_seetitle{
    local($_) = @_;
    my $url = next_optional_argument();
    my $title = next_argument();
    my $text = next_argument();
    if ($url) {
        my $icon = get_link_icon($url);
        return '<dl compact class="seetitle">'
          . "\n    <dt><em class=\"citetitle\"><a href=\"$url\""
          . "\n        >$title$icon</a></em>"
          . "\n    <dd>$text\n  </dl>"
          . $_;
    }
    return '<dl compact class="seetitle">'
      . "\n    <dt><em class=\"citetitle\""
      . "\n        >$title</em>"
      . "\n    <dd>$text\n  </dl>"
      . $_;
}

sub do_cmd_seeurl{
    local($_) = @_;
    my $url = next_argument();
    my $text = next_argument();
    my $icon = get_link_icon($url);
    return '<dl compact class="seeurl">'
      . "\n    <dt><a href=\"$url\""
      . "\n        class=\"url\">$url$icon</a>"
      . "\n    <dd>$text\n  </dl>"
      . $_;
}

sub do_cmd_seetext{
    local($_) = @_;
    my $content = next_argument();
    return '<div class="seetext"><p>' . $content . '</div>' . $_;
}


#
#  Definition list support.
#

sub do_env_definitions{
    return "<dl class='definitions'>" . @_[0] . "</dl>\n";
}

sub do_cmd_term{
    local($_) = @_;
    my $term = next_argument();
    my($name,$aname,$ahref) = new_link_info();
    # could easily add an index entry here...
    return "<dt><b>$aname" . $term . "</a></b>\n<dd>" . $_;
}


# I don't recall exactly why this was needed, but it was very much needed.
# We'll see if anything breaks when I move the "code" line out -- some
# things broke with it in.

#code # {}
process_commands_wrap_deferred(<<_RAW_ARG_DEFERRED_CMDS_);
declaremodule # [] # {} # {}
memberline # [] # {}
methodline # [] # {} # {}
modulesynopsis # {}
platform # {}
samp # {}
setindexsubitem # {}
withsubitem # {} # {}
_RAW_ARG_DEFERRED_CMDS_


$alltt_start = '<dl><dd><pre class="verbatim">';
$alltt_end = '</pre></dl>';

sub do_env_alltt {
    local ($_) = @_;
    local($closures,$reopens,@open_block_tags);

    # get the tag-strings for all open tags
    local(@keep_open_tags) = @$open_tags_R;
    ($closures,$reopens) = &preserve_open_tags() if (@$open_tags_R);

    # get the tags for text-level tags only
    $open_tags_R = [ @keep_open_tags ];
    local($local_closures, $local_reopens);
    ($local_closures, $local_reopens,@open_block_tags)
      = &preserve_open_block_tags
	if (@$open_tags_R);

    $open_tags_R = [ @open_block_tags ];

    do {
	local($open_tags_R) = [ @open_block_tags ];
	local(@save_open_tags) = ();

	local($cnt) = ++$global{'max_id'};
	$_ = join('',"$O$cnt$C\\tt$O", ++$global{'max_id'}, $C
		, $_ , $O, $global{'max_id'}, "$C$O$cnt$C");

	$_ = &translate_environments($_);
	$_ = &translate_commands($_) if (/\\/);

	# preserve space-runs, using &nbsp;
	while (s/(\S) ( +)/$1$2;SPMnbsp;/g){};
	s/(<BR>) /$1;SPMnbsp;/g;

	$_ = join('', $closures, $alltt_start, $local_reopens
		, $_
		, &balance_tags() #, $local_closures
		, $alltt_end, $reopens);
	undef $open_tags_R; undef @save_open_tags;
    };
    $open_tags_R = [ @keep_open_tags ];
    $_;
}


1;				# This must be the last line
