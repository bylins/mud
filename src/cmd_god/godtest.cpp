#include "godtest.h"

#include <iostream>
//#include <unordered_map>

#include "entities/char.h"
#include "modify.h"

#include "structs/global_objects.h"

// This is test command for different testings
void do_godtest(CharacterData *ch, char * /*argument*/, int /* cmd */, int /* subcmd */) {
	std::stringstream buffer;
	buffer << "В настоящий момент процiдурка пуста.\r\nЕсли вам хочется что-то godtest - придется ее реализовать."
		   << std::endl;
	page_string(ch->desc, buffer.str());

/*	const abilities::AbilityInfo &ability = GlobalObjects::Abilities()[abilities::EAbility::kKick];
	page_string(ch->desc, ability.Print());*/
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
