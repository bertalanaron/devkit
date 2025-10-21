#include <gtest/gtest.h>

#include <devkit/io/window.h>

namespace io_test {

TEST(Window, properties) {
	namespace window_props = dk::io::properties::window;

	dk::io::Window window;
	window.property(window_props::size(72, 36));
	window.property(window_props::title("title"));

	EXPECT_EQ(window.property<window_props::size>(), window_props::size(72, 36));
	EXPECT_EQ(window.property<window_props::title>(), std::string("title"));
}

TEST(Window, open) {
	EXPECT_NO_THROW(
		dk::io::Window window;
		window.open();
		window.close();
	);
}

}
