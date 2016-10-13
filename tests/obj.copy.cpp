#include <gtest/gtest.h>

#include <obj.hpp>

constexpr obj_vnum OBJECT_VNUM = 100500;
constexpr obj_vnum PROTOTYPE_VNUM = 100501;

constexpr obj_rnum OBJECT_RNUM = 100510;
constexpr obj_rnum PROTOTYPE_RNUM = 100511;

const std::string ACTION_DESCRIPTION = "action description";
const std::string CASE_PREFIX = "case ";
const CObjectPrototype::triggers_list_t TRIGGERS_LIST = { 1, 2, 3 };

auto create_empty_object()
{
	auto result = std::make_shared<OBJ_DATA>(OBJECT_VNUM);
	return result;
}

auto create_object()
{
	auto result = create_empty_object();
	result->set_rnum(OBJECT_RNUM);
	return result;
}

auto create_empty_prototype()
{
	auto result = std::make_shared<CObjectPrototype>(PROTOTYPE_VNUM);
	return result;
}

auto create_prototype()
{
	auto result = create_empty_prototype();

	result->set_action_description(ACTION_DESCRIPTION);

	for (auto i = 0u; i < CObjectPrototype::NUM_PADS; i++)
	{
		const std::string case_name = CASE_PREFIX + std::to_string(i);
		result->set_PName(i, case_name);
	}

	result->set_proto_script(TRIGGERS_LIST);
	result->set_rnum(PROTOTYPE_RNUM);

	return result;
}

TEST(Object_Copy, SimpleCopy)
{
	auto object = create_object();
	const auto prototype = create_empty_prototype();

	*object = *prototype;
	EXPECT_EQ(OBJECT_VNUM, object->get_vnum());
}

