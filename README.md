<pre>
<b>Programming:</b>
K=Key aaa=keynumber[%03d]
R=Relais i=device,jj=relais[%02d]
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
  11-499 ti be set

Conoff {Kaaa} {Rijj} {Ew} {Di,j}
Coff {Kaaa} {Rijj} {Ew} {Di,j}
Con {Kaaa} {Rijj} {Ew} {Di,j}
Ccondon {Kaaa} {Rijj} {Di,j} {Ti,j,s}
Ccondoff {Kaaa} {Rijj} {Di,j} {Ti,j,s}
Cset {Kaaa} {Ew} {Dijj} {Sw,t}
