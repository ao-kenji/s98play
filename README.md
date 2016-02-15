s98play
=======

s98play - An S98 file player with PC-9801-86 sound board on OpenBSD/luna88k


Preparation
-----------
To use this program, make sure that 'pcex0' is recognized in dmesg.
```
cbus0 at mainbus0
pcex0 at cbus0
```

Compile
-------
Build by 'make'.  The executable binary is 's98play'.

Run
---
```
% ./s98play [-d] S98file
```

Limitations
-----------
Due to the lack of machine power, some s98 files are not played at correct speed.
