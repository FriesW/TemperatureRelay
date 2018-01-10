<?php

require('config.php');

/*
//In progress query
SELECT c.time, c.temperature  FROM
(
    SELECT a.time, a.temperature,
    (
        SELECT count(*) FROM
            (SELECT * FROM $SQL_TABLE WHERE time > $tlim ORDER BY time ASC) b
        WHERE a.time > b.time
    ) AS row_number
    FROM
        (SELECT * FROM $SQL_TABLE WHERE time > $tlim ORDER BY time ASC) a
) c
WHERE c.row_number%15 = 0
*/

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
    <th>Last sensor check-in:</th><td>3 minutes ago</td>
</tr>
<tr>
    <th>12 hour low:</th><td>45 F</td>
</tr>
<tr>
    <th>36 hour low:</th><td>55 F</td>
</tr>
</table>


<h3>History: Day</h3>
<table>
<tr>
    <th>Time</th><th>Temperature (F)</th>
</tr>
<tr>
    <td>1/2/2018 6:39P</td><td>46.5</td>
</tr>
</table>

<h3>History: Week</h3>

<h3>History: Month</h3>


</body>

</html>