<?php
// Example 1
$pizza  = "todo_platform";
$pieces = explode("todo_", $pizza);
echo $pieces[0]; // piece1
echo "heehaaa";
echo $pieces[1]; // piece2

// Example 2
$data = "foo:*:1023:1000::/home/foo:/bin/sh";
list($user, $pass, $uid, $gid, $gecos, $home, $shell) = explode(":", $data);
echo $user; // foo
echo $pass; // *

?>