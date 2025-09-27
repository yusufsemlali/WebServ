<?php
header("Content-Type: text/html");
echo "<html>\n";
echo "<head><title>PHP CGI Test</title></head>\n";
echo "<body>\n";
echo "<h1>Hello from PHP CGI!</h1>\n";
echo "<p>Current time: " . date('Y-m-d H:i:s') . "</p>\n";
echo "<p>Server: " . $_SERVER['SERVER_NAME'] . "</p>\n";
echo "</body>\n";
echo "</html>\n";
?>
