#include <devkit/gfx/camera.h>

glm::mat4x4 dk::gfx::Camera::V() const
{
    return glm::lookAt(position, lookat, vup);
}

glm::mat4x4 dk::gfx::Camera::P() const
{
    if (projection == Camera::Projection::Perspective)
        return glm::perspective(fov, asp, np, fp);

    else { // Orhtographic
        float size = glm::length(lookat - position);
        float left = -asp * size;
        float right = asp * size;
        float bottom = -size;
        float top = size;
        return glm::ortho(left, right, bottom, top, np, fp);
    }
}

dk::geom::ray3 dk::gfx::Camera::castRay(const glm::vec2& ndc) const
{
    glm::vec2 _ndc = ndc * 2.f - glm::vec2(1, 1);

    // Calculate camera basis vectors
    glm::vec3 w = glm::normalize(position - lookat);
    glm::vec3 u = glm::normalize(cross(vup, w));
    glm::vec3 v = glm::cross(w, u);

    if (projection == Projection::Perspective) {
        // Perspective projection
        float tanFov = tanf(fov / 2.0f);
        float x = _ndc.x * tanFov * asp;
        float y = _ndc.y * tanFov;

        return geom::ray3(position, glm::normalize(u * x + v * y - w));
    }
    else {
        // Orthographic projection
        // In orthographic, direction is the same as -w (camera facing direction)
        glm::vec3 dir = -w;
        // Calculate the ray's origin based on NDC
        float zoom = glm::length(position - lookat);
        return geom::ray3(position + u * _ndc.x * zoom * asp + v * _ndc.y * zoom, -glm::normalize(position - lookat));
    }
}

void dk::gfx::Camera::Orbit::shift(Camera & camera, const glm::vec3& amount)
{
	camera.position += amount;
	camera.lookat += amount;
}

void dk::gfx::Camera::Orbit::tilt(Camera& camera, const glm::vec2& delta)
{
    glm::vec3 offset = camera.position - camera.lookat;

    glm::vec3 axis = camera.vup;
    glm::quat rotation = glm::angleAxis(delta.x, glm::normalize(axis));
    offset = offset * rotation;

    axis = glm::cross(axis, offset);
    rotation = glm::angleAxis(delta.y, glm::normalize(axis));
    offset = offset * rotation;

    camera.position = camera.lookat + offset;
}

void dk::gfx::Camera::Orbit::zoom(Camera& camera, float factor)
{
    glm::vec3 offset = camera.position - camera.lookat;
    camera.position = camera.lookat + offset * factor;
}

std::vector<dk::gfx::RGBADrawData> dk::gfx::draw(const Camera& camera, const glm::vec4& color) 
{
	auto direction = camera.lookat - camera.position;
	geom::plane NP(camera.position + direction * camera.np, direction);
	geom::plane FP(camera.position + direction * camera.fp, direction);

	float tanFov = std::tan(camera.fov);
	glm::dvec2 boxAtOne(tanFov * camera.asp, tanFov);

	camera.castRay(glm::vec2(boxAtOne.x, boxAtOne.y));

	auto rays = [&]() {
		return std::vector<dk::geom::ray3>{
			camera.castRay(glm::vec2( boxAtOne.x,  boxAtOne.y)),
			camera.castRay(glm::vec2(-boxAtOne.x,  boxAtOne.y)),
			camera.castRay(glm::vec2(-boxAtOne.x, -boxAtOne.y)),
			camera.castRay(glm::vec2( boxAtOne.x, -boxAtOne.y))
		};
	}();

	return std::vector<RGBADrawData>{
		RGBADrawData(Primitive::Points, { RGBAVertex(camera.position, color) }),
			RGBADrawData(Primitive::Lines, { 
			RGBAVertex(camera.position, color),
			RGBAVertex(camera.position + glm::vec3(rays.at(0).direction * rays.at(0).intersect(FP)), color),
			RGBAVertex(camera.position, color),
			RGBAVertex(camera.position + glm::vec3(rays.at(1).direction * rays.at(1).intersect(FP)), color),
			RGBAVertex(camera.position, color),
			RGBAVertex(camera.position + glm::vec3(rays.at(2).direction * rays.at(2).intersect(FP)), color),
			RGBAVertex(camera.position, color),
			RGBAVertex(camera.position + glm::vec3(rays.at(3).direction * rays.at(3).intersect(FP)), color),

			RGBAVertex(camera.position + glm::vec3(rays.at(0).direction * rays.at(0).intersect(NP)), color),
			RGBAVertex(camera.position + glm::vec3(rays.at(1).direction * rays.at(1).intersect(NP)), color),
			RGBAVertex(camera.position + glm::vec3(rays.at(1).direction * rays.at(1).intersect(NP)), color),
			RGBAVertex(camera.position + glm::vec3(rays.at(2).direction * rays.at(2).intersect(NP)), color),
			RGBAVertex(camera.position + glm::vec3(rays.at(2).direction * rays.at(2).intersect(NP)), color),
			RGBAVertex(camera.position + glm::vec3(rays.at(3).direction * rays.at(3).intersect(NP)), color),
			RGBAVertex(camera.position + glm::vec3(rays.at(3).direction * rays.at(3).intersect(NP)), color),
			RGBAVertex(camera.position + glm::vec3(rays.at(0).direction * rays.at(0).intersect(NP)), color),

			RGBAVertex(camera.position + glm::vec3(rays.at(0).direction * rays.at(0).intersect(FP)), color),
			RGBAVertex(camera.position + glm::vec3(rays.at(1).direction * rays.at(1).intersect(FP)), color),
			RGBAVertex(camera.position + glm::vec3(rays.at(1).direction * rays.at(1).intersect(FP)), color),
			RGBAVertex(camera.position + glm::vec3(rays.at(2).direction * rays.at(2).intersect(FP)), color),
			RGBAVertex(camera.position + glm::vec3(rays.at(2).direction * rays.at(2).intersect(FP)), color),
			RGBAVertex(camera.position + glm::vec3(rays.at(3).direction * rays.at(3).intersect(FP)), color),
			RGBAVertex(camera.position + glm::vec3(rays.at(3).direction * rays.at(3).intersect(FP)), color),
			RGBAVertex(camera.position + glm::vec3(rays.at(0).direction * rays.at(0).intersect(FP)), color),
		})
	};
}

