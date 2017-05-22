@echo off
set SRC_DIR=%~dp0

if not defined ARCH set ARCH=x86
if defined platform if "%platform%"=="x64" set ARCH=x64

@rem init/update submodules
cd %SRC_DIR%
git submodule update --init

@rem compile dependencies

if "%msbuild_platform%"=="x64" (
    set CMAKE_GENERATOR_NAME=Visual Studio 14 2015 Win64
) else (
    set CMAKE_GENERATOR_NAME=Visual Studio 14 2015
)

set GTEST_BUILD_DIR=%SRC_DIR%deps\googletest\build
if exist %GTEST_BUILD_DIR%\NUL rd /s /q %GTEST_BUILD_DIR%
md %GTEST_BUILD_DIR%
cd %GTEST_BUILD_DIR%
cmake .. -Dgtest_force_shared_crt=ON -G"%CMAKE_GENERATOR_NAME%"
msbuild googlemock\gmock.sln /p:Configuration=%config% /p:Platform="%msbuild_platform%" /clp:NoSummary;NoItemAndPropertyList;Verbosity=minimal /nologo /m
msbuild googlemock\gtest\gtest.sln /p:Configuration=%config% /p:Platform="%msbuild_platform%" /clp:NoSummary;NoItemAndPropertyList;Verbosity=minimal /nologo /m

@rem go back home
cd %SRC_DIR%
