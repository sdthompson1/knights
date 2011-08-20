<?

header("Content-Type: text/plain");

require_once("constants.php");

$db = mysql_connect(DATABASE_HOST, USERNAME, PASSWORD) or die("mysql_connect failed");
mysql_select_db(DATABASE_NAME) or die("mysql_select_db failed");

// Timeout in seconds
// Servers are supposed to report every 10 minutes, so if we haven't heard
// anything after 20 min, we ignore the server. (This gives it time to retry
// a couple of times in case it fails to send the update for some reason.)
$timeout = 1200;

$query = sprintf("SELECT ip_address, hostname, port, description, password_required, num_players FROM %s "
                ."WHERE unix_timestamp(last_updated) + %d > unix_timestamp()", DATABASE_TABLE, $timeout);

$result = mysql_query($query);

while ($row = mysql_fetch_assoc($result)) {
   print "[SERVER]\n";
   foreach (array_keys($row) as $key) {
      if ($row[$key] != "") {
         print "$key=" . $row[$key] . "\n";
      }
   }
}

mysql_close($db);

?>
