@echo off
setlocal
rem
rem link DYNAMIC against ucrt.lib
rem 
set out=.\build\
cl /nologo /Fo%OUT% /EHsc /Os .\src\dhcpsniffer.c .\src\dhcpdump.c ^
/link /out:%OUT%DHCPSniffer.exe /nodefaultlib ^
kernel32.lib Advapi32.lib user32.lib ole32.lib uuid.lib Ws2_32.lib ^
ucrt.lib libvcruntime.lib libcmt.lib
rem libcpmt.lib

endlocal