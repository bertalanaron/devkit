#pragma once
#include <devkit/gfx/vertex.h>
#include <devkit/gfx/draw_data.h>
#include <devkit/algo/geometry.h>

namespace dk::gfx {

struct Camera {
public:
	enum class Projection { Perspective, Orthographic };

	// @brief Get view matrix
	glm::mat4x4 V() const;
	// @brief Get projection matrix
	glm::mat4x4 P() const;

	glm::vec3  position{ 0, 0, -1 };
	glm::vec3  lookat  { 0, 0, 0 };
	glm::vec3  vup     { 0, 1, 0 };

	float	   fov = 1.f;
	float      asp = 1.f;
	float      np  = 0.01f;
	float      fp  = 1000.f;

	Projection projection = Projection::Perspective;

	//Camera() = default;
	//Camera(const Camera&) = default;
	//Camera(Camera&&) = default;
	//Camera& operator=(const Camera&) = default;

	struct Orbit {
		glm::vec3 center = { 0, 0, 0 };
		static void shift(Camera& camera, const glm::vec3& amount);
		static void tilt(Camera& camera, const glm::vec2& delta);
		static void zoom(Camera& camera, float factor);
	};

	geom::ray3 castRay(const glm::vec2&) const;

	glm::vec3 right() const;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Camera, position, lookat, vup, fov, asp, np, fp, projection);

} // dk::gfx

namespace dk::gfx {

[[nodiscard]] std::vector<RGBADrawData> draw(const Camera& camera, const glm::vec4& color);

}
