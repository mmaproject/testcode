<?php
  header("Expires: 0");
  header("Cache-Control: must-revalidate, post-check=0, pre-check=0");
  header("Cache-Control: public");

  require_once('htpasswd.inc');
  $pass_array = load_htpasswd();

  if (isset($_SERVER['PHP_AUTH_USER']) && test_htpasswd( $pass_array,  $_SERVER['PHP_AUTH_USER'], $_SERVER['PHP_AUTH_PW'] ))
  {
    print("Welcome!");
    // Do stuff
  }
  else
  {
    header('WWW-Authenticate: Basic realm="Restricted area"');
    header('HTTP/1.0 401 Unauthorized');
    echo 'Access denied. Please enter correct credentials.';
    exit;
  }
?>

