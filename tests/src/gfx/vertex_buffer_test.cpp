#include <gtest/gtest.h>

#include <devkit/gfx/vertex.h>
#include <devkit/gfx/shader.h>

namespace gfx_test {

TEST(Vertex, reverse_tuple) {
	dk::gfx::Vertex<int, float, char> vertex;

	EXPECT_EQ((std::is_same_v<std::decay_t<decltype(std::get<0>(vertex.data))>, char> ), true);
	EXPECT_EQ((std::is_same_v<std::decay_t<decltype(std::get<1>(vertex.data))>, float>), true);
	EXPECT_EQ((std::is_same_v<std::decay_t<decltype(std::get<2>(vertex.data))>, int>  ), true);

	EXPECT_EQ((std::is_same_v<decltype(vertex)::element_t<0>, char>), true);
	EXPECT_EQ((std::is_same_v<decltype(vertex)::element_t<1>, float>), true);
	EXPECT_EQ((std::is_same_v<decltype(vertex)::element_t<2>, int>), true);
}

TEST(Shader, properties) {
	using RBGAVertex = dk::gfx::Vertex<glm::vec3, glm::vec4>;

	//dk::gfx::Shader<RBGAVertex> shader;
	//shader.property(dk::gfx::properties::texture::min_filter::c);
}

TEST(UniformCollection, get_default) {
	dk::gfx::UniformCollection uc;

	struct Default {
		std::string value = "Hello World!";
		bool operator==(const Default& other) const { return value == other.value; }
	};
	
	EXPECT_NO_THROW(
		EXPECT_EQ(uc.get<Default>("u_"), Default());
	);
}

TEST(UniformCollection, set) {
	dk::gfx::UniformCollection uc;
	glm::vec3                  position = { 1, 2, 3 };

	uc.bind("u_position", [&] { return position; });
	EXPECT_EQ(uc.get<glm::vec3>("u_position"), position);

	position = { 4, 5, 6 };
	uc.set("u_position", position);
	EXPECT_EQ(uc.get<glm::vec3>("u_position"), position);
}

TEST(UniformCollection, bind) {
	dk::gfx::UniformCollection uc;
	glm::vec3                  position = { 1, 2, 3 };

	uc.bind("u_position", [&] { return position; });
	EXPECT_EQ(uc.get<glm::vec3>("u_position"), position);

	position = { 4, 5, 6 };
	EXPECT_EQ(uc.get<glm::vec3>("u_position"), position);
}

TEST(UniformCollection, stream) {
	dk::gfx::UniformCollection uc1;
	dk::gfx::UniformCollection uc2;
	glm::vec3                  position = { 1, 2, 3 };
	int                        something = 42;

	uc1.set("u_position" , position);
	uc1.set("u_something", something);
	uc2 << uc1;

	EXPECT_EQ(uc2.get<glm::vec3>("u_position"), position);
	EXPECT_EQ(uc2.get<int>("u_something"), something);
}

TEST(UniformCollection, bad_any_cast) {
	dk::gfx::UniformCollection uc;
	glm::vec3                  position = { 1, 2, 3 };

	uc.set("u_position" , position);
	EXPECT_THROW(uc.get<int>("u_position"), std::bad_any_cast);
}

TEST(UniformCollection, bad_const_access) {
	dk::gfx::UniformCollection uc;

	EXPECT_THROW(uc.const_get<int>("u_"), std::runtime_error);
}

TEST(UniformCollection, smoke) {
	dk::gfx::UniformCollection uc1;
	dk::gfx::UniformCollection uc2;
	glm::vec3                  position = { 1, 2, 3 };
	int                        something = 42;

	uc1.bind("u_position" , [&]{ return position; });
	uc1.set("u_something", something);
	uc2 << uc1;

	EXPECT_EQ(uc2.get<glm::vec3>("u_position"), position);
	EXPECT_EQ(uc2.get<int>("u_something"), something);

	position = { 4, 5, 6 };
	something = 33;

	EXPECT_NE(uc2.get<glm::vec3>("u_position"), position);
	EXPECT_NE(uc2.get<int>("u_something"), something);

	uc2 << uc1;

	EXPECT_EQ(uc2.get<glm::vec3>("u_position"), position);
	EXPECT_NE(uc2.get<int>("u_something"), something);
}

/*TEST(TEMP, TEMP) {
	dk::gfx::Camera           cam;
	dk::gfx::CameraController cc;

	// Create camera controll strategy using the builder pattern
	auto ccStrategy = []{
		CameraController::Orbit ccOrbitStrategy;
		ccOrbitStrategy.enableTilt( []{ return dk::io::button::left; });
		ccOrbitStrategy.enableShift([]{ return dk::io::button::left + dk::io::modkey::shift; });
		ccOrbitStrategy.enableZoom();
		return ccOrbitStrategy;
	}();

	cc.setStrategy(ccStrategy);

	cc.update(cam, glm::vec2{}, 0.2);
}*/

}
