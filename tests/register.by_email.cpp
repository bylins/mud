// Регистрация по мылу: список зарегистрированных адресов -- это тоже регистрация, а не только
// пометка на дискрипторе, которую ставил вход. Пока её видел один вход, зашедший с адреса первым
// оставался для секундной проверки незарегистрированным, и как только с того же адреса входил
// второй, его уносило в комнату для незарегистрированных -- без выхода оттуда.

#include "char.utilities.hpp"

#include <gtest/gtest.h>

#include "administration/proxy.h"
#include "engine/entities/char_data.h"
#include "utils/utils.h"

#include <cstring>

namespace register_by_email_test {

const char *kEmail = "test-register-by-email@example.invalid";

CharData::shared_ptr MakePlayerWithEmail(const char *email) {
	test_utils::CharacterBuilder builder;
	builder.create_new();
	auto ch = builder.get();
	std::strncpy(GET_EMAIL(ch), email, sizeof(GET_EMAIL(ch)) - 1);
	return ch;
}

TEST(RegisterByEmail, EmailInTheListCountsAsRegistered) {
	auto first = MakePlayerWithEmail(kEmail);
	auto second = MakePlayerWithEmail(kEmail);

	// Второй персонаж ничего не знает про регистрацию: ни флага, ни дискриптора с пометкой.
	EXPECT_FALSE(second->IsFlagged(EPlrFlag::kRegistred));
	EXPECT_FALSE(RegisterSystem::IsRegistered(second.get()));

	// Регистрируем первого -- его мыло попадает в список.
	RegisterSystem::add(first.get(), "Родовит", "человек");
	EXPECT_TRUE(RegisterSystem::IsRegisteredEmail(GET_EMAIL(first)));

	// Теперь второй зарегистрирован тем же мылом, хотя флага у него по-прежнему нет.
	EXPECT_TRUE(RegisterSystem::IsRegistered(second.get()));
	EXPECT_FALSE(second->IsFlagged(EPlrFlag::kRegistred));

	// Снятие регистрации убирает мыло из списка, и второй снова незарегистрирован.
	RegisterSystem::remove(first.get());
	EXPECT_FALSE(RegisterSystem::IsRegisteredEmail(kEmail));
	EXPECT_FALSE(RegisterSystem::IsRegistered(second.get()));
}

TEST(RegisterByEmail, NpcIsNeverRegisteredByEmail) {
	auto mob = MakePlayerWithEmail(kEmail);
	auto player = MakePlayerWithEmail(kEmail);
	RegisterSystem::add(player.get(), "Родовит", "человек");
	EXPECT_TRUE(RegisterSystem::IsRegistered(player.get()));

	// Неписем делаем только на время проверки: разрушать непися с выделенными player_specials
	// нельзя -- purge на таком ругается в лог через GET_MOB_VNUM, а mob_index в тестах пустой.
	mob->SetNpcAttribute(true);
	const bool mob_registered = RegisterSystem::IsRegistered(mob.get());
	mob->SetNpcAttribute(false);
	EXPECT_FALSE(mob_registered);

	RegisterSystem::remove(player.get());
}

} // namespace register_by_email_test

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
