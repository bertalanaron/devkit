#pragma once
#define NS_DEVKIT dk

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>

namespace NS_DEVKIT {

template <typename D>
class SingletonBase {
public:
	SingletonBase() = default;
	SingletonBase(const SingletonBase&) = delete;
	SingletonBase& operator=(const SingletonBase&) = delete;

	static D& instance() {
		static D c_instance;
		return c_instance;
	}
};

template <unsigned N>
struct StringLiteral {
	constexpr StringLiteral(const char(&str)[N]) {
		std::copy_n(str, N, value);
	}
	constexpr StringLiteral() { };
	char value[N]{};
	const char* c_str() const { return value; }
};

}

#define CAT_(A, B)                   A ## B
#define CAT(A, B)                    CAT_(A, B)
#define SKIP_IF_EMPTY_CHK(...)       __VA_OPT__(NONEMPTY)
#define SKIP_IF_EMPTY_(...)
#define SKIP_IF_EMPTY_NONEMPTY(...)  __VA_ARGS__
#define SKIP_IF_EMPTY(S, ...)        CAT(SKIP_IF_EMPTY_, SKIP_IF_EMPTY_CHK(S))(__VA_ARGS__)

namespace NS_DEVKIT {

// TODO: maybe merge IFrameProducer and IRenderTarget

class IFrameProducer {
public:
	virtual bool beginFrame() = 0;
	virtual void endFrame()	  = 0;
};

// RAII
class Frame {
public:
	Frame(IFrameProducer& producer)
		: m_producer(producer)
	{ m_beginSuccess = m_producer.beginFrame(); }

	~Frame() { m_producer.endFrame(); }
	operator bool() { return m_beginSuccess; }
private:
	IFrameProducer& m_producer;
	bool		   m_beginSuccess;
};

}
