#! usr/bin/perl
use IO::Socket;
use DBI;
use DBD::mysql;
use DBI qw(:sql_types);
use POSIX;



$platform_zero = 'MAC';
$platform_one ='WINDOWS';
$platform_two ='LINUX';
$platform_three = 'IOS';
$platform_four = 'WMOBILE';
$platform_five = 'ANDROID';

$platform_type_zero ='PC';
$platform_type_one ='MOBILE';

$application_zero = 'FACEBOOK';
$application_one = 'HTTP';
$application_two = 'SKYPE';
$application_three = 'YOUTUBE';

## Define database and its attributes

my $db = 'test';
my $host = '194.47.151.109';
my $user = 'xvk';
my $password = 'atslabb00';
my $table_name = 'linux_Laptop';

#define Ips to conect to

$windowspcip = '194.47.151.73';
$linuxpcip = '10.0.1.193'; # to be added after debug
$macpcip = '194.47.151.255'; # to be added after debug
$windowsmobilecontrolip = '194.47.151.255'; # to be added after debug
$capmarker_control = "192.168.186.103:4000";
$port = 1579; #port over which sockets are connected.
  
#Define sockets to Sending machine (linux_PC)

$socketreceiver = IO::Socket::INET->new(PeerAddr=>$linuxpcip,PeerPort=>$port,Proto=>"tcp",Type=>SOCK_STREAM)
or die ("Cannot connect to $linuxpcip:$port :$@\n");

my $dbh = DBI->connect ("DBI:mysql:$db:$host;user=$user;password=$password") or 
die ("LINUX PC CONTROL Could not connect to database : " .DBI->errstr);


@experiments_config = ( 
                        "2,$platform_two,$platform_type_zero,FACEBOOKLOGINLOGOUT,d,d,1,3,1101",
			"2,$platform_two,$platform_type_zero,FACEBOOKLOGINLOGOUT,d,d,2,3,1101",
			"2,$platform_two,$platform_type_zero,FACEBOOKLOGINLOGOUT,d,d,3,3,1101",
			"2,$platform_two,$platform_type_zero,FACEBOOKLOGINPROFILE,d,d,1,1,5",
			"2,$platform_two,$platform_type_zero,FACEBOOKPROFILEPOST,d,d,1,3,1109",
			"2,$platform_two,$platform_type_zero,FACEBOOKPROFILEPOST,d,d,2,3,1109",
			"2,$platform_two,$platform_type_zero,FACEBOOKPROFILEPOST,d,d,13,3,1109",
			"2,$platform_two,$platform_type_zero,FACEBOOKPROFILEFRIEND,d,d,1,3,1116",
			"2,$platform_two,$platform_type_zero,FACEBOOKPROFILEFRIEND,d,d,2,3,1116",
			"2,$platform_two,$platform_type_zero,FACEBOOKPROFILEFRIEND,d,d,3,3,1116",
			"2,$platform_two,$platform_type_zero,FACEBOOKPICTUREVIEW,d,d,1,3,1106",
			"2,$platform_two,$platform_type_zero,FACEBOOKPICTUREVIEW,d,d,2,3,1106",
			"2,$platform_two,$platform_type_zero,FACEBOOKPICTUREVIEW,d,d,3,3,1106",	
			"2,$platform_two,$platform_type_zero,FACEBOOKPROFILEPHOTO,d,d,1,3,1102",
			"2,$platform_two,$platform_type_zero,FACEBOOKPROFILEPHOTO,d,d,2,3,1102",
			"2,$platform_two,$platform_type_zero,FACEBOOKPROFILEPHOTO,d,d,3,3,1102",
		       );
# Parse the loop and obtain arguments.

for ($i = 0; $i < scalar (@experiments_config);$i++) {
$exp = $experiments_config[$i];
print "will run $exp \n";
($direction,$platform,$platform_type,$application,$application_config,$application_advconfig,$runid,$total_runid,$expid) = split (/,/,$exp);

if ($direction =~ /2/) {
print "LINUX CASE\n";
#collect timestamp and prepare the sql request
$timenow=  strftime "%Y-%m-%d %H:%M:%S",localtime;
my $sth = $dbh->prepare ("INSERT INTO $table_name (experiment_Number, run_Number, total_run_Number,experiment_Start,application_Name,application_Configuration) VALUES (?,?,?,?,?,?)");
$sth->bind_param(4, $timenow , SQL_DATETIME);

$sth->execute($expid,$runid,$total_runid,$timenow,$application,$application_config);


my $sth = $dbh-> prepare ("SELECT LAST_INSERT_ID()");
$sth->execute();
@sequence_Number = $sth->fetchrow_array();
system ("capmarker -e $expid -r $runid $capmarker_control -s $sequence_Number[0]");
informHost($socketreceiver, "LINUXPCSENDER:$platform:$platform_type:$application:$application_config:$application_advconfig:$runid");
waitforreply($socketreceiver,"LINUXPCSENDERDONE");

my $sth = $dbh->prepare ("UPDATE  $table_name SET experiment_END = ?  WHERE experiment_Number = $expid AND run_Number = $runid AND total_run_Number = $total_runid   ");
$timelater=  strftime "%Y-%m-%d %H:%M:%S",localtime;
$sth->bind_param(1, $timelater, SQL_DATETIME);
$sth->execute ($timelater);
sleep (60);
}

print "loop is $i, prepared for next loop\n";
}  #for loop

system ("capmarker -e $expid -r $runid $capmarker_control -s $sequence_Number[0]");
$sth->finish();
$dbh->disconnect();
informHost($socketreceiver, "BYE");
#CLOSE SOCKETS AND KILL MEASUREMENT POINTS



sub informHost{
my ($SAC,$message) = @_;
print $SAC "$message\n";
}

    
sub waitforreply {
my ($SAC,$str) = @_;
print "Am waiting.....for....$SAC.......to give me $str..............\n";
$SAC->autoflush(1);


line: while ($line = <$SAC>) {
 print " I got a msggggg ..:<br> $line </br>\n";
   # next unless /pig/;
 chomp($line);
last line if ($line=~ /$str/) 
	}
print "Hello World, My Wait for $SAC is over\n";}
               

