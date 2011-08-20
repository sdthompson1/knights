<?

require_once("constants.php");


// Used to escape data coming from _POST, independent of magic_quotes setting
function escape($x)
{
   // make sure string doesn't get escaped twice
   if (get_magic_quotes_gpc()) $x = stripslashes($x);

   // newline characters will throw things off later, so replace them with spaces here.
   $x = str_replace("\n", " ", $x);

   // escape the string for mysql
   return mysql_real_escape_string($x);
}


// Connect to the database

$db = mysql_connect(DATABASE_HOST, USERNAME, PASSWORD) or die("mysql_connect failed");
mysql_select_db(DATABASE_NAME) or die("mysql_select_db failed");


// Get the data to be inserted

$ip_address = $_SERVER['HTTP_X_FORWARDED_FOR'];
if ($ip_address == "") {
   $ip_address = $_SERVER['REMOTE_ADDR'];
}
$ip_address = escape($ip_address);
$port = escape($_POST['port']);
$description = escape($_POST['description']);
$num_players = escape($_POST['num_players']);
$password_required = escape($_POST['password_required']);

// Insert the data
// Hostname is set to same as ip_address at this point.

$query = sprintf("INSERT INTO %s (ip_address, port, hostname, last_updated, num_players, "
                 . " description, password_required) VALUES ('%s', '%s', '%s', now(), '%s', '%s', '%s')"
                 . " ON DUPLICATE KEY UPDATE last_updated=VALUES(last_updated),"
                                         . " num_players=VALUES(num_players),"
                                         . " description=VALUES(description),"
                                         . " password_required=VALUES(password_required)",
                 DATABASE_TABLE, $ip_address, $port, $ip_address, $num_players, $description, $password_required);
mysql_query($query, $db);


// Now update the table with the hostname. (Do this separately
// in case gethostbyaddr blocks for a long time.)

$hostname = escape(gethostbyaddr($ip_address));
$query = sprintf("UPDATE %s SET hostname='%s' WHERE ip_address='%s'", DATABASE_TABLE, $hostname, $ip_address);
mysql_query($query, $db);


// Close database

mysql_close($db);

?>
