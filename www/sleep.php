<?php
// sleep.php - CGI test script that sleeps for 5 seconds

header("Content-Type: text/plain");
sleep(5);
echo "Slept for 5 seconds.\n";
?>