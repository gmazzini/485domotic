<?php
$fp=fopen("config","r");
for(;;){
  if(feof($fp))break;
  $buf=trim(fgets($fp));
  $pp=explode(" ",$buf);
  foreach($pp as $v){
    if(ctype_upper($v[0])){
    }
  }
}
fclose($fp);

?>
