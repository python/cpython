package SynopsisTable;

sub new{
    return bless {names=>'', info=>{}, file=>''};
}

sub declare{
    my($self,$name,$key,$type) = @_;
    if ($self->{names}) {
        $self->{names} .= ",$name";
    }
    else {
        $self->{names} .= "$name";
    }
    $self->{info}{$name} = "$key,$type,";
}

# The 'file' attribute is used to store the filename of the node in which
# the table will be presented; this assumes that each table will be presented
# only once, which works for the current use of this object.

sub set_file{
    my($self, $filename) = @_;
    $self->{file} = "$filename";
}

sub get_file{
    my $self = shift;
    return $self->{file};
}

sub set_synopsis{
    my($self,$name,$synopsis) = @_;
    my($key,$type,$unused) = split ',', $self->{info}{$name}, 3;
    $self->{info}{$name} = "$key,$type,$synopsis";
}

sub get{
    my($self,$name) = @_;
    return split /,/, $self->{info}{$name}, 3;
}

sub show{
    my $self = shift;
    my $name;
    print "names: ", $self->{names}, "\n\n";
    foreach $name (split /,/, $self->{names}) {
        my($key,$type,$synopsis) = $self->get($name);
        print "$name($key) is $type: $synopsis\n";
    }
}

sub tohtml{
    my $self = shift;
    my $oddrow = 1;
    my $data = "<table class='synopsistable' valign='baseline'>\n";
    my $name;
    foreach $name (split /,/, $self->{names}) {
        my($key,$type,$synopsis) = $self->get($name);
        my $link = "<a href='module-$key.html'>";
        $synopsis =~ s/<tex2html_percent_mark>/%/g;
        $synopsis =~ s/<tex2html_ampersand_mark>/\&amp;/g;
        $data .= ('  <tr'
                  . ($oddrow ? " class='oddrow'>\n      " : '>')
                  . "<td><b><tt class='module'>$link$name</a></tt></b></td>\n"
                  . "      <td>\&nbsp;</td>\n"
                  . "      <td class='synopsis'>$synopsis</td></tr>\n");
        $oddrow = !$oddrow;
    }
    $data .= "</table>\n";
    $data;
}


package testSynopsisTable;

sub test{
    # this little test is mostly to debug the stuff above, since this is
    # my first Perl "object".
    my $st = SynopsisTable->new();
    $st->declare("sample", "sample", "standard");
    $st->set_synopsis("sample", "This is a little synopsis....");
    $st->declare("copy_reg", "copyreg", "standard");
    $st->set_synopsis("copy_reg", "pickle support stuff");
    $st->show();

    print "\n\n";

    my $st2 = SynopsisTable->new();
    $st2->declare("st2module", "st2module", "built-in");
    $st2->set_synopsis("st2module", "silly little synopsis");
    $st2->show();
}

1;      # This must be the last line -- Perl is bogus!
