@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Release ^
    "-DCMAKE_MAKE_PROGRAM=C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe" ^
    "-DCMAKE_CXX_COMPILER=C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.39.33519/bin/Hostx64/x64/cl.exe" ^
    "-DCMAKE_CUDA_HOST_COMPILER=C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.39.33519/bin/Hostx64/x64/cl.exe"
