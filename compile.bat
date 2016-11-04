set "HOUDINI_DIR=C:\Program Files\Side Effects Software\Houdini 15.5.523"
set "ARNOLD_DIR=deps/Arnold-4.2.14.4-windows"
set "PATH=%HFS%\bin;%PATH%"

"%HOUDINI_DIR%\bin\hcustom.exe" -e -i ./build src/vexrgb.cpp -I %ARNOLD_DIR%/include -L "%ARNOLD_DIR%\lib" -l ai.lib -l libCVEX.a
"%HOUDINI_DIR%\bin\hcustom.exe" -e -i ./build src/vexvolume.cpp -I %ARNOLD_DIR%/include -L "%ARNOLD_DIR%\lib" -l ai.lib -l libCVEX.a

rem -l UI.lib -l OPZ.lib -l OP3.lib -l OP2.lib -l OP1.lib -l SIM.lib -l GEO.lib -l PRM.lib -l UT.lib -l boost_system
rem add -g to compile with debug info

PAUSE
