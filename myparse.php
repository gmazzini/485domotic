<?php
$fp=fopen("config","r");
$nD=0;
for(;;){
  if(feof($fp))break;
  $buf=trim(fgets($fp));
  $pp=explode(" ",$buf);
  $nlK=0; $lK=array();
  $nlR=0; $lR=array();
  $nlE=0; $lE=array();
  $nlO=0; $lO=array();
  $nlD=0; $lD=array();
  foreach($pp as $v){
    switch($v[0]){
      case "K": // Ki,j -> q=10i+j
      $l=strpos($v,",");
      $q=10*((int)substr($v,1,$l))+(int)substr($v,$l+1);
      $lK[$nlK++]=$q;
      break;
      case "R": // Ri,j -> q=10i+j
      $l=strpos($v,",");
      $q=10*((int)substr($v,1,$l))+(int)substr($v,$l+1);
      $lR[$nlR++]=$q;
      break;
      case "E":
      $q=(int)substr($v,1);
      $lE[$nlE++]=$q;
      break;
      case "O":
      $q=(int)substr($v,1);
      $lO[$nlO++]=$q;
      break;
      case "D":
      $start=((int)substr($v,1,2))*60+((int)substr($v,3,2));
      $end=((int)substr($v,6,2))*60+((int)substr($v,8,2));
      
    }
  }
}
fclose($fp);

?>
