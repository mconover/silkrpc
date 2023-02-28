@echo off

setlocal EnableExtensions EnableDelayedExpansion

set SOLUTION=silkrpc.sln
set CLEAN=0
set BUILD=0
set PACKAGE=0
set INSTALL=0
set OUTDIR=INSTALL

if "%1" == "all" (
	shift
	set CLEAN=1
	set BUILD=1
	set PACKAGE=1
	set INSTALL=1
) else if "%1" == "clean" (
	shift
	set CLEAN=1
) else if "%1" == "build" (
	shift
	set BUILD=1
) else if "%1" == "package" (
	shift
	set PACKAGE=1
) else if "%1" == "install" (
	shift
	set INSTALL=1
)

echo CLEAN=%CLEAN% BUILD=%BUILD% PACKAGE=%PACKAGE% INSTALL=%INSTALL%
echo [%DATE%] Start CLEAN=%CLEAN% BUILD=%BUILD% PACKAGE=%PACKAGE% INSTALL=%INSTALL% > OUTPUT.txt

if exist "%SOLUTION%" (
	echo Solution "%SOLUTION%" already exists
	goto :solution_exists
)

if exist "..\run_cmake.bat" (
	echo [%DATE% %TIME%] Calling ..\run_cmake.bat
	echo [%DATE% %TIME%] Calling ..\run_cmake.bat >> OUTPUT.txt
	if "%~1" == "stdout" (
		call ..\run_cmake.bat
	) else (
		call ..\run_cmake.bat > OUTPUT.txt
	)
	if %ERRORLEVEL% NEQ 0 goto :EOF
	echo [%DATE% %TIME%] run_cmake.bat returned %ERRORLEVEL%
	echo [%DATE% %TIME%] run_cmake.bat returned %ERRORLEVEL% >> OUTPUT.txt
) else (
	echo [%DATE% %TIME%] cmake -S .. -B .
	echo [%DATE% %TIME%] cmake -S .. -B . >> OUTPUT.txt
	if exist "CMakeCache.txt" del CMakeCache.txt
	if "%~1" == "stdout" (
		cmake -S .. -B .
	) else (
		cmake -S .. -B . > OUTPUT.txt
	)
	echo [%DATE% %TIME%] cmake returned %ERRORLEVEL%
	echo [%DATE% %TIME%] cmake returned %ERRORLEVEL% >> OUTPUT.txt
)
echo.
if %errorlevel% NEQ 0 goto :EOF

:solution_exists
if %CLEAN% EQU 1 (
	rem echo [%DATE% %TIME%] msbuild %SOLUTION% -t:Clean -p:Configuration="Release"
	rem echo [%DATE% %TIME%] msbuild %SOLUTION% -t:Clean -p:Configuration="Release" >> OUTPUT.txt
	rem msbuild %SOLUTION% -t:Clean -p:Configuration="Release" >> OUTPUT.txt
	rem echo [%DATE% %TIME%] msbuild returned %ERRORLEVEL%
	rem echo [%DATE% %TIME%] msbuild returned %ERRORLEVEL% >> OUTPUT.txt

	echo [%DATE% %TIME%] devenv %SOLUTION% /clean "Release"
	echo [%DATE% %TIME%] devenv %SOLUTION% /clean "Release" >> OUTPUT.txt
	if "%~1" == "stdout" (
		devenv %SOLUTION% /clean "Release"
	) else (
		devenv %SOLUTION% /clean "Release"  >> OUTPUT.txt
	)
	echo [%DATE% %TIME%] devenv returned %ERRORLEVEL%
	echo [%DATE% %TIME%] devenv returned %ERRORLEVEL% >> OUTPUT.txt

	echo.
	if %errorlevel% NEQ 0 goto :EOF
)

rem
rem Find latest protoc.exe and add it to front of PATH
rem
set PROTOBUF_DIR=C:\vcpkg\packages\protobuf_x64-windows
if not exist "%PROTOBUF_DIR%" (
	echo ERROR: missing "%PROTOBUF_DIR%"
	goto :EOF
)
set PROTOC_DIR=%PROTOBUF_DIR%\tools\protobuf
if exist "c:\protoc-3.14.0-win64\bin" set PROTOC_DIR=c:\protoc-3.14.0-win64\bin
if exist "c:\protoc-3.21.5-win64\bin" set PROTOC_DIR=c:\protoc-3.21.5-win64\bin
protoc --version 2>NUL
if %errorlevel% equ 0 (
	set PROTOC_DIR=
	echo protoc is already in PATH
) else (
	echo Checking protoc in %PROTOC_DIR%
	%PROTOC_DIR%\protoc --version 2>NUL
	if %errorlevel% equ 0 echo protoc found in %PROTOC_DIR%
)
if %errorlevel% neq 0 (
	echo ERROR: Missing protoc error %ERRORLEVEL%
	goto :EOF
)
if exist "%PROTOC_DIR%" echo Adding %PROTOC_DIR% to PATH
if exist "%PROTOC_DIR%" set PATH=%PROTOC_DIR%;%PATH%

