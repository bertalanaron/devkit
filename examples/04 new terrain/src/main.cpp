#include <devkit/io/window.h>
#include <devkit/io/asset_manager.h>
#include <devkit/gfx/shader.h>
#include <devkit/gfx/texture.h>
#include <devkit/gfx/frame_buffer.h>

#include <mini/ini.h>

using namespace dk::io;
using namespace dk::gfx;

class Editor {
public:
	Editor()
	{
		// Parse ini file
		m_ini = [] {
			mINI::INIFile file(dk::common::executable_path().parent_path() / "examples.ini");
			mINI::INIStructure ini;
			file.read(ini);
			return ini;
		}();

	}

	void run()
	{
		while (m_window.isOpen())
		{
			const auto& frame = m_window.beginFrame();
			backBuffer().clear(Clear::Color | Clear::Depth, dk::colors::gray);

			m_window.endFrame();
		}
	}

private:
	Window             m_window;
	mINI::INIStructure m_ini;
	AssetManager       m_assets;

	ShaderCollection   m_shaders;

	std::optional<std::string> ini(const std::string& label, const std::string& value)
	{
		if (!m_ini.has(label) || !m_ini[label].has(value))
			return std::nullopt;
		return m_ini[label][value];
	}

	std::string ini_or(
		const std::string& label, 
		const std::string& value, 
		const std::string& fallback)
	{
		const auto result = ini(label, value);
		if (result.has_value())
			return result.value();
		return fallback;
	}
};

int main()
{
	Editor editor;
	editor.run();
}
