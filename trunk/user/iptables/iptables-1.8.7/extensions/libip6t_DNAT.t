:PREROUTING
*nat
-j DNAT --to-destination dead::beef;=;OK
-j DNAT --to-destination dead::beef-dead::fee7;=;OK
-j DNAT --to-destination [dead::beef]:1025-65535;;FAIL
-j DNAT --to-destination [dead::beef] --to-destination [dead::fee7];;FAIL
-p tcp -j DNAT --to-destination [dead::beef]:1025-65535;=;OK
-p tcp -j DNAT --to-destination [dead::beef-dead::fee7]:1025-65535;=;OK
-p tcp -j DNAT --to-destination [dead::beef-dead::fee7]:1025-65536;;FAIL
-p tcp -j DNAT --to-destination [dead::beef-dead::fee7]:1025-65535 --to-destination [dead::beef-dead::fee8]:1025-65535;;FAIL
-p tcp -j DNAT --to-destination [dead::beef-dead::fee7]:1000-2000/1000;=;OK
-p tcp -j DNAT --to-destination [dead::beef-dead::fee7]:1000-2000/3000;=;OK
-p tcp -j DNAT --to-destination [dead::beef-dead::fee7]:1000-2000/65535;=;OK
-p tcp -j DNAT --to-destination [dead::beef-dead::fee7]:1000-2000/0;;FAIL
-p tcp -j DNAT --to-destination [dead::beef-dead::fee7]:1000-2000/65536;;FAIL
-j DNAT;;FAIL
