#! /usr/bin/env perl
# html2texi.pl -- Convert HTML documentation to Texinfo format
# Michael Ernst <mernst@cs.washington.edu>
# Time-stamp: <1999-01-12 21:34:27 mernst>

# This program converts HTML documentation trees into Texinfo format.
# Given the name of a main (or contents) HTML file, it processes that file,
# and other files (transitively) referenced by it, into a Texinfo file
# (whose name is chosen from the file or directory name of the argument).
# For instance:
#   html2texi.pl api/index.html
# produces file "api.texi".

# Texinfo format can be easily converted to Info format (for browsing in
# Emacs or the standalone Info browser), to a printed manual, or to HTML.
# Thus, html2texi.pl permits conversion of HTML files to Info format, and
# secondarily enables producing printed versions of Web page hierarchies.

# Unlike HTML, Info format is searchable.  Since Info is integrated into
# Emacs, one can read documentation without starting a separate Web
# browser.  Additionally, Info browsers (including Emacs) contain
# convenient features missing from Web browsers, such as easy index lookup
# and mouse-free browsing.

# Limitations:
# html2texi.pl is currently tuned to latex2html output (and it corrects
# several latex2html bugs), but should be extensible to arbitrary HTML
# documents.  It will be most useful for HTML with a hierarchical structure
# and an index, and it recognizes those features as created by latex2html
# (and possibly by some other tools).  The HTML tree to be traversed must
# be on local disk, rather than being accessed via HTTP.
# This script requires the use of "checkargs.pm".  To eliminate that
# dependence, replace calls to check_args* by @_ (which is always the last
# argument to those functions).
# Also see the "to do" section, below.
# Comments, suggestions, bug fixes, and enhancements are welcome.

# Troubleshooting:
# Malformed HTML can cause this program to abort, so
# you should check your HTML files to make sure they are legal.


###
### Typical usage for the Python documentation:
###

# (Actually, most of this is in a Makefile instead.)
# The resulting Info format Python documentation is currently available at
# ftp://ftp.cs.washington.edu/homes/mernst/python-info.tar.gz

# Fix up HTML problems, eg <DT><DL COMPACT><DD> should be <DT><DL COMPACT><DD>.

# html2texi.pl /homes/fish/mernst/tmp/python-doc/html/api/index.html
# html2texi.pl /homes/fish/mernst/tmp/python-doc/html/ext/index.html
# html2texi.pl /homes/fish/mernst/tmp/python-doc/html/lib/index.html
# html2texi.pl /homes/fish/mernst/tmp/python-doc/html/mac/index.html
# html2texi.pl /homes/fish/mernst/tmp/python-doc/html/ref/index.html
# html2texi.pl /homes/fish/mernst/tmp/python-doc/html/tut/index.html

# Edit the generated .texi files:
#   * change @setfilename to prefix "python-"
#   * fix up any sectioning, such as for Abstract
#   * make Texinfo menus
#   * perhaps remove the @detailmenu ... @end detailmenu
# In Emacs, to do all this:
#   (progn (goto-char (point-min)) (replace-regexp "\\(@setfilename \\)\\([-a-z]*\\)$" "\\1python-\\2.info") (replace-string "@node Front Matter\n@chapter Abstract\n" "@node Abstract\n@section Abstract\n") (progn (mark-whole-buffer) (texinfo-master-menu 'update-all-nodes)) (save-buffer))

# makeinfo api.texi
# makeinfo ext.texi
# makeinfo lib.texi
# makeinfo mac.texi
# makeinfo ref.texi
# makeinfo tut.texi


###
### Structure of the code
###

# To be written...


###
### Design decisions
###

# Source and destination languages
# --------------------------------
# 
# The goal is Info files; I create Texinfo, so I don't have to worry about
# the finer details of Info file creation.  (I'm not even sure of its exact
# format.)
# 
# Why not start from LaTeX rather than HTML?
# I could hack latex2html itself to produce Texinfo instead, or fix up
# partparse.py (which already translates LaTeX to Teinfo).
#  Pros:
#   * has high-level information such as index entries, original formatting
#  Cons:
#   * those programs are complicated to read and understand
#   * those programs try to handle arbitrary LaTeX input, track catcodes,
#     and more:  I don't want to go to that effort.  HTML isn't as powerful
#     as LaTeX, so there are fewer subtleties.
#   * the result wouldn't work for arbitrary HTML documents; it would be
#     nice to eventually extend this program to HTML produced from Docbook,
#     Frame, and more.

# Parsing
# -------
# 
# I don't want to view the text as a linear stream; I'd rather parse the
# whole thing and then do pattern matching over the parsed representation (to
# find idioms such as indices, lists of child nodes, etc.).
#  * Perl provides HTML::TreeBuilder, which does just what I want.
#     * libwww-perl: http://www.linpro.no/lwp/
#     * TreeBuilder: HTML-Tree-0.51.tar.gz
#  * Python Parsers, Formatters, and Writers don't really provide the right
#    interface (and the version in Grail doesn't correspond to another
#    distributed version, so I'm confused about which to be using).  I could
#    write something in Python that creates a parse tree, but why bother?

# Other implementation language issues:
#  * Python lacks variable declarations, reasonable scoping, and static
#    checking tools.  I've written some of the latter for myself that make
#    my Perl programming a lot safer than my Python programming will be until
#    I have a similar suite for that language.


###########################################################################
### To do
###

# Section names:
#   Fix the problem with multiple sections in a single file (eg, Abstract in
#     Front Matter section).
#   Deal with cross-references, as in /homes/fish/mernst/tmp/python-doc/html/ref/types.html:310
# Index:
#   Perhaps double-check that every tag mentioned in the index is found
#     in the text.
# Python:  email to python-docs@python.org, to get their feedback.
#   Compare to existing lib/ Info manual
#   Write the hooks into info-look; replace pyliblookup1-1.tar.gz.
#   Postpass to remove extra quotation marks around typography already in
#     a different font (to avoid double delimiters as in "`code'"); or
#     perhaps consider using only font-based markup so that we don't get
#     the extra *bold* and `code' markup in Info.

## Perhaps don't rely on automatic means for adding up, next, prev; I have
## all that info available to me already, so it's not so much trouble to
## add it.  (Right?)  But it is *so* easy to use Emacs instead...


###########################################################################
### Strictures
###

# man HTML::TreeBuilder
# man HTML::Parser
# man HTML::Element

# require HTML::ParserWComment;
require HTML::Parser;
require HTML::TreeBuilder;
require HTML::Element;

use File::Basename;

use strict;
# use Carp;

use checkargs;


###########################################################################
### Variables
###

my @section_stack = ();		# elements are chapter/section/subsec nodetitles (I think)
my $current_ref_tdf;		# for the file currently being processed;
				#  used in error messages
my $html_directory;
my %footnotes;

# First element should not be used.
my @sectionmarker = ("manual", "chapter", "section", "subsection", "subsubsection");

my %inline_markup = ("b" => "strong",
		     "code" => "code",
		     "i" => "emph",
		     "kbd" => "kbd",
		     "samp" => "samp",
		     "strong" => "strong",
		     "tt" => "code",
		     "var" => "var");

my @deferred_index_entries = ();

my @index_titles = ();		# list of (filename, type) lists
my %index_info = ("Index" => ["\@blindex", "bl"],
		  "Concept Index" => ["\@cindex", "cp"],
		  "Module Index" => ["\@mdindex", "md"]);


###########################################################################
### Main/contents page
###

# Process first-level page on its own, or just a contents page?  Well, I do
# want the title, author, etc., and the front matter...  For now, just add
# that by hand at the end.


# data structure possibilities:
#  * tree-like (need some kind of stack when processing (or parent pointers))
#  * list of name and depth; remember old and new depths.

# Each element is a reference to a list of (nodetitle, depth, filename).
my @contents_list = ();

# The problem with doing fixups on the fly is that some sections may have
# already been processed (and no longer available) by the time we notice
# others with the same name.  It's probably better to fully construct the
# contents list (reading in all files of interest) upfront; that will also
# let me do a better job with cross-references, because again, all files
# will already be read in.
my %contents_hash = ();
my %contents_fixups = ();

my @current_contents_list = ();

