#!/usr/bin/env perl

use warnings;
use strict;
use File::Temp qw/tempfile/;
use Getopt::Long "GetOptions";
use File::Basename;
use FindBin qw($RealBin);
use Cwd 'abs_path';

my $TMPDIR = "tmp";
my $SCHEME = "D2";
my $KEEP_TMP = 0;
my $MADA_DIR;

GetOptions(
  "scheme=s" => \$SCHEME,
  "tmpdir=s" => \$TMPDIR,
  "keep-tmp" => \$KEEP_TMP,
  "mada-dir=s" => \$MADA_DIR
    ) or die("ERROR: unknown options");

$TMPDIR = abs_path($TMPDIR);
print STDERR "TMPDIR=$TMPDIR \n";

#binmode(STDIN, ":utf8");
#binmode(STDOUT, ":utf8");

$TMPDIR = "$TMPDIR/madamira.$$";
`mkdir -p $TMPDIR`;
`mkdir -p $TMPDIR/split`;
`mkdir -p $TMPDIR/out`;

my $infile = "$TMPDIR/input";
print STDERR $infile."\n";

open(TMP,">$infile");
while(<STDIN>) { 
    print TMP $_;
}
close(TMP);

my $cmd;

# split input file
my $SPLIT_EXEC = `gsplit --help 2>/dev/null`; 
if($SPLIT_EXEC) {
    $SPLIT_EXEC = 'gsplit';
}
else {
    $SPLIT_EXEC = 'split';
}

$cmd = "$SPLIT_EXEC -l 100 -a 7 -d  $TMPDIR/input $TMPDIR/split/x";
`$cmd`;

$cmd = "cd $MADA_DIR && parallel --jobs 5 java -Xmx2500m -Xms2500m -XX:NewRatio=3 -jar $MADA_DIR/MADAMIRA.jar -rawinput {} -rawoutdir  $TMPDIR/out -rawconfig $MADA_DIR/samples/sampleConfigFile.xml  ::: $TMPDIR/split/x*";
print STDERR "Executing: $cmd\n";
`$cmd`;

$cmd = "cat $TMPDIR/out/x*.mada > $infile.mada";
print STDERR "Executing: $cmd\n";
`$cmd`;

# get stuff out of mada output
open(MADA_OUT,"<$infile.mada");
#binmode(MADA_OUT, ":utf8");
while(my $line = <MADA_OUT>) { 
    chop($line);
  #print STDERR "line=$line \n";

    if (index($line, "SENTENCE BREAK") == 0) {
    # new sentence
    #print STDERR "BREAK\n";
	print "\n";
    }
    elsif (index($line, ";;WORD") == 0) {
    # word
	my $word = substr($line, 7, length($line) - 8);
    #print STDERR "FOund $word\n";
	print "$word ";
    }
    else {
    #print STDERR "NADA\n";
    }
}
close (MADA_OUT);


if ($KEEP_TMP == 0) {
#    `rm -rf $TMPDIR`;
}

