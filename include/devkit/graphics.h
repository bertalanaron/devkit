#pragma once
#include <memory>
#include <vector>
#include <optional>
#include <unordered_map>

#include <imgui.h>
#include "devkit/util.h"

// TODO: maybe don't include this here. instead move cam controller to io.h (could also include asset manager)
#include "devkit/devkit.h"

namespace NS_DEVKIT {

struct GLType {
	unsigned	type = 0;
	size_t		size = 0;
	unsigned	count = 0;

	template <typename T>
	constexpr static GLType get();
};

}

namespace NS_DEVKIT {

class Shader;

enum class PrimitiveType : unsigned int 
{ Points = 0x0000, Lines = 0x0001, LineLoop = 0x0002, LineStrip = 0x0003, Triangles = 0x0004 };

// TODO: check for initialization gl context
class VertexBufferBase {
public:
	template <typename... Ts>
	static VertexBufferBase init(size_t size = 0, uintptr_t data = (uintptr_t)nullptr) {
		static_assert(sizeof...(Ts) > 0);
		std::vector<GLType> types = { GLType::get<Ts>()...};
		return VertexBufferBase(size, std::move(types), data);
	}

	~VertexBufferBase();

	template <typename... Ts>
	const reverse_tuple<Ts...>& get(size_t index) const {
		return *reinterpret_cast<reverse_tuple<Ts...>*>(at_impl(index));
	}

	template <typename... Ts>
	void set(size_t index, reverse_tuple<Ts...>& value) {
		*reinterpret_cast<reverse_tuple<Ts...>*>(at_impl(index)) = value;
	}

	void bind() const;

	void draw(PrimitiveType primitive) const;
	void draw(PrimitiveType primitive, Shader& shader) const;

	void clear();

protected:
	void push(size_t count, uintptr_t data);

private:
	class Impl; std::unique_ptr<Impl> m_impl;

	VertexBufferBase(size_t size, std::vector<GLType>&& types, uintptr_t data);

	uintptr_t at_impl(size_t index) const;
};

template <typename... Ts>
class VertexBuffer : public VertexBufferBase {
public:
	using Vertex = reverse_tuple<Ts...>;

	VertexBuffer(size_t size = 0) 
		: VertexBufferBase(VertexBufferBase::init<Ts...>(size)) 
	{  }

	VertexBuffer(std::vector<Vertex>&& vertices) 
		: VertexBufferBase(VertexBufferBase::init<Ts...>(vertices.size(), reinterpret_cast<uintptr_t>(vertices.data()))) 
	{ }

	const Vertex& get(size_t index) const {
		return VertexBufferBase::get<Ts...>(index);
	}

	void set(size_t index, Vertex& vertex) {
		VertexBufferBase::set<Ts...>(index, vertex);
	}

	void push(const Vertex& vertex) {
		VertexBufferBase::push(1, reinterpret_cast<uintptr_t>(&vertex));
	}
};

}

namespace NS_DEVKIT {

class Texture;
class CameraController;

class ShaderSource {
public:
	ShaderSource(std::string&& source);

	static ShaderSource loadFromFile(const wchar_t* path);

	void updateFromFile(const wchar_t* path);

	bool compile(unsigned type, unsigned& id);

	bool updated() const;

private:
	std::string m_source;
	bool		m_updated;
};

// TODO: check for initialization gl context
class Shader {
public:
	using OptionalShaderRef = std::optional<std::reference_wrapper<ShaderSource>>;

	Shader(ShaderSource& vertexSource, ShaderSource& fragmentSource, OptionalShaderRef geometrySource = std::nullopt);
	Shader(const Shader&);

	void setUniform(const std::string& name, const int        & value);
	void setUniform(const std::string& name, const double     & value);
	void setUniform(const std::string& name, const float      & value);
	void setUniform(const std::string& name, const glm::vec2  & value);
	void setUniform(const std::string& name, const glm::vec3  & value);
	void setUniform(const std::string& name, const glm::vec4  & value);
	void setUniform(const std::string& name, const Texture    & value);
	void setUniform(const std::string& name, CameraController & value);

	void use();

	unsigned id() const;

