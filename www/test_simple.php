#!/usr/bin/php-cgi
<?php
echo "Content-Type: text/html\r\n";
echo "\r\n";
echo "<!DOCTYPE html>\n";
echo "<html>\n";
echo "<head><title>Simple PHP CGI Test</title></head>\n";
echo "<body>\n";
echo "<h1>🐘 Hello from PHP CGI! 🚀</h1>\n";
echo "<p>✅ PHP CGI is working correctly!</p>\n";
echo "<p>📁 Script location: <code>/www/test_simple.php</code></p>\n";
echo "<p>🕒 Current time: " . date('Y-m-d H:i:s') . "</p>\n";
echo "<p>🔧 PHP Version: " . phpversion() . "</p>\n";
echo "<hr>\n";
echo "<h2>Environment Variables:</h2>\n";
echo "<ul>\n";
echo "<li>REQUEST_METHOD: " . ($_ENV['REQUEST_METHOD'] ?? 'Not set') . "</li>\n";
echo "<li>SCRIPT_NAME: " . ($_ENV['SCRIPT_NAME'] ?? 'Not set') . "</li>\n";
echo "<li>SCRIPT_FILENAME: " . ($_ENV['SCRIPT_FILENAME'] ?? 'Not set') . "</li>\n";
echo "<li>SERVER_NAME: " . ($_ENV['SERVER_NAME'] ?? 'Not set') . "</li>\n";
echo "<li>SERVER_PORT: " . ($_ENV['SERVER_PORT'] ?? 'Not set') . "</li>\n";
echo "</ul>\n";
echo "</body>\n";
echo "</html>\n";
?>
