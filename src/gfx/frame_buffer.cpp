#include <devkit/gfx/frame_buffer.h>
#include "context.h"

#include <glad/glad.h>

void logFramebufferWarning(GLenum status);

const dk::gfx::RenderTarget& dk::gfx::FrameBuffer::Attachment::get() const {
	return std::visit(common::overload{
		[](const std::reference_wrapper<RenderTarget>& data) -> const RenderTarget& { return data.get(); },
		[](const std::unique_ptr<RenderTarget>& data) -> const RenderTarget& { return *data.get(); },
		[](const std::monostate) -> const RenderTarget& { return *((const RenderTarget*)nullptr); }
	}, m_data);
}

dk::gfx::RenderTarget& dk::gfx::FrameBuffer::Attachment::get() {
	return std::visit(common::overload{
		[](std::reference_wrapper<RenderTarget>& data) -> RenderTarget& { return data.get(); },
		[](std::unique_ptr<RenderTarget>& data) -> RenderTarget& { return *data.get(); },
		[](std::monostate) -> RenderTarget& { return *((RenderTarget*)nullptr); }
	}, m_data);
}

glm::ivec3 dk::gfx::FrameBuffer::Attachment::size() const
{
	if (std::holds_alternative<std::monostate>(m_data))
		return { 0, 0, 0 };
	return get().targetSize();
}

dk::gfx::FrameBuffer::FrameBuffer()
	: color(io::GlobalState::hardware().glMaxColorAttachments)
{ }

dk::gfx::FrameBuffer::FrameBuffer(dk::gfx::api::FrameBuffer::backbuffer_t)
	: color(0)
	, m_apiHandle(dk::gfx::api::FrameBuffer::backbuffer_t{})
{ }

void dk::gfx::FrameBuffer::clear(Clear mask, const glm::vec4& color)
{
	makeActive();
	glClearColor(color.r, color.g, color.b, color.a);
	glClear((unsigned)mask);
}

unsigned toUnderlying(dk::gfx::Texture::MagFilter filter)
{
	switch (filter)
	{
	case dk::gfx::Texture::MagFilter::Nearest : return GL_NEAREST;
	case dk::gfx::Texture::MagFilter::Linear  : return GL_LINEAR;
	default: throw std::runtime_error("unknown filter value");
	}
}

unsigned toUnderlying(dk::gfx::Mask mask)
{
	unsigned result = 0u;
	if ((unsigned)mask & (unsigned)dk::gfx::Mask::Color)   result |= GL_COLOR_BUFFER_BIT;
	if ((unsigned)mask & (unsigned)dk::gfx::Mask::Depth)   result |= GL_DEPTH_BUFFER_BIT;
	if ((unsigned)mask & (unsigned)dk::gfx::Mask::Stencil) result |= GL_STENCIL_BUFFER_BIT;
	return result;
}

void dk::gfx::FrameBuffer::blit(FrameBuffer& input, Mask mask, int inputColorIndex, int outputColorIndex, Texture::MagFilter filter)
{
	makeActive();

	const auto filter_api = toUnderlying(filter);
	const auto mask_api = toUnderlying(mask);

	// Input
	glBindFramebuffer(GL_READ_FRAMEBUFFER, input.m_apiHandle.handle());
	glReadBuffer(GL_COLOR_ATTACHMENT0 + inputColorIndex);
	
	// Output
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_apiHandle.handle());
	glDrawBuffer(GL_COLOR_ATTACHMENT0 + outputColorIndex);

	glBlitFramebuffer(0, 0, input.color[inputColorIndex].get().targetSize().x, input.color[inputColorIndex].get().targetSize().y, 
		0, 0, color[outputColorIndex].get().targetSize().x, color[outputColorIndex].get().targetSize().y, mask_api, filter_api);
}

void dk::gfx::FrameBuffer::blit(FrameBuffer& input, Rect srcRect, Rect dstRect, Mask mask, int inputColorIndex, int outputColorIndex, Texture::MagFilter filter)
{
	makeActive();

	const auto filter_api = toUnderlying(filter);
	const auto mask_api = toUnderlying(mask);

	// Input
	glBindFramebuffer(GL_READ_FRAMEBUFFER, input.m_apiHandle.handle());
	glReadBuffer(GL_COLOR_ATTACHMENT0 + inputColorIndex);

	// Output
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_apiHandle.handle());
	glDrawBuffer(GL_COLOR_ATTACHMENT0 + outputColorIndex);

	glBlitFramebuffer(srcRect.offset.x, srcRect.offset.y, srcRect.size.x, srcRect.size.y, 
		dstRect.offset.x, dstRect.offset.y, dstRect.size.x, dstRect.size.y, mask_api, filter_api);
}