	~Shader();

private:
	struct Impl; std::shared_ptr<Impl> m_impl;

	ShaderSource&				       m_vertexSource;
	ShaderSource&				       m_fragmentSource;
	OptionalShaderRef			       m_geometrySource;

	struct IUniformSource {
		virtual void set(const Shader& shader, const std::string& name) const = 0;
		virtual std::unique_ptr<IUniformSource> clone() const = 0;
	};
	using UniformMap = std::unordered_map<std::string, std::unique_ptr<IUniformSource>>;

	UniformMap						   m_uniforms{};
};

}

namespace NS_DEVKIT {

class Texture {
public:
	enum class Filter { NEAREST = 0x2600, LINEAR = 0x2601 };
	struct Properties;

	Texture();
	Texture(Texture::Properties properties);

	uint32_t id() const;
	glm::u32vec2 size() const;

	void bind() const;

	unsigned char* getPixelBuffer();
	void save(const wchar_t* path);
	static Texture load(const wchar_t* path);
	void update(const wchar_t* path);

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
	glm::u32vec2 frameSize() const override;
	void* context() override;

	Properties& properties();

	~FrameBuffer();

private:
	class Impl; std::unique_ptr<Impl> m_impl;
};

struct FrameBuffer::Properties {
	glm::vec4	clearColor = { 0.f, 0.f, 0.f, 1.f };
	bool		cullFaces  = true;
	bool		doClear    = true;
};

}

namespace NS_DEVKIT {

struct Camera {
public:
	enum class Projection { Perspective, Orthographic };

	glm::vec3  position{ 0, 0, 1 };
	glm::vec3  lookat  { 0, 0, 0 };
	glm::vec3  vup     { 0, 1, 0 };

	float	   fov = 1.f;
	float      asp = 1.f;
	float      np  = 0.01f;
	float      fp  = 1000.f;

	Projection projection = Projection::Perspective;

	void castRay(const glm::vec2& ndc, glm::vec3& origin, glm::vec3& direction);
	void castRay(const glm::vec2& ndc, glm::dvec3& origin, glm::dvec3& direction);

	bool operator==(const Camera&) const = default;
	Camera& operator=(const Camera&) = default;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Camera, 
	position, lookat, vup, fov, asp, np, fp, projection);

NLOHMANN_JSON_SERIALIZE_ENUM(Camera::Projection, {
	{ Camera::Projection::Perspective  , "perspective"  },
	{ Camera::Projection::Orthographic , "orthographic" } });

struct CameraControllHandler {
	InputCombination tiltWith;    
	InputCombination shiftWith;	  
	InputCombination zoomInWith;  
	InputCombination zoomOutWith; 

	glm::vec2        tiltMultiplier { 1.f, 1.f };
	glm::vec2        shiftMultiplier{ 1.f, 1.f };
	float			 zoomMultiplier = 1.f;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CameraControllHandler,
	tiltWith, shiftWith, zoomInWith, zoomOutWith, tiltMultiplier, shiftMultiplier, zoomMultiplier);

class CameraController;

class ICameraControllerStrategy {
public:
	virtual void tilt (CameraController& controller, const glm::vec2&) = 0;
	virtual void shift(CameraController& controller, const glm::vec2&) = 0;
	virtual void zoom (CameraController& controller, float delta)      = 0;
};

class CameraController {
public:
	CameraController(ICameraControllerStrategy* strategy) 
		: m_strategy(*strategy)
	{ }

	Camera& camera();
	const Camera& camera() const;

	const glm::mat4& viewMatrix();
	const glm::mat4& projectionMatrix();

	void tilt(const glm::vec2& delta)  { m_strategy.tilt(*this, delta); }
	void shift(const glm::vec2& delta) { m_strategy.shift(*this, delta); }
	void zoom(float delta)             { m_strategy.zoom(*this, delta); }

	void handle(const CameraControllHandler& handler, const glm::vec2 tiltShift, float zoom);

private:
	Camera                     m_previousCamera;
	Camera                     m_currentCamera = Camera{ .asp = -1.f };

	mutable glm::mat4          m_viewMatrix      { 1.f };
	mutable glm::mat4          m_projectionMatrix{ 1.f };

