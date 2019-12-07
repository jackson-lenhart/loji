@echo off

mkdir ..\build
pushd ..\build
cl -FC -Zi C:\projects\loji\code\loji_win32.cpp user32.lib gdi32.lib
popd