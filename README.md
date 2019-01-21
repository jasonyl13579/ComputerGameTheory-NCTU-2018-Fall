# Threes- Computer Games Theories
Player：TD learning using 6-tuple weight with 2-4 layers alpha-beta search<br>
Environment：TD learning using 6-tuple weight with 6-8 layers alpha-beta search<br>
<p>
Training example：<br>
./2048 --play="init=enhance_hint load=w_pj5.bin save=w_pj5.bin" --evil="layer=0" --total=10000 --block=1000 --limit=1000<br>
--layer =0 indicates disabling alpha-beta search. Note that only even number is valid.<br>
--pattern enhance_hint:6-tuple with hint, enhance:6-tuple, simple:4-tuple.<br>
Playing example：<br>
./2048 --save="stat.txt" --play="name=TD init=enhance_hint load=w_pj5.bin layer=4" --evil="name=TD_ENV init=enhance_hint load=w_pj5.bin layer=6"<br>
<p>
<p>
Yu-Ming Huang<br>
Mobile Intelligent Network Technology (MINT) Lab, NCTU, Taiwan<br>
http://mint.cm.nctu.edu.tw/<br>
<p>