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
    vcpkg_installed\x64-windows\include\imstb_truetype.h
    vcpkg_installed\x64-windows\include\imgui_impl_opengl3.h
    vcpkg_installed\x64-windows\include\imgui_impl_opengl3_loader.h
    vcpkg_installed\x64-windows\include\imgui_impl_sdl2.h) do xcopy /d %%f ..\include\imgui\
cmake --build . --config Debug
cmake --build . --config Release
cd ..