void dk::gfx::FrameBuffer::render(Shader& shader)
{
	struct PostProcessHandler {
		VertexBuffer vertexBuffer;

		PostProcessHandler()
			: vertexBuffer(common::id<Vertex<glm::vec3, glm::vec2>>)
		{
			auto& cont = vertexBuffer.modify();
			cont.push_back(Vertex(glm::vec3(-1, -1, 0), glm::vec2(0, 0)));
			cont.push_back(Vertex(glm::vec3( 1, -1, 0), glm::vec2(1, 0)));
			cont.push_back(Vertex(glm::vec3( 1,  1, 0), glm::vec2(1, 1)));
			cont.push_back(Vertex(glm::vec3( 1,  1, 0), glm::vec2(1, 1)));
			cont.push_back(Vertex(glm::vec3(-1,  1, 0), glm::vec2(0, 1)));
			cont.push_back(Vertex(glm::vec3(-1, -1, 0), glm::vec2(0, 0)));
		}
	};

	static std::unordered_map<void*, PostProcessHandler> s_handlers{};
	auto& handler = s_handlers[(void*)this];

	shader.layout(handler.vertexBuffer);
	render(shader, handler.vertexBuffer, Primitive::Triangles);
}

void dk::gfx::FrameBuffer::render(Texture2D& texture)
{
	static Shader s_shader = []{
		Shader s;
		s.source(ShaderSource::postProcessVertexSource());
		s.source(ShaderSource::passthoughTextureFragmentSource());
		return s;
	}();
	s_shader.uniformTexture("u_texture", texture);
	render(s_shader);
}

void dk::gfx::FrameBuffer::render(MultisampledTexture2D& texture)
{
	using namespace shader_literals;
	static Shader s_shader = []{
		Shader s;
		s.source(ShaderSource::postProcessVertexSource());
		s.source(R"(
			#version 330 core

			in vec2 UV;
			out vec4 FragColor;

			uniform sampler2DMS u_texture;
			uniform int         u_sampleCount;

			void main()
			{
			ivec2 texelCoord = ivec2(UV * textureSize(u_texture));
			vec4 color = vec4(0.0);
			for (int i = 0; i < u_sampleCount; ++i)
				color += texelFetch(u_texture, texelCoord, i);
			FragColor = color / float(u_sampleCount);
			}
		)"_fs);
		return s;
		}();
	s_shader.uniforms().set("u_sampleCount", texture.samples());
	s_shader.uniformTexture("u_texture", texture);
	render(s_shader);
}

void dk::gfx::FrameBuffer::render(Shader& shader, VertexBuffer& vertexBuffer, Primitive primitive, unsigned count)
{
	makeActive();
	shader.makeActive();
	(count == 1)
		? glDrawArrays(details::gfx::toUnderlying(primitive), 0, vertexBuffer.size())
		: glDrawArraysInstanced(details::gfx::toUnderlying(primitive), 0, vertexBuffer.size(), count);
}

void dk::gfx::FrameBuffer::render(Shader& shader, ElementBuffer& elementBuffer, Primitive primitive, unsigned count)
{
	makeActive();
	shader.makeActive();
	elementBuffer.makeActive();
	(count == 1)
		? glDrawElements(details::gfx::toUnderlying(primitive), elementBuffer.count(), GL_UNSIGNED_INT, 0)
		: glDrawElementsInstanced(details::gfx::toUnderlying(primitive), elementBuffer.count(), GL_UNSIGNED_INT, 0, count);
}

void dk::gfx::FrameBuffer::setViewport(const gfx::Viewport& viewport)
{
	m_viewport = viewport;
}

float dk::gfx::FrameBuffer::aspectRatio() const
{
	return [&]{ return m_viewport.has_value() ? m_viewport.value() : Viewport(color[0].size()); }().aspectRatio();
}

void dk::gfx::FrameBuffer::resize(const glm::ivec2& size)
{
	if (depth.has_value()) 
		depth.get().resize(size);
	if (stencil.has_value())
		stencil.get().resize(size);
	for (auto& c : color)
	{
		if (!c.has_value())
			continue;
		c.get().resize(size);
	}
}

