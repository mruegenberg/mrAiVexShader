set "HFS=C:\Program Files\Side Effects Software\Houdini 15.5.523"
set "PATH=%HFS%\bin;%PATH%"

hcustom.exe -i ../../publish vexrgb.cpp -I eigen -I libigl\include
rem add -g to compile with debug info

PAUSE
