#LaTeX2HTML Version 96.1 : dot.latex2html-init		-*- perl -*-
#
#  Significantly revised by Fred L. Drake, Jr. <fdrake@acm.org> for use
#  with the Python documentation.
#
#  New name to avoid distributing "dot" files with the Python documentation.
#

$INFO = 1;              # 0 = do not make a "About this document..." section
$MAX_LINK_DEPTH = 3;

$NUMBERED_FOOTNOTES = 1;

# Python documentation uses section numbers to support references to match
# in the printed and online versions.
#
$SHOW_SECTION_NUMBERS = 1;

$ICONSERVER = '../icons';

$CHILDLINE = "\n<p><hr>\n";
$VERBOSITY = 0;

# Locate a file that's been "require"d.  Assumes that the file name of interest
# is unique within the set of loaded files, after directory names have been
# stripped.  Only the directory is returned.
#
sub find_my_file{
    local($myfile,$key,$tmp,$mydir) = (@_[0], '', '', '');
    foreach $key (keys %INC) {
	$tmp = "$key";
	$tmp =~ s|^.*/||o;
	if ($tmp eq $myfile) {
	    #print "\nfound $tmp: $key --> ", $INC{$key}, "\n";
	    $mydir = $INC{$key};
	}
    }
    $mydir =~ s|/[^/]*$||;
    $mydir;
}


# A little painful, but lets us clean up the top level directory a little,
# and not be tied to the current directory (as far as I can tell).
#
use Cwd;
use File::Basename;
($myname, $mydir, $myext) = fileparse(__FILE__, '\..*');
chop $mydir;			# remove trailing '/'
$mydir = getcwd() . "$dd$mydir"
  unless $mydir =~ s|^/|/|;
$LATEX2HTMLSTYLES = "$mydir$envkey$LATEX2HTMLSTYLES";


sub make_nav_panel{
    ($NEXT_TITLE ? "$NEXT\n" : '')
      . ($UP_TITLE ? "$UP\n" : '')
      . ($PREVIOUS_TITLE ? "$PREVIOUS\n" : '')
      . "$CONTENTS\n$INDEX"
#      . " $CUSTOM_BUTTONS"
      . "<br>\n"
      . ($NEXT_TITLE ? "<b> Next:</b> $NEXT_TITLE\n" : '')
      . ($UP_TITLE ? "<b>Up:</b> $UP_TITLE\n" : '')
      . ($PREVIOUS_TITLE ? "<b>Previous:</b> $PREVIOUS_TITLE\n" : '');
}

sub top_navigation_panel {
    "<div class=navigation>\n"
      . &make_nav_panel
      . "<br><hr><p></div>"
}

sub bot_navigation_panel {
    "<div class=navigation><hr>"
      . &make_nav_panel
      . "</div>"
}


sub gen_index_id {
    # this is used to ensure common index key generation and a stable sort
    local($str,$extra) = @_;
    sprintf("%s###%s%010d", $str, $extra, ++$global{'max_id'});
}

sub make_index_entry {
    local($br_id,$str) = @_;
    # If TITLE is not yet available (i.e the \index command is in the title of the
    # current section), use $ref_before.
    $TITLE = $ref_before unless $TITLE;
    # Save the reference
    $str = gen_index_id($str, '');
    $index{$str} .= &make_half_href("$CURRENT_FILE#$br_id");
    "<a name=\"$br_id\">$anchor_invisible_mark<\/a>";
}

