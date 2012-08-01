#The aim of the script is to control the PC (Laptop) from the XPS ( Control)
#Date: 20 March 2012
# Technically, this script forms a TCP server to be hosted in the XPS server that communicates with the TCP client hosted in the Linux Laptop;
# Functional use: 
# 1. Primarily used to listen to the TCP client polls and respond appropriately.
# 2. Query the table Linux_config from the database test and store the results to an array and send it to TCP Client to parse.
# 3. Log the experimental start and stop times. Ideally, must be able to log the exceptions. If not flag the case when experiment is not properly done. 

# SUCCESS
#Delete from TODO, update LOG, if runs == total runs, delete from general_todo
# FAILURE
#Delete from TODO , update LOG , insert into TODO, increment into general todo, send email
# Default, must send email to the experiment guy (vkk) log failure, increment total run count, 
# QUERY
# Query from todo_... return HOLD / PROCEED, 
# CONFIG
#check time , if experimental time is greater than current time continue
#return TODO_list
# configuration files

#! usr/bin/perl

use IO::Socket;
use Sys::Hostname;
use Getopt::Long;
use DBI;
use DBD::mysql;
use DBI qw(:sql_types);
use POSIX;

## Define database and its attributes

my $db = 'test';
my $host = '194.47.151.109';
my $user = 'xvk'; #Option 2 GET USER NAME
my $password = 'atslabb00'; # PASSWORD

my $general_todo = 'general_Todo'; #option #5
my $todo_table_name = 'todo_Linux_Laptop'; #Option#4 obtain the list to do.
my $log_table_name = 'linux_Laptop'; #Option 5  used to insert logs
my $platform = 'LINUXPCSENDER'; #Option6 name of platform used in the experiments
#fetch basic info 
my $server_port = 1579;  # expects a server port using options. Example(1579)

$host_name = hostname();
$pid = $$;

$host_IP = gethostbyname($host_name) or die ("could not resolve host IP of $host_name: $!");

$host_IP = inet_ntoa($host_IP);
#@args = @ARGV;
#Provide Options for commandline arguments
#Command line arguments can be a. Port b. Username  of Database c.Password of database; 
GetOptions ("port=i" => \$server_port,
	    "username=s" => \$user,
            "password=s" => \$password,
            "todo_table=s" => \$todo_table_name,
            "log_table=s" => \$log_table_name,
			"general_todo=s" => \$general_todo,
            "platform=s"  => \$platform,       
	);
#Connect to Database. 


my $dbh = DBI->connect ("DBI:mysql:$db:$host;user=$user;password=$password") or 
die ("$platform PC CONTROL Could not connect to database : " .DBI->errstr);

#Start the TCP server
print("Starting server on $host_name($host_IP) on port $server_port\n");

$serv = "$host_name:$server_port($pid)";

$server = IO::Socket::INET->new (LocalPort => $server_port,
                                  Type => SOCK_STREAM,
                                  Reuse => 1,
                                  Listen => 10)
or die "TG could not be a server port on port $server_port: $@ \n" ;


$server->autoflush();
print "TG: Server started\n";

