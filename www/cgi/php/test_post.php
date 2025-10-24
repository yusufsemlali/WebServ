#!/usr/bin/php-cgi
<?php
echo "Content-Type: text/html\r\n\r\n";
echo "<h3>Environment Test</h3>";
echo "<p>REQUEST_METHOD: " . getenv('REQUEST_METHOD') . "</p>";
echo "<p>CONTENT_LENGTH: " . getenv('CONTENT_LENGTH') . "</p>";
echo "<p>CONTENT_TYPE: " . getenv('CONTENT_TYPE') . "</p>";

$stdin = fopen('php://stdin', 'r');
$data = '';
$len = intval(getenv('CONTENT_LENGTH'));
if ($len > 0) {
    $data = fread($stdin, $len);
}
fclose($stdin);

echo "<p>RAW POST Data: [" . htmlspecialchars($data) . "]</p>";
echo "<p>Data length: " . strlen($data) . "</p>";
?>
