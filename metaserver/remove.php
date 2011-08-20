<?

require_once("constants.php");

// Connect to the database

$db = mysql_connect(DATABASE_HOST, USERNAME, PASSWORD) or die("mysql_connect failed");
mysql_select_db(DATABASE_NAME) or die("mysql_select_db failed");


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


// Figure out the address/port of the server

$ip_address = $_SERVER['HTTP_X_FORWARDED_FOR'];
if ($ip_address == "") {
   $ip_address = $_SERVER['REMOTE_ADDR'];
}
$ip_address = escape($ip_address);
$port = escape($_POST['port']);


// Remove the server from the database.

$query = sprintf("DELETE FROM %s WHERE ip_address='%s' AND port='%s'", 
                 DATABASE_TABLE, $ip_address, $port);
mysql_query($query, $db);


// Close database
mysql_close($db);

?>
