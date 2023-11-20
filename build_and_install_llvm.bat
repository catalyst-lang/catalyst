@echo off

if exist "C:\Program Files\Microsoft Visual Studio\2022\Community" (
	call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise" (
	call "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
) else (
	echo Visual Studio 2022 is not installed
	exit 1
)


if not exist llvm mkdir llvm
cd llvm

git clone --config core.autocrlf=false --branch llvmorg-17.0.5 --depth 1 --single-branch https://github.com/llvm/llvm-project.git

if not exist build mkdir build
cd build

cmake -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX="%cd%\..\package" ../llvm-project/llvm
cmake --build . --config Debug

echo.
echo LLVM Build Complete!
echo 

if not exist ..\package mkdir ..\package
cmake --build . --target install --config Debug