# Merge @current_contents_list into @contents_list,
# and set @current_contents_list to be empty.
sub merge_contents_lists ( )
{ check_args(0, @_);

  # Three possibilities:
  #  * @contents_list is empty: replace it by @current_contents_list.
  #  * prefixes of the two lists are identical: do nothing
  #  * @current_contents_list is all at lower level than $contents_list[0];
  #    prefix @contents_list by @current_contents_list

  if (scalar(@current_contents_list) == 0)
    { die "empty current_contents_list"; }

  #   if (scalar(@contents_list) == 0)
  #     { @contents_list = @current_contents_list;
  #       @current_contents_list = ();
  #       return; }

  #   if (($ {$contents_list[0]}[1]) < ($ {$current_contents_list[0]}[1]))
  #     { unshift @contents_list, @current_contents_list;
  #       @current_contents_list = ();
  #       return; }

  for (my $i=0; $i<scalar(@current_contents_list); $i++)
    { my $ref_c_tdf = $current_contents_list[$i];
      if ($i >= scalar(@contents_list))
	{ push @contents_list, $ref_c_tdf;
	  my $title = $ {$ref_c_tdf}[0];
	  if (defined $contents_hash{$title})
	    { $contents_fixups{$title} = 1; }
	  else
	    { $contents_hash{$title} = 1; }
	  next; }
      my $ref_tdf = $contents_list[$i];
      my ($title, $depth, $file) = @{$ref_tdf};
      my ($c_title, $c_depth, $c_file) = @{$ref_c_tdf};

      if (($title ne $c_title)
	  && ($depth < $c_depth)
	  && ($file ne $c_file))
	{ splice @contents_list, $i, 0, $ref_c_tdf;
	  if (defined $contents_hash{$c_title})
	    { $contents_fixups{$c_title} = 1; }
	  else
	    { $contents_hash{$c_title} = 1; }
	  next; }

      if (($title ne $c_title)
	  || ($depth != $c_depth)
	  || ($file ne $c_file))
	{ die ("while processing $ {$current_ref_tdf}[2] at depth $ {$current_ref_tdf}[1], mismatch at index $i:",
	       "\n  main:  <<<$title>>> $depth $file",
	       "\n  curr:  <<<$c_title>>> $c_depth $c_file"); }
    }
  @current_contents_list = ();
}



# Set @current_contents_list to a list of (title, href, sectionlevel);
#  then merge that list into @contents_list.
# Maybe this function should also produce a map
#  from title (or href) to sectionlevel (eg "chapter"?).
sub process_child_links ( $ )
{ my ($he) = check_args(1, @_);

  # $he->dump();
  if (scalar(@current_contents_list) != 0)
    { die "current_contents_list nonempty: @current_contents_list"; }
  $he->traverse(\&increment_current_contents_list, 'ignore text');

  # Normalize the depths; for instance, convert 1,3,5 into 0,1,2.
  my %depths = ();
  for my $ref_tdf (@current_contents_list)
    { $depths{$ {$ref_tdf}[1]} = 1; }
  my @sorted_depths = sort keys %depths;
  my $current_depth = scalar(@section_stack)-1;
  my $current_depth_2 = $ {$current_ref_tdf}[1];
  if ($current_depth != $current_depth_2)
    { die "mismatch in current depths: $current_depth $current_depth_2; ", join(", ", @section_stack); }
  for (my $i=0; $i<scalar(@sorted_depths); $i++)
    { $depths{$sorted_depths[$i]} = $i + $current_depth+1; }
  for my $ref_tdf (@current_contents_list)
    { $ {$ref_tdf}[1] = $depths{$ {$ref_tdf}[1]}; }

  # Eliminate uninteresting sections.  Hard-coded hack for now.
  if ($ {$current_contents_list[-1]}[0] eq "About this document ...")
    { pop @current_contents_list; }
  if ((scalar(@current_contents_list) > 1)
      && ($ {$current_contents_list[1]}[0] eq "Contents"))
    { my $ref_first_tdf = shift @current_contents_list;
      $current_contents_list[0] = $ref_first_tdf; }

  for (my $i=0; $i<scalar(@current_contents_list); $i++)
    { my $ref_tdf = $current_contents_list[$i];
      my $title = $ {$ref_tdf}[0];
      if (exists $index_info{$title})
	{ my $index_file = $ {$ref_tdf}[2];
	  my ($indexing_command, $suffix) = @{$index_info{$title}};
	  process_index_file($index_file, $indexing_command);
	  print TEXI "\n\@defindex $suffix\n";
	  push @index_titles, $title;
	  splice @current_contents_list, $i, 1;
	  $i--; }
      elsif ($title =~ /\bIndex$/)
	{ print STDERR "Warning: \"$title\" might be an index; if so, edit \%index_info.\n"; } }

  merge_contents_lists();

  # print_contents_list();
  # print_index_info();
}


sub increment_current_contents_list ( $$$ )
{ my ($he, $startflag, $depth) = check_args(3, @_);
  if (!$startflag)
    { return; }

  if ($he->tag eq "li")
    { my @li_content = @{$he->content};
      if ($li_content[0]->tag ne "a")
	{ die "first element of <LI> should be <A>"; }
      my ($name, $href, @content) = anchor_info($li_content[0]);
      # unused $name
      my $title = join("", collect_texts($li_content[0]));
      $title = texi_remove_punctuation($title);
      # The problem with these is that they are formatted differently in
      # @menu and @node!
      $title =~ s/``/\"/g;
      $title =~ s/''/\"/g;
      $title =~ s/ -- / /g;
      push @current_contents_list, [ $title, $depth, $href ]; }
  return 1;
}

# Simple version for section titles
sub html_to_texi ( $ )
{ my ($he) = check_args(1, @_);
  if (!ref $he)
    { return $he; }

  my $tag = $he->tag;
  if (exists $inline_markup{$tag})
    { my $result = "\@$inline_markup{$tag}\{";
      for my $elt (@{$he->content})
	{ $result .= html_to_texi($elt); }
      $result .= "\}";
      return $result; }
  else
    { $he->dump();
      die "html_to_texi confused by <$tag>"; }
}



sub print_contents_list ()
{ check_args(0, @_);
  print STDERR "Contents list:\n";
  for my $ref_tdf (@contents_list)
    { my ($title, $depth, $file) = @{$ref_tdf};
      print STDERR "$title $depth $file\n"; }
}



###########################################################################
### Index
###

my $l2h_broken_link_name = "l2h-";


# map from file to (map from anchor name to (list of index texts))
# (The list is needed when a single LaTeX command like \envvar
# expands to multiple \index commands.)
my %file_index_entries = ();
my %this_index_entries;		# map from anchor name to (list of index texts)

my %file_index_entries_broken = (); # map from file to (list of index texts)
my @this_index_entries_broken;

my $index_prefix = "";
my @index_prefixes = ();

my $this_indexing_command;

sub print_index_info ()
{ check_args(0, @_);
  my ($key, $val);
  for my $file (sort keys %file_index_entries)
    { my %index_entries = %{$file_index_entries{$file}};
      print STDERR "file: $file\n";
      for my $aname (sort keys %index_entries)
	{ my @entries = @{$index_entries{$aname}};
	  if (scalar(@entries) == 1)
	    { print STDERR "  $aname : $entries[0]\n"; }
	  else
	    { print STDERR "  $aname : ", join("\n     " . (" " x length($aname)), @entries), "\n"; } } }
  for my $file (sort keys %file_index_entries_broken)
    { my @entries = @{$file_index_entries_broken{$file}};
      print STDERR "file: $file\n";
      for my $entry (@entries)
	{ print STDERR "  $entry\n"; }
    }
}


sub process_index_file ( $$ )
{ my ($file, $indexing_command) = check_args(2, @_);
  # print "process_index_file $file $indexing_command\n";

  my $he = file_to_tree($html_directory . $file);
  # $he->dump();

  $this_indexing_command = $indexing_command;
  $he->traverse(\&process_if_index_dl_compact, 'ignore text');
  undef $this_indexing_command;
  # print "process_index_file done\n";
}


sub process_if_index_dl_compact ( $$$ )
{ my ($he, $startflag) = (check_args(3, @_))[0,1]; #  ignore depth argument
  if (!$startflag)
    { return; }

  if (($he->tag() eq "dl") && (defined $he->attr('compact')))
    { process_index_dl_compact($he);
      return 0; }
  else
    { return 1; }
}


