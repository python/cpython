# LaTeX2HTML support base for use with Python documentation.

package main;

use L2hos;

$HTML_VERSION = 4.0;

$MAX_LINK_DEPTH = 2;
$ADDRESS = '';

$NO_FOOTNODE = 1;
$NUMBERED_FOOTNOTES = 1;

# Python documentation uses section numbers to support references to match
# in the printed and online versions.
#
$SHOW_SECTION_NUMBERS = 1;

$ICONSERVER = '.';
$IMAGE_TYPE = 'gif';

# Control where the navigation bars should show up:
$TOP_NAVIGATION = 1;
$BOTTOM_NAVIGATION = 1;
$AUTO_NAVIGATION = 0;

$BODYTEXT = '';
$CHILDLINE = "\n<p><br /></p><hr class='online-navigation' />\n";
$VERBOSITY = 0;

# default # of columns for the indexes
$INDEX_COLUMNS = 2;
$MODULE_INDEX_COLUMNS = 4;

$HAVE_MODULE_INDEX = 0;
$HAVE_GENERAL_INDEX = 0;
$HAVE_TABLE_OF_CONTENTS = 0;

$AESOP_META_TYPE = 'information';


# A little painful, but lets us clean up the top level directory a little,
# and not be tied to the current directory (as far as I can tell).  Testing
# an existing definition of $mydir is needed since it cannot be computed when
# run under mkhowto with recent versions of LaTeX2HTML, since this file is
# not read directly by LaTeX2HTML any more.  mkhowto is required to prepend
# the required definition at the top of the actual input file.
#
if (!defined $mydir) {
    use Cwd;
    use File::Basename;
    ($myname, $mydir, $myext) = fileparse(__FILE__, '\..*');
    chop $mydir;                        # remove trailing '/'
    $mydir = getcwd() . "$dd$mydir"
        unless $mydir =~ s|^/|/|;
}
$LATEX2HTMLSTYLES = "$mydir$envkey$LATEX2HTMLSTYLES";
push (@INC, $mydir);

($myrootname, $myrootdir, $myext) = fileparse($mydir, '\..*');
chop $myrootdir;


# Hackish way to get the appropriate paper-*/ directory into $TEXINPUTS;
# pass in the paper size (a4 or letter) as the environment variable PAPER
# to add the right directory.  If not given, the current directory is
# added instead for use with HOWTO processing.
#
if (defined $ENV{'PAPER'}) {
    $mytexinputs = "$myrootdir${dd}paper-$ENV{'PAPER'}$envkey";
}
else {
    $mytexinputs = getcwd() . $envkey;
}
$mytexinputs .= "$myrootdir${dd}texinputs";


# Change this variable to change the text added in "About this document...";
# this should be an absolute pathname to get it right.
#
$ABOUT_FILE = "$myrootdir${dd}html${dd}stdabout.dat";


sub custom_driver_hook {
    #
    # This adds the directory of the main input file to $TEXINPUTS; it
    # seems to be sufficiently general that it should be fine for HOWTO
    # processing.
    #
    # XXX This still isn't quite right; we should actually be inserting
    # $mytexinputs just before any empty entry in TEXINPUTS is one
    # exists instead of just concatenating the pieces like we do here.
    #
    my $file = $_[0];
    my($jobname, $dir, $ext) = fileparse($file, '\..*');
    $dir = L2hos->Make_directory_absolute($dir);
    $dir =~ s/$dd$//;
    $TEXINPUTS = "$dir$envkey$mytexinputs";
    # Push everything into $TEXINPUTS since LaTeX2HTML doesn't pick
    # this up on it's own; we clear $ENV{'TEXINPUTS'} so the value set
    # for this by the main LaTeX2HTML script doesn't contain duplicate
    # directories.
    if ($ENV{'TEXINPUTS'}) {
        $TEXINPUTS .= "$envkey$ENV{'TEXINPUTS'}";
        $ENV{'TEXINPUTS'} = undef;
    }
    print "\nSetting \$TEXINPUTS to $TEXINPUTS\n";
}


