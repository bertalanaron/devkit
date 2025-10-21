#pragma once
#include <devkit/common/utils.h>

namespace details::gfx {

template <typename T>
void setUniform(unsigned program, const std::string& name, const T& value);

}

namespace dk::gfx {

struct UniformCollection {
private:
	struct UniformBase {
		virtual void set(unsigned program, const std::string& name) const = 0;
		virtual std::unique_ptr<UniformBase> clone() const = 0;
		// @brief Evaluate bound uniforms
		virtual std::unique_ptr<UniformBase> evalClone() const = 0;
	};

	using uniform_map_t = std::unordered_map<std::string, std::unique_ptr<UniformBase>>;
	using const_uniform_iterator = uniform_map_t::const_iterator;

	template <typename T>
	class Uniform : public UniformBase {
	public:
		Uniform(const T& value)
			: m_value(value)
		{ }

		void set(unsigned program, const std::string& name) const override 
		{
			details::gfx::setUniform(program, name, m_value);
		}

		std::unique_ptr<UniformBase> clone() const override
		{
			return std::move(std::make_unique<Uniform>(m_value));
		}

		std::unique_ptr<UniformBase> evalClone() const override
		{
			return clone();
		}

	private:
		T m_value;
	};

	template <typename T>
	class BoundUniform : public UniformBase {
	public:
		BoundUniform(const std::function<T()>& getter)
			: m_getter(getter)
		{ }
		
		void set(unsigned program, const std::string& name) const override
		{
			details::gfx::setUniform(program, name, m_getter());
		}
		
		std::unique_ptr<UniformBase> clone() const override
		{
			return std::move(std::make_unique<BoundUniform>(m_getter));
		}

		std::unique_ptr<UniformBase> evalClone() const override
		{
			return std::move(std::make_unique<Uniform<T>>(m_getter()));
		}

	private:
		std::function<T()> m_getter;
	};

public:
	template<typename Setter>
	void bind(const std::string& name, Setter setter) 
	{
		using Ret = typename dk::common::function_traits<std::decay_t<Setter>>::result_type;
		m_uniforms[name] = std::make_unique<BoundUniform<Ret>>(setter);
	}

	template <typename T>
	void set(const std::string& name, const T& value)
	{
		m_uniforms[name] = std::make_unique<Uniform<T>>(value);
	}

	bool contains(const std::string& name) const 
	{
		return m_uniforms.find(name) != m_uniforms.end();
	}

	UniformCollection& operator<<(const UniformCollection& uc) 
	{
		for (const auto& [name, other] : uc.m_uniforms) {
			m_uniforms.erase(name);
			m_uniforms.emplace(name, std::move(other->clone()));
		}
		return *this;
	}

	void makeActive(unsigned program)
	{
		for (const auto& uniform : m_uniforms)
			uniform.second->set(program, uniform.first);
	}

private:
	uniform_map_t m_uniforms{};
};

} // dk::gfx
