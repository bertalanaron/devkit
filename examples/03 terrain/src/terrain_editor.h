#pragma once
#include <devkit/gfx/frame_buffer.h>
#include <devkit/gfx/vertex_sink.h>
#include <devkit/gfx/camera.h>
#include <devkit/io/window.h>
#include <devkit/io/asset_manager.h>
#include <devkit/algo/draw.h>
#include <devkit/io/input_combination.h>
#include <devkit/gfx/scene.h>

#include <imgui.h>

#include "terrain.h"
#include "view.h"

class TerrainEditor {
public:
	TerrainEditor(dk::io::AssetManager& assets)
		: m_vOut(std::move(dk::gfx::VertexSink::create<dk::gfx::RGBAVertex>()))
		, m_vOutNavmeshGeneration(std::move(dk::gfx::VertexSink::create<dk::gfx::RGBAVertex>()))
		, m_draw2d(dk::geom::plane::Y(), -dk::geom::axis::X)
	{
		// Set up shaders
		// Debug
		m_shaders.insert("rgba", assets.getMultipleWeak<dk::gfx::ShaderSource>("/shaders/rgba_vs.glsl", "/shaders/rgba_fs.glsl"));
		// Terrain
		m_shaders.insert("terrain", assets.getMultipleWeak<dk::gfx::ShaderSource>("/shaders/rts/terrain_vs.glsl", "/shaders/rts/terrain_fs.glsl"));
		m_shaders["terrain"].property(dk::gfx::properties::depth_test::enabled);

		// Setup global debug outputs
		dk::dbg::store<dk::gfx::VertexSink*, "navmesh_poly_out">() = &m_vOutNavmeshGeneration;

		// Setup inputs
		m_inputs.define("apply"         , dk::io::button::left);
		m_inputs.define("apply_positive", dk::io::modkey::none  + dk::io::button::left);
		m_inputs.define("apply_negative", dk::io::modkey::shift + dk::io::button::left);
		m_inputs.define("save"          , dk::io::modkey::ctrl + dk::io::key::s);
		m_inputs.define("load"          , dk::io::modkey::ctrl + dk::io::key::l);
	}

	void update(Terrain& terrain, View& view)
	{
		const auto cursor = dk::geom::xz(dk::geom::intersection(view.cursorRay(), dk::geom::plane::Y()));
		
		m_vOut << dk::gfx::draw(dk::geom::edge3{glm::vec3(0, 0, 0), dk::geom::axis::X}, dk::colors::red)
			   << dk::gfx::draw(dk::geom::edge3{glm::vec3(0, 0, 0), dk::geom::axis::Y}, dk::colors::lime)
			   << dk::gfx::draw(dk::geom::edge3{glm::vec3(0, 0, 0), dk::geom::axis::Z}, dk::colors::blue);

		changeTerrainHeight(terrain, cursor);

		const auto offsetFromVertexCurr = dk::dbg::store<float, "offset_from_vertex">();
		if (ImGui::Begin("marching squares"))
		{
			ImGui::Text("offset_from_vertex:");
			ImGui::SameLine();
			ImGui::DragFloat("##offsetFromVertex", &dk::dbg::store<float, "offset_from_vertex">(), 0.001f, 0.501f, 0.999f);
			ImGui::End();
		}
		if (offsetFromVertexCurr != dk::dbg::store<float, "offset_from_vertex">())
			regenerateNavmesh(terrain);
		if (dk::io::key::r)
			regenerateNavmesh(terrain);

		if (m_inputs.activated("save"))
			saveHeightMap(terrain);
		if (m_inputs.activated("load"))
			loadHeightMap(terrain);

		m_vOut << m_draw2d(cursor, dk::colors::aqua)
			   << m_draw2d(dk::geom::circle2(cursor, 1.5), dk::colors::white);
	}

	void render(Terrain& terrain, dk::gfx::FrameBuffer& frameBuffer, dk::io::AssetManager& assets, View& view)
	{
		// Bind camera
		m_shaders["rgba"].uniforms()    << view.uniforms();
		m_shaders["terrain"].uniforms() << view.uniforms();

		// Render debug data
		m_vOut.flush(m_shaders["rgba"], frameBuffer);
		m_vOutNavmeshGeneration.draw(m_shaders["rgba"], frameBuffer);

		// Render terrain
		m_shaders["terrain"].uniformTexture("u_grassTexture", assets.get<dk::gfx::Texture>("/textures/terrain/grass.png"));
		m_shaders["terrain"].uniformTexture("u_rockTexture" , assets.get<dk::gfx::Texture>("/textures/terrain/rock.png"));
		terrain.render(frameBuffer, m_shaders["terrain"]);

		// Render ground
		//m_shaders["terrain"].layout(std::ref(m_groundMesh.vertices()));
		//frameBuffer.render(m_shaders["terrain"], m_groundMesh.indices(), dk::gfx::Primitive::Triangles);
	}

	void regenerateNavmesh(Terrain& terrain)
	{
		m_vOutNavmeshGeneration.clear();
		for (int x = 0; x < terrain.m_accessor.sizeInChunks().x; ++x)
			for (int y = 0; y < terrain.m_accessor.sizeInChunks().y; ++y)
				terrain.m_changedChunks.insert(glm::ivec2(x, y));
	}

	bool saveHeightMap(Terrain& terrain);

	void loadHeightMap(Terrain& terrain);

private:
	dk::gfx::VertexSink        m_vOut;
	dk::gfx::VertexSink        m_vOutNavmeshGeneration;
	dk::gfx::drawer2d          m_draw2d;
	dk::gfx::ShaderCollection  m_shaders;
	dk::io::InputManager       m_inputs;

	int m_hightAtCursor = 0;

	void changeTerrainHeight(Terrain& terrain, const glm::ivec2& cursor)
	{
		if (!terrain.m_accessor.isInbounds(cursor))
			return;
		if (m_inputs.activated("apply")) 
			m_hightAtCursor = terrain.m_cells[terrain.m_accessor.indexOf(cursor)].height;
		if (m_inputs.active("apply_positive"))
			terrain.setHeight(cursor, m_hightAtCursor + 1);
		if (m_inputs.active("apply_negative"))
			terrain.setHeight(cursor, m_hightAtCursor - 1);
		if (m_inputs.active("apply"))
			m_vOutNavmeshGeneration.clear();
	}
};
