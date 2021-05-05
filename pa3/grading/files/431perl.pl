#!/usr/bin/perl -w

$assignname=$ARGV[0];
$executable="command";

$TEST_OUTPUT_DIR="../test-output";

print "Grading $assignname...\n\n";

system("chmod u+x parser");
system("chmod u+x lexer");
system("chmod u+x command");
system("chmod u+x semant");
system("mkdir -p $TEST_OUTPUT_DIR");
system("make -C ../../src clean");
system("make -C ../../src/");

print "\n-------------GRADING--------------\n\n";

$total = 0;
$count = 0;
$outfile = ".out";
my $dir = "../";
opendir DIR,$dir;
my @dir = readdir(DIR);
close DIR;
foreach(@dir){
    if($_ =~ /[^"]*\.(test|cl)$/) {
        $total = $total + 1;
        $retval = system("./$executable ../$_ > $TEST_OUTPUT_DIR/$_ 2>&1");
        if($retval > 0) {
            print "$_ faild\n";
            open(IN,"$TEST_OUTPUT_DIR/$_");
            @data = <IN> ;            
            close(IN);
            foreach (@data) {
                print;
            }
            print "\n";
        }
        else {
            $count = $count + 1;
        }
    }
}
print "----------------------------------\n";

print "SCORE : $count / $total\n\n";