#! usr/bin/perl

use IO::Socket;
use Sys::Hostname;

#fetch basic info
$server_port = 1579;

$host_name = hostname();
$pid = $$;
$serv = "$host_name:$server_port($pid)";
$host_IP = gethostbyname($host_name) or die ("could not resolve host IP of $host_name: $!");

$host_IP = inet_ntoa($host_IP);
@args = @ARGV;
print("Starting server on $host_name($host_IP) on port $server_port\n");
$uploadentrydag = 'd11';


$server = IO::Socket::INET->new (LocalPort => $server_port,
                                  Type => SOCK_STREAM,
                                  Reuse => 1,
                                  Listen => 10)
or die "TG could not be a server port on port $server_port: $@ \n" ;

$server->autoflush();
print "TG: Server started\n";
$traceindex = 1;
$noDenys = 0;

#main loop waits for clients to connect, 
#once connected client enters the internal CLIENT loop
#client can terminate its own connection by sending a "BYE" and closing the socket
#client can also shutdown the server by sending shutdown

SERVER: while (($client, $client_address) = $server->accept()) {
($port,$iaddr) = sockaddr_in($client_address);
print "connected " ."\n"; # Timestamp doesnt work in windows

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

elsif ($msg =~/LINUXPCSENDER/) {
@args = split (/:/,$msg); 
$platform_name = $args[1];
$platform_type = $args[2];
$application_name = $args[3];
$application_config_basic = $args[4];
$application_config_advanced = $args[5];
$application_run_num = $args[6];

#check if Platform name exists else make a directory by that name to store results;
if ( -d "$platform_name") {
print "$platform_name  directory exists\n";
} #End of If

else { 
mkdir "$platform_name", 0777 unless -d "$platform_name"; 
 print "$platform_name directory created\n";
} #End of Else

#Phasing this loop# if ($args[3] =~ /SKYPE/) {

my $execstr = sprintf ("/home/com/Sikuli/Sikuli-IDE/sikuli-ide.sh -r /home/com/Sikuli_scripts/$platform_name-$platform_type-$application_name.skl");
print "->$execstr|";
	      open PS, "$execstr|";
	     my $response="CRAP";

	      while(my $myIn=<PS>){
		print $myIn;

	      if($myIn=~/output_file/){
		      $response="$myIn";
		  } #End of If
		  
	      } #End of While
close PS;
	  print "LINUXPCSENDERDONE\n";
	open FILE, ">> Experiment.txt" or die $! ;
	print FILE "$args[1]-$args[2]-$args[3]-$args[4]-$args[5]-$args[6] Done \n";
	close FILE ;
	  print $client "LINUXPCSENDERDONE\n";
#} #($args[3] =~ /SKYPE/)  Skype loop # this is not needed, instead directly use application name, 



}# End of Elsif msg is Windows PC sender
} # End of IF message is CLIENT: While Message == CLIENT
}# End of SERVER: while (($client, $client_address) = $server->accept()) 
