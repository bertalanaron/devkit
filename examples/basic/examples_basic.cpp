#include "devkit/devkit.h"
#include <iostream>

int main(int argc, char** argv) {
	dk::Window window({ .title = "Examples: basic" });
	window.open();
	window.properties().useDarkTheme = true;

	while (window.isOpen()) {
		window.render();
	}

	return 0;
}
