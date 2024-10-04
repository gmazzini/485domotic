<pre>
<b>Programming:</b>
K=Key i=keybord,j=key
R=Relais i=device,j=relais
E=Event On w=event
O=Event Off w=event
D=deactive i=HHMM start,j=HHMM stop
w=1 SunSet,2 Sunrise

onoff {Ki,j} {Ri,j} {Ew} {Ow} D{i,j}
off {Ki,j} {Ri,j} {Ow} D{i,j}
on {Ki,j} {Ri,j} {Ew} D{i,j}
push {Ki,j} {Ri,j} D{i,j}
  
<b>Hardware notes:</b>
1. In the 8relais cut the 1KOhm resistance of RX led to avoid multiple impedence parallel
2. In any MAX485 board cut the 120Ohm resistance from A and B (add only in the head and tail) it is labelled R7
