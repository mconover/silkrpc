@echo off

setlocal EnableExtensions EnableDelayedExpansion

set HUNTER_BASE=C:\.hunter\_Base\10738b5\f31dc6c\e120df3
set VCPKG_DEFAULT_HOST_TRIPLET=x64-windows
set CMAKE_ARGS=

rem Clean up previous runs
if exist old.9 rd /s /q old.9
if exist old.8 move old.8 old.9
if exist old.7 move old.7 old.8
if exist old.6 move old.6 old.7
if exist old.5 move old.5 old.6
if exist old.4 move old.4 old.5
if exist old.3 move old.3 old.4
if exist old.2 move old.2 old.3
if exist old.1 move old.1 old.2
if exist old move old old.1
mkdir old
move INSTALL old 2>NUL
move *.out old 2>NUL
move *.log old 2>NUL
move *.txt old 2>NUL
if "%~1" == "clean" (
	shift
	echo Cleaning subdirectories
	move CMakeFiles old 2>NUL
	move _3rdparty old 2>NUL
	move cmd old 2>NUL
	move silkrpc old 2>NUL
	move silkworm old 2>NUL
)

set OLD_INCLUDE=%INCLUDE%
set OLD_PATH=%PATH%

rem echo.
rem echo *** Before
rem echo INCLUDE %INCLUDE% >> before.txt
rem echo PATH %PATH% >> before.txt
rem type before.txt

if exist "C:\vcpkg\downloads\tools\perl\5.32.1.1\perl\bin" set PERL_DIR=C:\vcpkg\downloads\tools\perl\5.32.1.1\perl\bin
if exist "C:\perl64\perl\bin" set PERL_DIR=C:\perl64\perl\bin
perl --version 2>NUL
if %errorlevel% neq 0 set PATH=%PERL_DIR%;%PATH%
if %errorlevel% neq 0 perl --version 2>NUL
if %errorlevel% neq 0 (
	echo ERROR: Missing perl error %ERRORLEVEL%
	goto :EOF
)

if "%VCPKG_DEFAULT_TRIPLET%" == "" (
  echo Setting VCPKG_DEFAULT_TRIPLET to x64-windows
  set VCPKG_DEFAULT_TRIPLET=x64-windows
)
echo VCPKG_DEFAULT_TRIPLET="%VCPKG_DEFAULT_TRIPLET%"

if not exist "%VCPKG_ROOT%" (
  echo VCPKG_ROOT is not set
  set VCPKG_ROOT=c:\vcpkg
)
echo VCPKG_ROOT="%VCPKG_ROOT%"

set CMAKE_ARGS=-Wno-dev -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_LIBDIR=lib
set CMAKE_ARGS=%CMAKE_ARGS% -DCMAKE_VERBOSE_MAKEFILE=ON -DDEBUG=ON -DHUNTER_STATUS_DEBUG=ON -DCABLE_DEBUG=ON
set CMAKE_ARGS=%CMAKE_ARGS% -DBUILD_SHARED_LIBS=OFF -DHUNTER_BUILD_SHARED_LIBS=OFF -DOPENSSL_USE_STATIC_LIBS=ON -DOPENSSL_MSVC_STATIC_RT=ON -Dprotobuf_MSVC_STATIC_RUNTIME=ON

if exist "C:\vcpkg\downloads\tools\cmake-3.22.2-windows\cmake-3.22.2-windows-i386\bin" set CMAKE_PATH=C:\vcpkg\downloads\tools\cmake-3.22.2-windows\cmake-3.22.2-windows-i386\bin
if exist "C:\cmake\bin" set CMAKE_PATH=C:\cmake\bin
cmake --version 2>NUL
if %errorlevel% neq 0 set PATH=%CMAKE_PATH%;%PATH%
if %errorlevel% neq 0 cmake --version 
if %errorlevel% neq 0 (
	echo ERROR: Missing cmake error %ERRORLEVEL%
	goto :EOF
)

if "%~1" == "clang" (
	shift
	set CMAKE_ARGS=-DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang+ -DCMAKE_TOOLCHAIN_FILE=../cmake/clang-libcxx20-fpic.cmake %CMAKE_ARGS%
) else (
	set CMAKE_ARGS=%CMAKE_ARGS% -DCMAKE_CXX_STANDARD_COMPUTED_DEFAULT=MSVC -DCMAKE_CXX_EXTENSIONS_COMPUTED_DEFAULT=MSVC
	rem set CMAKE_ARGS=%CMAKE_ARGS% -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
	rem set CMAKE_ARGS=%CMAKE_ARGS% -DVCPKG_PREFER_SYSTEM_LIBS=ON
	rem set CMAKE_ARGS=%CMAKE_ARGS% -DX_VCPKG_APPLOCAL_DEPS_INSTALL=ON
)
set CFLAGS=-D_WINNT_WINDOWS=0x0A00 -D_WIN32_WINDOWS=0x0A00 -DWIN32 -D_WIN32 -DWIN64 -D_WIN64 -DWINDOWS -D_WINDOWS -D_CRT_SECURE_NO_WARNINGS
set CMAKE_ARGS=%CMAKE_ARGS% -DCMAKE_C_FLAGS="%CFLAGS%"

