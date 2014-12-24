@echo off
skcmaputil.exe append global.pak global.pak.tmp gamecontrollerdb.txt
copy /y global.pak.tmp global.pak
del global.pak.new

