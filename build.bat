cl genpal.cpp
genpal
yasm-1.3.0-win64.exe -o cgargb.drv cgargb.s
yasm-1.3.0-win64.exe -o cgacomp.drv cgacomp.s
yasm-1.3.0-win64.exe -o cgagrey.drv cgagrey.s
copy cgargb.drv "d:\SteamLibrary\steamapps\common\Space Quest Collection\sq1vga"
copy cgacomp.drv "d:\SteamLibrary\steamapps\common\Space Quest Collection\sq1vga"
