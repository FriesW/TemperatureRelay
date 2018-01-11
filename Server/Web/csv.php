<?php

require('config.php');

function format_time($val)
{
    global $TIMEZONE;
    $date = new DateTime("@$val");
    $date->setTimezone(new DateTimeZone($TIMEZONE));
    return $date->format('Y-m-d H:i:s'); //Y-m-d h:i:s a
}

function format_temperature($val)
{
    return number_format((int)$val / 10, 1);
}


if(!isset($_REQUEST['t']) || !preg_match('/^[0-9]+$/', $_REQUEST['t']))
{
    http_response_code(400);
    die();
}

$t = $_REQUEST['t'];

$f_name = format_time($t) . ' to ' . format_time(time()) . ' in ' . $TIMEZONE;

header("Content-Disposition: attachment; filename=\"$f_name.csv\";");

try
{
    //Connect to db
    $conn = new PDO("mysql:host=$SQL_HOST;dbname=$SQL_DB_NAME", $SQL_USER, $SQL_PASSWORD);
    $conn->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
    
    //Get data
    $stmt = $conn->prepare("SELECT * FROM `$SQL_TABLE` WHERE time > $t ORDER BY time DESC");
    $stmt->execute();
    
    //Print
    echo 'Epoch Time,Formatted Time, Temperature C' . PHP_EOL;
    while($row = $stmt->fetch(PDO::FETCH_ASSOC))
    {
        echo '' . $row['time'] . ',' . format_time($row['time']) . ',' . format_temperature($row['temperature']) . PHP_EOL;
    }
    
}
catch(PDOException $e)
{
    http_response_code(500);
    die();
}

?>