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

# collect labels that have been used
%newnames = ();

while (<>) {
  # don't want to do one s/// per line per node
  # so look for lines with hrefs, then do s/// on nodes present
  if (/HREF=\"([^\#\"]*)html[\#\"]/) {
    @parts = split(/HREF\=\"/);
    shift @parts;
    for $node (@parts) {
      $node =~ s/[\#\"].*$//g;
      chop($node);
      if (defined($nodes{$node})) {
	$label = $nodes{$node};
	if (s/HREF=\"$node([\#\"])/HREF=\"$label.html$1/g) {
	  s/HREF=\"$label.html#SECTION\d+/HREF=\"$label.html/g;
	  $newnames{$node} = "$label.html";
	}
      }
    }
  }
  print;
}

foreach $oldname (keys %newnames) {
# or ln -s
  system("mv $oldname $newnames{$oldname}");
}
