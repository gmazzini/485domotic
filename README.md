<pre>
<b>Programming:</b>
K=Key i=keybord,j=key
R=Relais i=device,j=relais
E=Event w=event
D=deactive i=HHMM start,j=HHMM stop
T=test i=device,j=relais,s=status(0|1)
w:
  1 SunSet
  2 Sunrise
  3 Every hour
  4 Every minute
  5 Every day at 0600
  6 Every day at 1200
  7 Every day at 1800
  8 Every day at 0000

onoff {Ki,j} {Ri,j} {Ew} {Di,j}
off {Ki,j} {Ri,j} {Ew} {Di,j}
on {Ki,j} {Ri,j} {Ew} {Di,j}
condon {Ki,j} {Ri,j} {Di,j} {Ti,j,s}
condoff {Ki,j} {Ri,j} {Di,j} {Ti,j,s}
  
  
<b>Hardware notes:</b>
1. In the 8relais cut the 1KOhm resistance of RX led to avoid multiple impedence parallel
2. In any MAX485 board cut the 120Ohm resistance from A and B (add only in the head and tail) it is labelled R7
