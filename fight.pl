#!/usr/bin/perl

use strict;
use IPC::Open2;

my $count = scalar @ARGV;
die unless $count;

my @bots;

for my $bot (@ARGV) {
    my $pid = open2(my $out, my $in, $bot);
    my $x = int(rand(30));
    my $y = int(rand(20));

    push @bots, {
        bot => $bot,
        pid => $pid,
        in => $in,
        out => $out,
        hx => $x,
        hy => $y,
        tx => $x,
        ty => $y
    };
}

while (1) {
    my $i = 0;
    for my $bot (@bots) {
        print { $bot->{in} } "$count $i\n";
        for my $b (@bots) {
            print { $bot->{in} } "$b->{tx} $b->{ty} $b->{hx} $b->{hy}\n";
        }
        my $move = readline($bot->{out});
        chomp $move;

        if ($move eq "GULP") {
            print "$bot->{bot} lost\n";
            exit;
        } elsif ($move eq "LEFT") {
            $bot->{hx}--;
        } elsif ($move eq "RIGHT") {
            $bot->{hx}++;
        } elsif ($move eq "UP") {
            $bot->{hy}--;
        } elsif ($move eq "DOWN") {
            $bot->{hy}++;
        }
        $i++;
    }
}