void dk::gfx::FrameBuffer::makeActive()
{
	m_apiHandle.bind();

	const Viewport viewport = [&]{ return m_viewport.has_value() ? m_viewport.value() : Viewport(color[0].size()); }();
	viewport.makeActive();

	// Bind properties to global gl context
	config.for_each([&](const auto& prop) {
		// Skip if value is currently active in the global context
		auto& currentlyBound = s_currentContextConfig[(const void*)dk::io::GlobalState::currentWindowContext()];
		if (prop == currentlyBound.get<std::decay_t<decltype(prop)>>())
			return;
		// Update gl context
		//spdlog::debug("FrameBuffer.{}={}", Config::property_name(prop), std::format("{}", prop));
		setFrameBufferProperty(*this, prop);
		// Update gl context tracker used for skipping
		currentlyBound.set<std::decay_t<decltype(prop)>>(prop);
	});
	
	// Set color attachments
	std::vector<unsigned> activeColorAttachmentIndices;
	for (int i = 0; i < color.size(); ++i) {
		if (!color[i].has_value())
			continue;

		color[i].get().setAsTarget(api::Attachment::Color0, i);

		activeColorAttachmentIndices.push_back(GL_COLOR_ATTACHMENT0 + i);
	}
	if (color.size() > 0)
		glDrawBuffers(activeColorAttachmentIndices.size(), activeColorAttachmentIndices.data());
	glReadBuffer(GL_COLOR_ATTACHMENT0);

	// Set depth attachment	
	if (depth.has_value())
		depth.get().setAsTarget(api::Attachment::Depth);

	// Check framebuffer status and log warnings if necessary
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	logFramebufferWarning(status);
}

dk::gfx::FrameBuffer& dk::gfx::backBuffer() 
{
	static std::mutex s_backbufferMut;
	static std::unordered_map<const io::WindowContext*, std::unique_ptr<FrameBuffer>> s_frameBuffers;

	std::lock_guard lock(s_backbufferMut);
	auto& fb_ptr = s_frameBuffers[io::GlobalState::currentWindowContext()];
	if (!fb_ptr)
		fb_ptr.reset(new FrameBuffer(api::FrameBuffer::backbuffer_t{}));
	return *fb_ptr;
}

#define __DK_TABLE_FRAMEBUFFER_TOGGLE_PROPERTY(F, ...)           \
	/*                                  Type | GL Enum        */ \
	F( __VA_ARGS__ __VA_OPT__(,) DepthTest   , GL_DEPTH_TEST   ) \
	F( __VA_ARGS__ __VA_OPT__(,) Blend       , GL_BLEND        ) \
	F( __VA_ARGS__ __VA_OPT__(,) CullFace    , GL_CULL_FACE    ) \
	F( __VA_ARGS__ __VA_OPT__(,) ScissorTest , GL_SCISSOR_TEST ) \
	F( __VA_ARGS__ __VA_OPT__(,) Multisample , GL_MULTISAMPLE  ) \
	/* end of table */
#define __DK_GL_ENABLEDISABLE(type, glEnum)                                                        \
	if (value == std::decay_t<decltype(value)>::Enabled) glEnable(glEnum); else glDisable(glEnum); \
	/* end of macro */
#define __DK_FRAMEBUFFER_DECL_GLTOGGLE_PROPERTYSETTER(type, glEnum)                               \
	template<> void dk::gfx::setFrameBufferProperty(FrameBuffer&, const FrameBuffer::type& value) \
	{ __DK_GL_ENABLEDISABLE(type, glEnum); }                                                      \
	/* end of macro */
__DK_TABLE_FRAMEBUFFER_TOGGLE_PROPERTY(__DK_FRAMEBUFFER_DECL_GLTOGGLE_PROPERTYSETTER)

template<>
void dk::gfx::setFrameBufferProperty(FrameBuffer&, const FrameBuffer::DepthFunc& depthFunc)
{ 
	auto underlying = [&]{
		switch (depthFunc) {
		case FrameBuffer::DepthFunc::Less    : return GL_LESS;
		case FrameBuffer::DepthFunc::Never   : return GL_NEVER;
		case FrameBuffer::DepthFunc::Equal   : return GL_EQUAL;
		case FrameBuffer::DepthFunc::Lequal  : return GL_LEQUAL;
		case FrameBuffer::DepthFunc::Greater : return GL_GREATER;
		case FrameBuffer::DepthFunc::NotEqual: return GL_NOTEQUAL;
		case FrameBuffer::DepthFunc::Gequal  : return GL_GEQUAL;
		case FrameBuffer::DepthFunc::Always  : return GL_ALWAYS;
		default: return 0;
		}
	}();
	glDepthFunc(underlying);
}

template <typename E>
unsigned blendFactorToUnderlying(const E& value)
{
	switch (value)
	{
	case E::One             : GL_ONE;
	case E::Zero            : GL_ZERO;
	case E::SrcAlpha        : GL_SRC_ALPHA;
	case E::OneMinusSrcAlpha: GL_ONE_MINUS_SRC_ALPHA;
	default: return 0;
	}
}

