<pre>
<b>Programming:</b>
K=Key i=keybord,j=key
R=Relais i=device,j=relais
E=Event w=event
D=deactive i=HHMM start,j=HHMM stop
T=test i=device,j=relais,s=status(0|1)
w:
  0 Reserved for internal communications
  1 Every minute
  2 Every 10 minutes
  3 Every 30 minutes
  4 Every hour
  5 Every day at 0000
  6 Every day at 0600
  7 Every day at 1200
  8 Every day at 1800
  9 Every sunrise
  10 Every sunset

Conoff {Ki,j} {Ri,j} {Ew} {Di,j}
Coff {Ki,j} {Ri,j} {Ew} {Di,j}
Con {Ki,j} {Ri,j} {Ew} {Di,j}
Ccondon {Ki,j} {Ri,j} {Di,j} {Ti,j,s}
Ccondoff {Ki,j} {Ri,j} {Di,j} {Ti,j,s}
  
  
<b>Hardware notes:</b>
1. In the 8relais cut the 1KOhm resistance of RX led to avoid multiple impedence parallel
2. In any MAX485 board cut the 120Ohm resistance from A and B (add only in the head and tail) it is labelled R7


It seems that serial mode has embedded this command not good to put more devices in parallel
pinMode(_hwserial_rx_pin, INPUT_PULLUP);
