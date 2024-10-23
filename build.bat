mkdir build
cd build
cmake ..
mkdir include\imgui
for %%f in (
    vcpkg_installed\x64-windows\include\imconfig.h 
    vcpkg_installed\x64-windows\include\imgui.h 
    vcpkg_installed\x64-windows\include\imgui_internal.h 
    vcpkg_installed\x64-windows\include\imgui_stdlib.h 
    vcpkg_installed\x64-windows\include\imstb_rectpack.h 
    vcpkg_installed\x64-windows\include\imstb_textedit.h 
    vcpkg_installed\x64-windows\include\imstb_truetype.h) do xcopy /d %%f ..\include\imgui\
cmake --build . --config Debug
cmake --build . --config Release
cd ..