# The elements of a <DL COMPACT> list from a LaTeX2HTML index:
#  * a single space: text to be ignored
#  * <DT> elements with an optional <DD> element following each one
#    Two types of <DT> elements:
#     * Followed by a <DD> element:  the <DT> contains a single
#       string, and the <DD> contains a whitespace string to be ignored, a
#       <DL COMPACT> to be recursively processed (with the <DT> string as a
#       prefix), and a whitespace string to be ignored.
#     * Not followed by a <DD> element:  contains a list of anchors
#       and texts (ignore the texts, which are only whitespace and commas).
#       Optionally contains a <DL COMPACT> to be recursively processed (with
#       the <DT> string as a prefix)
sub process_index_dl_compact ( $ )
{ my ($h) = check_args(1, @_);
  my @content = @{$h->content()};
  for (my $i = 0; $i < scalar(@content); $i++)
    { my $this_he = $content[$i];
      if ($this_he->tag ne "dt")
	{ $this_he->dump();
	  die "Expected <DT> tag: " . $this_he->tag; }
      if (($i < scalar(@content) - 1) && ($content[$i+1]->tag eq "dd"))
	{ process_index_dt_and_dd($this_he, $content[$i+1]);
	  $i++;	}
      else
	{ process_index_lone_dt($this_he); } } }



# Argument is a <DT> element.  If it contains more than one anchor, then
# the texts of all subsequent ones are "[Link]".  Example:
#       <DT>
#         <A HREF="embedding.html#l2h-201">
#           "$PATH"
#         ", "
#         <A HREF="embedding.html#l2h-205">
#           "[Link]"
# Optionally contains a <DL COMPACT> as well.  Example:
# <DT>
#   <A HREF="types.html#l2h-616">
#     "attribute"
#   <DL COMPACT>
#     <DT>
#       <A HREF="assignment.html#l2h-3074">
#         "assignment"
#       ", "
#       <A HREF="assignment.html#l2h-3099">
#         "[Link]"
#     <DT>
#       <A HREF="types.html#l2h-">
#         "assignment, class"

