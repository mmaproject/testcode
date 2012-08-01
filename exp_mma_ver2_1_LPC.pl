# The aim of the script is to Query the controller for experimental configuration; If an experimental configuration is found, it would execute else it would sleep for 60 seconds and query again. 

# Date 21 March 2012.
# Technically, this script forms a TCP Client to be hosted in the traffic generating machines (Laptops/Mobile Controller)
# This script is supposed to read an experimental configuration, and depending on the platform execute the Sikuli scripts at the locations.

# Define IP address and Port  and platform for host to connect to ; 

# send QUERy ( informhost,)
# wait for reply ()
# if msg is FETCH inform host CONFIG, wait for response, parse it REPORT SUCCESS OR FAILURE and resend QUERY
# if msg is REPEATQUERY Sleep for 120 seconds, resend request

$server_ipaddress = '10.0.1.198'; # Controller IP address Option 1
$server_port = '1579' ; # Server Option 2
$platform = 'LINUX' ; # Platform, used for running the traffic generators ( Sikuli in mma) # Option3


# Initiate communication
$socketreceiver = IO::Socket::INET->new(PeerAddr=>$linuxpcip,PeerPort=>$port,Proto=>"tcp",Type=>SOCK_STREAM)
or die ("Cannot connect to $server_ipaddress:$server_port :$@\n");

for ($i = 0 ; $i > -1; $i++)
{ ;
# Inform server QUERY
informHost ($socketreceiver, "QUERY");
waitforreply ($socketreceiver,"HOLD","PROCEED");
informhost ($socketreceiver, "CONFIG");
#wait for CONFIG and PARSE IT, EXECUTE IT ; report SUCCESS or FAILURE
waitforconfig ($socketreceiver, "CONFIG");
# wait for reply 
sleep (20);
# if reply is NOINFORMATION  sleep (60) else execute a subroutine
    
}


# Sub_routines ( note the subroutine is slightly modified than original one)
sub informHost{
my ($SAC,$message) = @_;
print $SAC "$message\n";
}

    
sub waitforreply {
my ($SAC,$str,$str2) = @_;
print "Am waiting.....for....$SAC.......to give me $str..............\n";
$SAC->autoflush(1);


line: while ($line = <$SAC>) {
 print " I got a msggggg ..:<br> $line </br>\n";
   # next unless /pig/;
 chomp($line);
 if ($line=~ /$str/) {
  sleep (120);
  print $SAC ,"$str\n" ;
  waitforreply ($SAC,$str1,$str2);
}
last line if ($line=~ /$str2/)
	}
print "Hello World, My Wait for $SAC is over\n";}

sub waitforconfig {
my ($SAC,$str) = @_;
print "Am waiting.....for....$SAC.......to give me $str..............\n";
$SAC->autoflush(1);


line: while ($line = <$SAC>) {
 print " I got a msggggg ..:<br> $line </br>\n";
   # next unless /pig/;
 chomp($line);
 if ($line=~ /$str/) {
  @args = split (/:/,$line);
  $exp_id = $args[1];
  $run_id = $args[2];
  $total_run_id = $args[3];
  $application_command = $args[4];  
  
my $execstr = sprintf ("$application_command");
print "->$execstr|";
	      open PS, "$execstr|";
	     my $response="CRAP";

	      while(my $myIn=<PS>){
		print $myIn;

	      if($myIn=~/SUCCESS/){
		      $response = "SUCCESS";
			  last line;
		  } #End of If
		  if($myIn=~/FAILURE/){
		      $response = "FAILURE";
			  last line;
		  }
		  
	      } #End of While
close PS;
	  print "LINUXPCSENDERDONE\n";
	open FILE, ">> Experiment.txt" or die $! ;
	print FILE "$args[1]-$args[2]-$args[3]-$args[4]-$response Done \n";
	close FILE ;
	 print $SAC "$response:$exp_id:$run_id:$total_run_id:$application_command"; 

  }
	}
print "Hello World, My Wait for $SAC is over\n";}

               

