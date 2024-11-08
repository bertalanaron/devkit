#include <devkit/devkit.h>
#include <devkit/graphics.h>
#include <imgui/imgui.h>

int main(int argc, char** argv) {
	dk::AssetManager assetManager;
	assetManager.attachInitHandler(L"png", dk::Texture::load);

	dk::Window window;
	window.open();

	while (window.isOpen()) {
		dk::Frame frame(window);

		assetManager.synchronize(L"assets/");
	}

	return 0;
}