sub process_index_lone_dt ( $ )
{ my ($dt) = check_args(1, @_);
  my @dtcontent = @{$dt->content()};
  my $acontent;
  my $acontent_suffix;
  for my $a (@dtcontent)
    { if ($a eq ", ")
	{ next; }
      if (!ref $a)
	{ $dt->dump;
	  die "Unexpected <DT> string element: $a"; }

      if ($a->tag eq "dl")
	{ push @index_prefixes, $index_prefix;
	  if (!defined $acontent_suffix)
	    { die "acontent_suffix not yet defined"; }
	  $index_prefix .= $acontent_suffix . ", ";
	  process_index_dl_compact($a);
	  $index_prefix = pop(@index_prefixes);
	  return; }

      if ($a->tag ne "a")
	{ $dt->dump;
	  $a->dump;
	  die "Expected anchor in lone <DT>"; }

      my ($aname, $ahref, @acontent) = anchor_info($a);
      # unused $aname
      if (scalar(@acontent) != 1)
	{ die "Expected just one content of <A> in <DT>: @acontent"; }
      if (ref $acontent[0])
	{ $acontent[0]->dump;
	  die "Expected string content of <A> in <DT>: $acontent[0]"; }
      if (!defined($acontent))
	{ $acontent = $index_prefix . $acontent[0];
	  $acontent_suffix = $acontent[0]; }
      elsif (($acontent[0] ne "[Link]") && ($acontent ne ($index_prefix . $acontent[0])))
	{ die "Differing content: <<<$acontent>>>, <<<$acontent[0]>>>"; }

      if (!defined $ahref)
	{ $dt->dump;
	  die "no HREF in nachor in <DT>"; }
      my ($ahref_file, $ahref_name) = split(/\#/, $ahref);
      if (!defined $ahref_name)
	{ # Reference to entire file
	  $ahref_name = ""; }

      if ($ahref_name eq $l2h_broken_link_name)
	{ if (!exists $file_index_entries_broken{$ahref_file})
	    { $file_index_entries_broken{$ahref_file} = []; }
	  push @{$file_index_entries_broken{$ahref_file}}, "$this_indexing_command $acontent";
	  next; }

      if (!exists $file_index_entries{$ahref_file})
	{ $file_index_entries{$ahref_file} = {}; }
      # Don't do this!  It appears to make a copy, which is not desired.
      # my %index_entries = %{$file_index_entries{$ahref_file}};
      if (!exists $ {$file_index_entries{$ahref_file}}{$ahref_name})
	{ $ {$file_index_entries{$ahref_file}}{$ahref_name} = []; }
      # 	{ my $oldcontent = $ {$file_index_entries{$ahref_file}}{$ahref_name};
      # 	  if ($acontent eq $oldcontent)
      # 	    { die "Multiple identical index entries?"; }
      # 	  die "Trying to add $acontent, but already have index entry pointing at $ahref_file\#$ahref_name: ${$file_index_entries{$ahref_file}}{$ahref_name}"; }

      push @{$ {$file_index_entries{$ahref_file}}{$ahref_name}}, "$this_indexing_command $acontent";
      # print STDERR "keys: ", keys %{$file_index_entries{$ahref_file}}, "\n";
    }
}

sub process_index_dt_and_dd ( $$ )
{ my ($dt, $dd) = check_args(2, @_);
  my $dtcontent;
  { my @dtcontent = @{$dt->content()};
    if ((scalar(@dtcontent) != 1) || (ref $dtcontent[0]))
      { $dd->dump;
	$dt->dump;
	die "Expected single string (actual size = " . scalar(@dtcontent) . ") in content of <DT>: @dtcontent"; }
    $dtcontent = $dtcontent[0];
    $dtcontent =~ s/ +$//; }
  my $ddcontent;
  { my @ddcontent = @{$dd->content()};
    if (scalar(@ddcontent) != 1)
      { die "Expected single <DD> content, got ", scalar(@ddcontent), " elements:\n", join("\n", @ddcontent), "\n "; }
    $ddcontent = $ddcontent[0]; }
  if ($ddcontent->tag ne "dl")
    { die "Expected <DL> as content of <DD>, but saw: $ddcontent"; }

  push @index_prefixes, $index_prefix;
  $index_prefix .= $dtcontent . ", ";
  process_index_dl_compact($ddcontent);
  $index_prefix = pop(@index_prefixes);
}


###########################################################################
### Ordinary sections
###

sub process_section_file ( $$$ )
{ my ($file, $depth, $nodetitle) = check_args(3, @_);
  my $he = file_to_tree(($file =~ /^\//) ? $file : $html_directory . $file);

  # print STDERR "process_section_file: $file $depth $nodetitle\n";

  # Equivalently:
  #   while ($depth >= scalar(@section_stack)) { pop(@section_stack); }
  @section_stack = @section_stack[0..$depth-1];

  # Not a great nodename fixup scheme; need a more global view
  if ((defined $contents_fixups{$nodetitle})
      && (scalar(@section_stack) > 0))
    { my $up_title = $section_stack[$#section_stack];
      # hack for Python Standard Library
      $up_title =~ s/^(Built-in|Standard) Module //g;
      my ($up_first_word) = split(/ /, $up_title);
      $nodetitle = "$up_first_word $nodetitle";
    }

  push @section_stack, $nodetitle;
  # print STDERR "new section_stack: ", join(", ", @section_stack), "\n";

  $he->traverse(\&process_if_child_links, 'ignore text');
  %footnotes = ();
  # $he->dump;
  $he->traverse(\&process_if_footnotes, 'ignore text');

  # $he->dump;

  if (exists $file_index_entries{$file})
    { %this_index_entries = %{$file_index_entries{$file}};
      # print STDERR "this_index_entries:\n ", join("\n ", keys %this_index_entries), "\n";
    }
  else
    { # print STDERR "Warning: no index entries for file $file\n";
      %this_index_entries = (); }

  if (exists $file_index_entries_broken{$file})
    { @this_index_entries_broken = @{$file_index_entries_broken{$file}}; }
  else
    { # print STDERR "Warning: no index entries for file $file\n";
      @this_index_entries_broken = (); }


  if ($he->tag() ne "html")
    { die "Expected <HTML> at top level"; }
  my @content = @{$he->content()};
  if ((!ref $content[0]) or ($content[0]->tag ne "head"))
    { $he->dump;
      die "<HEAD> not first element of <HTML>"; }
  if ((!ref $content[1]) or ($content[1]->tag ne "body"))
    { $he->dump;
      die "<BODY> not second element of <HTML>"; }

  $content[1]->traverse(\&output_body);
}

# stack of things we're inside that are preventing indexing from occurring now.
# These are "h1", "h2", "h3", "h4", "h5", "h6", "dt" (and possibly others?)
my @index_deferrers = ();

sub push_or_pop_index_deferrers ( $$ )
{ my ($tag, $startflag) = check_args(2, @_);
  if ($startflag)
    { push @index_deferrers, $tag; }
  else
    { my $old_deferrer = pop @index_deferrers;
      if ($tag ne $old_deferrer)
	{ die "Expected $tag at top of index_deferrers but saw $old_deferrer; remainder = ", join(" ", @index_deferrers); }
      do_deferred_index_entries(); }
}


sub label_add_index_entries ( $;$ )
{ my ($label, $he) = check_args_range(1, 2, @_);
  # print ((exists $this_index_entries{$label}) ? "*" : " "), " label_add_index_entries $label\n";
  # $he is the anchor element
  if (exists $this_index_entries{$label})
    { push @deferred_index_entries, @{$this_index_entries{$label}};
      return; }

  if ($label eq $l2h_broken_link_name)
    { # Try to find some text to use in guessing which links should point here
      # I should probably only look at the previous element, or if that is
      # all punctuation, the one before it; collecting all the previous texts
      # is a bit of overkill.
      my @anchor_texts = collect_texts($he);
      my @previous_texts = collect_texts($he->parent, $he);
      # 4 elements is arbitrary; ought to filter out punctuation and small words
      # first, then perhaps keep fewer.  Perhaps also filter out formatting so
      # that we can see a larger chunk of text?  (Probably not.)
      # Also perhaps should do further chunking into words, in case the
      # index term isn't a chunk of its own (eg, was in <tt>...</tt>.
      my @candidate_texts = (@anchor_texts, (reverse(@previous_texts))[0..min(3,$#previous_texts)]);

      my $guessed = 0;
      for my $text (@candidate_texts)
	{ # my $orig_text = $text;
	  if ($text =~ /^[\"\`\'().?! ]*$/)
	    { next; }
	  if (length($text) <= 2)
	    { next; }
	  # hack for Python manual; maybe defer until failure first time around?
	  $text =~ s/^sys\.//g;
	  for my $iterm (@this_index_entries_broken)
	    { # I could test for zero:  LaTeX2HTML's failures in the Python
	      # documentation are only for items of the form "... (built-in...)"
	      if (index($iterm, $text) != -1)
		{ push @deferred_index_entries, $iterm;
		  # print STDERR "Guessing index term `$iterm' for text `$orig_text'\n";
		  $guessed = 1;
		} } }
      if (!$guessed)
	{ # print STDERR "No guess in `", join("'; `", @this_index_entries_broken), "' for texts:\n `", join("'\n `", @candidate_texts), "'\n";
	}
    }
}


# Need to add calls to this at various places.
# Perhaps add HTML::Element argument and do the check for appropriateness
# here (ie, no action if inside <H1>, etc.).
sub do_deferred_index_entries ()
{ check_args(0, @_);
  if ((scalar(@deferred_index_entries) > 0)
      && (scalar(@index_deferrers) == 0))
    { print TEXI "\n", join("\n", @deferred_index_entries), "\n";
      @deferred_index_entries = (); }
}

my $table_columns;		# undefined if not in a table
my $table_first_column;		# boolean

sub output_body ( $$$ )
{ my ($he, $startflag) = (check_args(3, @_))[0,1]; #  ignore depth argument

  if (!ref $he)
    { my $space_index = index($he, " ");
      if ($space_index != -1)
	{ # Why does
	  #   print TEXI texi_quote(substr($he, 0, $space_index+1));
	  # give:  Can't locate object method "TEXI" via package "texi_quote"
	  # (Because the definition texi_quote hasn't been seen yet.)
	  print TEXI &texi_quote(substr($he, 0, $space_index+1));
	  do_deferred_index_entries();
	  print TEXI &texi_quote(substr($he, $space_index+1)); }
      else
	{ print TEXI &texi_quote($he); }
      return; }

  my $tag = $he->tag();

  # Ordinary text markup first
  if (exists $inline_markup{$tag})
    { if ($startflag)
	{ print TEXI "\@$inline_markup{$tag}\{"; }
      else
	{ print TEXI "\}"; } }
  elsif ($tag eq "a")
    { my ($name, $href, @content) = anchor_info($he);
      if (!$href)
	{ # This anchor is only here for indexing/cross referencing purposes.
	  if ($startflag)
	    { label_add_index_entries($name, $he); }
	}
      elsif ($href =~ "^(ftp|http|news):")
	{ if ($startflag)
	    { # Should avoid second argument if it's identical to the URL.
	      print TEXI "\@uref\{$href, "; }
	  else
	    { print TEXI "\}"; }
	}
      elsif ($href =~ /^\#(foot[0-9]+)$/)
	{ # Footnote
	  if ($startflag)
	    { # Could double-check name and content, but I'm not
	      # currently storing that information.
	      print TEXI "\@footnote\{";
	      $footnotes{$1}->traverse(\&output_body);
	      print TEXI "\}";
	      return 0; } }
      else
	{ if ($startflag)
	    { # cross-references are not active Info links, but no text is lost
	      print STDERR "Can't deal with internal HREF anchors yet:\n";
	      $he->dump; }
	}
    }
  elsif ($tag eq "br")
    { print TEXI "\@\n"; }
  elsif ($tag eq "body")
    { }
  elsif ($tag eq "center")
    { if (has_single_content_string($he)
	  && ($ {$he->content}[0] =~ /^ *$/))
	{ return 0; }
      if ($startflag)
	{ print TEXI "\n\@center\n"; }
      else
	{ print TEXI "\n\@end center\n"; }
    }
  elsif ($tag eq "div")
    { my $align = $he->attr('align');
      if (defined($align) && ($align eq "center"))
	{ if (has_single_content_string($he)
	      && ($ {$he->content}[0] =~ /^ *$/))
	    { return 0; }
	  if ($startflag)
	    { print TEXI "\n\@center\n"; }
	  else
	    { print TEXI "\n\@end center\n"; } }
    }
  elsif ($tag eq "dl")
    { # Recognize "<dl><dd><pre> ... </pre></dl>" paradigm for "@example"
      if (has_single_content_with_tag($he, "dd"))
	{ my $he_dd = $ {$he->content}[0];
	  if (has_single_content_with_tag($he_dd, "pre"))
	    { my $he_pre = $ {$he_dd->content}[0];
	      print_pre($he_pre);
	      return 0; } }
      if ($startflag)
	{ # Could examine the elements, to be cleverer about formatting.
	  # (Also to use ftable, vtable...)
	  print TEXI "\n\@table \@asis\n"; }
      else
	{ print TEXI "\n\@end table\n"; }
    }
  elsif ($tag eq "dt")
    { push_or_pop_index_deferrers($tag, $startflag);
      if ($startflag)
	{ print TEXI "\n\@item "; }
      else
	{ } }
  elsif ($tag eq "dd")
    { if ($startflag)
	{ print TEXI "\n"; }
      else
	{ }
      if (scalar(@index_deferrers) != 0)
	{ $he->dump;
	  die "Unexpected <$tag> while inside: (" . join(" ", @index_deferrers) . "); bad HTML?"; }
      do_deferred_index_entries();
    }
  elsif ($tag =~ /^(font|big|small)$/)
    { # Do nothing for now.
    }
  elsif ($tag =~ /^h[1-6]$/)
    { # We don't need this because we never recursively enter the heading content.
      # push_or_pop_index_deferrers($tag, $startflag);
      my $secname = "";
      my @seclabels = ();
      for my $elt (@{$he->content})
	{ if (!ref $elt)
	    { $secname .= $elt; }
	  elsif ($elt->tag eq "br")
	    { }
	  elsif ($elt->tag eq "a")
	    { my ($name, $href, @acontent) = anchor_info($elt);
              if ($href)
                { $he->dump;
                  $elt->dump;
                  die "Nonsimple anchor in <$tag>"; }
	      if (!defined $name)
		{ die "No NAME for anchor in $tag"; }
	      push @seclabels, $name;
	      for my $subelt (@acontent)
		{ $secname .= html_to_texi($subelt); } }
	  else
	    { $secname .= html_to_texi($elt); } }
      if ($secname eq "")
	{ die "No section name in <$tag>"; }
      if (scalar(@section_stack) == 1)
	{ if ($section_stack[-1] ne "Top")
	    { die "Not top? $section_stack[-1]"; }
	  print TEXI "\@settitle $secname\n";
	  print TEXI "\@c %**end of header\n";
	  print TEXI "\n";
	  print TEXI "\@node Top\n";
	  print TEXI "\n"; }
      else
	{ print TEXI "\n\@node $section_stack[-1]\n";
	  print TEXI "\@$sectionmarker[scalar(@section_stack)-1] ", texi_remove_punctuation($secname), "\n"; }
      for my $seclabel (@seclabels)
	{ label_add_index_entries($seclabel); }
      # This should only happen once per file.
      label_add_index_entries("");
      if (scalar(@index_deferrers) != 0)
	{ $he->dump;
	  die "Unexpected <$tag> while inside: (" . join(" ", @index_deferrers) . "); bad HTML?"; }
      do_deferred_index_entries();
      return 0;
    }
  elsif ($tag eq "hr")
    { }
  elsif ($tag eq "ignore")
    { # Hack for ignored elements
      return 0;
    }
  elsif ($tag eq "li")
    { if ($startflag)
	{ print TEXI "\n\n\@item\n";
	  do_deferred_index_entries(); } }
  elsif ($tag eq "ol")
    { if ($startflag)
	{ print TEXI "\n\@enumerate \@bullet\n"; }
      else
	{ print TEXI "\n\@end enumerate\n"; } }
  elsif ($tag eq "p")
    { if ($startflag)
	{ print TEXI "\n\n"; }
      if (scalar(@index_deferrers) != 0)
	{ $he->dump;
	  die "Unexpected <$tag> while inside: (" . join(" ", @index_deferrers) . "); bad HTML?"; }
      do_deferred_index_entries(); }
  elsif ($tag eq "pre")
    { print_pre($he);
      return 0; }
  elsif ($tag eq "table")
    { # Could also indicate common formatting for first column, or
      # determine relative widths for columns (or determine a prototype row)
      if ($startflag)
	{ if (defined $table_columns)
	    { $he->dump;
	      die "Can't deal with table nested inside $table_columns-column table"; }
	  $table_columns = table_columns($he);
	  if ($table_columns < 2)
	    { $he->dump;
	      die "Column with $table_columns columns?"; }
	  elsif ($table_columns == 2)
	    { print TEXI "\n\@table \@asis\n"; }
	  else
	    { print TEXI "\n\@multitable \@columnfractions";
	      for (my $i=0; $i<$table_columns; $i++)
		{ print TEXI " ", 1.0/$table_columns; }
	      print TEXI "\n"; } }
      else
	{ if ($table_columns == 2)
	    { print TEXI "\n\@end table\n"; }
	  else
	    { print TEXI "\n\@end multitable\n"; }
	  undef $table_columns; } }
  elsif (($tag eq "td") || ($tag eq "th"))
    { if ($startflag)
	{ if ($table_first_column)
	    { print TEXI "\n\@item ";
	      $table_first_column = 0; }
	  elsif ($table_columns > 2)
	    { print TEXI "\n\@tab "; } }
      else
	{ print TEXI "\n"; } }
  elsif ($tag eq "tr")
    { if ($startflag)
	{ $table_first_column = 1; } }
  elsif ($tag eq "ul")
    { if ($startflag)
	{ print TEXI "\n\@itemize \@bullet\n"; }
      else
	{ print TEXI "\n\@end itemize\n"; } }
  else
    { # I used to have a newline before "output_body" here.
      print STDERR "output_body: ignoring <$tag> tag\n";
      $he->dump;
      return 0; }

  return 1;
}

sub print_pre ( $ )
{ my ($he_pre) = check_args(1, @_);
  if (!has_single_content_string($he_pre))
    { die "Multiple or non-string content for <PRE>: ", @{$he_pre->content}; }
  my $pre_content = $ {$he_pre->content}[0];
  print TEXI "\n\@example";
  print TEXI &texi_quote($pre_content);
  print TEXI "\@end example\n";
}

sub table_columns ( $ )
{ my ($table) = check_args(1, @_);
  my $result = 0;
  for my $row (@{$table->content})
    { if ($row->tag ne "tr")
	{ $table->dump;
	  $row->dump;
	  die "Expected <TR> as table row."; }
      $result = max($result, scalar(@{$row->content})); }
  return $result;
}


###########################################################################
### Utilities
###

sub min ( $$ )
{ my ($x, $y) = check_args(2, @_);
  return ($x < $y) ? $x : $y;
}

sub max ( $$ )
{ my ($x, $y) = check_args(2, @_);
  return ($x > $y) ? $x : $y;
}

sub file_to_tree ( $ )
{ my ($file) = check_args(1, @_);

  my $tree = new HTML::TreeBuilder;
  $tree->ignore_unknown(1);
  # $tree->warn(1);
  $tree->parse_file($file);
  cleanup_parse_tree($tree);
  return $tree
}


sub has_single_content ( $ )
{ my ($he) = check_args(1, @_);
  if (!ref $he)
    { # return 0;
      die "Non-reference argument: $he"; }
  my $ref_content = $he->content;
  if (!defined $ref_content)
    { return 0; }
  my @content = @{$ref_content};
  if (scalar(@content) != 1)
    { return 0; }
  return 1;
}


# Return true if the content of the element contains only one element itself,
# and that inner element has the specified tag.
sub has_single_content_with_tag ( $$ )
{ my ($he, $tag) = check_args(2, @_);
  if (!has_single_content($he))
    { return 0; }
  my $content = $ {$he->content}[0];
  if (!ref $content)
    { return 0; }
  my $content_tag = $content->tag;
  if (!defined $content_tag)
    { return 0; }
  return $content_tag eq $tag;
}

sub has_single_content_string ( $ )
{ my ($he) = check_args(1, @_);
  if (!has_single_content($he))
    { return 0; }
  my $content = $ {$he->content}[0];
  if (ref $content)
    { return 0; }
  return 1;
}


# Return name, href, content.  First two may be undefined; third is an array.
# I don't see how to determine if there are more attributes.
sub anchor_info ( $ )
{ my ($he) = check_args(1, @_);
  if ($he->tag ne "a")
    { $he->dump;
      die "passed non-anchor to anchor_info"; }
  my $name = $he->attr('name');
  my $href = $he->attr('href');
  my @content = ();
  { my $ref_content = $he->content;
    if (defined $ref_content)
      { @content = @{$ref_content}; } }
  return ($name, $href, @content);
}


sub texi_quote ( $ )
{ my ($text) = check_args(1, @_);
  $text =~ s/([\@\{\}])/\@$1/g;
  $text =~ s/ -- / --- /g;
  return $text;
}

# Eliminate bad punctuation (that confuses Makeinfo or Info) for section titles.
sub texi_remove_punctuation ( $ )
{ my ($text) = check_args(1, @_);

  $text =~ s/^ +//g;
  $text =~ s/[ :]+$//g;
  $text =~ s/^[1-9][0-9.]* +//g;
  $text =~ s/,//g;
  # Both embedded colons and " -- " confuse makeinfo.  (Perhaps " -- "
  # gets converted into " - ", just as "---" would be converted into " -- ",
  # so the names end up differing.)
  # $text =~ s/:/ -- /g;
  $text =~ s/://g;
  return $text;
}


## Do not use this inside `traverse':  it throws off the traversal.  Use
## html_replace_by_ignore or html_replace_by_meta instead.
# Returns 1 if success, 0 if failure.
sub html_remove ( $;$ )
{ my ($he, $parent) = check_args_range(1, 2, @_);
  if (!defined $parent)
    { $parent = $he->parent; }
  my $ref_pcontent = $parent->content;
  my @pcontent = @{$ref_pcontent};
  for (my $i=0; $i<scalar(@pcontent); $i++)
    { if ($pcontent[$i] eq $he)
	{ splice @{$ref_pcontent}, $i, 1;
	  $he->parent(undef);
	  return 1; } }
  die "Didn't find $he in $parent";
}


sub html_replace ( $$;$ )
{ my ($orig, $new, $parent) = check_args_range(2, 3, @_);
  if (!defined $parent)
    { $parent = $orig->parent; }
  my $ref_pcontent = $parent->content;
  my @pcontent = @{$ref_pcontent};
  for (my $i=0; $i<scalar(@pcontent); $i++)
    { if ($pcontent[$i] eq $orig)
	{ $ {$ref_pcontent}[$i] = $new;
	  $new->parent($parent);
	  $orig->parent(undef);
	  return 1; } }
  die "Didn't find $orig in $parent";
}

sub html_replace_by_meta ( $;$ )
{ my ($orig, $parent) = check_args_range(1, 2, @_);
  my $meta = new HTML::Element "meta";
  if (!defined $parent)
    { $parent = $orig->parent; }
  return html_replace($orig, $meta, $parent);
}

sub html_replace_by_ignore ( $;$ )
{ my ($orig, $parent) = check_args_range(1, 2, @_);
  my $ignore = new HTML::Element "ignore";
  if (!defined $parent)
    { $parent = $orig->parent; }
  return html_replace($orig, $ignore, $parent);
}



###
### Collect text elements
###

my @collected_texts;
my $collect_texts_stoppoint;
my $done_collecting;

sub collect_texts ( $;$ )
{ my ($root, $stop) = check_args_range(1, 2, @_);
  # print STDERR "collect_texts: $root $stop\n";
  $collect_texts_stoppoint = $stop;
  $done_collecting = 0;
  @collected_texts = ();
  $root->traverse(\&collect_if_text); # process texts
  # print STDERR "collect_texts => ", join(";;;", @collected_texts), "\n";
  return @collected_texts;
}

sub collect_if_text ( $$$ )
{ my $he = (check_args(3, @_))[0]; #  ignore depth and startflag arguments
  if ($done_collecting)
    { return 0; }
  if (!defined $he)
    { return 0; }
  if (!ref $he)
    { push @collected_texts, $he;
      return 0; }
  if ((defined $collect_texts_stoppoint) && ($he eq $collect_texts_stoppoint))
    { $done_collecting = 1;
      return 0; }
  return 1;
}


###########################################################################
### Clean up parse tree
###

sub cleanup_parse_tree ( $ )
{ my ($he) = check_args(1, @_);
  $he->traverse(\&delete_if_navigation, 'ignore text');
  $he->traverse(\&delete_extra_spaces, 'ignore text');
  $he->traverse(\&merge_dl, 'ignore text');
  $he->traverse(\&reorder_dt_and_dl, 'ignore text');
  return $he;
}


## Simpler version that deletes contents but not the element itself.
# sub delete_if_navigation ( $$$ )
# { my $he = (check_args(3, @_))[0]; # ignore startflag and depth
#   if (($he->tag() eq "div") && ($he->attr('class') eq 'navigation'))
#     { $he->delete();
#       return 0; }
#   else
#     { return 1; }
# }

sub delete_if_navigation ( $$$ )
{ my ($he, $startflag) = (check_args(3, @_))[0,1]; #  ignore depth argument
  if (!$startflag)
    { return; }

  if (($he->tag() eq "div") && (defined $he->attr('class')) && ($he->attr('class') eq 'navigation'))
    { my $ref_pcontent = $he->parent()->content();
      # Don't try to modify @pcontent, which appears to be a COPY.
      # my @pcontent = @{$ref_pcontent};
      for (my $i = 0; $i<scalar(@{$ref_pcontent}); $i++)
	{ if (${$ref_pcontent}[$i] eq $he)
	    { splice(@{$ref_pcontent}, $i, 1);
	      last; } }
      $he->delete();
      return 0; }
  else
    { return 1; }
}

sub delete_extra_spaces ( $$$ )
{ my ($he, $startflag) = (check_args(3, @_))[0,1]; #  ignore depth argument
  if (!$startflag)
    { return; }

  my $tag = $he->tag;
  if ($tag =~ /^(head|html|table|tr|ul)$/)
    { delete_child_spaces($he); }
  delete_trailing_spaces($he);
  return 1;
}


sub delete_child_spaces ( $ )
{ my ($he) = check_args(1, @_);
  my $ref_content = $he->content();
  for (my $i = 0; $i<scalar(@{$ref_content}); $i++)
    { if ($ {$ref_content}[$i] =~ /^ *$/)
	{ splice(@{$ref_content}, $i, 1);
	  $i--; } }
}

sub delete_trailing_spaces ( $ )
{ my ($he) = check_args(1, @_);
  my $ref_content = $he->content();
  if (! defined $ref_content)
    { return; }
  # Could also check for previous element = /^h[1-6]$/.
  for (my $i = 0; $i<scalar(@{$ref_content})-1; $i++)
    { if ($ {$ref_content}[$i] =~ /^ *$/)
	{ my $next_elt = $ {$ref_content}[$i+1];
	  if ((ref $next_elt) && ($next_elt->tag =~ /^(br|dd|dl|dt|hr|p|ul)$/))
	    { splice(@{$ref_content}, $i, 1);
	      $i--; } } }
  if ($he->tag =~ /^(dd|dt|^h[1-6]|li|p)$/)
    { my $last_elt = $ {$ref_content}[$#{$ref_content}];
      if ((defined $last_elt) && ($last_elt =~ /^ *$/))
	{ pop @{$ref_content}; } }
}


# LaTeX2HTML sometimes creates
#   <DT>text
#   <DL COMPACT><DD>text
# which should actually be:
#   <DL COMPACT>
#   <DT>text
#   <DD>text
# Since a <DL> gets added, this ends up looking like
# <P>
#   <DL>
#     <DT>
#       text1...
#       <DL COMPACT>
#         <DD>
#           text2...
#         dt_or_dd1...
#     dt_or_dd2...
# which should become
# <P>
#   <DL COMPACT>
#     <DT>
#       text1...
#     <DD>
#       text2...
#     dt_or_dd1...
#     dt_or_dd2...

sub reorder_dt_and_dl ( $$$ )
{ my ($he, $startflag) = (check_args(3, @_))[0,1]; #  ignore depth argument
  if (!$startflag)
    { return; }

  if ($he->tag() eq "p")
    { my $ref_pcontent = $he->content();
      if (defined $ref_pcontent)
	{ my @pcontent = @{$ref_pcontent};
	  # print "reorder_dt_and_dl found a <p>\n"; $he->dump();
	  if ((scalar(@pcontent) >= 1)
	      && (ref $pcontent[0]) && ($pcontent[0]->tag() eq "dl")
	      && $pcontent[0]->implicit())
	    { my $ref_dlcontent = $pcontent[0]->content();
	      # print "reorder_dt_and_dl found a <p> and implicit <dl>\n";
	      if (defined $ref_dlcontent)
		{ my @dlcontent = @{$ref_dlcontent};
		  if ((scalar(@dlcontent) >= 1)
		      && (ref $dlcontent[0]) && ($dlcontent[0]->tag() eq "dt"))
		    { my $ref_dtcontent = $dlcontent[0]->content();
		      # print "reorder_dt_and_dl found a <p>, implicit <dl>, and <dt>\n";
		      if (defined $ref_dtcontent)
			{ my @dtcontent = @{$ref_dtcontent};
			  if ((scalar(@dtcontent) > 0)
			      && (ref $dtcontent[$#dtcontent])
			      && ($dtcontent[$#dtcontent]->tag() eq "dl"))
			    { my $ref_dl2content = $dtcontent[$#dtcontent]->content();
			      # print "reorder_dt_and_dl found a <p>, implicit <dl>, <dt>, and <dl>\n";
			      if (defined $ref_dl2content)
				{ my @dl2content = @{$ref_dl2content};
				  if ((scalar(@dl2content) > 0)
				      && (ref ($dl2content[0]))
				      && ($dl2content[0]->tag() eq "dd"))
			    {
			      # print "reorder_dt_and_dl found a <p>, implicit <dl>, <dt>, <dl>, and <dd>\n";
			      # print STDERR "CHANGING\n"; $he->dump();
			      html_replace_by_ignore($dtcontent[$#dtcontent]);
			      splice(@{$ref_dlcontent}, 1, 0, @dl2content);
			      # print STDERR "CHANGED TO:\n"; $he->dump();
			      return 0; # don't traverse children
			    } } } } } } } } }
  return 1;
}


# If we find a paragraph that looks like
# <P>
#   <HR>
#   <UL>
# then accumulate its links into a contents_list and delete the paragraph.
sub process_if_child_links ( $$$ )
{ my ($he, $startflag) = (check_args(3, @_))[0,1]; #  ignore depth argument
  if (!$startflag)
    { return; }

  if ($he->tag() eq "p")
    { my $ref_content = $he->content();
      if (defined $ref_content)
	{ my @content = @{$ref_content};
	  if ((scalar(@content) == 2)
	      && (ref $content[0]) && $content[0]->tag() eq "hr"
	      && (ref $content[1]) && $content[1]->tag() eq "ul")
	    { process_child_links($he);
	      $he->delete();
	      return 0; } } }
  return 1;
}


# If we find
#     <H4>
#       "Footnotes"
#     <DL>
#       <DT>
#         <A NAME="foot560">
#           "...borrow"
#         <A HREF="refcountsInPython.html#tex2html2" NAME="foot560">
#           "1.2"
#       <DD>
#         "The metaphor of ``borrowing'' a reference is not completely correct: the owner still has a copy of the reference. "
#       ...
# then record the footnote information and delete the section and list.

my $process_if_footnotes_expect_dl_next = 0;

sub process_if_footnotes ( $$$ )
{ my ($he, $startflag) = (check_args(3, @_))[0,1]; #  ignore depth argument
  if (!$startflag)
    { return; }

  if (($he->tag() eq "h4")
      && has_single_content_string($he)
      && ($ {$he->content}[0] eq "Footnotes"))
    { html_replace_by_ignore($he);
      $process_if_footnotes_expect_dl_next = 1;
      return 0; }

  if ($process_if_footnotes_expect_dl_next && ($he->tag() eq "dl"))
    { my $ref_content = $he->content();
      if (defined $ref_content)
	{ $process_if_footnotes_expect_dl_next = 0;
	  my @content = @{$ref_content};
	  for (my $i=0; $i<$#content; $i+=2)
	    { my $he_dt = $content[$i];
	      my $he_dd = $content[$i+1];
	      if (($he_dt->tag ne "dt") || ($he_dd->tag ne "dd"))
		{ $he->dump;
		  die "expected <DT> and <DD> at positions $i and ", $i+1; }
	      my @dt_content = @{$he_dt->content()};
	      if ((scalar(@dt_content) != 2)
		  || ($dt_content[0]->tag ne "a")
		  || ($dt_content[1]->tag ne "a"))
		{ $he_dt->dump;
		  die "Expected 2 anchors as content of <DT>"; }
	      my ($dt1_name, $dt1_href, $dt1_content) = anchor_info($dt_content[0]);
	      my ($dt2_name, $dt2_href, $dt2_content) = anchor_info($dt_content[0]);
	      # unused: $dt1_href, $dt1_content, $dt2_href, $dt2_content
	      if ($dt1_name ne $dt2_name)
		{ $he_dt->dump;
		  die "Expected identical names for anchors"; }
	      html_replace_by_ignore($he_dd);
	      $he_dd->tag("div"); # has no effect
	      $footnotes{$dt1_name} = $he_dd; }
	  html_replace_by_ignore($he);
	  return 0; } }

  if ($process_if_footnotes_expect_dl_next)
    { $he->dump;
      die "Expected <DL> for footnotes next"; }

  return 1;
}



## Merge two adjacent paragraphs containing <DL> items, such as:
#     <P>
#       <DL>
#         <DT>
#           ...
#         <DD>
#           ...
#     <P>
#       <DL>
#         <DT>
#           ...
#         <DD>
#           ...

sub merge_dl ( $$$ )
{ my ($he, $startflag) = (check_args(3, @_))[0,1]; #  ignore depth argument
  if (!$startflag)
    { return; }

  my $ref_content = $he->content;
  if (!defined $ref_content)
    { return; }
  my $i = 0;
  while ($i < scalar(@{$ref_content})-1)
    { my $p1 = $ {$ref_content}[$i];
      if ((ref $p1) && ($p1->tag eq "p")
	  && has_single_content_with_tag($p1, "dl"))
	{ my $dl1 = $ {$p1->content}[0];
	  # In this loop, rhs, not lhs, of < comparison changes,
	  # because we are removing elements from the content of $he.
	  while ($i < scalar(@{$ref_content})-1)
	    { my $p2 = $ {$ref_content}[$i+1];
	      if (!((ref $p2) && ($p2->tag eq "p")
		    && has_single_content_with_tag($p2, "dl")))
		{ last; }
	      # Merge these two elements.
	      splice(@{$ref_content}, $i+1, 1); # remove $p2
	      my $dl2 = $ {$p2->content}[0];
	      $dl1->push_content(@{$dl2->content}); # put $dl2's content in $dl1
	    }
	  # extra increment because next element isn't a candidate for $p1
	  $i++; }
      $i++; }
  return 1;
}



###########################################################################
### Testing
###

sub test ( $$ )
{ my ($action, $file) = check_args(2, @_);

  # General testing
  if (($action eq "view") || ($action eq ""))
    { # # $file = "/homes/gws/mernst/www/links.html";
      # # $file = "/homes/gws/mernst/www/index.html";
      # # $file = "/homes/fish/mernst/java/gud/doc/manual.html";
      # # $file = "/projects/cecil/cecil/doc/manuals/stdlib-man/stdlib/stdlib.html";
      # # $file = "/homes/fish/mernst/tmp/python-doc/html/index.html";
      # $file = "/homes/fish/mernst/tmp/python-doc/html/api/complexObjects.html";
      my $tree = file_to_tree($file);

      ## Testing
      # print STDERR $tree->as_HTML;
      $tree->dump();

      # print STDERR $tree->tag(), "\n";
      # print STDERR @{$tree->content()}, "\n";
      # 
      # for (@{ $tree->extract_links(qw(a img)) }) {
      #   my ($link, $linkelem) = @$_;
      #   print STDERR "$link ", $linkelem->as_HTML;
      #   }
      # 
      # print STDERR @{$tree->extract_links()}, "\n";

      # my @top_level_elts = @{$tree->content()};

      # if scalar(@{$tree->content()})
      return;
    }

  elsif ($action eq "raw")
    { my $tree = new HTML::TreeBuilder;
      $tree->ignore_unknown(1);
      # $tree->warn(1);
      $tree->parse_file($file);

      $tree->dump();

      # cleanup_parse_tree($tree);
      # $tree->dump();
      return;
    }

  # Test dealing with a section.
  elsif ($action eq "section")
    { # my $file;
      # $file = "/homes/fish/mernst/tmp/python-doc/html/api/intro.html";
      # $file = "/homes/fish/mernst/tmp/python-doc/html/api/includes.html";
      # $file = "/homes/fish/mernst/tmp/python-doc/html/api/complexObjects.html";
      process_section_file($file, 0, "Title");
    }

  # Test dealing with many sections
  elsif (0)
    { my @files = ("/homes/fish/mernst/tmp/python-doc/html/api/about.html",
		   "/homes/fish/mernst/tmp/python-doc/html/api/abstract.html",
		   "/homes/fish/mernst/tmp/python-doc/html/api/api.html",
		   "/homes/fish/mernst/tmp/python-doc/html/api/cObjects.html",
		   "/homes/fish/mernst/tmp/python-doc/html/api/complexObjects.html",
		   "/homes/fish/mernst/tmp/python-doc/html/api/concrete.html",
		   # "/homes/fish/mernst/tmp/python-doc/html/api/contents.html",
		   "/homes/fish/mernst/tmp/python-doc/html/api/countingRefs.html",
		   "/homes/fish/mernst/tmp/python-doc/html/api/debugging.html",
		   "/homes/fish/mernst/tmp/python-doc/html/api/dictObjects.html",
		   "/homes/fish/mernst/tmp/python-doc/html/api/embedding.html",
		   "/homes/fish/mernst/tmp/python-doc/html/api/exceptionHandling.html",
		   "/homes/fish/mernst/tmp/python-doc/html/api/exceptions.html",
		   "/homes/fish/mernst/tmp/python-doc/html/api/fileObjects.html",
		   "/homes/fish/mernst/tmp/python-doc/html/api/floatObjects.html",
		   "/homes/fish/mernst/tmp/python-doc/html/api/front.html",
		   "/homes/fish/mernst/tmp/python-doc/html/api/fundamental.html",
		   # "/homes/fish/mernst/tmp/python-doc/html/api/genindex.html",
		   "/homes/fish/mernst/tmp/python-doc/html/api/importing.html",
		   "/homes/fish/mernst/tmp/python-doc/html/api/includes.html",
		   "/homes/fish/mernst/tmp/python-doc/html/api/index.html",
		   "/homes/fish/mernst/tmp/python-doc/html/api/initialization.html",
		   "/homes/fish/mernst/tmp/python-doc/html/api/intObjects.html",
		   "/homes/fish/mernst/tmp/python-doc/html/api/intro.html",
		   "/homes/fish/mernst/tmp/python-doc/html/api/listObjects.html",
		   "/homes/fish/mernst/tmp/python-doc/html/api/longObjects.html",
		   "/homes/fish/mernst/tmp/python-doc/html/api/mapObjects.html",
		   "/homes/fish/mernst/tmp/python-doc/html/api/mapping.html",
		   "/homes/fish/mernst/tmp/python-doc/html/api/newTypes.html",
		   "/homes/fish/mernst/tmp/python-doc/html/api/node24.html",
		   "/homes/fish/mernst/tmp/python-doc/html/api/noneObject.html",
		   "/homes/fish/mernst/tmp/python-doc/html/api/number.html",
		   "/homes/fish/mernst/tmp/python-doc/html/api/numericObjects.html",
		   "/homes/fish/mernst/tmp/python-doc/html/api/object.html",
		   "/homes/fish/mernst/tmp/python-doc/html/api/objects.html",
		   "/homes/fish/mernst/tmp/python-doc/html/api/os.html",
		   "/homes/fish/mernst/tmp/python-doc/html/api/otherObjects.html",
		   "/homes/fish/mernst/tmp/python-doc/html/api/processControl.html",
		   "/homes/fish/mernst/tmp/python-doc/html/api/refcountDetails.html",
		   "/homes/fish/mernst/tmp/python-doc/html/api/refcounts.html",
		   "/homes/fish/mernst/tmp/python-doc/html/api/sequence.html",
		   "/homes/fish/mernst/tmp/python-doc/html/api/sequenceObjects.html",
		   "/homes/fish/mernst/tmp/python-doc/html/api/standardExceptions.html",
		   "/homes/fish/mernst/tmp/python-doc/html/api/stringObjects.html",
		   "/homes/fish/mernst/tmp/python-doc/html/api/threads.html",
		   "/homes/fish/mernst/tmp/python-doc/html/api/tupleObjects.html",
		   "/homes/fish/mernst/tmp/python-doc/html/api/typeObjects.html",
		   "/homes/fish/mernst/tmp/python-doc/html/api/types.html",
		   "/homes/fish/mernst/tmp/python-doc/html/api/utilities.html",
		   "/homes/fish/mernst/tmp/python-doc/html/api/veryhigh.html");
      for my $file (@files)
	{ print STDERR "\n", "=" x 75, "\n", "$file:\n";
	  process_section_file($file, 0, "Title");
	}
    }

  # Test dealing with index.
  elsif ($action eq "index")
    { # my $file;
      # $file = "/homes/fish/mernst/tmp/python-doc/html/api/genindex.html";

      process_index_file($file, "\@cindex");
      print_index_info();
    }

  else
    { die "Unrecognized action `$action'"; }
}


###########################################################################
### Main loop
###

sub process_contents_file ( $ )
{ my ($file) = check_args(1, @_);

  # could also use File::Basename
  my $info_file = $file;
  $info_file =~ s/(\/?index)?\.html$//;
  if ($info_file eq "")
    { chomp($info_file = `pwd`); }
  $info_file =~ s/^.*\///;	# not the most efficient way to remove dirs

  $html_directory = $file;
  $html_directory =~ s/(\/|^)[^\/]+$/$1/;

  my $texi_file = "$info_file.texi";
  open(TEXI, ">$texi_file");

  print TEXI "\\input texinfo   \@c -*-texinfo-*-\n";
  print TEXI "\@c %**start of header\n";
  print TEXI "\@setfilename $info_file\n";

  # 2. Summary Description and Copyright
  #      The "Summary Description and Copyright" segment describes the
  #      document and contains the copyright notice and copying permissions
  #      for the Info file.  The segment must be enclosed between `@ifinfo'
  #      and `@end ifinfo' commands so that the formatters place it only in
  #      the Info file.
  # 
  # The summary description and copyright segment does not appear in the
  # printed document.
  # 
  #      @ifinfo
  #      This is a short example of a complete Texinfo file.
  #      
  #      Copyright @copyright{} 1990 Free Software Foundation, Inc.
  #      @end ifinfo


  # 3. Title and Copyright
  #      The "Title and Copyright" segment contains the title and copyright
  #      pages and copying permissions for the printed manual.  The segment
  #      must be enclosed between `@titlepage' and `@end titlepage'
  #      commands.  The title and copyright page appear only in the printed
  #      manual.
  # 
  # The titlepage segment does not appear in the Info file.
  # 
  #      @titlepage
  #      @sp 10
  #      @comment The title is printed in a large font.
  #      @center @titlefont{Sample Title}
  #      
  #      @c The following two commands start the copyright page.
  #      @page
  #      @vskip 0pt plus 1filll
  #      Copyright @copyright{} 1990 Free Software Foundation, Inc.
  #      @end titlepage


  # 4. `Top' Node and Master Menu
  #      The "Master Menu" contains a complete menu of all the nodes in the
  #      whole Info file.  It appears only in the Info file, in the `Top'
  #      node.
  # 
  # The `Top' node contains the master menu for the Info file.  Since a
  # printed manual uses a table of contents rather than a menu, the master
  # menu appears only in the Info file.
  # 
  #      @node    Top,       First Chapter, ,         (dir)
  #      @comment node-name, next,          previous, up
  # 
  #      @menu
  #      * First Chapter::    The first chapter is the
  #                           only chapter in this sample.
  #      * Concept Index::    This index has two entries.
  #      @end menu



  $current_ref_tdf = [ "Top", 0, $ARGV[0] ];
  process_section_file($file, 0, "Top");
  while (scalar(@contents_list))
  { $current_ref_tdf = shift @contents_list;
    process_section_file($ {$current_ref_tdf}[2], $ {$current_ref_tdf}[1], $ {$current_ref_tdf}[0]);
  }

  print TEXI "\n";
  for my $indextitle (@index_titles)
    { print TEXI "\@node $indextitle\n";
      print TEXI "\@unnumbered $indextitle\n";
      print TEXI "\@printindex $ {$index_info{$indextitle}}[1]\n";
      print TEXI "\n"; }

  print TEXI "\@contents\n";
  print TEXI "\@bye\n";
  close(TEXI);
}

# This needs to be last so global variable initializations are reached.

if (scalar(@ARGV) == 0)
{ die "No arguments supplied to html2texi.pl"; }

if ($ARGV[0] eq "-test")
{ my @test_args = @ARGV[1..$#ARGV];
  if (scalar(@test_args) == 0)
    { test("", "index.html"); }
  elsif (scalar(@test_args) == 1)
    { test("", $test_args[0]); }
  elsif (scalar(@test_args) == 2)
    { test($test_args[0], $test_args[1]); }
  else
    { die "Too many test arguments passed to html2texi: ", join(" ", @ARGV); }
  exit();
}

if (scalar(@ARGV) != 1)
{ die "Pass one argument, the main/contents page"; }

process_contents_file($ARGV[0]);

# end of html2texi.pl