set mimalloc_DIR=c:/vcpkg/packages/mimalloc_x64-windows
if exist "c:\vcpkg\packages\mimalloc_x64-windows" (
	echo Using mimalloc at %mimalloc_DIR%
) else (
	set mimalloc_DIR=
	set CMAKE_ARGS=%CMAKE_ARGS% -DSILKRPC_USE_MIMALLOC=OFF
)

set PROTOBUF_ROOT=C:/vcpkg/packages/protobuf_x64-windows
set PROTOBUF_ROOT_DIR=%PROTOBUF_ROOT%
set PROTOBUF_DIR=C:\vcpkg\packages\protobuf_x64-windows
if not exist "%PROTOBUF_DIR%" (
	echo ERROR: missing "%PROTOBUF_DIR%"
	goto :EOF
)
rem set INCLUDE=%PROTOBUF_DIR%\include;%INCLUDE%

rem
rem Find latest protoc.exe and add it to front of PATH
rem
set PROTOC_DIR=%PROTOBUF_DIR%\tools\protobuf
if exist "c:\protoc-3.14.0-win64\bin" set PROTOC_DIR=c:\protoc-3.14.0-win64\bin
if exist "c:\protoc-3.21.5-win64\bin" set PROTOC_DIR=c:\protoc-3.21.5-win64\bin
echo Checking protoc in %PROTOC_DIR%
%PROTOC_DIR%\protoc --version 2>NUL
if %errorlevel% equ 0 echo protoc found in %PROTOC_DIR%
if %errorlevel% neq 0 (
	echo Checking protoc in %PROTOBUF_DIR%\bin
	set PROTOC_DIR=%PROTOBUF_DIR%\bin
)
if %errorlevel% neq 0 set PATH=%PROTOC_DIR%;%PATH%
if %errorlevel% neq 0 protoc --version 2>NUL
if %errorlevel% neq 0 (
	echo ERROR: Missing protoc error %ERRORLEVEL%
	goto :EOF
)
echo Adding %PROTOC_DIR% to PATH
set PATH=%PROTOC_DIR%;%PATH%

set GRPC_ROOT=c:/vcpkg/packages/grpc_x64-windows
set GRPC_ROOT_DIR=%GRPC_ROOT%
set GRPC_DIR=C:\vcpkg\packages\grpc_x64-windows
rem set LIB=%GRPC_DIR%\lib;%LIB%
rem set INCLUDE=%GRPC_DIR%\include;%INCLUDE%

set GMP_LIBRARY=C:\vcpkg\packages\mpir_x64-windows-static\lib\mpir.lib
set GMP_INCLUDE_DIR=C:\vcpkg\packages\mpir_x64-windows-static\include
set INCLUDE=%GMP_INCLUDE_DIR%;%INCLUDE%
set CMAKE_ARGS=%CMAKE_ARGS% -DGMP_LIBRARY=%GMP_LIBRARY%
rem set LIB=%GMP_LIBRARY%;%LIB%

rem set BOOST_INCLUDEDIR=c:\boost\include
rem set BOOST_LIBRARYDIR=c:\boot\lib
rem set BOOST_ROOT=c:\boost
rem set INCLUDE=%BOOST_INCLUDEDIR%:%INCLUDE%
rem set LIBRARY=%BOOST_LIBRARYDIR%:%LIBRARY%
set CMAKE_ARGS=%CMAKE_ARGS% -DBoost_USE_STATIC_LIBS=ON -DBoost_USE_STATIC_RUNTIME=ON

rem echo.
rem echo *** After
rem echo INCLUDE %INCLUDE% >> after.txt
rem echo PATH %PATH% >> after.txt
rem type after.txt

echo.
echo *** Running: cmake %CMAKE_ARGS% -S .. -B .
cmake %CMAKE_ARGS%  -S .. -B .
echo cmake returned %ERRORLEVEL%

:finished
set EXITVALUE=%ERRORLEVEL%
set INCLUDE=%OLD_INCLUDE%
set PATH=%OLD_PATH%

rem echo *** Finished exit value %EXITVALUE%
rem echo INCLUDE %OLD_INCLUDE% >> finished.txt
rem echo PATH %OLD_PATH% >> finished.txt
rem type finished.txt

exit /b %EXITVALUE%
