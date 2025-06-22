#pragma once
#include <devkit/common/utils.h>

namespace details::io {

void commitInputState(uint8_t, uint8_t, uint64_t);

}

namespace dk::io {

class InputCombination {
private:
	using button_mask_t = uint8_t;
	using modkey_mask_t = uint8_t;
	using key_mask_t    = uint64_t;
	
	struct state_t {
		button_mask_t button;
		modkey_mask_t modkey;
		key_mask_t    key;
	};

public:
	class Evaluator;

	constexpr InputCombination(button_mask_t button, modkey_mask_t modkey, key_mask_t key)
		: mask({ button, modkey, key })
	{ }

	constexpr InputCombination operator+(const InputCombination& rhs) const 
	{
		return InputCombination(mask.button + rhs.mask.button, mask.modkey + rhs.mask.modkey, mask.key + rhs.mask.key);
	}

private:
	state_t mask;

	friend void details::io::commitInputState(uint8_t, uint8_t, uint64_t);
};

#define DK_IO_DECL_BUTTON(name, index) constexpr InputCombination name   = InputCombination(1u << index, 0, 0)
#define DK_IO_DECL_MODKEY(name, index) constexpr InputCombination name   = InputCombination(0, 1u << index, 0)
#define DK_IO_DECL_KEY(name, index)    constexpr InputCombination name   = InputCombination(0, 0, uint64_t(1) << (uint64_t)index)

namespace button {

#define DK_IO_BUTTONS_TABLE(FV)\
	FV( left   , 0 );          \
	FV( middle , 1 );          \
	FV( right  , 2 );          \
	FV( x1     , 3 );          \
	FV( x2     , 4 );          \
	/* end of table */

DK_IO_BUTTONS_TABLE(DK_IO_DECL_BUTTON)

}

namespace modkey {

#define DK_IO_MODKEYS_TABLE(FV)\
	FV( shift , 0 );           \
	FV( ctrl  , 1 );           \
	FV( alt   , 2 );           \
	FV( caps  , 3 );           \
	/* end of table */

DK_IO_MODKEYS_TABLE(DK_IO_DECL_MODKEY)

}

namespace key {

#define DK_IO_KEYS_TABLE(FV)\
	FV( _0 , 0 );           \
	FV( _1 , 1 );           \
	FV( _2 , 2 );           \
	FV( _3 , 3 );           \
	FV( _4 , 4 );           \
	FV( _5 , 5 );           \
	FV( _6 , 6 );           \
	FV( _7 , 7 );           \
	FV( _8 , 8 );           \
	FV( _9 , 9 );           \
                            \
	FV( a , 10 );           \
	FV( b , 11 );           \
	FV( c , 12 );           \
	FV( d , 13 );           \
	FV( e , 14 );           \
	FV( f , 15 );           \
	FV( g , 16 );           \
	FV( h , 17 );           \
	FV( i , 18 );           \
	FV( j , 19 );           \
	FV( k , 20 );           \
	FV( l , 21 );           \
	FV( m , 22 );           \
	FV( n , 23 );           \
	FV( o , 24 );           \
	FV( p , 25 );           \
	FV( q , 26 );           \
	FV( r , 27 );           \
	FV( s , 28 );           \
	FV( t , 29 );           \
	FV( u , 30 );           \
	FV( v , 31 );           \
	FV( w , 32 );           \
	FV( x , 33 );           \
	FV( y , 34 );           \
	FV( z , 35 );           \
	                        \
	FV( f1  , 35 );         \
	FV( f2  , 36 );         \
	FV( f3  , 37 );         \
	FV( f4  , 38 );         \
	FV( f5  , 39 );         \
	FV( f6  , 40 );         \
	FV( f7  , 41 );         \
	FV( f8  , 42 );         \
	FV( f9  , 43 );         \
	FV( f10 , 44 );         \
	FV( f11 , 45 );         \
	FV( f12 , 46 );         \
	                        \
	FV( esc   , 47 );       \
	FV( tilda , 48 );       \
	FV( space , 48 );       \
	/* end of table */

DK_IO_KEYS_TABLE(DK_IO_DECL_KEY)

}

#undef DK_IO_BUTTONS_TABLE
#undef DK_IO_MODKEYS_TABLE
#undef DK_IO_DECL_BUTTON
#undef DK_IO_DECL_MODKEY
#undef DK_IO_DECL_KEY

class InputCombination::Evaluator {
private:
	enum Inclusivity { Inclusive, Exclusive };
	enum Repeatability { Continous, Trigger };

public:
	constexpr Evaluator triggered() const
	{
		Evaluator res(*this);
		res.m_repeatability = Trigger;
		return res;
	}

	operator bool() const
	{
		const bool forCurrent = evaluateForState(m_inputCombination, s_currentState, m_inclusivity);
		if (m_repeatability == Continous)
			return forCurrent;
		const bool forPrevious = evaluateForState(m_inputCombination, s_previousState, m_inclusivity);
		return forCurrent && !forPrevious;
	}

private:
	Inclusivity      m_inclusivity;
	Repeatability    m_repeatability;
	InputCombination m_inputCombination;

	inline static InputCombination::state_t s_currentState{};
	inline static InputCombination::state_t s_previousState{};

	constexpr static bool evaluateForState(const InputCombination& ic, const state_t& state, Inclusivity inclusivity) 
	{
		if (inclusivity == Inclusive)
			return ((ic.mask.button & state.button) || (!ic.mask.button))
			    && ((ic.mask.modkey & state.modkey) || (!ic.mask.modkey)) 
			    && ((ic.mask.key & state.key)       || (!ic.mask.key));
		return (ic.mask.button == state.button) && (ic.mask.modkey == state.modkey) && (ic.mask.key == state.key);
	}

	constexpr Evaluator(const Evaluator&) = default;

	constexpr Evaluator(Inclusivity inc, Repeatability rep, InputCombination ic)
		: m_inclusivity(inc)
		, m_repeatability(rep)
		, m_inputCombination(ic)
	{ }

	friend void details::io::commitInputState(uint8_t, uint8_t, uint64_t);
	friend constexpr InputCombination::Evaluator exclusive(const InputCombination& ic);
	friend constexpr InputCombination::Evaluator inclusive(const InputCombination& ic);
};

// @brief Evaluates input connection to true only when no other inputs are active than the ones required by the combination
inline constexpr InputCombination::Evaluator exclusive(const InputCombination& ic) 
{
	return InputCombination::Evaluator(InputCombination::Evaluator::Exclusive, InputCombination::Evaluator::Continous, ic);
}

// @brief Evaluates input connection to true even when other inputs are active, not just the ones required by the combination
inline constexpr InputCombination::Evaluator inclusive(const InputCombination& ic) 
{
	return InputCombination::Evaluator(InputCombination::Evaluator::Inclusive, InputCombination::Evaluator::Continous, ic);
}

}

namespace details::io {

inline void commitInputState(uint8_t button, uint8_t modkey, uint64_t key)
{
	dk::io::InputCombination::Evaluator::s_previousState = dk::io::InputCombination::Evaluator::s_currentState;
	dk::io::InputCombination::Evaluator::s_currentState.button = button;
	dk::io::InputCombination::Evaluator::s_currentState.modkey = modkey;
	dk::io::InputCombination::Evaluator::s_currentState.key = key;
}

}
