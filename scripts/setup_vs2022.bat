@echo off

pushd ..
call conan install . --build=missing
call .\vendor\premake\premake5.exe vs2022
popd
pause