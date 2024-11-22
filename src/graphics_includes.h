#pragma once
#include <iostream>
#include <optional>

#include <SDL.h>
#undef main
#include <GL/glew.h>
#include <imgui.h>
#include <glm/glm.hpp>

void pushViewportSize(glm::i32vec2 size);
void popViewportSize();
void pushViewportOffset(glm::i32vec2 offset);
void popViewportOffset();

std::optional<glm::i32vec2> currentViewportSize();
std::optional<glm::i32vec2> currentViewportOffset();

SDL_GLContext currentGlContext();