	mutable bool               m_accessed = true;

	ICameraControllerStrategy& m_strategy;

	void update();
};

struct CameraControllerOrbit : ICameraControllerStrategy {
	void tilt(CameraController& controller, const glm::vec2& delta) override;
	void shift(CameraController& controller, const glm::vec2& delta) override;
	void zoom(CameraController& controller, float delta) override;
};

struct CameraControllerRTS : ICameraControllerStrategy {
	void tilt(CameraController& controller, const glm::vec2&) override;
	void shift(CameraController& controller, const glm::vec2&) override;
	void zoom(CameraController& controller, float delta) override;
};

struct CameraControllerFPS : ICameraControllerStrategy {
	void tilt(CameraController& controller, const glm::vec2&) override;
	void shift(CameraController& controller, const glm::vec2&) override;
	void zoom(CameraController& controller, float delta) override;
};

}

namespace NS_DEVKIT {

namespace primitives {

struct Line 
{ glm::vec3 a; glm::vec3 b; glm::vec4 color; };

struct LineGradient 
{ glm::vec3 a; glm::vec4 aColor; glm::vec3 b; glm::vec4 bColor; };

struct Rect 
{ glm::vec3 center; glm::vec2 size; glm::vec3 normal; glm::vec3 right; glm::vec4 color; };

struct UVRect 
{ glm::vec3 center; glm::vec2 size; glm::vec3 normal; glm::vec3 right; };

}

using Primitive = std::variant<
	primitives::Line, 
	primitives::LineGradient, 
	primitives::Rect, 
	primitives::UVRect>;

class PrimitiveStream {
public:
	PrimitiveStream& operator<<(Shader& shader);
	PrimitiveStream& operator<<(Texture& texture);
	PrimitiveStream& operator<<(const Primitive& primitive);

	void draw();
	void clear();
	void flush();

	PrimitiveStream();
	~PrimitiveStream();

private:
	class Impl; std::unique_ptr<Impl> m_impl;
};

class ConcurrentStream {
public:
	ConcurrentStream(size_t threadCount = std::thread::hardware_concurrency())
		: m_streams(threadCount)
		, m_createdOn(std::this_thread::get_id())
	{
		m_indices.reserve(threadCount);
	}

	ConcurrentStream& operator<<(Shader& shader);
	ConcurrentStream& operator<<(Texture& texture);
	ConcurrentStream& operator<<(const Primitive& primitive);

	void draw();
	void clear();
	void flush();

private:
	std::vector<PrimitiveStream>              m_streams;
	std::unordered_map<std::jthread::id, int> m_indices;

	std::mutex                                m_mut;
	std::jthread::id                          m_createdOn;

	int index();
};

//class DebugPrimitives {
//public:
//	virtual void flus(Shader& shader) = 0;
//};
//
//class DebugLines : public DebugPrimitives {
//public:
//	void flus(Shader& shader) override;
//
//	void draw(glm::vec3 a, glm::vec3 b, glm::vec4 color = colors::white);
//
//	DebugLines();
//	~DebugLines();
//
//private:
//	class Impl; std::unique_ptr<Impl> m_impl;
//};
}

namespace NS_DEVKIT {

template <typename... Ts>
class Mesh {
public:
	// Example: Mesh<glm::vec3, glm::vec2>::loadObjFile(L"file.obj", "vn", "vt");
	static Mesh importObjFile(const wchar_t* path)
	{
		return {};
	}
private:
	VertexBuffer<glm::vec3, Ts...> m_vertices;
	// TODO: IndexBuffer<unsigned>
};

}

namespace NS_DEVKIT {

template <> GLType GLType::get<float         >();
template <> GLType GLType::get<double        >();
template <> GLType GLType::get<char          >();
template <> GLType GLType::get<unsigned char >();
template <> GLType GLType::get<std::byte     >();
template <> GLType GLType::get<short         >();
template <> GLType GLType::get<unsigned short>();
template <> GLType GLType::get<int           >();
template <> GLType GLType::get<unsigned int  >();
template <> GLType GLType::get<glm::vec2     >();
template <> GLType GLType::get<glm::vec3     >();
template <> GLType GLType::get<glm::vec4     >();

}
