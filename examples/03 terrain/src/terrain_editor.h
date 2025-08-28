#pragma once
#include "editor_client.h"

class EditorClient::TerrainEditStrategy
	: public EditorClient::EditStrategyBase
{
public:
	using EditStrategyBase::EditStrategyBase;

	void setup() override
	{
		editor().m_inputs.define("select_edit_strategy_terrain", 
			dk::io::modkey::alt + dk::io::key::_1);
	}

	bool showToolbarIcon() override
	{
		// Show button
		bool result = ImGui::Button("terrain")
			|| editor().m_inputs.activated("select_edit_strategy_terrain");
		// Show hotkey in tooltip
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("alt + 1");
		return result;
	}

	void showOptionsPanel() override
	{
		ImGui::Text("Edit terrain");
	}
};
