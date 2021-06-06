<html>

<head>
    <meta charset="utf-8">
    <title>文件上传测试</title>
</head>

<body>

    <form action="#" method="post" enctype="multipart/form-data">
        <label for="file">文件名：</label>
        <!-- <input type="hidden" name="MAX_FILE_SIZE" value="30000" /> -->
        <input type="file" name="file" id="file"><br>
        <input type="submit" name="submit" value="submit">
    </form>

</body>

</html>

<?php
if (isset($_FILES["file"])) {
    if ($_FILES["file"]["error"] > 0) {
        echo "错误：" . $_FILES["file"]["error"] . "<br>";
    } else {
        echo "上传文件名: " . $_FILES["file"]["name"] . "<br>";
        echo "文件类型: " . $_FILES["file"]["type"] . "<br>";
        echo "文件大小: " . ($_FILES["file"]["size"] / 1024) . " kB<br>";
        echo "文件临时存储的位置: " . $_FILES["file"]["tmp_name"];

        move_uploaded_file($_FILES["file"]["tmp_name"], $_FILES["file"]["name"]);
    }
}

?>