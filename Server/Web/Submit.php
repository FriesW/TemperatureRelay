<?php

require('config.php');

if($_SERVER['REQUEST_METHOD'] === 'POST')
{
    //Assure that vars are set in post
    if(!isset($_REQUEST['pass']) || !isset($_REQUEST['t0']))
    {
        http_response_code(400);
        die();
    }
    //Check local password
    if($_REQUEST['pass'] != $LOCAL_PASSWORD)
    {
        http_response_code(403);
        die();
    }
    //Check temperature pattern
    if( !preg_match('/^(-)?[0-9]+$/', $_REQUEST['t0']) )
    {
        http_response_code(400);
        die();
    }
    
    try
    {
        //Connect to db
        $conn = new PDO("mysql:host=$SQL_HOST;dbname=$SQL_DB_NAME", $SQL_USER, $SQL_PASSWORD);
        $conn->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
        
        //Process temperatures
        $index = 0;
        $t = time();
        while(isset($_REQUEST["t$index"]))
        {
            //Unpack
            $val = (int)$_REQUEST["t$index"];
            //Push to db
            $stmt = $conn->prepare("INSERT INTO $SQL_TABLE (time, temperature) VALUES (:time, :temp)");
            $stmt->bindValue(':time', $t);
            $stmt->bindValue(':temp', $val);
            $stmt->execute();
            //Increment for next
            $t -= $TIME_STEP;
            $index++;
        }
        
    }
    catch(PDOException $e)
    {
        http_response_code(500);
        die();
    }
    
}



?>

<!DOCTYPE html>
<html lang="en-US">
<head>

<meta charset="utf-8">

<title>Submit Temperature</title>

</head>
<body>

<h1>Submit</h1>

<form method="post">
Password:<input type="text" name="pass"><br>
Temperature:<input type="text" name="t0"> hundredths of a degree C<br>
<input type="submit">
</form>

</body>

</html>
