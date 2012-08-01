<?php
$test_Equipment = $_POST['item'];
$test_Field = "experiment_End";
$view_Length = $_POST['quantity'];
$con = mysql_connect("127.0.0.1","xvk","");
if (!$con)
  {
  die('Could not connect: ' . mysql_error());
  }

mysql_select_db("test", $con);

$result = mysql_query("SELECT * FROM $test_Equipment ORDER BY serial_Number DESC LIMIT $view_Length");
#echo "$result";
echo "<table border='1'>
<tr>
<th>serial_Number</th>
<th>experiment_Number</th>
<th>run_Number</th>
<th>total_run_Number</th>
<th>application_Name</th>
<th>application_Configuration</th>
<th>experiment_Start</th>
<th>experiment_End</th>
</tr>";

while($row = mysql_fetch_array($result))
  {
  echo "<tr>";
  echo "<td>" . $row['serial_Number'] . "</td>";
  echo "<td>" . $row['experiment_Number'] . "</td>";
  echo "<td>" . $row['run_Number'] . "</td>";
   echo "<td>" . $row['total_run_Number'] . "</td>";
   echo "<td>" . $row['application_Name'] . "</td>";
   echo "<td>" . $row['application_Configuration'] . "</td>";
   echo "<td>" . $row['experiment_Start'] . "</td>";
   echo "<td>" . $row['experiment_End'] . "</td>";
  echo "</tr>";
  }
echo "</table>";

$null_Query  = mysql_query("SELECT * FROM $test_Equipment ORDER BY serial_Number DESC LIMIT 1");
#echo "$null_Query";
if ($null_Query) {
while ($null_Values = mysql_fetch_assoc ($null_Query))
      if (is_null ($null_Values["$test_Field"]))
      echo "<h1>"."$test_Equipment is running\n"."</h1>";
      else
      echo "<h1>"."$test_Equipment is not running\n"."</h1>";
}      
mysql_close($con);
?> 