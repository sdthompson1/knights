<?php

header("Content-Type: text/plain");

require_once("../constants.php");

$db = new mysqli(DATABASE_HOST, USERNAME, PASSWORD, DATABASE_NAME);
if ($db->connect_errno) {
   die("Connect Error " . $db->connect_errno . " " . $db->connect_error);
}

// Timeout in seconds
// Servers are supposed to report every 10 minutes, so if we haven't heard
// anything after 20 min, we ignore the server. (This gives it time to retry
// a couple of times in case it fails to send the update for some reason.)
$timeout = 1200;

$query = sprintf("SELECT ip_address, hostname, port, description, password_required, num_players FROM %s "
                ."WHERE unix_timestamp(last_updated) + %d > unix_timestamp()", DATABASE_TABLE, $timeout);

$result = $db->query($query);

while ($row = $result->fetch_assoc()) {
   print "[SERVER]\n";
   foreach (array_keys($row) as $key) {
      if ($row[$key] != "") {
         print "$key=" . $row[$key] . "\n";
      }
   }
}

$db->close();

?>
