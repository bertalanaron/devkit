#pragma once
#include <glm/glm.hpp>

#define _DEVKIT_COLOR_EXPAND_HEX_CHANNELS_RGBA(color, FV) FV(((color >> 24) & 0xFF)), FV(((color >> 16) & 0xFF)), FV(((color >> 8) & 0xFF)), FV((color & 0xFF))
#define _DEVKIT_COLOR_HEX_TO_FLOAT(hex) (float)hex / 255.0
#define _DEVKIT_COLOR_EXPAND_AND_TO_FLOAT(color) _DEVKIT_COLOR_EXPAND_HEX_CHANNELS_RGBA(color, _DEVKIT_COLOR_HEX_TO_FLOAT)

#define DK_COLOR(hex) glm::vec4{ _DEVKIT_COLOR_EXPAND_AND_TO_FLOAT(hex) }
#define _DEVKIT_COLOR_DECL_STATIC(name, hex) inline static constexpr glm::vec4 name = DK_COLOR(hex);

namespace dk::colors {

#define _DEVKIT_COLOR_TABLE(FV)  \
	FV( white      , 0xffffffff )\
	FV( black      , 0x000000ff )\
	FV( red        , 0xff0000ff )\
	FV( lime       , 0x00ff00ff )\
	FV( blue       , 0x0000ffff )\
	          					 \
	FV( silver	   , 0xc0c0c0ff )\
	FV( gray	   , 0x808080ff )\
	FV( maroon	   , 0x800000ff )\
	FV( yellow	   , 0xffff00ff )\
	FV( olive	   , 0x808000ff )\
	FV( green      , 0x008000ff )\
	FV( aqua	   , 0x00ffffff )\
	FV( teal	   , 0x008080ff )\
	FV( navy	   , 0x000080ff )\
	FV( fuchsia    , 0xff00ffff )\
	FV( purple	   , 0x800080ff )\
	             				 \
	FV( dodgerBlue , 0x1e90ffff )\
	FV( orangeRed  , 0xff4500ff )\
	FV( orange     , 0xffa500ff )\
	/* end of table */

_DEVKIT_COLOR_TABLE(_DEVKIT_COLOR_DECL_STATIC)
#undef _DEVKIT_COLOR_TABLE

}

