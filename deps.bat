@echo off
setlocal

set project_root=%CD%

set clean=0
set build=0
set build_dir=build
set build_config=Release

:parse_cmdline_options
if "%1" == "" goto end_parsing_cmdline_options
if "%1" == "clean" set clean=1
if "%1" == "build" set build=1
if "%1" == "debug" (
  set build_dir=debug
  set build_config=Debug
)
if "%1" == "release" (
  set build_dir=build
  set build_config=Release
)
shift
goto parse_cmdline_options
:end_parsing_cmdline_options

if %clean% == 0 (
if %build% == 0 (
  set build=1
))

if %clean% == 1 (
  rmdir /s /q deps\capnproto\build
)

set deps_cmake_flags=-G%CMAKE_GENERATOR%^
 -A%ARCH%^
 -T%PLATFORM_TOOLSET%^
 -DCMAKE_CONFIGURATION_TYPES:STRING="%build_config%"^
 -DCMAKE_CXX_FLAGS_RELEASE:STRING="/MT /O2 /Ob2 /DNDEBUG"^
 -DCMAKE_C_FLAGS_RELEASE:STRING="/MT /O2 /Ob2 /DNDEBUG"^
 -DCMAKE_CXX_FLAGS_DEBUG:STRING="/MTd /Od"^
 -DCMAKE_C_FLAGS_DEBUG:STRING="/MTd /Od"^
 -DCMAKE_INSTALL_PREFIX:PATH="%project_root%"

if %build% == 1 (
  echo building capnproto.
  pushd deps\capnproto
  cmake . -B%build_dir% %deps_cmake_flags%^
	-DBUILD_SHARED_LIBS:BOOL=OFF^
	-DBUILD_TESTING:BOOL=OFF
  if errorlevel 1 goto error
  cmake --build %build_dir% --config %build_config% --target INSTALL
  if errorlevel 1 goto error
  popd
)


echo.
echo done.
echo.
goto exit

:error
set exitcode=%errorlevel%
echo.
echo error building deps.
echo.

:exit
set PATH=%OLD_PATH%
exit /b %exitcode%
