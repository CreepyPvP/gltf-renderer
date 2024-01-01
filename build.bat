@echo off
cd build
del Debug\*.exe
msbuild /nologo /v:q strategy.sln