sub add_idx {
    print "\nDoing the index ...";
    local($key, $str, @keys, $index, $level, $count, @previous, @current);
    @keys = keys %index;
    @keys = sort keysort  @keys;
    $level = 0;
    foreach $key (@keys) {
	@current = split(/!/, $key);
	$count = 0;
	while ($current[$count] eq $previous[$count]) {
	    $count++;
	}
	while ($count > $level) {
	    $index .= "<dl compact>\n";
	    $level++;
	}
	while ($count < $level) {
	    $index .= "</dl>\n";
	    $level--;
	}
	foreach $term (@current[$count .. $#current-1]) {
	    # need to "step in" a little
	    $index .= "<dt>" . $term . "\n<dl compact>\n";
	    $level++;
	}
	$str = $current[$#current];
	$str =~ s/\#\#\#\d+$//o;	# Remove the unique id's
	$str =~ s/\#\#\#[DR]EF\d+$//o;	# Remove the unique id's
	if (&index_key_eq(join('',@current), join('',@previous))) {
	    $index .= ",\n$index{$key}" . $cross_ref_visible_mark . "</a>"; }
	else {
	    $index .= "\n<dt>$index{$key}" . $str . "</a>"; }
	@previous = @current;
    }
    while ($count < $level) {
	$index .= "</dl>\n";
	$level--;
    }
    s/$idx_mark/<dl compact>$index<\/dl>/o;
}


sub index_key_eq{
    local($a,$b) = @_;
    $a = &clean_key($a);
    $a =~ s/\#\#\#\d+$//o;  		# Remove the unique id's
    $a =~ s/\#\#\#[dr]ef\d+$//o;	# Remove the unique id's
    $b = &clean_key($b);
    $b =~ s/\#\#\#\d+$//o;  		# Remove the unique id's
    $b =~ s/\#\#\#[dr]ef\d+$//o;	# Remove the unique id's
    $a eq $b;
}

# need to remove leading <...>
sub clean_key{
    local ($_) = @_;
    tr/A-Z/a-z/;
    s/\s//;
    s/^<[a-z][-._a-z0-9]*>//;	# Remove leading <gi> (no attributes)
    $_;
}


$idx_module_mark = '<tex2html_idx_module_mark>';
$idx_module_title = 'Module Index';

sub add_module_idx{
    print "\nDoing the module index ...";
    local($key, @keys, $index);
    $index = "<p>";
    @keys = keys %Modules;
    @keys = sort keysort  @keys;
    foreach $key (@keys) {
	$index .= "$Modules{$key}$key</a><br>\n";
    }
    s/$idx_module_mark/$index<p>/o;
}


# sub remove_general_markers {
#     s/$lof_mark/<UL>$figure_captions<\/UL>/o;
#     s/$lot_mark/<UL>$table_captions<\/UL>/o;
#     &replace_citations if /$bbl_mark/;
#     &add_toc if (/$toc_mark/);
#     &add_idx if (/$idx_mark/);
#     &add_module_idx if (/$idx_module_mark/);
#     &replace_cross_references if /$cross_ref_mark/;
#     &replace_external_references if /$external_ref_mark/;
#     &replace_cite_references if /$cite_mark/;
#     if (defined &replace_user_references) {
#  	&replace_user_references if /$user_ref_mark/;
#     }
# }

# In addition to the standard stuff, add label to allow named node files.
sub do_cmd_tableofcontents {
    local($_) = @_;
    $TITLE = $toc_title;
    $tocfile = $CURRENT_FILE;
    local($closures,$reopens) = &preserve_open_tags();
    &anchor_label("contents",$CURRENT_FILE,$_);		# this is added
    join('', "<BR>\n", $closures
	 , &make_section_heading($toc_title, "H2"), $toc_mark
	 , $reopens, $_);
}
# In addition to the standard stuff, add label to allow named node files.
sub do_cmd_listoffigures {
    local($_) = @_;
    $TITLE = $lof_title;
    $loffile = $CURRENT_FILE;
    local($closures,$reopens) = &preserve_open_tags();
    &anchor_label("lof",$CURRENT_FILE,$_);		# this is added
    join('', "<BR>\n", $closures
	 , &make_section_heading($lof_title, "H2"), $lof_mark
	 , $reopens, $_);
}
# In addition to the standard stuff, add label to allow named node files.
sub do_cmd_listoftables {
    local($_) = @_;
    $TITLE = $lot_title;
    $lotfile = $CURRENT_FILE;
    local($closures,$reopens) = &preserve_open_tags();
    &anchor_label("lot",$CURRENT_FILE,$_);		# this is added
    join('', "<BR>\n", $closures
	 , &make_section_heading($lot_title, "H2"), $lot_mark
	 , $reopens, $_);
}
# In addition to the standard stuff, add label to allow named node files.
sub do_cmd_textohtmlinfopage {
    local($_) = @_;
    if ($INFO) {					# 
	&anchor_label("about",$CURRENT_FILE,$_);	# this is added
    }							#
    ( ($INFO == 1)
     ? join('', $close_all
	, "<STRONG>$t_title</STRONG><P>\nThis document was generated using the\n"
	, "<A HREF=\"$TEX2HTMLADDRESS\"><STRONG>LaTeX</STRONG>2<tt>HTML</tt></A>"
	, " translator Version $TEX2HTMLVERSION\n"
	, "<P>Copyright &#169; 1993, 1994, 1995, 1996, 1997,\n"
	, "<A HREF=\"$AUTHORADDRESS\">Nikos Drakos</A>, \n"
	, "Computer Based Learning Unit, University of Leeds.\n"
	, "<P>The command line arguments were: <BR>\n "
	, "<STRONG>latex2html</STRONG> <tt>$argv</tt>.\n"
	, "<P>The translation was initiated by $address_data[0] on $address_data[1]"
	, $open_all, $_)
      : join('', $close_all, $INFO,"\n", $open_all, $_))
}

# $idx_mark will be replaced with the real index at the end
sub do_cmd_textohtmlindex {
    local($_) = @_;
    $TITLE = $idx_title;
    $idxfile = $CURRENT_FILE;
    if (%index_labels) { &make_index_labels(); }
    if (($SHORT_INDEX) && (%index_segment)) { &make_preindex(); }
    else { $preindex = ''; }
    local($heading) = join('',&make_section_heading($idx_title, "H2"),
			   $idx_mark);
    local($pre,$post) = &minimize_open_tags($heading);
    &anchor_label("genindex",$CURRENT_FILE,$_);		# this is added
    join('',"<BR>\n" , $pre, $_);
}

# $idx_module_mark will be replaced with the real index at the end
sub do_cmd_textohtmlmoduleindex {
    local($_) = @_;
    local($key) = q/modindex/;
    $TITLE = $idx_module_title;
    &anchor_label("modindex",$CURRENT_FILE,$_);
    join('', '<p>' , &make_section_heading($idx_module_title, "h2"),
	 $idx_module_mark, $_);
}

# The bibliography and the index should be treated as separate sections
# in their own HTML files. The \bibliography{} command acts as a sectioning command
# that has the desired effect. But when the bibliography is constructed 
# manually using the thebibliography environment, or when using the
# theindex environment it is not possible to use the normal sectioning 
# mechanism. This subroutine inserts a \bibliography{} or a dummy 
# \textohtmlindex command just before the appropriate environments
# to force sectioning.

# XXX	This *assumes* that if there are two {theindex} environments, the
#	first is the module index and the second is the standard index.  This
#	is sufficient for the current Python documentation, but that's about
#	it.

sub add_bbl_and_idx_dummy_commands {
    local($id) = $global{'max_id'};

    $section_commands{'textohtmlmoduleindex'} = 2;

    s/([\\]begin\s*$O\d+$C\s*thebibliography)/$bbl_cnt++; $1/eg;
    s/([\\]begin\s*$O\d+$C\s*thebibliography)/$id++; "\\bibliography$O$id$C$O$id$C $1"/geo
      #if ($bbl_cnt == 1)
    ;
    #}
    #----------------------------------------------------------------------#
    # (FLD) This was added						   #
    local(@parts) = split(/\\begin\s*$O\d+$C\s*theindex/);		   #
    if (scalar(@parts) == 3) {						   #
	print "\n&add_bbl_and_idx_dummy_commands ==> adding module index"; #
	s/([\\]begin\s*$O\d+$C\s*theindex)/\\textohtmlmoduleindex $1/o;    #
    }									   #
    #----------------------------------------------------------------------#
    $global{'max_id'} = $id;
    s/([\\]begin\s*$O\d+$C\s*theindex)/\\textohtmlindex $1/o;
    s/[\\]printindex/\\textohtmlindex /o;
    &lib_add_bbl_and_idx_dummy_commands() if defined(&lib_add_bbl_and_idx_dummy_commands);
}

# The bibliographic references, the appendices, the lists of figures and tables
# etc. must appear in the contents table at the same level as the outermost
# sectioning command. This subroutine finds what is the outermost level and
# sets the above to the same level;

#%section_commands = ('textohtmlmoduleindex', 1, %section_commands);

sub set_depth_levels {
    # Sets $outermost_level
    local($level);
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

    %unnumbered_section_commands = (
          'tableofcontents', $level
	, 'listoffigures', $level
	, 'listoftables', $level
	, 'bibliography', $level
	, 'textohtmlindex', $level
	, 'textohtmlmoduleindex', $level
        );

    %section_commands = ( 
	  %unnumbered_section_commands
        , %section_commands
        );
}


# Fix from Ross Moore for ']' in \item[...]; this can be removed once the next
# patch to LaTeX2HTML is released and tested.
#
sub protect_useritems {
    local(*_) = @_;
    local($preitems, $thisitem);
    while (/\\item\s*\[/) {
        $preitems .= $`; $_ = $';
        $thisitem = $&.'<<'.++$global{'max_id'}.'>>';
        s/^(((($O|$OP)\d+($C|$CP)).*\3|<[^<>]*>|[^\]<]+)*)\]/$thisitem.=$1;''/e;
        $preitems .= $thisitem.'<<'.$global{'max_id'}.'>>]'; s/^]//;
    }
    $_ = $preitems . $_;
}

1;	# This must be the last line
