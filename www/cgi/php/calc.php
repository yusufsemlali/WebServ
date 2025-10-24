#!/usr/bin/php-cgi
<?php

function parsePostData() {
    // Try to get CONTENT_LENGTH from environment
    $contentLength = getenv('CONTENT_LENGTH');
    if ($contentLength === false || $contentLength == '') {
        $contentLength = 0;
    } else {
        $contentLength = intval($contentLength);
    }
    
    if ($contentLength > 0) {
        $stdin = fopen('php://stdin', 'r');
        $postData = fread($stdin, $contentLength);
        fclose($stdin);
        parse_str($postData, $data);
        return $data;
    }
    return array();
}

function calculate($num1, $num2, $operation) {
    $n1 = floatval($num1);
    $n2 = floatval($num2);
    
    $operations = array(
        'add' => array('result' => $n1 + $n2, 'symbol' => '+'),
        'subtract' => array('result' => $n1 - $n2, 'symbol' => '-'),
        'multiply' => array('result' => $n1 * $n2, 'symbol' => '×'),
        'divide' => array('result' => $n2 != 0 ? $n1 / $n2 : null, 'symbol' => '÷'),
        'power' => array('result' => pow($n1, $n2), 'symbol' => '^'),
        'modulo' => array('result' => $n2 != 0 ? fmod($n1, $n2) : null, 'symbol' => '%')
    );
    
    if (!array_key_exists($operation, $operations)) {
        return array('error' => 'Invalid operation', 'symbol' => null, 'result' => null);
    }
    
    $op = $operations[$operation];
    
    if ($op['result'] === null) {
        return array('error' => 'Division by zero', 'symbol' => $op['symbol'], 'result' => null);
    }
    
    return array('result' => $op['result'], 'symbol' => $op['symbol'], 'error' => null);
}

// Send HTTP headers first
echo "Content-Type: text/html\r\n";
echo "\r\n";

// Parse POST data
$data = parsePostData();

// Extract parameters
$num1 = isset($data['num1']) ? $data['num1'] : '';
$num2 = isset($data['num2']) ? $data['num2'] : '';
$operation = isset($data['operation']) ? $data['operation'] : '';

// Validate input
if (empty($num1) || empty($num2) || empty($operation)) {
    echo "<h3>❌ Error</h3>";
    echo "<p>Missing parameters. Please provide num1, num2, and operation.</p>";
    exit;
}

// Calculate
$calc = calculate($num1, $num2, $operation);

if ($calc['error']) {
    echo "<h3>❌ Error</h3>";
    echo "<p>" . htmlspecialchars($calc['error']) . "</p>";
} else {
    echo "<h3>✅ Result</h3>";
    echo "<div class='result-value'>" . htmlspecialchars($num1) . " " . htmlspecialchars($calc['symbol']) . " " . htmlspecialchars($num2) . " = " . htmlspecialchars($calc['result']) . "</div>";
    echo "<p><strong>Operation:</strong> " . htmlspecialchars(ucfirst($operation)) . "</p>";
    echo "<p><strong>Language:</strong> PHP 🐘</p>";
}
?>
