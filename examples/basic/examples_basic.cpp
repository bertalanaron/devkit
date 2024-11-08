#include "devkit/devkit.h"
#include "devkit/graphics.h"
#include "devkit/log.h"
#include <iostream>
#include <fstream>

//https://stackoverflow.com/questions/17511496/how-to-create-a-shared-library-with-cmake

#define FRAME(producer) if (dk::Frame frame(producer);frame)

int main(int argc, char** argv) {
	std::ofstream ofs("example_basic.log");
	if (ofs.is_open())
		logStream(&ofs);

	dk::Window window({ 
		.title			 = argv[0],
		.backgroundColor = dk::Color{ 68, 129, 156, 255 }});
	window.properties().useDarkTheme = true;
	dk::Window window2;

	window.open();
	dk::Texture texture;
	dk::FrameBuffer fb(texture, {
		.clearColor = { 0, 1, 0, 1 } });

	window2.open();

	while (dk::Window::isAnyOpen()) {
		if (dk::Frame frame(window2); frame)
			ImGui::ShowAboutWindow();

		if (dk::Frame frame(window); frame) {
			ImGui::ShowDemoWindow();
			{
				dk::Frame frame(fb);
				texture.save(L"tmp.png");
			}
		}

		if (dk::Mouse::clicked() && !dk::ImGuiAnyHovered())
			dk::Window::focused().properties().borderEnabled = !dk::Window::focused().properties().borderEnabled;
	}

	return 0;
}
