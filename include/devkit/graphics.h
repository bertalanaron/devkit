#pragma once
#include <memory>

#include <imgui.h>
#include "devkit/util.h"

namespace NS_DEVKIT {

class Texture {
public:
	enum class Filter { NEAREST = 0x2600, LINEAR = 0x2601 };
	struct Properties;

	Texture();
	Texture(Texture::Properties properties);

	uint32_t id() const;
	glm::u32vec2 size() const;

	void bind();

	unsigned char* getPixelBuffer();
	void save(const wchar_t* path);
	static Texture load(const wchar_t* path);

	~Texture();

private:
	class Impl; std::shared_ptr<Impl> m_impl;

	Texture(unsigned char* pixels, glm::i32vec2 size);
};

struct Texture::Properties {
	Filter		 minFilter = Filter::NEAREST;
	Filter		 magFilter = Filter::LINEAR;
	glm::u32vec2 size = { 32, 32 };
};

}

namespace NS_DEVKIT {

// needs to be initialized in the same gl context it will be used in. 
class FrameBuffer : public IFrameProducer {
public:
	struct Properties;

	FrameBuffer(Texture& texture);
	FrameBuffer(Texture& texture, Properties properties);

	bool beginFrame() override;
	void endFrame() override;

	~FrameBuffer();

private:
	class Impl; std::unique_ptr<Impl> m_impl;
};

struct FrameBuffer::Properties {
	glm::vec4	clearColor = { 0.f, 0.f, 0.f, 1.f };
	bool		cullFaces  = true;
};

}
