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

void dk::gfx::Camera::Orbit::shift(Camera & camera, const glm::vec3& amount)
{

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
