@echo off
set "PATH=C:\Users\n_t\AppData\Local\Programs\CLion\bin\mingw\bin;C:\Users\n_t\AppData\Local\Programs\CLion\bin\ninja\win\x64;%PATH%"
set "CLION_CMAKE=C:\Users\n_t\AppData\Local\Programs\CLion\bin\cmake\win\x64\bin\cmake.exe"
set "CLION_NINJA=C:\Users\n_t\AppData\Local\Programs\CLion\bin\ninja\win\x64\ninja.exe"
set "CLION_GXX=C:\Users\n_t\AppData\Local\Programs\CLion\bin\mingw\bin\g++.exe"
set "CLION_GCC=C:\Users\n_t\AppData\Local\Programs\CLion\bin\mingw\bin\gcc.exe"

echo --- Cleaning up build directory ---
if exist build (
    rmdir /s /q build
)
mkdir build

echo --- Configuring project with CMake ---
"%CLION_CMAKE%" -G Ninja ^
    -DCMAKE_CXX_COMPILER="%CLION_GXX%" ^
    -DCMAKE_C_COMPILER="%CLION_GCC%" ^
    -DCMAKE_MAKE_PROGRAM="%CLION_NINJA%" ^
    -S . -B build

if %ERRORLEVEL% neq 0 (
    echo [ERROR] CMake configuration failed!
    exit /b %ERRORLEVEL%
)

echo --- Building project ---
"%CLION_CMAKE%" --build build

if %ERRORLEVEL% neq 0 (
    echo [ERROR] Build failed!
    exit /b %ERRORLEVEL%
)

echo --- Done! ---
echo Executable is located at: build\matrix_ai.exe
