@echo off
REM Build the raylib Dark Souls clone with MSVC + Ninja (bundled in VS Build Tools).
setlocal
set "VS=C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools"
set "CMAKE=%VS%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
set "NINJA=%VS%\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"

call "%VS%\VC\Auxiliary\Build\vcvars64.bat" || goto :err

if not exist build ( mkdir build )
"%CMAKE%" -S "%~dp0." -B "%~dp0build" -G Ninja -DCMAKE_MAKE_PROGRAM="%NINJA%" -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=cl -DCMAKE_CXX_COMPILER=cl || goto :err
"%CMAKE%" --build "%~dp0build" || goto :err
echo BUILD_OK
exit /b 0
:err
echo BUILD_FAILED errorlevel=%errorlevel%
exit /b 1
