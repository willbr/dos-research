10 input n
20 i=1
30 c=1
40 j=0
50 t=n-i
60 if j-t goto 80
70 goto 110
80 print " ";
90 j=j+1
100 goto 50
110 k=1
120 if k-i-1 goto 140
130 goto 190
140 print c;
150 c=c*(i-k)/k
160 print " ";
170 k=k+1
180 goto 120
190 print
200 i=i+1
210 if i-n-1 goto 30
run
5
