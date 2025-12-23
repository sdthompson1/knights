<?php

require_once("../constants.php");


// Connect to the database

$db = new mysqli(DATABASE_HOST, USERNAME, PASSWORD, DATABASE_NAME);
if ($db->connect_errno) {
   die("Connect Error " . $db->connect_errno . " " . $db->connect_error);
}



// Get the data to be inserted

$ip_address = $_SERVER['HTTP_X_FORWARDED_FOR'];
if ($ip_address == "") {
   $ip_address = $_SERVER['REMOTE_ADDR'];
}
$ip_address = $db->escape_string($ip_address);
$port = $db->escape_string($_POST['port']);
$description = $db->escape_string($_POST['description']);
$num_players = $db->escape_string($_POST['num_players']);

// Insert the data
// Hostname is set to same as ip_address at this point.

$query = sprintf("INSERT INTO %s (ip_address, port, hostname, last_updated, num_players, "
                 . " description) VALUES ('%s', '%s', '%s', now(), '%s', '%s')"
                 . " ON DUPLICATE KEY UPDATE last_updated=VALUES(last_updated),"
                                         . " num_players=VALUES(num_players),"
                                         . " description=VALUES(description),"
                 DATABASE_TABLE, $ip_address, $port, $ip_address, $num_players, $description);
$db->query($query);


// Now update the table with the hostname. (Do this separately
// in case gethostbyaddr blocks for a long time.)

$hostname = $db->escape_string(gethostbyaddr($ip_address));
$query = sprintf("UPDATE %s SET hostname='%s' WHERE ip_address='%s'", DATABASE_TABLE, $hostname, $ip_address);
$db->query($query);


// Close database

$db->close();

?>
