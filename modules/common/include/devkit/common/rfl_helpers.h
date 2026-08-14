#pragma once

#include <array>
#include <cstddef>

#include <glm/glm.hpp>
#include <rfl.hpp>

namespace dk::common::detail {

template <typename Vector>
struct glm_vector_formatter;

template <glm::length_t Length, typename Scalar, glm::qualifier Qualifier>
struct glm_vector_formatter<glm::vec<Length, Scalar, Qualifier>> {
    using ReflType = std::array<Scalar, Length>;
    using ValueType = glm::vec<Length, Scalar, Qualifier>;

    static ValueType to(const ReflType& values) noexcept
    {
        ValueType result;
        for (glm::length_t i = 0; i < Length; ++i)
            result[i] = values[static_cast<std::size_t>(i)];
        return result;
    }

    static ReflType from(const ValueType& value) noexcept
    {
        ReflType result{};
        for (glm::length_t i = 0; i < Length; ++i)
            result[static_cast<std::size_t>(i)] = value[i];
        return result;
    }
};

template <typename Matrix>
struct glm_matrix_formatter;

template <glm::length_t Columns, typename Scalar, glm::qualifier Qualifier>
struct glm_matrix_formatter<glm::mat<Columns, Columns, Scalar, Qualifier>> {
    using ReflType = std::array<std::array<Scalar, Columns>, Columns>;
    using ValueType = glm::mat<Columns, Columns, Scalar, Qualifier>;

    static ValueType to(const ReflType& values) noexcept
    {
        ValueType result(static_cast<Scalar>(0));
        for (glm::length_t column = 0; column < Columns; ++column)
            for (glm::length_t row = 0; row < Columns; ++row)
                result[column][row] = values[static_cast<std::size_t>(column)][static_cast<std::size_t>(row)];
        return result;
    }

    static ReflType from(const ValueType& value) noexcept
    {
        ReflType result{};
        for (glm::length_t column = 0; column < Columns; ++column)
            for (glm::length_t row = 0; row < Columns; ++row)
                result[static_cast<std::size_t>(column)][static_cast<std::size_t>(row)] = value[column][row];
        return result;
    }
};

} // namespace dk::common::detail

namespace rfl {

template <glm::length_t Length, typename Scalar, glm::qualifier Qualifier>
    requires (Length == 2 || Length == 3 || Length == 4)
struct Reflector<glm::vec<Length, Scalar, Qualifier>>
    : dk::common::detail::glm_vector_formatter<glm::vec<Length, Scalar, Qualifier>> {};

template <glm::length_t Columns, typename Scalar, glm::qualifier Qualifier>
    requires (Columns == 3 || Columns == 4)
struct Reflector<glm::mat<Columns, Columns, Scalar, Qualifier>>
    : dk::common::detail::glm_matrix_formatter<glm::mat<Columns, Columns, Scalar, Qualifier>> {};

} // namespace rfl
