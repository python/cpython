#!/uns/bin/perl

package checkargs;
require 5.004;			# uses "for my $var"
require Exporter;
@ISA = qw(Exporter);
@EXPORT = qw(check_args check_args_range check_args_at_least);
use strict;
use Carp;

=head1 NAME

checkargs -- Provide rudimentary argument checking for perl5 functions

=head1 SYNOPSIS

  check_args(cArgsExpected, @_)
  check_args_range(cArgsMin, cArgsMax, @_)
  check_args_at_least(cArgsMin, @_)
where "@_" should be supplied literally.

=head1 DESCRIPTION

As the first line of user-written subroutine foo, do one of the following:

  my ($arg1, $arg2) = check_args(2, @_);
  my ($arg1, @rest) = check_args_range(1, 4, @_);
  my ($arg1, @rest) = check_args_at_least(1, @_);
  my @args = check_args_at_least(0, @_);

These functions may also be called for side effect (put a call to one
of the functions near the beginning of the subroutine), but using the
argument checkers to set the argument list is the recommended usage.

The number of arguments and their definedness are checked; if the wrong
number are received, the program exits with an error message.

=head1 AUTHOR

Michael D. Ernst <F<mernst@cs.washington.edu>>

=cut

## Need to check that use of caller(1) really gives desired results.
## Need to give input chunk information.
## Is this obviated by Perl 5.003's declarations?  Not entirely, I think.

sub check_args ( $@ )
{
  my ($num_formals, @args) = @_;
  my ($pack, $file_arg, $line_arg, $subname, $hasargs, $wantarr) = caller(1);
  if (@_ < 1) { croak "check_args needs at least 7 args, got ", scalar(@_), ": @_\n "; }
  if ((!wantarray) && ($num_formals != 0))
    { croak "check_args called in scalar context"; }
  # Can't use croak below here: it would only go out to caller, not its caller
  my $num_actuals = @args;
  if ($num_actuals != $num_formals)
    { die "$file_arg:$line_arg: function $subname expected $num_formals argument",
      (($num_formals == 1) ? "" : "s"),
      ", got $num_actuals",
      (($num_actuals == 0) ? "" : ": @args"),
      "\n"; }
  for my $index (0..$#args)
    { if (!defined($args[$index]))
	{ die "$file_arg:$line_arg: function $subname undefined argument ", $index+1, ": @args[0..$index-1]\n"; } }
  return @args;
}

sub check_args_range ( $$@ )
{
  my ($min_formals, $max_formals, @args) = @_;
  my ($pack, $file_arg, $line_arg, $subname, $hasargs, $wantarr) = caller(1);
  if (@_ < 2) { croak "check_args_range needs at least 8 args, got ", scalar(@_), ": @_"; }
  if ((!wantarray) && ($max_formals != 0) && ($min_formals !=0) )
    { croak "check_args_range called in scalar context"; }
  # Can't use croak below here: it would only go out to caller, not its caller
  my $num_actuals = @args;
  if (($num_actuals < $min_formals) || ($num_actuals > $max_formals))
    { die "$file_arg:$line_arg: function $subname expected $min_formals-$max_formals arguments, got $num_actuals",
      ($num_actuals == 0) ? "" : ": @args", "\n"; }
  for my $index (0..$#args)
    { if (!defined($args[$index]))
	{ die "$file_arg:$line_arg: function $subname undefined argument ", $index+1, ": @args[0..$index-1]\n"; } }
  return @args;
}

sub check_args_at_least ( $@ )
{
  my ($min_formals, @args) = @_;
  my ($pack, $file_arg, $line_arg, $subname, $hasargs, $wantarr) = caller(1);
  # Don't do this, because we want every sub to start with a call to check_args*
  # if ($min_formals == 0)
  #   { die "Isn't it pointless to check for at least zero args to $subname?\n"; }
  if (scalar(@_) < 1)
    { croak "check_args_at_least needs at least 1 arg, got ", scalar(@_), ": @_"; }
  if ((!wantarray) && ($min_formals != 0))
    { croak "check_args_at_least called in scalar context"; }
  # Can't use croak below here: it would only go out to caller, not its caller
  my $num_actuals = @args;
  if ($num_actuals < $min_formals)
    { die "$file_arg:$line_arg: function $subname expected at least $min_formals argument",
      ($min_formals == 1) ? "" : "s",
      ", got $num_actuals",
      ($num_actuals == 0) ? "" : ": @args", "\n"; }
  for my $index (0..$#args)
    { if (!defined($args[$index]))
	{ warn "$file_arg:$line_arg: function $subname undefined argument ", $index+1, ": @args[0..$index-1]\n"; last; } }
  return @args;
}

1;				# successful import
__END__