template<>
void dk::gfx::setFrameBufferProperty(FrameBuffer& frameBuffer, const FrameBuffer::SrcBlendFactor& srcFactor)
{
	glBlendFunc(blendFactorToUnderlying(srcFactor), 
		        blendFactorToUnderlying(frameBuffer.config.get<FrameBuffer::DstBlendFactor>()));
}

template<>
void dk::gfx::setFrameBufferProperty(FrameBuffer& frameBuffer, const FrameBuffer::DstBlendFactor& dstFactor)
{
	glBlendFunc(blendFactorToUnderlying(frameBuffer.config.get<FrameBuffer::SrcBlendFactor>()), 
		        blendFactorToUnderlying(dstFactor));
}

template <>
void dk::gfx::setFrameBufferProperty(FrameBuffer& frameBuffer, const FrameBuffer::SampleShading& sampleShading)
{
	// if (!GLEW_ARB_sample_shading)
	// {
	// 	static bool logged = false;
	// 	if (!logged)
	// 	{
	// 		logged = true;
	// 		spdlog::warn("[gfx] Sample shading is not available");
	// 	}
	// 	return;
	// }
	spdlog::warn("Sample shading check not implemented using glad");

	if (sampleShading == std::decay_t<decltype(sampleShading)>::Enabled) {
		glEnable(GL_SAMPLE_SHADING);
		glMinSampleShading(1.0);
	}
	else 
		glDisable(GL_SAMPLE_SHADING);
}

template <>
void dk::gfx::setFrameBufferProperty(FrameBuffer&, const FrameBuffer::LineWidth& lineWidth)
{ glLineWidth(lineWidth.value); }

template <>
void dk::gfx::setFrameBufferProperty(FrameBuffer&, const FrameBuffer::PointSize& pointSize)
{ glPointSize(pointSize.value); }

#include <GL/glu.h>

void logFramebufferWarning(GLenum status) {
	const auto warning = [=] -> std::pair<bool, std::string>{
		switch (status) {
		case GL_FRAMEBUFFER_COMPLETE: 
			return { false, "" };
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
			return { true, "Framebuffer incomplete: Incomplete attachment" };
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
			return { true, "Framebuffer incomplete: Missing attachment" };
		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
			return { true, "Framebuffer incomplete: Incomplete draw buffer" };
		case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
			return { true, "Framebuffer incomplete: Incomplete read buffer" };
		case GL_FRAMEBUFFER_UNSUPPORTED:
			return { true, "Framebuffer incomplete: Unsupported configuration" };
		case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
			return { true, "Framebuffer incomplete: Incomplete multisample buffer" };
		case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
			return { true, "Framebuffer incomplete: Incomplete layer targets" };
		default:
			return { true, "Unknown framebuffer status" };
		}
	}();

	if (warning.first)
	{
		spdlog::warn("{}", warning.second);
	}
}

#include "demo_scene.h"

void dk::gfx::FrameBuffer::render(DemoScene ds)
{
	struct Scene {
		ShaderSource vertShader = g_vssource;
		ShaderSource fragShader = g_fssource;
		Shader       shader;

		VertexBuffer                  vertexBuffer;
		Camera                        camera;

		Scene()
		{
			// Setup shader
			shader.source(vertShader, ShaderSource::Vertex);
			shader.source(fragShader, ShaderSource::Fragment);

			// Setup vertex and element buffers
			std::vector<Vertex<glm::vec3, glm::vec3>> vertices = {__DK_DEMOSCENE_MONKEY_VERTICES};
			vertexBuffer = VertexFlags::Position | VertexFlags::Normal;
			vertexBuffer.modify().resize(vertices.size());
			std::memcpy(vertexBuffer.modify().data(), vertices.data(), vertexBuffer.get().elem_size() * vertexBuffer.get().size());

			// Setup layout
			shader.layout(vertexBuffer);
		}
	};

	static std::unordered_map<void*, Scene> s_scenes{};
	auto& scene = s_scenes[(void*)this];

	// Setup uniforms
	const auto& cam = ds.camera.value_or(scene.camera);
	scene.camera.position = glm::vec3(0, -3, 0.01);
	scene.camera.asp = aspectRatio();
	scene.shader.uniforms().set("u_camera.VP",        cam.P() * cam.V());
	scene.shader.uniforms().set("u_camera.position",  cam.position);
	scene.shader.uniforms().set("u_camera.direction", cam.lookat - cam.position);

	// Execute draw calls
	clear(Clear::Color | Clear::Depth, DK_COLOR(0x333333ff));
	render(scene.shader, scene.vertexBuffer, Primitive::Triangles);
}
