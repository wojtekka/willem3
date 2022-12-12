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

```
                                             J1
                                              .
                                             ..
                                             ##

                                             J2
                                             ##
                                             ..
                                             .
                                             .
J3
#           #     #  #           #  #
#     #  #     #        #  #  #        #
.     1  2  3  4  5  6  7  8  9  10 11 12
```

