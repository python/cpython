package SynopsisTable;

sub new{
    return bless {names=>'', info=>{}};
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
    my $data = "<dl>\n";
    my $name;
    foreach $name (split /,/, $self->{names}) {
	my($key,$type,$synopsis) = $self->get($name);
	$data .= "<dt><b><tt>$name</tt></b>\n<dd>$synopsis\n";
    }
    $data .= "</dl>\n";
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

1;	# This must be the last line -- Perl is bogus!
