<?php
header("Content-Type: text/html");
header("Status: 200 OK");
?>
<!DOCTYPE html>
<html>
<head>
    <title>PHP CGI Test</title>
</head>
<body>
    <h1>Hello from PHP CGI!</h1>
    <p>This is a test CGI script written in PHP.</p>
    <p>Current time: <?php echo date('Y-m-d H:i:s'); ?></p>
    <p>Server: <?php echo $_SERVER['SERVER_SOFTWARE'] ?? 'WebServ'; ?></p>
</body>
</html>
