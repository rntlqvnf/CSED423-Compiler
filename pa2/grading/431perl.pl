#!/usr/bin/perl -w

$assignname=$ARGV[0];
$executable="myparser";

$TEST_OUTPUT_DIR="test-output";

print "Grading $assignname...\n\n";

system("mkdir -p $TEST_OUTPUT_DIR");

$outfile = ".out";
my $dir = "./";
opendir DIR,$dir;
my @dir = readdir(DIR);
close DIR;
foreach(@dir){
    if($_ =~ /[^"]*\.test$/) {
        $retval = system("./$executable $_ > $TEST_OUTPUT_DIR/$_ 2>&1");
        if($retval > 0) {
            print "$_ file faild\n";
            open(IN,"$TEST_OUTPUT_DIR/$_");
            @data = <IN> ;            
            close(IN);
            foreach (@data) {
                print;
            }
            print "\n";
        }
    }
}