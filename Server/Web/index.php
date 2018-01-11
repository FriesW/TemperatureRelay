<?php

ini_set('display_errors',1);
error_reporting(E_ALL);

require('config.php');

//epoch converter
function ec($val)
{
    return (new DateTime("@$val"))->format('Y-m-d H:i:s');
}

//temperature converter
function tc($val)
{
    $v = (string)round( (((int)$val / 10) * 9 / 5 + 32), 1);
    return str_pad($v, strlen('000.0'), ' ', STR_PAD_LEFT);
}

function prep_stmt($step, $oldest_time)
{
    global $SQL_TABLE;
    $step = (int)$step;
    $oldest_time = (int)$oldest_time;
    if($step < 2)
        return "SELECT * FROM `$SQL_TABLE` WHERE time > $oldest_time ORDER BY time DESC";
    return <<<EOT
SELECT c.time, c.temperature  FROM
(
    SELECT a.time, a.temperature,
    (
        SELECT count(*) FROM
            (SELECT * FROM $SQL_TABLE WHERE time > $oldest_time ORDER BY time ASC) b
        WHERE a.time > b.time
    ) AS row_number
    FROM
        (SELECT * FROM $SQL_TABLE WHERE time > $oldest_time ORDER BY time ASC) a
) c
WHERE c.row_number%$step = 0
ORDER BY c.time DESC
EOT;
}

function print_table($step, $time_period)
{
    echo '<table><tr><th>Time</th><th>Temperature (F)</th></tr>';
    global $conn;
    $stmt = $conn->prepare(prep_stmt($step, time() - $time_period));
    $stmt->execute();
    while($row = $stmt->fetch(PDO::FETCH_ASSOC))
    {
        $time = ec($row['time']);
        $temp = tc($row['temperature']);
        echo "<tr><td>$time</td><td>$temp</td></tr>";
    }
    echo '</table>';
}

try
{
    //Connect to db
    $conn = new PDO("mysql:host=$SQL_HOST;dbname=$SQL_DB_NAME", $SQL_USER, $SQL_PASSWORD);
    $conn->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
    
    //Get most recent time from table
    $stmt = $conn->prepare("SELECT * FROM `$SQL_TABLE` ORDER BY `time` DESC LIMIT 1, 1");
    $stmt->execute();
    $row = $stmt->fetch(PDO::FETCH_ASSOC);
    $newest_time = $row['time'];
    
    //Get 12 & 36 hour lows
    $mins = array();
    foreach(array(time() - 12*60*60, time() - 36*60*60) as &$t)
    {
        $stmt = $conn->prepare("SELECT * FROM `$SQL_TABLE` WHERE `time` > :t ORDER BY `temperature` ASC LIMIT 1, 1");
        $stmt->bindValue(':t', $t);
        $stmt->execute();
        $row = $stmt->fetch(PDO::FETCH_ASSOC);
        array_push($mins, tc($row['temperature']));
    }

?>

<!DOCTYPE html>
<html lang="en-US">
<head>

<meta charset="utf-8">

<title>Temp History</title>

</head>
<body>

<h1>Town House Temperatures</h1>

<h3>Quick Glance</h3>
<table id='quick'>
<tr>
    <th>Item</th><th>Value</th>
</tr>
<tr>
    <th>Last sensor check-in:</th><td><?php echo (int)((time() - $newest_time) / 60);?> minutes ago</td>
</tr>
<tr>
    <th>12 hour low:</th><td><?php echo array_shift($mins); ?> F</td>
</tr>
<tr>
    <th>36 hour low:</th><td><?php echo array_shift($mins); ?> F</td>
</tr>
</table>


<h3>History: Day</h3>
<?php print_table(1, 24*60*60); //288 samples. If report every 5 minutes, Total samples = 24*60/5, Displayed samples = total samples / 1 ?> 

<h3>History: Week</h3>
<?php print_table(7, 7*24*60*60); //288 samples. Displayed samples = (7*24*60/5) / 7 ?>

<h3>History: Month</h3>
<?php print_table(31, 31*24*60*60); //288 samples ?>

</body>

</html>

<?php

}
catch(PDOException $e)
{
    http_response_code(500);
    die();
}

?>