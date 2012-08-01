<?php
$general_Todo = "general_Todo";
#$test_Equipment = "general_Todo";
#$test_Field = "experiment_End";
$expid = $_POST['expid'];
$con = mysql_connect("127.0.0.1","xvk","");

#obtain log field from ExpID

#echo $test_Equipment . "</br>"; 
if (!$con)
  {
  die('Could not connect: ' . mysql_error());
  }

mysql_select_db("test", $con);

#obtain log field from ExpID
$result = mysql_query(" select * FROM $general_Todo WHERE serial_Number = $expid") or die(mysql_error());
while($row = mysql_fetch_array($result))
  {
  
  $test_Equipment = $row['platform_Name'] ;
  #$pieces = explode("todo_", $temp);
  #$test_Equipment = "log_".$pieces[1];
  
  }



$result = mysql_query(" DELETE FROM $general_Todo WHERE serial_Number = $expid")or die(mysql_error());
$result = mysql_query(" DELETE FROM $test_Equipment WHERE exp_Number = $expid")or die(mysql_error());
echo "	<a href=\"general_List.php\"> View general Experiments</a> <br/>";
echo "	<a href=\"Experimental_config.php\"> Go Back to experimental configuration</a> <br/>";
echo "	<a href=\"Experimental_log.html\"> Go Back to experimental Logs</a> <br/>";
echo "	<a href=\"Experimental_delete.html\"> Delete an Experiment</a> <br/>";
echo "	<a href=\"experimental_Status.php\"> Go Back to experimental status</a> <br/>";
mysql_close($con);
?>
