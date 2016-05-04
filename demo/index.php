<?php
error_reporting(E_ALL);
header("Content-type: text/html; charset=UTF-8");
echo 'memory usage:',memory_get_peak_usage()/1024,'kb.<br/>';
$stime=microtime(true); 


$app = new gene_application();
$app
->load("router.ini.php")
->load("config.ini.php")
->run()
;



$etime=microtime(true);
$total=$etime-$stime;
echo "<br/><br/>[页面执行时间：{$total} ]秒 ";
echo 'memory usage:',memory_get_peak_usage()/1024,'kb.';