if %BUILD% EQU 1 (
	rem echo [%DATE% %TIME%] msbuild %SOLUTION% -t:ALL_BUILD -p:Configuration="Release"
	rem echo [%DATE% %TIME%] msbuild %SOLUTION% -t:ALL_BUILD -p:Configuration="Release" >> OUTPUT.txt
	rem msbuild %SOLUTION% -t:ALL_BUILD  -p:Configuration="Release" >> OUTPUT.txt
	rem echo [%DATE% %TIME%] msbuild returned %ERRORLEVEL%
	rem echo [%DATE% %TIME%] msbuild returned %ERRORLEVEL% >> OUTPUT.txt

	rem set LIB=%PROTOBUF_DIR%\lib;%LIB%
	rem set INCLUDE=%PROTOBUF_DIR%\include;%INCLUDE%

	echo [%DATE% %TIME%] devenv %SOLUTION% /build "Release" /project ALL_BUILD
	echo [%DATE% %TIME%] devenv %SOLUTION% /build "Release" /project ALL_BUILD >> OUTPUT.txt
	if "%~1" == "stdout" (
		devenv %SOLUTION% /build "Release" /project ALL_BUILD
	) else (
		devenv %SOLUTION% /build "Release" /project ALL_BUILD >> OUTPUT.txt
	)
	echo [%DATE% %TIME%] devenv returned %ERRORLEVEL%
	echo [%DATE% %TIME%] devenv returned %ERRORLEVEL% >> OUTPUT.txt
	
	echo.
	if %errorlevel% NEQ 0 goto :EOF
)

if %PACKAGE% EQU 1 (
	if not exist "PACKAGE.vcxproj" (
		echo Missing PACKAGE.vcxproj
	) else (
		rem Same as: cpack -C Debug --config ./CPackConfig.cmake

		rem echo [%DATE% %TIME%] msbuild PACKAGE.vcxproj -p:Configuration="Release"
		rem echo [%DATE% %TIME%] msbuild PACKAGE.vcxproj -p:Configuration="Release" >> OUTPUT.txt
		rem msbuild PACKAGE.vcxproj -p:Configuration="Release" >> OUTPUT.txt
		rem echo [%DATE% %TIME%] msbuild returned %ERRORLEVEL% >> OUTPUT.txt

		echo [%DATE% %TIME%] devenv %SOLUTION% /build "Release" /project PACKAGE
		echo [%DATE% %TIME%] devenv %SOLUTION% /build "Release" /project PACKAGE >> OUTPUT.txt
		if "%~1" == "stdout" (
			devenv %SOLUTION% /build "Release" /project PACKAGE
		) else (
			devenv %SOLUTION% /build "Release" /project PACKAGE >> OUTPUT.txt
		)
		echo [%DATE% %TIME%] devenv returned %ERRORLEVEL%
		echo [%DATE% %TIME%] devenv returned %ERRORLEVEL% >> OUTPUT.txt
		
		echo.
		if %errorlevel% NEQ 0 goto :EOF
	)
)

if %INSTALL% EQU 1 (
	if exist "%OUTDIR%" rd /s /q "%OUTDIR%"
	mkdir "%OUTDIR%"
	if not exist "%OUTDIR%" (
		echo ERROR: unable to create "%OUTDIR%"
		goto :EOF
	)

	if not exist "INSTALL.vcxproj" (
		echo Missing INSTALL.vcxproj
	) else (
		rem Same as: cmake -DBUILD_TYPE=Release -P cmake_install.cmake
		
		echo [%DATE% %TIME%] devenv %SOLUTION% /build "Release" /project INSTALL
		echo [%DATE% %TIME%] devenv %SOLUTION% /build "Release" /project INSTALL >> OUTPUT.txt
		if "%~1" == "stdout" (
			devenv %SOLUTION% /build "Release" /project INSTALL
		) else (
			devenv %SOLUTION% /build "Release" /project INSTALL >> OUTPUT.txt
		)
		echo [%DATE% %TIME%] devenv returned %ERRORLEVEL%
		echo [%DATE% %TIME%] devenv returned %ERRORLEVEL% >> OUTPUT.txt

		if exist "silkrpc\proto-bindings" (
			mkdir %OUTDIR%\silkrpc
			move silkrpc\proto-bindings %OUTDIR%\silkrpc\interfaces
		) else (
			echo ERROR: missing %OUTDIR%\silkrpc
			goto :EOF
		)
		
		move cmd\Release\* "%OUTDIR%" 2>NUL
		move silkrpc\Release\* "%OUTDIR%"
		move silkworm\Release\* "%OUTDIR%"
		move silkworm\core\Release\* "%OUTDIR%"
		move silkworm\node\Release\* "%OUTDIR%"
		move silkworm\sentry\Release\* "%OUTDIR%"
		move silkworm\third_party\cbor-cpp\Release\* "%OUTDIR%"
		move silkworm\third_party\evmone\evmc\lib\hex\Release\* "%OUTDIR%"
		move silkworm\third_party\evmone\evmc\lib\instructions\Release\* "%OUTDIR%"
		move silkworm\third_party\evmone\evmc\lib\loader\Release\* "%OUTDIR%"
		move silkworm\third_party\evmone\evmc\lib\tooling\Release\* "%OUTDIR%"

		move silkworm\third_party\libmdbx\Release\mdbx.lib "%OUTDIR%"
		move silkworm\third_party\libmdbx\Release\*.exe "%OUTDIR%"
		move silkworm\third_party\silkpre\Release\* "%OUTDIR%"
		move silkworm\third_party\silkpre\lib\Release\* "%OUTDIR%"

		echo.
		dir "%OUTDIR%"
	)
)

echo [%DATE% %TIME%] Finished
echo [%DATE% %TIME%] Finished >> OUTPUT.txt
