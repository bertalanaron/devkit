#include "devkit/devkit.h"
#include "devkit/graphics.h"
#include "devkit/log.h"
#include <iostream>
#include <fstream>

#include "../src/graphics_includes.h"

//https://stackoverflow.com/questions/17511496/how-to-create-a-shared-library-with-cmake

#define FRAME(producer) if (dk::Frame frame(producer);frame)

#define MAX_RATE(rate, body) do {							\
	static auto start = std::chrono::system_clock::now();	\
	if (std::chrono::system_clock::now() - start > rate)	\
	{ start = std::chrono::system_clock::now(); body }		\
    } while(false)

int main(int argc, char** argv) {
	// Create and open window
	dk::Window window({ 
		.title			 = argv[0],
		.backgroundColor = DEVKIT_COLOR(0x597c8cff) });
	window.properties().useDarkTheme = true;
	window.open();

	// Initialize asset manager
	dk::AssetManager assetManager;
	auto& assets = 
	assetManager.directory(L"assets/", 
		assetManager.ext(L".glsl" ,  dk::ShaderSource::loadFromFile),
		assetManager.ext(L".glsl" , &dk::ShaderSource::updateFromFile),
		assetManager.ext(L".png"  ,  dk::Texture::load));
	assets.synchronize();

	// Create framebuffer
	dk::Texture fbTexture({ .size = { 1800, 1800 } });
	dk::FrameBuffer fb(fbTexture, {
		.clearColor = DEVKIT_COLOR(0xB0E470FF) });

	// Create shaders
	dk::Shader shader(
		assets.get<dk::ShaderSource>(L"assets/vert.glsl"),
		assets.get<dk::ShaderSource>(L"assets/frag.glsl"));
	dk::Shader fbShader(
		assets.get<dk::ShaderSource>(L"assets/revVert.glsl"),
		assets.get<dk::ShaderSource>(L"assets/revFrag.glsl"));

	// Create a vertex buffer of a UV rect
	dk::VertexBuffer<glm::vec3, glm::vec2> uvRect({
	//	|    UV	   |  POSITION  |
		{ { 0, 0 }, {-1,-1, 0 } },
		{ { 1, 0 }, { 1,-1, 0 } },
		{ { 1, 1 }, { 1, 1, 0 } },
		{ { 0, 0 }, {-1,-1, 0 } },
		{ { 0, 1 }, {-1, 1, 0 } },
		{ { 1, 1 }, { 1, 1, 0 } } });

	// Create camera and set up handler
	dk::CameraController camera(new dk::CameraControllerOrbit());
	dk::CameraControllHandler cameraHandler{
		.tiltWith       = { dk::KeyMod::NONE , dk::Mouse::Button::Left },
		.shiftWith      = { dk::KeyMod::SHIFT, dk::Mouse::Button::Left },
		.zoomInWith     = { dk::Mouse::WheelDirection::Up              },
		.zoomOutWith    = { dk::Mouse::WheelDirection::Down            },
		.tiltMultiplier = { .01f, -.01f }, .shiftMultiplier = { -.002f, .002f }, .zoomMultiplier = -.1f };

	std::cout << nlohmann::json(cameraHandler).dump(1) << std::endl;

	// Begin main loop
	while (dk::Window::isAnyOpen()) {
		// Synchronize assets every 500ms
		MAX_RATE(std::chrono::milliseconds(500), {
			assets.synchronize();
			});

		// Update camera
		if (!dk::ImGuiAnyHovered())
			camera.handle(cameraHandler, window.cursor().delta(dk::Cursor::Window), fabs((float)dk::Mouse::wheel()));

		// Begin window frame
		if (dk::Frame frame(window); frame) {
			if (ImGui::Begin("Example: Basics")) {
				ImGui::Text("Tilt the camera by dragging with left mouse button and tilt SHIFT dragging.");

				ImGui::Checkbox("window.alwaysOnTop", &window.properties().alwaysOnTop);
				ImGui::Checkbox("window.borderEnabled", &window.properties().borderEnabled);
				ImGui::Checkbox("window.useDarkTheme", &window.properties().useDarkTheme);
				ImGui::ColorEdit4("framebuffer.clearColor", &fb.properties().clearColor.r);
			} ImGui::End();

			// Begin frame in framebuffer
			dk::Viewport viewport(fb, {
				.offset = { 50, 50 },
				.size   = { fb.frameSize().x - 100, fb.frameSize().y - 100 } });
			viewport.beginFrame();

			// Bind texture
			assets.get<dk::Texture>(L"assets/texture.png").bind();
			fbShader.setUniform("u_texture", assets.get<dk::Texture>(L"assets/texture.png"));

			// Draw texture to UV rect
			camera.camera().asp = viewport.aspectRatio();
			fbShader.setUniform("u_camera", camera);
			uvRect.draw(dk::Primitive::Triangles, fbShader);
			viewport.endFrame();

			// Draw framebuffer to UV rect in window
			fbTexture.bind();
			camera.camera().asp = window.aspectRatio();
			shader.setUniform("u_camera", camera);
			shader.setUniform("u_texture", fbTexture);
			uvRect.draw(dk::Primitive::Triangles, shader);
		}

		// Save screenshot
		if (dk::InputCombination(dk::KeyMod::CTRL, dk::Key::S).activated()) {
			DBG("Screenshot saved to screenshot.png");
			fbTexture.save(L"screenshot.png");
		}
	}

	return 0;
}
