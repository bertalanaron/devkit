#include <gtest/gtest.h>

#include <devkit/common/utils.h>
#include <devkit/common/properties.h>


namespace dbg_test {

TEST(DebugStore, smoke) {
	EXPECT_EQ((dk::dbg::store_or<int, "hi">(3)), 3);
	dk::dbg::store<int, "hi">() = 4;
	EXPECT_EQ((dk::dbg::store_or<int, "hi">(3)), 4);
}

}

enum class Prop1 { a, b, c };
enum class Prop2 { a = 3, b, c };
struct Type : dk::common::DeferredPropertyCollection<Type> { 
	void _callPropertySetters() { callPropertySetters(); } 
};
static std::unordered_map<std::type_index, int> g_setProperties{};

template <> void details::common::setProperty<Type, Prop1>(Type& object, const Prop1& p) {
	g_setProperties[typeid(Prop1)] = (int)p;
}

template <> void details::common::setProperty<Type, Prop2>(Type& object, const Prop2& p) {
	g_setProperties[typeid(Prop2)] = (int)p;
}

namespace common_test {

TEST(DefferedPropertyCollection, smoke) {
	Type object;
	object.property(Prop1::a);
	object.property(Prop1::b);
	object.property(Prop2::c);
	
	EXPECT_EQ(object.property<Prop1>(), Prop1::b);
	EXPECT_EQ(object.property<Prop2>(), Prop2::c);

	object._callPropertySetters();

	EXPECT_EQ(g_setProperties[typeid(Prop1)], (int)Prop1::b);
	EXPECT_EQ(g_setProperties[typeid(Prop2)], (int)Prop2::c);
}

}
