#pragma once
#include "game_state.h"
#include "ui_utils.h"

class GameClient 
	: public ClientBase
{
public:
	using ClientBase::ClientBase;

	void update(const dk::io::Frame& frame) override
	{
		if (m_inputs.activated("close"))
			stop();

		m_assets.synchronize();

		if (ImGui::Begin("Hi")) {
			for (auto [path, texture] : m_assets.each<dk::gfx::Texture>()) {
				auto imId = texture.imguiTextureId();
				ImGui::Image(imId, ImVec2(32, 32));
			} 
		} ImGui::End();
	}

	void render(const dk::io::Frame& frame) override
	{
		dk::gfx::backBuffer().clear(dk::gfx::FrameBuffer::ClearMask::Color, dk::colors::black);
	}

private:
	void setup() override
	{
		setupAssetManager(m_assets, ini());
		m_inputs.define("close", dk::io::key::x);
	}
};