SERVER: while (($client, $client_address) = $server->accept()) {
($port,$iaddr) = sockaddr_in($client_address);
print "connected " ."\n"; 

CLIENT: while ( $msg = <$client>) {

print "Got $msg ....\n";
chomp($msg);
if ($msg =~/\bBYE\b/) {
print "TG got BYE\n";
print "terminating connection with server and closing SOCKET\n";
last CLIENT;

} # End of IF Message is BYE

 elsif ($msg=~/\bSHUTDOWN \b/) {
print "TG got Shutdown\n";
print $client "SERVER SHUTTING DOWN\n";
last SERVER;
} # End of Elsif MSG is equal to Shut down

elsif ($msg =~ /SUCCESS/) {
print "GOT message SUCCESS\n";
#parse message, delete from todo, update timestamp and result field in log, if runs == total runs, delete from general todo

@args = split (/:/,$msg);
$status = $args[0];
$expid = $args[1];
$runid = $args[2];
$total_runid = $args[3];
#update timestamp and result in log table 
my $sth = $dbh->prepare ("UPDATE  $log_table_name SET experiment_end_Time = ?, Status = ?  WHERE exp_Number = $expid AND run_Number = $runid AND total_run_Number = $total_runid ");
$timelater=  strftime "%Y-%m-%d %H:%M:%S",localtime;
$sth->bind_param(1, $timelater, SQL_DATETIME);
$sth->execute ($timelater,$status);

my $sth = $dbh->prepare ("DELETE FROM $todo_table_name WHERE exp_Number = $expid AND $run_Number =$runid AND $total_run_Number = $total_runid");
$sth->execute();
print "DELETE FROM $todo_table_name WHERE exp_Number = $expid AND $run_Number =$runid AND $total_run_Number = $total_runid\n";
if ($runid == $total_runid)
{
my $sth = $dbh->prepare ("DELETE FROM $general_todo WHERE exp_Number = $expid AND $run_Number =$runid AND $total_run_Number = $total_runid");
$sth->execute();
print "DELETE FROM $general_todo WHERE exp_Number = $expid AND $run_Number =$runid AND $total_run_Number = $total_runid\n";
}


}

elsif ($msg =~ /FAILURE/) {
print "GOT message FAILURE\n";
#parse message, delete from todo, update timestamp and result field in log, if runs == total runs, delete from general todo

@args = split (/:/,$msg);
$status = $args[0];
$expid = $args[1];
$runid = $args[2];
$total_runid = $args[3];
$application_command = $args[4];
#update timestamp and result in log table 
my $sth = $dbh->prepare ("UPDATE  $log_table_name SET experiment_end_Time = ?, Status = ?  WHERE exp_Number = $expid AND run_Number = $runid AND total_run_Number = $total_runid ");
$timelater=  strftime "%Y-%m-%d %H:%M:%S",localtime;
$sth->bind_param(1, $timelater, SQL_DATETIME);
$sth->execute ($timelater,$status);

my $sth = $dbh->prepare ("DELETE FROM $todo_table_name WHERE exp_Number = $expid AND $run_Number =$runid AND $total_run_Number = $total_runid");
$sth->execute();
print "DELETE FROM $todo_table_name WHERE exp_Number = $expid AND $run_Number =$runid AND $total_run_Number = $total_runid\n";

#increment general todo total run number
$total_runid = $total_runid +1;
my $sth = $dbh->prepare ("UPDATE  $general_todo  SET total_run_Number = $total_runid  WHERE serial_Number = $expid");
$sth->execute();
print "UPDATE  $general_todo  SET total_run_Number = $total_runid  WHERE serial_Number = $expid";
# insert into todo
$timenow=  strftime "%Y-%m-%d %H:%M:%S",localtime;
my $sth = $dbh->prepare ("INSERT INTO $todo_table_name (exp_Number, run_Number, total_run_Number,experiment_Start,application_Command,preferred_Time) VALUES (?,?,?,?,?)");
$sth->bind_param(5, $timenow , SQL_DATETIME);

$sth->execute($expid,$total_runid,$total_runid,$application_command,$timenow);
# email 
sendEmail("vkk\@bth.se", "pal\@bth.se", "Experiment Failure", "This is to notify you for the failure of the run number $runid, $expid -- $runid -- $timenow --- $application_command"); 
}

elsif ($msg=~/QUERY/) {
my $sth = $dbh-> prepare ("SELECT * from $todo_table_name");
$sth->execute();
# if the sql returns 0E0 it means ; return NORESULTFORQUERY to the client, else obtain the experimental configuration and send the configuration to the client; 
if (! ($sth ne "0E0")) {
print $client "HOLD";
}
else {
print $client "PROCEED";
}
}
elsif ($msg=~/CONFIG/) {
my $sth = $dbh->prepare ("SELECT * from $todo_table_name ")
#check if the current timestamp is greater than timestamp obtained, if so execute else continue 
while ( my @experiments_config = sth->fetchrow_array()) {
# This input comes from SQL QUERY which is inturn fed from a webpage.
$platform_name = $experiments_config[0];  #LINUX
$platform_type= $experiments_config[1];  #PC
$application= $experiments_config[2];   #YOUTUBE
$application_config= $experiments_config[3];  #d
$application_advconfig= $experiments_config[4]; #d
$total_runid= $experiments_config[5]; #10
$expid= $experiments_config[6]; #20

for ($runid = 1; $runid<= $total_runid ; $runid++) {
print "$platform_type CASE\n";
#Insert the experiment into log table for the platform with the START TIME OF EXPERIMENT;
$timenow=  strftime "%Y-%m-%d %H:%M:%S",localtime;
my $sth = $dbh->prepare ("INSERT INTO $log_table_name (experiment_Number, run_Number, total_run_Number,experiment_Start,application_Name,application_Configuration) VALUES (?,?,?,?,?,?)");
$sth->bind_param(4, $timenow , SQL_DATETIME);
$sth->execute($expid,$runid,$total_runid,$timenow,$application,$application_config);

# Send the Capmarker with the last insert ID ( Sequence Number) 
my $sth = $dbh-> prepare ("SELECT LAST_INSERT_ID()");
$sth->execute();
@sequence_Number = $sth->fetchrow_array();
system ("capmarker -e $expid -r $runid $capmarker_control -s $sequence_Number[0]");

# Send the parameters to the Platform, to generate the traffic. 
informHost($client, "$platform:$platform_name:$platform_type:$application:$application_config:$application_advconfig:$runid");

#wait for reply from the sender that experiment is done
waitforreply($client,"$platform"."DONE");

#Insert the timestamp into the log;
my $sth = $dbh->prepare ("UPDATE  $log_table_name SET experiment_END = ?  WHERE experiment_Number = $expid AND run_Number = $runid AND total_run_Number = $total_runid   ");
$timelater=  strftime "%Y-%m-%d %H:%M:%S",localtime;
$sth->bind_param(1, $timelater, SQL_DATETIME);
$sth->execute ($timelater);
sleep (60);

# Do the analysis for the experiment.

} #for
#Delete the experiment from the todo list
my $sth = $dbh-> prepare ("DELETE FROM $todo_table_name WHERE experiment_Number = $expid AND  total_run_Number = $total_runid AND application_Name = $application AND application_Configuration = $application_config AND application_advanced_Configuration = $application_advconfig");
$sth->execute();
} #while
# send capmarker for the last experiment.
system ("capmarker -e $expid -r $runid $capmarker_control -s $sequence_Number[0]");

} #else
#DISCONNECT FROM DATABASE
$sth->finish();
$dbh->disconnect();
} # elsif query 
} # Client 
} # Server







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
               


sub sendEmail
 {
 	my ($to, $from, $subject, $message) = @_;
 	my $sendmail = '/usr/lib/sendmail';
 	open(MAIL, "|$sendmail -oi -t");
 		print MAIL "From: $from\n";
 		print MAIL "To: $to\n";
 		print MAIL "Subject: $subject\n\n";
 		print MAIL "$message\n";
 	close(MAIL);
 } 
 