# $CUSTOM_BUTTONS is only used for the module index link.
$CUSTOM_BUTTONS = '';

sub make_nav_sectref($$$) {
    my($label, $linktype, $title) = @_;
    if ($title) {
        if ($title =~ /\<[aA] /) {
            $title =~ s/\<[aA] /<a class="sectref" rel="$linktype" /;
            $title =~ s/ HREF=/ href=/;
        }
        else {
            $title = "<span class=\"sectref\">$title</span>";
        }
        return "<b class=\"navlabel\">$label:</b>\n$title\n";
    }
    return '';
}

@my_icon_tags = ();
$my_icon_tags{'next'} = 'Next Page';
$my_icon_tags{'next_page'} = 'Next Page';
$my_icon_tags{'previous'} = 'Previous Page';
$my_icon_tags{'previous_page'} = 'Previous Page';
$my_icon_tags{'up'} = 'Up One Level';
$my_icon_tags{'contents'} = 'Contents';
$my_icon_tags{'index'} = 'Index';
$my_icon_tags{'modules'} = 'Module Index';

@my_icon_names = ();
$my_icon_names{'previous_page'} = 'previous';
$my_icon_names{'next_page'} = 'next';

sub get_my_icon($) {
    my $name = $_[0];
    my $text = $my_icon_tags{$name};
    if ($my_icon_names{$name}) {
        $name = $my_icon_names{$name};
    }
    if ($text eq '') {
        $name = 'blank';
    }
    my $iconserver = ($ICONSERVER eq '.') ? '' : "$ICONSERVER/";
    return "<img src='$iconserver$name.$IMAGE_TYPE'\n  border='0'"
           . " height='32'  alt='$text' width='32' />";
}

sub unlinkify($) {
    my $text = "$_[0]";
    $text =~ s|</[aA]>||;
    $text =~ s|<a\s+[^>]*>||i;
    return $text;
}

sub use_icon($$$) {
    my($rel,$str,$title) = @_;
    if ($str) {
        my $s = "$str";
        if ($s =~ /\<tex2html_([a-z_]+)_visible_mark\>/) {
            my $r = get_my_icon($1);
            $s =~ s/\<tex2html_[a-z_]+_visible_mark\>/$r/;
        }
        $s =~ s/<[aA] /<a rel="$rel" title="$title"\n  /;
        $s =~ s/ HREF=/ href=/;
        return $s;
    }
    else {
        return get_my_icon('blank');
    }
}

sub make_nav_panel() {
    my $s;
    # new iconic         rel         iconic     page title
    my $next     = use_icon('next',     $NEXT,     unlinkify($NEXT_TITLE));
    my $up       = use_icon('parent',   $UP,       unlinkify($UP_TITLE));
    my $previous = use_icon('prev',     $PREVIOUS, unlinkify($PREVIOUS_TITLE));
    my $contents = use_icon('contents', $CONTENTS, 'Table of Contents');
    my $index    = use_icon('index',    $INDEX,    'Index');
    if (!$CUSTOM_BUTTONS) {
        $CUSTOM_BUTTONS = get_my_icon('blank');
    }
    $s = ('<table align="center" width="100%" cellpadding="0" cellspacing="2">'
          . "\n<tr>"
          # left-hand side
          . "\n<td class='online-navigation'>$previous</td>"
          . "\n<td class='online-navigation'>$up</td>"
          . "\n<td class='online-navigation'>$next</td>"
          # title box
          . "\n<td align=\"center\" width=\"100%\">$t_title</td>"
          # right-hand side
          . "\n<td class='online-navigation'>$contents</td>"
          # module index
          . "\n<td class='online-navigation'>$CUSTOM_BUTTONS</td>"
          . "\n<td class='online-navigation'>$index</td>"
          . "\n</tr></table>\n"
          # textual navigation
          . "<div class='online-navigation'>\n"
          . make_nav_sectref("Previous", "prev", $PREVIOUS_TITLE)
          . make_nav_sectref("Up", "parent", $UP_TITLE)
          . make_nav_sectref("Next", "next", $NEXT_TITLE)
          . "</div>\n"
          );
    # remove these; they are unnecessary and cause errors from validation
    $s =~ s/ NAME="tex2html\d+"\n */ /g;
    return $s;
}

