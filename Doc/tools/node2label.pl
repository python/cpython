#! /usr/bin/env perl

use English;
$INPLACE_EDIT = '';

# read the labels, then reverse the mappings
require "labels.pl";

%nodes = ();
my $key;
# sort so that we get a consistent assignment for nodes with multiple labels 
foreach $label (sort keys %external_labels) {
  $key = $external_labels{$label};
  $key =~ s|^/||;
  $nodes{$key} = $label;
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
  if (/(HREF|href)=[\"\']([^\#\"\']*)html[\#\"\']/) {
    @parts = split(/(HREF|href)\=[\"\']/);
    shift @parts;
    for $node (@parts) {
      $node =~ s/[\#\"\'].*$//g;
      chop($node);
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
