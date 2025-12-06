#include "engine/entities/char_data.h"

#include <gtest/gtest.h>

// п╒п╣я│я┌ п╫п╟ п╠п╟пЁ: п©п╬я│п╩п╣ п╨п╬п©п╦я─п╬п╡п╟п╫п╦я▐ CharData я┤п╣я─п╣п╥ operator=,
// current_morph_->ch_ я┐п╨п╟п╥я▀п╡п╟п╣я┌ п╫п╟ я│я┌п╟я─я▀п╧ (п╡п╬п╥п╪п╬п╤п╫п╬ я┐п╢п╟п╩я▒п╫п╫я▀п╧) п╬п╠я┼п╣п╨я┌.
// get_wis() п╡я▀п╥я▀п╡п╟п╣я┌ current_morph_->GetWis() -> ch_->GetInbornWis(),
// я┤я┌п╬ п©я─п╦п╡п╬п╢п╦я┌ п╨ segfault п╣я│п╩п╦ п╬я─п╦пЁп╦п╫п╟п╩ я┐п╢п╟п╩я▒п╫.

TEST(CHAR_MorphCopy, AssignmentOperator_MorphPointerValid)
{
	// п║п╬п╥п╢п╟я▒п╪ п╬я─п╦пЁп╦п╫п╟п╩я▄п╫я▀п╧ п©п╣я─я│п╬п╫п╟п╤
	CharData original;
	original.player_specials = std::make_shared<player_special_data>();
	original.set_wis(15);

	// п║п╬п╥п╢п╟я▒п╪ п╫п╬п╡я▀п╧ п©п╣я─я│п╬п╫п╟п╤ п╦ п╨п╬п©п╦я─я┐п╣п╪ п╡ п╫п╣пЁп╬ я┤п╣я─п╣п╥ operator=
	CharData copy;
	copy.player_specials = std::make_shared<player_special_data>();
	copy = original;

	// п÷п╬я│п╩п╣ п╨п╬п©п╦я─п╬п╡п╟п╫п╦я▐ get_wis() п╢п╬п╩п╤п╣п╫ я─п╟п╠п╬я┌п╟я┌я▄ п╨п╬я─я─п╣п╨я┌п╫п╬
	// п╦ п╡п╬п╥п╡я─п╟я┴п╟я┌я▄ я│п╨п╬п©п╦я─п╬п╡п╟п╫п╫п╬п╣ п╥п╫п╟я┤п╣п╫п╦п╣
	EXPECT_EQ(15, copy.get_wis());
}

TEST(CHAR_MorphCopy, AssignmentOperator_OriginalDeleted_CopyStillWorks)
{
	CharData copy;
	copy.player_specials = std::make_shared<player_special_data>();

	{
		// п║п╬п╥п╢п╟я▒п╪ п╬я─п╦пЁп╦п╫п╟п╩ п╡ п╬я┌п╢п╣п╩я▄п╫п╬п╪ я│п╨п╬я┐п©п╣
		CharData original;
		original.player_specials = std::make_shared<player_special_data>();
		original.set_wis(20);

		// п п╬п©п╦я─я┐п╣п╪
		copy = original;

		// original п╠я┐п╢п╣я┌ я┐п╫п╦я┤я┌п╬п╤п╣п╫ п©я─п╦ п╡я▀я┘п╬п╢п╣ п╦п╥ я│п╨п╬я┐п©п╟
	}

	// п÷п╬я│п╩п╣ я┐п╫п╦я┤я┌п╬п╤п╣п╫п╦я▐ п╬я─п╦пЁп╦п╫п╟п╩п╟ copy.get_wis() п²п∙ п╢п╬п╩п╤п╣п╫ п©п╟п╢п╟я┌я▄
	// п╜я┌п╬ пЁп╩п╟п╡п╫я▀п╧ я┌п╣я│я┌ п╫п╟ п╠п╟пЁ - current_morph_->ch_ п╢п╬п╩п╤п╣п╫ я┐п╨п╟п╥я▀п╡п╟я┌я▄ п╫п╟ copy, п╟ п╫п╣ п╫п╟ original
	EXPECT_EQ(20, copy.get_wis());
}

TEST(CHAR_MorphCopy, ArrayReallocation_SimulatesMobProtoResize)
{
	// п║п╦п╪я┐п╩я▐я├п╦я▐ я┌п╬пЁп╬, я┤я┌п╬ п©я─п╬п╦я│я┘п╬п╢п╦я┌ п╡ medit.cpp п©я─п╦ п╢п╬п╠п╟п╡п╩п╣п╫п╦п╦ п╫п╬п╡п╬пЁп╬ п╪п╬п╠п╟:
	// 1. п║п╬п╥п╢п╟я▒я┌я│я▐ п╫п╬п╡я▀п╧ п╪п╟я│я│п╦п╡
	// 2. п п╬п©п╦я─я┐я▌я┌я│я▐ я█п╩п╣п╪п╣п╫я┌я▀ п╦п╥ я│я┌п╟я─п╬пЁп╬
	// 3. п║я┌п╟я─я▀п╧ п╪п╟я│я│п╦п╡ я┐п╢п╟п╩я▐п╣я┌я│я▐
	// 4. п·п╠я─п╟я┴п╣п╫п╦п╣ п╨ я█п╩п╣п╪п╣п╫я┌п╟п╪ п╫п╬п╡п╬пЁп╬ п╪п╟я│я│п╦п╡п╟

	const int OLD_SIZE = 3;
	const int NEW_SIZE = 4;

	// "п║я┌п╟я─я▀п╧" п╪п╟я│я│п╦п╡ (я│п╦п╪я┐п╩я▐я├п╦я▐ mob_proto)
	CharData* old_array = new CharData[OLD_SIZE];
	for (int i = 0; i < OLD_SIZE; ++i) {
		old_array[i].player_specials = std::make_shared<player_special_data>();
		old_array[i].set_wis(10 + i);
	}

	// "п²п╬п╡я▀п╧" п╪п╟я│я│п╦п╡
	CharData* new_array = new CharData[NEW_SIZE];
	for (int i = 0; i < NEW_SIZE; ++i) {
		new_array[i].player_specials = std::make_shared<player_special_data>();
	}

	// п п╬п©п╦я─я┐п╣п╪ я█п╩п╣п╪п╣п╫я┌я▀ (п╨п╟п╨ п╡ medit.cpp я│я┌я─п╬п╨п╦ 413, 418)
	for (int i = 0; i < OLD_SIZE; ++i) {
		new_array[i] = old_array[i];
	}

	// пёп╢п╟п╩я▐п╣п╪ я│я┌п╟я─я▀п╧ п╪п╟я│я│п╦п╡ (п╨п╟п╨ п╡ medit.cpp я│я┌я─п╬п╨п╟ 437)
	delete[] old_array;
	old_array = nullptr;

	// п╒п╣п©п╣я─я▄ п╬п╠я─п╟я┴п╟п╣п╪я│я▐ п╨ get_wis() п╫п╟ я█п╩п╣п╪п╣п╫я┌п╟я┘ п╫п╬п╡п╬пЁп╬ п╪п╟я│я│п╦п╡п╟
	// п╜я┌п╬ п╢п╬п╩п╤п╫п╬ я─п╟п╠п╬я┌п╟я┌я▄, п╟ п╫п╣ п©п╟п╢п╟я┌я▄ я│ segfault
	for (int i = 0; i < OLD_SIZE; ++i) {
		EXPECT_EQ(10 + i, new_array[i].get_wis());
	}

	delete[] new_array;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
