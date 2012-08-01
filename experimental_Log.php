<?php
$general_Todo = "general_Todo";
#$test_Equipment = "general_Todo";
$test_Field = "experiment_End";
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
$result = mysql_query(" select * FROM $general_Todo WHERE serial_Number = $expid");
while($row = mysql_fetch_array($result))
  {
  
  $temp = $row['platform_Name'] ;
  $pieces = explode("todo_", $temp);
  $test_Equipment = "log_".$pieces[1];
  
  }




$result = mysql_query(" select exp_Number,run_Number,total_run_Number, experiment_start_Time,experiment_end_Time, Status, UNIX_TIMESTAMP(experiment_end_Time)-UNIX_TIMESTAMP(experiment_start_Time) as \"exectime\" FROM $test_Equipment WHERE exp_Number = $expid");
#echo "$result";
echo "<table border='1'>
<tr>
<th>exp_Number</th>
<th>run_Number</th>
<th>total_run_Number</th>
<th>experiment_start_Time</th>
<th>experiment_end_Time</th>
<th>Status</th>
<th>Execution_time (s)</th>
<th>Log</th>

</tr>";

while($row = mysql_fetch_array($result))
  {
  echo "<tr>";
  echo "<td>" . $row['exp_Number'] . "</td>";
  echo "<td>" . $row['run_Number'] . "</td>";
   echo "<td>" . $row['total_run_Number'] . "</td>";
   echo "<td>" . $row['experiment_start_Time'] . "</td>";
   echo "<td>" . $row['experiment_end_Time'] . "</td>";
    echo "<td>" . $row['Status'] . "</td>";
     echo "<td>" . $row['exectime'] . "</td>";	
     echo "<td>" . "<a href=\"log.php?exp_Number=".$row['exp_Number']."&run_Number=".$row['run_Number']."&test_Equipment=".$test_Equipment." \"> Log</a>". "</td>";	
  echo "</tr>";
  }
echo "</table>";
echo "	<a href=\"general_List.php\"> View general Experiments</a> <br/>";
echo "	<a href=\"Experimental_config.php\"> Go to experimental configuration</a> <br/>";
echo "	<a href=\"Experimental_log.html\"> Go to experimental Logs</a> <br/>";
echo "	<a href=\"Experimental_delete.html\"> Delete an Experiment</a> <br/>";
echo "	<a href=\"experimental_Status.php\"> Go Back to experimental status</a> <br/>";

mysql_close($con);
?>
