@echo off
skcmaputil.exe clear global.pak global.pak.tmp
copy /y global.pak.tmp global.pak
del global.pak.new

