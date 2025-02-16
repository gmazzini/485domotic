<?php
$server_ip="89.44.206.147";
$server_port=55556;
$message=$argv[1];
if(isset($argv[2]))$message=$message." ".$argv[2];
if(isset($argv[3]))$message=$message." ".$argv[3];
$socket=socket_create(AF_INET,SOCK_DGRAM,SOL_UDP);
socket_set_option($socket,SOL_SOCKET,SO_RCVTIMEO,array("sec"=>1,"usec"=>0));
socket_sendto($socket,$message,strlen($message),0,$server_ip,$server_port);

for(;;){
  $ret=socket_recvfrom($socket,$buf,2000,0,$remote_ip,$remote_port);
  if($ret===false)break;
  $a=strpos($buf,"<next>");
  if($a!==false)echo substr($buf,0,$a).substr($buf,$a+6);
  $a=strpos($buf,"<end>");
  if($a!==false){
    echo substr($buf,0,$a);
    break;
  }
}
socket_close($socket);

?>
