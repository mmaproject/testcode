<?php
$general_Todo = "general_Todo";
#$test_Equipment = "general_Todo";
$test_Field = "experiment_End";
#$expid = $_POST['expid'];
$con = mysql_connect("127.0.0.1","xvk","");

#obtain log field from ExpID

#echo $test_Equipment . "</br>"; 
if (!$con)
  {
  die('Could not connect: ' . mysql_error());
  }

mysql_select_db("test", $con);

#obtain log field from ExpID
$result = mysql_query(" select * FROM $general_Todo ");

echo "<table border='1'>
<tr>
<th>exp_Number</th>
<th>platform_Name</th>
<th>total_run_Number</th>
<th>application_Command</th>
<th>preferred_Time</th>
<th>Inter-Experiment-Run-Time</th>
<th>Status</th>
<th>Person</th>
<th>Link</th>

</tr>";

while($row = mysql_fetch_array($result))
  {  $temp = $row['platform_Name'] ;
  $pieces = explode("todo_", $temp);
  $test_Equipment = $pieces[1];

  
    echo "<tr>";
  echo "<td>" . $row['serial_Number'] . "</td>";
  echo "<td>" . $test_Equipment . "</td>";
   echo "<td>" . $row['total_run_Number'] . "</td>";
   echo "<td>" . $row['application_Command'] . "</td>";
   echo "<td>" . $row['preferred_Time'] . "</td>";
   echo "<td>" . $row['Frequency'] . "</td>";
    echo "<td>" . $row['Status'] . "</td>";
     echo "<td>" . $row['Person'] . "</td>";	
     echo "<td>" . "<a href=\"experimental_Log2.php?expid=".$row['serial_Number']."\"> Log</a>". "</td>";	
  echo "</tr>";

  
  }

echo "</table>";
echo "	<a href=\"Experimental_config.php\"> Go Back to experimental configuration</a> <br/>";
echo "	<a href=\"Experimental_log.html\"> Go Back to experimental Logs</a> <br/>";
echo "	<a href=\"experimental_Status.php\"> Go Back to experimental status</a> <br/>";
mysql_close($con);
?>