TEST(Object_Copy, Assignment_Operator)
{
	auto object = create_object();
	const auto prototype = create_prototype();

	ASSERT_NE(object->get_vnum(), prototype->get_vnum());

	// Commented out lines should be uncommented when they are initialized in prototype.
	//ASSERT_NE(object->get_type(), prototype->get_type());
	//ASSERT_NE(object->get_weight(), prototype->get_weight());
	//ASSERT_NE(object->get_affected(), prototype->get_affected());
	//ASSERT_NE(object->get_aliases(), prototype->get_aliases());
	//ASSERT_NE(object->get_description(), prototype->get_description());
	//ASSERT_NE(object->get_short_description(), prototype->get_short_description());
	ASSERT_NE(object->get_action_description(), prototype->get_action_description());
	/*ASSERT_NE(object->get_ex_description(), prototype->get_ex_description());*/
	ASSERT_NE(object->get_proto_script(), prototype->get_proto_script());
	//ASSERT_NE(object->get_max_in_world(), prototype->get_max_in_world());
	//ASSERT_NE(object->get_values(), prototype->get_values());
	//ASSERT_NE(object->get_destroyer(), prototype->get_destroyer());
	//ASSERT_NE(object->get_spell(), prototype->get_spell());
	//ASSERT_NE(object->get_level(), prototype->get_level());
	//ASSERT_NE(object->get_skill(), prototype->get_skill());
	//ASSERT_NE(object->get_maximum_durability(), prototype->get_maximum_durability());
	//ASSERT_NE(object->get_current_durability(), prototype->get_current_durability());
	//ASSERT_NE(object->get_material(), prototype->get_material());
	//ASSERT_NE(object->get_sex(), prototype->get_sex());
	//ASSERT_NE(object->get_extra_flags(), prototype->get_extra_flags());
	//ASSERT_NE(object->get_affect_flags(), prototype->get_affect_flags());
	//ASSERT_NE(object->get_anti_flags(), prototype->get_anti_flags());
	//ASSERT_NE(object->get_no_flags(), prototype->get_no_flags());
	//ASSERT_NE(object->get_wear_flags(), prototype->get_wear_flags());
	//ASSERT_NE(object->get_timer(), prototype->get_timer());
	//ASSERT_NE(object->get_skills(), prototype->get_skills());
	//ASSERT_NE(object->get_minimum_remorts(), prototype->get_minimum_remorts());
	//ASSERT_NE(object->get_cost(), prototype->get_cost());
	//ASSERT_NE(object->get_rent_on(), prototype->get_rent_on());
	//ASSERT_NE(object->get_rent_off(), prototype->get_rent_off());
	//ASSERT_NE(object->get_ilevel(), prototype->get_ilevel());
	ASSERT_NE(object->get_rnum(), prototype->get_rnum());

	for (auto i = 0u; i < CObjectPrototype::NUM_PADS; i++)
	{
		ASSERT_NE(object->get_PName(i), prototype->get_PName(i));
	}

	//for (auto i = 0u; i < CObjectPrototype::VALS_COUNT; ++i)
	//{
	//	ASSERT_NE(object->get_val(i), prototype->get_val(i));
	//}

	*object = *prototype;

	// Commented out lines should be uncommented when they are initialized in prototype.
	EXPECT_NE(object->get_vnum(), prototype->get_vnum());
	//EXPECT_EQ(object->get_type(), prototype->get_type());
	//EXPECT_EQ(object->get_weight(), prototype->get_weight());
	//EXPECT_EQ(object->get_affected(), prototype->get_affected());
	//EXPECT_EQ(object->get_aliases(), prototype->get_aliases());
	//EXPECT_EQ(object->get_description(), prototype->get_description());
	//EXPECT_EQ(object->get_short_description(), prototype->get_short_description());
	EXPECT_EQ(object->get_action_description(), prototype->get_action_description());
	/*EXPECT_EQ(object->get_ex_description(), prototype->get_ex_description());*/
	EXPECT_EQ(object->get_proto_script(), prototype->get_proto_script());
	//EXPECT_EQ(object->get_max_in_world(), prototype->get_max_in_world());
	//EXPECT_EQ(object->get_values(), prototype->get_values());
	//EXPECT_EQ(object->get_destroyer(), prototype->get_destroyer());
	//EXPECT_EQ(object->get_spell(), prototype->get_spell());
	//EXPECT_EQ(object->get_level(), prototype->get_level());
	//EXPECT_EQ(object->get_skill(), prototype->get_skill());
	//EXPECT_EQ(object->get_maximum_durability(), prototype->get_maximum_durability());
	//EXPECT_EQ(object->get_current_durability(), prototype->get_current_durability());
	//EXPECT_EQ(object->get_material(), prototype->get_material());
	//EXPECT_EQ(object->get_sex(), prototype->get_sex());
	//EXPECT_EQ(object->get_extra_flags(), prototype->get_extra_flags());
	//EXPECT_EQ(object->get_affect_flags(), prototype->get_affect_flags());
	//EXPECT_EQ(object->get_anti_flags(), prototype->get_anti_flags());
	//EXPECT_EQ(object->get_no_flags(), prototype->get_no_flags());
	//EXPECT_EQ(object->get_wear_flags(), prototype->get_wear_flags());
	//EXPECT_EQ(object->get_timer(), prototype->get_timer());
	//EXPECT_EQ(object->get_skills(), prototype->get_skills());
	//EXPECT_EQ(object->get_minimum_remorts(), prototype->get_minimum_remorts());
	//EXPECT_EQ(object->get_cost(), prototype->get_cost());
	//EXPECT_EQ(object->get_rent_on(), prototype->get_rent_on());
	//EXPECT_EQ(object->get_rent_off(), prototype->get_rent_off());
	//EXPECT_EQ(object->get_ilevel(), prototype->get_ilevel());
	EXPECT_EQ(object->get_rnum(), prototype->get_rnum());

	for (auto i = 0u; i < CObjectPrototype::NUM_PADS; i++)
	{
		EXPECT_EQ(object->get_PName(i), prototype->get_PName(i));
	}

	//for (auto i = 0u; i < CObjectPrototype::VALS_COUNT; ++i)
	//{
	//	EXPECT_EQ(object->get_val(i), prototype->get_val(i));
	//}
}

// Currently I don't know a way to test "clone" because it uses a lot of global structures.
// TEST(Object_Copy, Clone) {}