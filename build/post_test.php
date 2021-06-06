<html>

<body>
    <?php
    header("Content-type: text/html; charset=utf-8");
    ?>
    <form action="" method="post">
        Name: <input type="text" name="name">
        Email: <input type="text" name="email">
        <input type="submit" value="submit">
    </form>
</body>

</html>
<?php
foreach ($_SERVER as $key => $value) {
    echo  $key . " : " . $value . "<br/>";
}
echo  "<br/>";
print_r($_FILES);
echo  "<br/>";
print_r($_REQUEST);
echo  "<br/>";
print_r($_GET);
echo  "<br/>";
print_r($_POST);
echo  "<br/>";
?>