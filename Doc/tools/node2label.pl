#! /usr/bin/env perl

# On Cygwin, we actually have to generate a temporary file when doing
# the inplace edit, or we'll get permission errors.  Not sure who's
# bug this is, except that it isn't ours.  To deal with this, we
# generate backups during the edit phase and remove them at the end.
#
use English;
$INPLACE_EDIT = '.bak';

# read the labels, then reverse the mappings
require "labels.pl";

%nodes = ();
my $key;
# sort so that we get a consistent assignment for nodes with multiple labels 
foreach $label (sort keys %external_labels) {
  #
  # If the label can't be used as a filename on non-Unix platforms,
  # skip it.  Such labels may be used internally within the documentation,
  # but will never be used for filename generation.
  #
  if ($label =~ /^([-.a-zA-Z0-9]+)$/) {
    $key = $external_labels{$label};
    $key =~ s|^/||;
    $nodes{$key} = $label;
  }
}

# This adds the "internal" labels added for indexing.  These labels will not
# be used for file names.
require "intlabels.pl";
foreach $label (keys %internal_labels) {
  $key = $internal_labels{$label};
  $key =~ s|^/||;
  if (defined($nodes{$key})) {
    $nodes{$label} = $nodes{$key};
  }
}

# collect labels that have been used
%newnames = ();

while (<>) {
  # don't want to do one s/// per line per node
  # so look for lines with hrefs, then do s/// on nodes present
  if (/(HREF|href)=[\"\']node\d+\.html[\#\"\']/) {
    @parts = split(/(HREF|href)\=[\"\']/);
    shift @parts;
    for $node (@parts) {
      $node =~ s/[\#\"\'].*$//g;
      chomp($node);
      if (defined($nodes{$node})) {
	$label = $nodes{$node};
	if (s/(HREF|href)=([\"\'])$node([\#\"\'])/href=$2$label.html$3/g) {
	  s/(HREF|href)=([\"\'])$label.html/href=$2$label.html/g;
	  $newnames{$node} = "$label.html";
	}
      }
    }
  }
  print;
}

foreach $oldname (keys %newnames) {
  rename($oldname, $newnames{$oldname});
}

foreach $filename (glob('*.bak')) {
  unlink($filename);
}
