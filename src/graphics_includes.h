#pragma once
#include <SDL.h>
#undef main
#include <GL/glew.h>
#include <iostream>
#include <imgui/imgui.h>

#include <glm/glm.hpp>

void pushViewport(glm::i32vec2 offset, glm::i32vec2 size);

void popViewport();

SDL_GLContext currentGlContext();
