@echo off

if not *%1 == *-p goto noarg
set flags=-p
shift
:noarg

if *%1 == * goto nofile
if exist %1 goto isok
echo ÿ
echo printmail: cannot open folder %1
goto exit
:isok
set flags=%flags% -f %1
:nofile

readmsg %flags% *

:exit
set flags=
