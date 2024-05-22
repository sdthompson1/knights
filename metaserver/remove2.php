<?php

require_once("../constants.php");

// Connect to the database

$db = new mysqli(DATABASE_HOST, USERNAME, PASSWORD, DATABASE_NAME);
if ($db->connect_errno) {
   die("Connect Error " . $db->connect_errno . " " . $db->connect_error);
}


// Figure out the address/port of the server

$ip_address = $_SERVER['HTTP_X_FORWARDED_FOR'];
if ($ip_address == "") {
   $ip_address = $_SERVER['REMOTE_ADDR'];
}
$ip_address = $db->escape_string($ip_address);
$port = $db->escape_string($_POST['port']);


// Remove the server from the database.

$query = sprintf("DELETE FROM %s WHERE ip_address='%s' AND port='%s'", 
                 DATABASE_TABLE, $ip_address, $port);
$db->query($query);


// Close database
$db->close();

?>
