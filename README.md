A simple tool to flash some chips with a WillemProg 3.0
=======================================================

The chips supported and tested:
* AT29C010
* W49F002A
* AM29F040

One can add more by simply putting their ids, sizes, sector sizes and timings into the source code.

It supports both raw parallel port access via port `0x378` and `/dev/parportX`. The latter should be find for most of the chips, but some (e.g. AT29Cxxx) require strict timing which `/dev/parportX` cannot fulfil, at least on my system. With WillemProg 3.0 every memory access required a full address to be shifted over and over again making it a bit slow.

To make it a bit more reliable, the application also sets real-time scheduling if possible (requiring root privileges or `CAP_SYS_NICE` capability). Note that running this application with elevated privileges is not recommended because it was not written with security in mind.

All the chips mentioned above should work with the following jumper settings. Please treat it as reference only because my board had too many errors on silk screen to be reliable source of information. J6, J7 settings should not matter because they set VPP which is not used here. J8 should be set to 5 volts.

Jumper configuration
--------------------

Chip configuration:
```
   Normal  2716  2732  2816  i28F001  AT29C256  W27C/SST27Xxxx
                                                 (erase only)

J1    •      •     •     •      •         •            █
     ••     █•    █•    █•     ██        ••           •█
     ██     █•    █•    █•     ▇▇        ██           ▇▇

J2   ██     █•    ██    █•     ██        ██           ██
     ••     █•    ••    █•     ••        ••           ••
     •      •     •     ▇      •         █            •
     •      •     •     █      •         █            •
```

Programming voltage:
```
  12V     15V     21V     25V

 J7 J6   J7 J6   J7 J6   J7 J6
 █  •    •  •    █  █    •  █
 █  █    █  █    █  █    █  █
 •  █    █  █    •  •    █  •
```

2716:
```
J3
•     █  █  ░  ░  ░  █  ░  █  █  ░  ░  ░  ON
•     ░  ░  █  █  █  ░  █  ░  ░  █  █  █
•     1  2  3  4  5  6  7  8  9  10 11 12
```

2732:
```
J3
•     ░  ░  ░  ░  ░  ░  █  █  █  ░  ░  ░  ON
•     █  █  █  █  █  █  ░  ░  ░  █  █  █
•     1  2  3  4  5  6  7  8  9  10 11 12
```

2764:
```
J3
•     █  █  ░  █  ░  █  ░  ░  █  ░  ░  ░  ON
•     ░  ░  █  ░  █  ░  █  █  ░  █  █  █
•     1  2  3  4  5  6  7  8  9  10 11 12
```

27128:
```
J3
•     █  █  ░  █  ░  █  ░  ░  █  ░  ░  ░  ON
•     ░  ░  █  ░  █  ░  █  █  ░  █  █  █  
•     1  2  3  4  5  6  7  8  9  10 11 12 
```

27256, W27C256:
```
J3
•     █  █  ░  ░  █  █  ░  █  █  ░  ░  ░  ON
•     ░  ░  █  █  ░  ░  █  ░  ░  █  █  █
•     1  2  3  4  5  6  7  8  9  10 11 12
```

27512, W27C512:
```
J3
•     ░  ░  █  ░  █  ░  █  █  █  ░  ░  ░  ON
•     █  █  ░  █  ░  █  ░  ░  ░  █  █  █  
•     1  2  3  4  5  6  7  8  9  10 11 12 
```

27010, 27020, W27C010, W27C020:
```
J3
•     ░  ░  █  ░  █  █  ░  ░  ░  █  █  ░  ON
█     █  █  ░  █  ░  ░  █  █  █  ░  ░  █  
█     1  2  3  4  5  6  7  8  9  10 11 12 
```

27C010, 27C020:
```
J3
•     ░  █  █  ░  █  █  ░  ░  ░  █  █  ░  ON
█     █  ░  ░  █  ░  ░  █  █  █  ░  ░  █  
█     1  2  3  4  5  6  7  8  9  10 11 12 
```

27040, W27C040:
```
J3
•     ░  ░  █  ░  █  █  ░  █  ░  █  ░  █  ON
█     █  █  ░  █  ░  ░  █  ░  █  ░  █  ░  
█     1  2  3  4  5  6  7  8  9  10 11 12 
```

27C080:
```
J3
•     ░  ░  █  ░  █  ░  █  █  █  ░  ░  ░  ON
•A19  █  █  ░  █  ░  █  ░  ░  ░  █  █  █
•     1  2  3  4  5  6  7  8  9  10 11 12
```

29x010, 29x020, 29x040:
```
J3
█     ░  ░  █  ░  █  █  ░  ░  ░  █  █  ░  ON
█     █  █  ░  █  ░  ░  █  █  █  ░  ░  █
•     1  2  3  4  5  6  7  8  9  10 11 12
```