sub add_child_links {
    my $toc = add_real_child_links(@_);
    $toc =~ s|\s*</[aA]>|</a>|g;
    $toc =~ s/ NAME=\"tex2html\d+\"\s*href=/ href=/gi;
    $toc =~ s|</UL>(\s*<BR( /)?>)?|</ul>|gi;
    if ($toc =~ / NAME=["']CHILD_LINKS["']/) {
        return "<div class='online-navigation'>\n$toc</div>\n";
    }
    return $toc;
}

sub get_version_text() {
    if ($PACKAGE_VERSION ne '' && $t_date) {
        return ("<span class=\"release-info\">"
                . "Release $PACKAGE_VERSION$RELEASE_INFO,"
                . " documentation updated on $t_date.</span>");
    }
    if ($PACKAGE_VERSION ne '') {
        return ("<span class=\"release-info\">"
                . "Release $PACKAGE_VERSION$RELEASE_INFO.</span>");
    }
    if ($t_date) {
        return ("<span class=\"release-info\">Documentation released on "
                . "$t_date.</span>");
    }
    return '';
}


sub top_navigation_panel() {
    return "\n<div id='top-navigation-panel' xml:id='top-navigation-panel'>\n"
           . make_nav_panel()
           . "<hr /></div>\n";
}

sub bot_navigation_panel() {
    return "\n<div class='online-navigation'>\n"
           . "<p></p><hr />\n"
           . make_nav_panel()
           . "</div>\n"
           . "<hr />\n"
           . get_version_text()
           . "\n";
}

sub add_link {
    # Returns a pair (iconic link, textual link)
    my($icon, $current_file, @link) = @_;
    my($dummy, $file, $title) = split($delim,
                                      $section_info{join(' ',@link)});
    if ($icon =~ /\<tex2html_([_a-z]+)_visible_mark\>/) {
        my $r = get_my_icon($1);
        $icon =~ s/\<tex2html_[_a-z]+_visible_mark\>/$r/;
    }
    if ($title && ($file ne $current_file)) {
        $title = purify($title);
        $title = get_first_words($title, $WORDS_IN_NAVIGATION_PANEL_TITLES);
        return (make_href($file, $icon), make_href($file, "$title"))
        }
    elsif ($icon eq get_my_icon('up') && $EXTERNAL_UP_LINK) {
        return (make_href($EXTERNAL_UP_LINK, $icon),
                make_href($EXTERNAL_UP_LINK, "$EXTERNAL_UP_TITLE"))
        }
    elsif ($icon eq get_my_icon('previous')
           && $EXTERNAL_PREV_LINK && $EXTERNAL_PREV_TITLE) {
        return (make_href($EXTERNAL_PREV_LINK, $icon),
                make_href($EXTERNAL_PREV_LINK, "$EXTERNAL_PREV_TITLE"))
        }
    elsif ($icon eq get_my_icon('next')
           && $EXTERNAL_DOWN_LINK && $EXTERNAL_DOWN_TITLE) {
        return (make_href($EXTERNAL_DOWN_LINK, $icon),
                make_href($EXTERNAL_DOWN_LINK, "$EXTERNAL_DOWN_TITLE"))
        }
    return (&inactive_img($icon), "");
}

sub add_special_link($$$) {
    my($icon, $file, $current_file) = @_;
    if ($icon =~ /\<tex2html_([_a-z]+)_visible_mark\>/) {
        my $r = get_my_icon($1);
        $icon =~ s/\<tex2html_[_a-z]+_visible_mark\>/$r/;
    }
    return (($file && ($file ne $current_file))
            ? make_href($file, $icon)
            : undef)
}

# The img_tag() function seems only to be called with the parameter
# 'anchor_invisible_mark', which we want to turn into ''.  Since
# replace_icon_marks() is the only interesting caller, and all it really
# does is call img_tag(), we can just define the hook alternative to be
# a no-op instead.
#
sub replace_icons_hook {}

sub do_cmd_arabic {
    # get rid of that nasty <SPAN CLASS="arabic">...</SPAN>
    my($ctr, $val, $id, $text) = &read_counter_value($_[0]);
    return ($val ? farabic($val) : "0") . $text;
}


sub gen_index_id($$) {
    # this is used to ensure common index key generation and a stable sort
    my($str, $extra) = @_;
    sprintf('%s###%s%010d', $str, $extra, ++$global{'max_id'});
}

sub insert_index($$$$$) {
    my($mark, $datafile, $columns, $letters, $prefix) = @_;
    my $prog = "$myrootdir/tools/buildindex.py";
    my $index;
    if ($letters) {
        $index = `$prog --columns $columns --letters $datafile`;
    }
    else {
        $index = `$prog --columns $columns $datafile`;
    }
    if (!s/$mark/$prefix$index/) {
        print "\nCould not locate index mark: $mark";
    }
}

sub add_idx() {
    print "\nBuilding HTML for the index ...";
    close(IDXFILE);
    insert_index($idx_mark, 'index.dat', $INDEX_COLUMNS, 1, '');
}


$idx_module_mark = '<tex2html_idx_module_mark>';
$idx_module_title = 'Module Index';

sub add_module_idx() {
    print "\nBuilding HTML for the module index ...";
    my $key;
    my $first = 1;
    my $prevplat = '';
    my $allthesame = 1;
    my $prefix = '';
    foreach $key (keys %Modules) {
        $key =~ s/<tt>([a-zA-Z0-9._]*)<\/tt>/$1/;
        my $plat = "$ModulePlatforms{$key}";
        $plat = ''
          if ($plat eq $IGNORE_PLATFORM_ANNOTATION);
        if (!$first) {
            $allthesame = 0
              if ($prevplat ne $plat);
        }
        else { $first = 0; }
        $prevplat = $plat;
    }
    open(MODIDXFILE, '>modindex.dat') || die "\n$!\n";
    foreach $key (keys %Modules) {
        # dump the line in the data file; just use a dummy seqno field
        my $nkey = $1;
        my $moditem = "$Modules{$key}";
        my $plat = '';
        $key =~ s/<tt>([a-zA-Z0-9._]*)<\/tt>/$1/;
        if ($ModulePlatforms{$key} && !$allthesame) {
            $plat = (" <em>(<span class=\"platform\">$ModulePlatforms{$key}"
                     . '</span>)</em>');
        }
        print MODIDXFILE $moditem . $IDXFILE_FIELD_SEP
              . "<tt class=\"module\">$key</tt>$plat###\n";
    }
    close(MODIDXFILE);

    if ($GLOBAL_MODULE_INDEX) {
        $prefix = <<MODULE_INDEX_PREFIX;

<p> This index only lists modules documented in this manual.
  The <em class="citetitle"><a href="$GLOBAL_MODULE_INDEX">Global Module
     Index</a></em> lists all modules that are documented in this set
  of manuals.</p>
MODULE_INDEX_PREFIX
    }
    if (!$allthesame) {
        $prefix .= <<PLAT_DISCUSS;

<p> Some module names are followed by an annotation indicating what
platform they are available on.</p>

PLAT_DISCUSS
    }
    insert_index($idx_module_mark, 'modindex.dat', $MODULE_INDEX_COLUMNS, 0,
                 $prefix);
}

# replace both indexes as needed:
sub add_idx_hook {
    add_idx() if (/$idx_mark/);
    process_python_state();
    if ($MODULE_INDEX_FILE) {
        local ($_);
        open(MYFILE, "<$MODULE_INDEX_FILE");
        sysread(MYFILE, $_, 1024*1024);
        close(MYFILE);
        add_module_idx();
        open(MYFILE,">$MODULE_INDEX_FILE");
        print MYFILE $_;
        close(MYFILE);
    }
}


# In addition to the standard stuff, add label to allow named node files and
# support suppression of the page complete (for HTML Help use).
$MY_CONTENTS_PAGE = '';
sub do_cmd_tableofcontents {
    local($_) = @_;
    $TITLE = $toc_title;
    $tocfile = $CURRENT_FILE;
    my($closures, $reopens) = preserve_open_tags();
    anchor_label('contents', $CURRENT_FILE, $_);        # this is added
    $MY_CONTENTS_PAGE = "$CURRENT_FILE";
    join('', "\\tableofchildlinks[off]", $closures
         , make_section_heading($toc_title, 'h2'), $toc_mark
         , $reopens, $_);
}
# In addition to the standard stuff, add label to allow named node files.
sub do_cmd_listoffigures {
    local($_) = @_;
    $TITLE = $lof_title;
    $loffile = $CURRENT_FILE;
    my($closures, $reopens) = preserve_open_tags();
    anchor_label('lof', $CURRENT_FILE, $_);             # this is added
    join('', "<br />\n", $closures
         , make_section_heading($lof_title, 'h2'), $lof_mark
         , $reopens, $_);
}
# In addition to the standard stuff, add label to allow named node files.
sub do_cmd_listoftables {
    local($_) = @_;
    $TITLE = $lot_title;
    $lotfile = $CURRENT_FILE;
    my($closures, $reopens) = preserve_open_tags();
    anchor_label('lot', $CURRENT_FILE, $_);             # this is added
    join('', "<br />\n", $closures
         , make_section_heading($lot_title, 'h2'), $lot_mark
         , $reopens, $_);
}
# In addition to the standard stuff, add label to allow named node files.
sub do_cmd_textohtmlinfopage {
    local($_) = @_;
    if ($INFO) {                                        #
        anchor_label("about",$CURRENT_FILE,$_);         # this is added
    }                                                   #
    my $the_version = '';                               # and the rest is
    if ($t_date) {                                      # mostly ours
        $the_version = ",\n$t_date";
        if ($PACKAGE_VERSION) {
            $the_version .= ", Release $PACKAGE_VERSION$RELEASE_INFO";
        }
    }
    my $about;
    open(ABOUT, "<$ABOUT_FILE") || die "\n$!\n";
    sysread(ABOUT, $about, 1024*1024);
    close(ABOUT);
    $_ = (($INFO == 1)
          ? join('',
                 $close_all,
                 "<strong>$t_title</strong>$the_version\n",
                 $about,
                 $open_all, $_)
          : join('', $close_all, $INFO,"\n", $open_all, $_));
    $_;
}

$GENERAL_INDEX_FILE = '';
$MODULE_INDEX_FILE = '';

# $idx_mark will be replaced with the real index at the end
sub do_cmd_textohtmlindex {
    local($_) = @_;
    $TITLE = $idx_title;
    $idxfile = $CURRENT_FILE;
    $GENERAL_INDEX_FILE = "$CURRENT_FILE";
    if (%index_labels) { make_index_labels(); }
    if (($SHORT_INDEX) && (%index_segment)) { make_preindex(); }
    else { $preindex = ''; }
    my $heading = make_section_heading($idx_title, 'h2') . $idx_mark;
    my($pre, $post) = minimize_open_tags($heading);
    anchor_label('genindex',$CURRENT_FILE,$_);          # this is added
    return "<br />\n" . $pre . $_;
}

# $idx_module_mark will be replaced with the real index at the end
sub do_cmd_textohtmlmoduleindex {
    local($_) = @_;
    $TITLE = $idx_module_title;
    anchor_label('modindex', $CURRENT_FILE, $_);
    $MODULE_INDEX_FILE = "$CURRENT_FILE";
    $_ = ('<p></p>' . make_section_heading($idx_module_title, 'h2')
          . $idx_module_mark . $_);
    return $_;
}

# The bibliography and the index should be treated as separate
# sections in their own HTML files. The \bibliography{} command acts
# as a sectioning command that has the desired effect. But when the
# bibliography is constructed manually using the thebibliography
# environment, or when using the theindex environment it is not
# possible to use the normal sectioning mechanism. This subroutine
# inserts a \bibliography{} or a dummy \textohtmlindex command just
# before the appropriate environments to force sectioning.

# XXX   This *assumes* that if there are two {theindex} environments,
#       the first is the module index and the second is the standard
#       index.  This is sufficient for the current Python documentation,
#       but that's about it.

sub add_bbl_and_idx_dummy_commands {
    my $id = $global{'max_id'};

    if (/[\\]tableofcontents/) {
        $HAVE_TABLE_OF_CONTENTS = 1;
    }
    s/([\\]begin\s*$O\d+$C\s*thebibliography)/$bbl_cnt++; $1/eg;
    s/([\\]begin\s*$O\d+$C\s*thebibliography)/$id++; "\\bibliography$O$id$C$O$id$C $1"/geo;
    my(@parts) = split(/\\begin\s*$O\d+$C\s*theindex/);
    if (scalar(@parts) == 3) {
        # Be careful to re-write the string in place, since $_ is *not*
        # returned explicity;  *** nasty side-effect dependency! ***
        print "\nadd_bbl_and_idx_dummy_commands ==> adding general index";
        print "\nadd_bbl_and_idx_dummy_commands ==> adding module index";
        my $rx = "([\\\\]begin\\s*$O\\d+$C\\s*theindex[\\s\\S]*)"
          . "([\\\\]begin\\s*$O\\d+$C\\s*theindex)";
        s/$rx/\\textohtmlmoduleindex $1 \\textohtmlindex $2/o;
        # Add a button to the navigation areas:
        $CUSTOM_BUTTONS .= ('<a href="modindex.html" title="Module Index">'
                            . get_my_icon('modules')
                            . '</a>');
        $HAVE_MODULE_INDEX = 1;
        $HAVE_GENERAL_INDEX = 1;
    }
    elsif (scalar(@parts) == 2) {
        print "\nadd_bbl_and_idx_dummy_commands ==> adding general index";
        my $rx = "([\\\\]begin\\s*$O\\d+$C\\s*theindex)";
        s/$rx/\\textohtmlindex $1/o;
        $HAVE_GENERAL_INDEX = 1;
    }
    elsif (scalar(@parts) == 1) {
        print "\nadd_bbl_and_idx_dummy_commands ==> no index found";
        $CUSTOM_BUTTONS .= get_my_icon('blank');
        $global{'max_id'} = $id; # not sure why....
        s/([\\]begin\s*$O\d+$C\s*theindex)/\\textohtmlindex $1/o;
            s/[\\]printindex/\\textohtmlindex /o;
    }
    else {
        die "\n\nBad number of index environments!\n\n";
    }
    #----------------------------------------------------------------------
    lib_add_bbl_and_idx_dummy_commands()
        if defined(&lib_add_bbl_and_idx_dummy_commands);
}

# The bibliographic references, the appendices, the lists of figures
# and tables etc. must appear in the contents table at the same level
# as the outermost sectioning command. This subroutine finds what is
# the outermost level and sets the above to the same level;

sub set_depth_levels {
    # Sets $outermost_level
    my $level;
    #RRM:  do not alter user-set value for  $MAX_SPLIT_DEPTH
    foreach $level ("part", "chapter", "section", "subsection",
                    "subsubsection", "paragraph") {
        last if (($outermost_level) = /\\($level)$delimiter_rx/);
    }
    $level = ($outermost_level ? $section_commands{$outermost_level} :
              do {$outermost_level = 'section'; 3;});

    #RRM:  but calculate value for $MAX_SPLIT_DEPTH when a $REL_DEPTH was given
    if ($REL_DEPTH && $MAX_SPLIT_DEPTH) {
        $MAX_SPLIT_DEPTH = $level + $MAX_SPLIT_DEPTH;
    } elsif (!($MAX_SPLIT_DEPTH)) { $MAX_SPLIT_DEPTH = 1 };

    %unnumbered_section_commands = ('tableofcontents' => $level,
                                    'listoffigures' => $level,
                                    'listoftables' => $level,
                                    'bibliography' => $level,
                                    'textohtmlindex' => $level,
                                    'textohtmlmoduleindex' => $level);
    $section_headings{'textohtmlmoduleindex'} = 'h1';

    %section_commands = (%unnumbered_section_commands,
                         %section_commands);

    make_sections_rx();
}


# This changes the markup used for {verbatim} environments, and is the
# best way I've found that ensures the <dl> goes on the outside of the
# <pre>...</pre>.
#
# Note that this *must* be done in the init file, not the python.perl
# style support file.  The %declarations must be set before
# initialize() is called in the main LaTeX2HTML script (which happens
# before style files are loaded).
#
%declarations = ('preform' => '<div class="verbatim"><pre></pre></div>',
                 %declarations);


# This is a modified version of what's provided by LaTeX2HTML; see the
# comment on the middle stanza for an explanation of why we keep our
# own version.
#
# This routine must be called once on the text only,
# else it will "eat up" sensitive constructs.
sub text_cleanup {
    # MRO: replaced $* with /m
    s/(\s*\n){3,}/\n\n/gom;	# Replace consecutive blank lines with one
    s/<(\/?)P>\s*(\w)/<$1P>\n$2/gom;      # clean up paragraph starts and ends
    s/$O\d+$C//go;		# Get rid of bracket id's
    s/$OP\d+$CP//go;		# Get rid of processed bracket id's
    s/(<!)?--?(>)?/(length($1) || length($2)) ? "$1--$2" : "-"/ge;
    # Spacing commands
    s/\\( |$)/ /go;
    #JKR: There should be no more comments in the source now.
    #s/([^\\]?)%/$1/go;        # Remove the comment character
    # Cannot treat \, as a command because , is a delimiter ...
    s/\\,/ /go;
    # Replace tilde's with non-breaking spaces
    s/ *~/&nbsp;/g;

    # This is why we have this copy of this routine; the following
    # isn't so desirable as the author/maintainers of LaTeX2HTML seem
    # to think.  It's not commented out in the main script, so we have
    # to override the whole thing.  In particular, we don't want empty
    # table cells to disappear.

    ### DANGEROUS ?? ###
    # remove redundant (not <P></P>) empty tags, incl. with attributes
    #s/\n?<([^PD >][^>]*)>\s*<\/\1>//g;
    #s/\n?<([^PD >][^>]*)>\s*<\/\1>//g;
    # remove redundant empty tags (not </P><P> or <TD> or <TH>)
    #s/<\/(TT|[^PTH][A-Z]+)><\1>//g;
    #s/<([^PD ]+)(\s[^>]*)?>\n*<\/\1>//g;

    #JCL(jcl-hex)
    # Replace ^^ special chars (according to p.47 of the TeX book)
    # Useful when coming from the .aux file (german umlauts, etc.)
    s/\^\^([^0-9a-f])/chr((64+ord($1))&127)/ge;
    s/\^\^([0-9a-f][0-9a-f])/chr(hex($1))/ge;
}

# This is used to map the link rel attributes LaTeX2HTML uses to those
# currently recommended by the W3C.
sub custom_REL_hook {
    my($rel,$junk) = @_;
    return 'parent' if $rel eq 'up';
    return 'prev' if $rel eq 'previous';
    return $rel;
}

# This is added to get rid of the long comment that follows the
# doctype declaration; MSIE5 on NT4 SP4 barfs on it and drops the
# content of the page.
$MY_PARTIAL_HEADER = '';
sub make_head_and_body($$) {
    my($title, $body) = @_;
    $body = " $body" unless ($body eq '');
    my $DTDcomment = '';
    my($version, $isolanguage) = ($HTML_VERSION, 'EN');
    my %isolanguages = (  'english',  'EN'   , 'USenglish', 'EN.US'
                        , 'original', 'EN'   , 'german'   , 'DE'
                        , 'austrian', 'DE.AT', 'french'   , 'FR'
                        , 'spanish',  'ES');
    $isolanguage = $isolanguages{$default_language};
    $isolanguage = 'EN' unless $isolanguage;
    $title = &purify($title,1);
    eval("\$title = ". $default_title ) unless ($title);

    # allow user-modification of the <title> tag; thanks Dan Young
    if (defined &custom_TITLE_hook) {
        $title = &custom_TITLE_hook($title, $toc_sec_title);
    }

    if ($DOCTYPE =~ /\/\/[\w\.]+\s*$/) { # language spec included
        $DTDcomment = "<!DOCTYPE html PUBLIC \"$DOCTYPE\">\n";
    } else {
        $DTDcomment = "<!DOCTYPE html PUBLIC \"$DOCTYPE//"
            . ($ISO_LANGUAGE ? $ISO_LANGUAGE : $isolanguage) . "\">\n";
    }
    if ($MY_PARTIAL_HEADER eq '') {
        my $favicon = '';
        if ($FAVORITES_ICON) {
            my($myname, $mydir, $myext) = fileparse($FAVORITES_ICON, '\..*');
            my $favtype = '';
            if ($myext eq '.gif' || $myext eq '.png') {
                $myext =~ s/^[.]//;
                $favtype = " type=\"image/$myext\"";
            }
            $favicon = (
                "\n<link rel=\"SHORTCUT ICON\" href=\"$FAVORITES_ICON\""
                . "$favtype />");
        }
        $STYLESHEET = $FILE.".css" unless $STYLESHEET;
        $MY_PARTIAL_HEADER = join('',
            ($DOCTYPE ? $DTDcomment : ''),
            "<html>\n<head>",
            ($BASE ? "\n<base href=\"$BASE\" />" : ''),
            "\n<link rel=\"STYLESHEET\" href=\"$STYLESHEET\" type='text/css'",
            " />",
            $favicon,
            ($EXTERNAL_UP_LINK
             ? ("\n<link rel='start' href='" . $EXTERNAL_UP_LINK
                . ($EXTERNAL_UP_TITLE ?
                   "' title='$EXTERNAL_UP_TITLE' />" : "' />"))
             : ''),
            "\n<link rel=\"first\" href=\"$FILE.html\"",
            ($t_title ? " title='$t_title'" : ''),
            ' />',
            ($HAVE_TABLE_OF_CONTENTS
             ? ("\n<link rel='contents' href='$MY_CONTENTS_PAGE'"
                . ' title="Contents" />')
             : ''),
            ($HAVE_GENERAL_INDEX
             ? ("\n<link rel='index' href='$GENERAL_INDEX_FILE'"
                . " title='Index' />")
             : ''),
            # disable for now -- Mozilla doesn't do well with multiple indexes
            # ($HAVE_MODULE_INDEX
            #  ? ("<link rel="index" href='$MODULE_INDEX_FILE'"
            #     . " title='Module Index' />\n")
            #  : ''),
            ($INFO
             # XXX We can do this with the Python tools since the About...
             # page always gets copied to about.html, even when we use the
             # generated node###.html page names.  Won't work with the
             # rest of the Python doc tools.
             ? ("\n<link rel='last' href='about.html'"
                . " title='About this document...' />"
                . "\n<link rel='help' href='about.html'"
                . " title='About this document...' />")
             : ''),
            $more_links_mark,
            "\n",
            ($CHARSET && $HTML_VERSION ge "2.1"
             ? ('<meta http-equiv="Content-Type" content="text/html; '
                . "charset=$CHARSET\" />\n")
             : ''),
            ($AESOP_META_TYPE
             ? "<meta name='aesop' content='$AESOP_META_TYPE' />\n" : ''));
    }
    if (!$charset && $CHARSET) {
        $charset = $CHARSET;
        $charset =~ s/_/\-/go;
    }
    join('',
         $MY_PARTIAL_HEADER,
         "<title>", $title, "</title>\n</head>\n<body$body>");
}

sub replace_morelinks {
    $more_links =~ s/ REL=/ rel=/g;
    $more_links =~ s/ HREF=/ href=/g;
    $more_links =~ s/<LINK /<link /g;
    $more_links =~ s/">/" \/>/g;
    $_ =~ s/$more_links_mark/$more_links/e;
}

1;      # This must be the last line
