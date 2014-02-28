#!/usr/bin/perl
use strict;

my $bot1 = shift;
my $bot2 = shift;

for my $i (1..10) {
    my $x1 = int(rand(30));
    my $y1 = int(rand(20));
    my $x2 = int(rand(30));
    my $y2 = int(rand(20));

    print `./fight.pl $bot1 $bot2 $x1 $y1 $x2 $y2 2>out`;
    print `./fight.pl $bot2 $bot1 $x1 $y1 $x2 $y2 2>out`;
}
