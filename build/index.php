<?php
echo " hello";
$conn = mysqli_connect("127.0.0.1", "root", "root", "db_test", "3306");
$conn->query("use db_test");
$res  = $conn->query("select *from tb_test");

if ($res->num_rows > 0) {
    while (($row = $res->fetch_assoc())) {
        echo  $row["username"] . "  " . $row["passwd"] . "</br>";
    }
}

$conn->close();

if (!isset($_SERVER['PHP_AUTH_USER']) || !isset($_SERVER['PHP_AUTH_PW'])) {
    header('WWW-Authenticate: Basic realm="user login"');
    header('HTTP/1.0 401 Unauthorized');
    echo 'Please input username and password.';
} else {
    echo "<p>Hello \"{$_SERVER['PHP_AUTH_USER']}\".</p>";
    echo "<p>You entered \"{$_SERVER['PHP_AUTH_PW']}\" as your password.</p>";
}

echo phpinfo();
