<?php

require('config.php');

if($_SERVER['REQUEST_METHOD'] === 'POST')
{
    //Assure that vars are set in post
    if(!isset($_REQUEST['pass']) || !isset($_REQUEST['t1']))
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
    if( !preg_match('/^[0-9]+(.[0-9]*)?$/', $_REQUEST['t1']) )
    {
        http_response_code(400);
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
Temperature:<input type="text" name="t1"><br>
<input type="submit">
</form>

</body>

</html>
