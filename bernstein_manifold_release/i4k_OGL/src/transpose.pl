#! /usr/bin/perl -w

use strict;
use warnings;

my @lines = ();
open(my $instfile, "instruments.txt");
while (my $line = <$instfile>) {
	chomp($line);
	chomp($line);
	push(@lines, $line);
}

for (my $param = 0; $param < 25; $param++) {
	for (my $inst = 0; $inst < 18; $inst++) {
		my @args = split(" ", $lines[$inst]);
		print "$args[$param], ";
	}
	print "\n";
}