::@ECHO OFF

::REM 10 tracs, 16 heads, 63 sectors-per-track = 10080 sectors
SET maxbytes=5160960
SET mbr=..\bin\mbr.img
SET bbp=..\bin\bbp.img
SET output=..\bin\disk.img

buildimg.exe -m %mbr% -b %bbp% -s %maxbytes% %output%

ECHO Cleanup
::DEL %mbr%
::DEL %bbp%

::PAUSE