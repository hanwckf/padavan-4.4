:INPUT,FORWARD,OUTPUT
-p esp -m esp --espspi 0;=;OK
-p esp -m esp --espspi :32;-p esp -m esp --espspi 0:32;OK
-p esp -m esp --espspi 0:4294967295;-p esp -m esp;OK
-p esp -m esp ! --espspi 0:4294967294;=;OK
-p esp -m esp --espspi -1;;FAIL
-p esp -m esp;=;OK
-m esp;;FAIL
