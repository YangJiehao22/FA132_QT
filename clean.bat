if "%1"=="" set dir="."   
goto _lab  
set dir="%1"  
:_lab  
for /R %dir% %%i in (*.tlog *.sdf *.vsp *.exp *.obj *.pch *.idb *.ilk *.sbr *.tmp *.res *.dcu *.opt *.ncb *.plg *.bsc *.aps *.exp *.log *.wrn *.mac) do @del /f /s /q "%%i"  
for /R %dir% %%i in (ipch) do @rmdir /q /s "%%i"  


pause
