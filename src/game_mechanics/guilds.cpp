/**
\authors Created by Sventovit
\date 14.05.2022.
\brief Модуль механики учителей умений/заклинаний/способностей.
*/

#include "guilds.h"

//#include <iostream>

#include "boot/boot_constants.h"
#include "color.h"
#include "game_mechanics/glory_const.h"
#include "game_magic/magic_utils.h"
#include "game_magic/spells_info.h"
#include "game_magic/magic_temp_spells.h"
#include "structs/global_objects.h"

typedef int special_f(CharData *, void *, int, char *);
extern void ASSIGNMASTER(MobVnum mob, special_f, int learn_info);

int DoGuildLearn(CharData *ch, void *me, int cmd, char *argument) {
	if (ch->IsNpc()) {
		return 0;
	}
	if (!CMD_IS("учить") && !CMD_IS("practice")) {
		return 0;
	}

	/*
	 *  Это не слишком красиво, потому что кто-нибудь может затереть поле stored в индексе и гильдия перестанет работать,
	 *  но позволяет не искать каждый раз тренера по всем гильдиям. По уму, нужно, чтобы поле stored как-то конструировалось
	 *  в комплекте с самой спецфункцией, и независимо его нельзя было бы перезаписать.
	 */
	auto *trainer = (CharData *) me;
	Vnum guild_vnum{-1};
	if (auto rnum = trainer->get_rnum(); rnum >= 0) {
		guild_vnum = mob_index[rnum].stored;
	}
	const auto &guild = MUD::Guilds(guild_vnum);

	if (guild.GetId() < 0) {
		act("$N сказал$G : 'Извини, $n, я уже в отставке.'", false, ch, nullptr, trainer, kToChar);
		err_log("try to call DoGuildLearn wuthout assigned guild vnum.");
		return 0;
	}

	std::string params{argument};
	utils::Trim(params);
	guild.Process(trainer, ch, params);
	return 1;
}

namespace guilds {

using ItemPtr = GuildInfoBuilder::ItemPtr;

void GuildsLoader::Load(DataNode data) {
	MUD::Guilds().Init(data.Children());
	AssignGuildsToTrainers();
}

void GuildsLoader::Reload(DataNode data) {
	MUD::Guilds().Reload(data.Children());
	AssignGuildsToTrainers();
}

void GuildsLoader::AssignGuildsToTrainers() {
	for (const auto &guild: MUD::Guilds()) {
		guild.AssignToTrainers();
	}
}

const std::string &GuildInfo::GetMessage(EGuildMsg msg_id) {
	static const std::unordered_map<EGuildMsg, std::string> guild_msgs = {
		{EGuildMsg::kGreeting, "$N сказал$G: 'Я могу научить тебя следующему:'"},
		{EGuildMsg::kDidNotTeach, "$N уставил$U на $n3 и прорычал$G: 'Я никогда и никого ЭТОМУ не учил$G!'"},
		{EGuildMsg::kInquiry, "$n о чем-то спросил$g $N3."},
		{EGuildMsg::kCannotToChar, "$N сказал$G: 'Я не могу тебя этому научить'."},
		{EGuildMsg::kCannotToRoom, "$N сказал$G $n2: 'Я не могу тебя этому научить'."},
		{EGuildMsg::kAskToChar, "Вы попросились в обучение к $N2."},
		{EGuildMsg::kAskToRoom, "$n попросил$u в ученики к $N2."},
		{EGuildMsg::kLearnToChar, "Вы получили несколько уроков и мудрых советов от $N1."},
		{EGuildMsg::kLearnToRoom, "$N дал$G $n2 несколько наставлений."},
		{EGuildMsg::kAllSkills,
		 "$N сказал$G: '$n, нельзя научиться всем умениям или способностям сразу. Выбери необходимые!'"},
		{EGuildMsg::kTalentEarned, "Под наставничеством $N1 вы изучили "},
		{EGuildMsg::kNothingLearned, "$N ничему новому вас не научил$G."},
		{EGuildMsg::kListEmpty, "$N сказал$G : 'Похоже, $n, я не смогу тебе помочь'."},
		{EGuildMsg::kIsInsolvent,
		 "$N сказал$G : 'Вот у меня забот нет - голодранцев наставлять! Иди-ка, $n, подзаработай сначала!"},
		{EGuildMsg::kFree, "бесплатно"},
		{EGuildMsg::kTemporary, "временно"},
		{EGuildMsg::kError, "У кодера какие-то проблемы."},
	};

	if (guild_msgs.contains(msg_id)) {
		return guild_msgs.at(msg_id);
	} else {
		return guild_msgs.at(EGuildMsg::kError);
	}
}

ItemPtr GuildInfoBuilder::Build(DataNode &node) {
	try {
		return ParseGuild(node);
	} catch (std::exception &e) {
		err_log("Guild parsing error: '%s'", e.what());
		return nullptr;
	}
}

ItemPtr GuildInfoBuilder::ParseGuild(DataNode node) {
	auto vnum = std::clamp(parse::ReadAsInt(node.GetValue("vnum")), 0, kMaxProtoNumber);
	auto mode = SkillInfoBuilder::ParseItemMode(node, EItemMode::kEnabled);

	std::string text_id{"kUndefined"};
	std::string name{"undefined"};
	try {
		text_id = parse::ReadAsStr(node.GetValue("text_id"));
		name = parse::ReadAsStr(node.GetValue("name"));
	} catch (...) {
	}

	auto guild_info = std::make_shared<GuildInfo>(vnum, text_id, name, mode);

	if (node.GoToChild("trainers")) {
		try {
			parse::ReadAsIntSet(guild_info->trainers_, node.GetValue("vnums"));
		} catch (std::runtime_error &e) {
			err_log("trainers error (%s) in guild '%s'.", e.what(), guild_info->GetName().c_str());
		}
	}

	if (node.GoToSibling("talents")) {
		ParseTalents(guild_info, node);
	}

	return guild_info;
}

void GuildInfoBuilder::ParseTalents(ItemPtr &info, DataNode &node) {
	for (auto &talent_node: node.Children()) {
		std::unordered_set<ECharClass> char_classes;
		if (talent_node.GoToChild("class")) {
			try {
				parse::ReadAsConstantsSet<ECharClass>(char_classes, talent_node.GetValue("val"));
			} catch (std::exception &e) {
				err_log("classes list (%s) in guild '%s'.", e.what(), info->GetName().c_str());
			}
			talent_node.GoToParent();
		}

		try {
			if (strcmp(talent_node.GetName(), "skill") == 0) {
				info->learning_talents_.emplace_back(std::make_unique<GuildInfo::GuildSkill>(talent_node));
			} else if (strcmp(talent_node.GetName(), "spell") == 0) {
				info->learning_talents_.emplace_back(std::make_unique<GuildInfo::GuildSpell>(talent_node));
			} else if (strcmp(talent_node.GetName(), "feat") == 0) {
				info->learning_talents_.emplace_back(std::make_unique<GuildInfo::GuildFeat>(talent_node));
			}
		} catch (std::exception &e) {
			err_log("talent format error (%s) in guild '%s'.", e.what(), info->GetName().c_str());
		}

		char_classes.clear();
	}
}

void GuildInfo::AssignToTrainers() const {
	for (const auto trainer_vnum: trainers_) {
		ASSIGNMASTER(trainer_vnum, DoGuildLearn, GetId());
	}
};

void GuildInfo::DisplayMenu(CharData *trainer, CharData *ch) const {
	std::ostringstream out;
	auto count{0};
	table_wrapper::Table table;
	//table << table_wrapper::kHeader << "#" << "Талант" << "Название" << "Цена" << table_wrapper::kEndRow;
	for (const auto &talent : learning_talents_) {
		if (talent->IsUnlearnable(ch)) {
			continue;
		}

		++count;
		table << (KCYN + std::to_string(count) + KNRM + ")" + KGRN)
			<< talent->GetTalentTypeName()
			<< ("'" + static_cast<std::string>(talent->GetName()) + "'" + KNRM);

		auto price = talent->CalcPrice(ch);
		if (price) {
			table << PrintNumberByDigits(price) << talent->GetPriceCurrencyStr(price);
		} else {
			table << "--" << GetMessage(EGuildMsg::kFree);
		}
		table << talent->GetAnnotation();
		table << table_wrapper::kEndRow;
	}

	act(GetMessage(EGuildMsg::kAskToChar), false, ch, nullptr, trainer, kToChar);
	act(GetMessage(EGuildMsg::kAskToRoom), false, ch, nullptr, trainer, kToRoom);
	if (count) {
		act(GetMessage(EGuildMsg::kGreeting), false, ch, nullptr, trainer, kToChar);
		table_wrapper::DecorateNoBorderTable(ch, table);
		table_wrapper::PrintTableToStream(out, table);
		out << std::endl;
		SendMsgToChar(out.str(), ch);
	} else {
		act(GetMessage(EGuildMsg::kListEmpty), false, ch, nullptr, trainer, kToChar);
		act(GetMessage(EGuildMsg::kListEmpty), false, ch, nullptr, trainer, kToRoom);
	}
}

void GuildInfo::Process(CharData *trainer, CharData *ch, const std::string &argument) const {
	if (argument.empty()) {
		DisplayMenu(trainer, ch);
		return;
	}

	act(GetMessage(EGuildMsg::kInquiry), false, ch, nullptr, trainer, kToRoom);

	if (utils::IsAbbrev(argument, "все") || utils::IsAbbrev(argument, "all")) {
		LearnAll(trainer, ch);
		return;
	}

	try {
		std::size_t talent_num = std::stoi(argument);
		LearnWithTalentNum(trainer, ch, talent_num);
	} catch (std::exception &) {
		LearnWithTalentName(trainer, ch, argument);
	}
};

void GuildInfo::LearnWithTalentNum(CharData *trainer, CharData *ch, std::size_t talent_num) const {
	talent_num = std::clamp(talent_num, 1UL, learning_talents_.size());

	for (const auto &talent : learning_talents_) {
		if (talent->IsUnlearnable(ch)) {
			continue;
		}

		--talent_num;

		if (talent_num == 0) {
			LearnSingle(trainer, ch, talent);
			return;
		}
	}

	act(GetMessage(EGuildMsg::kCannotToChar), false, ch, nullptr, trainer, kToChar);
	act(GetMessage(EGuildMsg::kCannotToRoom), false, ch, nullptr, trainer, kToRoom);
}

void GuildInfo::LearnWithTalentName(CharData *trainer, CharData *ch, const std::string &talent_name) const {
	auto result = std::find_if(learning_talents_.begin(), learning_talents_.end(),
							   [ch, &talent_name](const TalentPtr &talent) {
								   if (talent->IsUnlearnable(ch)) {
									   return false;
								   }
								   if (IsEquivalent(talent_name,
													static_cast<std::string>(talent->GetName()))) {
									   return true;
								   }
								   return false;
							   });

	if (result != learning_talents_.end()) {
		LearnSingle(trainer, ch, *result);
	} else {
		act(GetMessage(EGuildMsg::kListEmpty), false, ch, nullptr, trainer, kToChar);
		act(GetMessage(EGuildMsg::kListEmpty), false, ch, nullptr, trainer, kToRoom);
	}
}

void GuildInfo::LearnSingle(CharData *trainer, CharData *ch, const TalentPtr &talent) {
	if (ProcessPayment(trainer, ch, talent)) {
		act(GetMessage(EGuildMsg::kLearnToChar), false, ch, nullptr, trainer, kToChar);
		act(GetMessage(EGuildMsg::kLearnToRoom), false, ch, nullptr, trainer, kToRoom);
		Learn(trainer, ch, talent);
	}
}

void GuildInfo::LearnAll(CharData *trainer, CharData *ch) const {
	auto skill_feat_count{0};
	auto spell_count{0};
	auto need_msg{true};
	for (const auto &talent : learning_talents_) {
		if (talent->IsUnlearnable(ch)) {
			continue;
		}
		if (talent->GetTalentType() != ETalent::kSpell) {
			++skill_feat_count;
			continue;
		}
		if (ProcessPayment(trainer, ch, talent)) {
			if (need_msg) {
				act(GetMessage(EGuildMsg::kLearnToChar), false, ch, nullptr, trainer, kToChar);
				act(GetMessage(EGuildMsg::kLearnToRoom), false, ch, nullptr, trainer, kToRoom);
				need_msg = false;
			}
			Learn(trainer, ch, talent);
		} else {
			return;
		}
		++spell_count;
	}

	if (!skill_feat_count && !spell_count) {
		act(GetMessage(EGuildMsg::kNothingLearned), false, ch, nullptr, trainer, kToChar);
		act(GetMessage(EGuildMsg::kDidNotTeach), false, ch, nullptr, trainer, kToRoom);
	} else if (skill_feat_count && !spell_count) {
		act(GetMessage(EGuildMsg::kAllSkills), false, ch, nullptr, trainer, kToChar);
		act(GetMessage(EGuildMsg::kAllSkills), false, ch, nullptr, trainer, kToRoom);
	}
}

void GuildInfo::Learn(CharData *trainer, CharData *ch, const TalentPtr &talent) {
	std::ostringstream out;
	out << GetMessage(EGuildMsg::kTalentEarned) << talent->GetTalentTypeName() <<
		KIYEL << " '" << talent->GetName() << "'" << KNRM << ".";
	act(out.str(), false, ch, nullptr, trainer, kToChar);

	talent->SetTalent(ch);
};

bool GuildInfo::ProcessPayment(CharData *trainer, CharData *ch, const TalentPtr &talent) {
	if (!talent->TakePayment(ch)) {
		act(GetMessage(EGuildMsg::kIsInsolvent), false, ch, nullptr, trainer, kToChar);
		act(GetMessage(EGuildMsg::kIsInsolvent), false, ch, nullptr, trainer, kToRoom);
		return false;
	}
// \todo Сделать сообщение о взымании платы.
	return true;
}

void GuildInfo::Print(CharData *ch, std::ostringstream &buffer) const {
	buffer << "Print guild:" << std::endl
		   << " Vnum: " << KGRN << GetId() << KNRM << std::endl
		   << " TextId: " << KGRN << GetTextId() << KNRM << std::endl
		   << " Name: " << KGRN << name_ << KNRM << std::endl;

	if (!trainers_.empty()) {
		buffer << " Trainers vnums: " << KGRN;
		for (const auto vnum: trainers_) {
			buffer << vnum << ", ";
		}
		buffer.seekp(-2, std::ios_base::end);
		buffer << "." << KNRM << std::endl;
	}

	if (!learning_talents_.empty()) {
		buffer << " Trained talents: " << std::endl;
		table_wrapper::Table table;
		table << table_wrapper::kHeader << "Id" << "Name" << "Classes" << table_wrapper::kEndRow;
		for (const auto &talent: learning_talents_) {
			table << talent->GetIdAsStr() << talent->GetName() << talent->GetClassesList() << table_wrapper::kEndRow;
		}
		table_wrapper::DecorateNoBorderTable(ch, table);
		table_wrapper::PrintTableToStream(buffer, table);
	}

	buffer << std::endl;
}

bool GuildInfo::IGuildTalent::IsLearnable(CharData *ch) const {
	if (ch->IsNpc() || currency_vnum_ == info_container::kUndefinedVnum) {
		return false;
	}
	return ((trained_classes_.empty() || trained_classes_.contains(ch->GetClass())) && IsAvailable(ch));
}

std::string GuildInfo::IGuildTalent::GetClassesList() const {
	std::ostringstream buffer;
	if (!trained_classes_.empty()) {
		for (const auto class_id: trained_classes_) {
			buffer << NAME_BY_ITEM(class_id) << ", ";
		}
		buffer.seekp(-2, std::ios_base::end);
		buffer << ".";
	} else {
		buffer << "all";
	}

	return buffer.str();
}

GuildInfo::IGuildTalent::IGuildTalent(ETalent talent_type, DataNode &node) {

	talent_type_ = talent_type;

	if (node.GoToChild("class")) {
		try {
			parse::ReadAsConstantsSet<ECharClass>(trained_classes_, node.GetValue("val"));
		} catch (std::exception &e) {
			err_log("wrong class list format (%s).", e.what());
		}
		node.GoToParent();
	}

	if (node.GoToChild("price")) {
		try {
			auto currency_text_id = parse::ReadAsStr(node.GetValue("currency"));
			currency_vnum_ = MUD::Currencies().FindAvailableItem(currency_text_id).GetId();
			start_price_ = parse::ReadAsInt(node.GetValue("start"));
			remort_percemt_ = parse::ReadAsInt(node.GetValue("remort_percent"));
		} catch (std::exception &e) {
			err_log("wrong price format (%s).", e.what());
		}
		node.GoToParent();
	}
}

long GuildInfo::IGuildTalent::CalcPrice(CharData *buyer) const {
	return start_price_ + (start_price_*remort_percemt_*buyer->get_remort())/100;
}

std::string GuildInfo::IGuildTalent::GetPriceCurrencyStr(uint64_t price) const {
	if (currency_vnum_ != info_container::kUndefinedVnum) {
		return MUD::Currencies(currency_vnum_).GetNameWithAmount(price);
	} else {
		return GetMessage(EGuildMsg::kError);
	}
}

bool HasEnoughCurrency(CharData *ch, Vnum currency_id, long amount);
void WithdrawCurrency(CharData *ch, Vnum currency_id, long amount);

bool GuildInfo::IGuildTalent::TakePayment(CharData *ch) const {
	auto price = CalcPrice(ch);

	if (HasEnoughCurrency(ch, currency_vnum_, price)) {
		WithdrawCurrency(ch, currency_vnum_, price);
		return true;
	}

	return false;
}

void GuildInfo::GuildSkill::ParseSkillNode(DataNode &node) {
	id_ = parse::ReadAsConstant<ESkill>(node.GetValue("id"));
	if (node.GoToChild("upgrade")) {
		try {
			amount_ = parse::ReadAsInt(node.GetValue("amount"));
			min_skill_ = parse::ReadAsInt(node.GetValue("min"));
			max_skill_ = parse::ReadAsInt(node.GetValue("max"));
		} catch (std::exception &e) {
			err_log("wrong upgrade format (%s).", e.what());
		}
		node.GoToParent();
	}
}

const std::string &GuildInfo::GuildSkill::GetIdAsStr() const {
	return NAME_BY_ITEM<ESkill>(id_);
}

std::string_view GuildInfo::GuildSkill::GetName() const {
	return MUD::Skills(id_).name;
}

bool GuildInfo::GuildSkill::IsAvailable(CharData *ch) const {
	// \todo Сделать учет апгрейда
	return (CanGetSkill(ch, id_) && ch->GetSkill(id_) == 0);
}

void GuildInfo::GuildSkill::SetTalent(CharData *ch) const {
	ch->set_skill(id_, 10);
}

std::string GuildInfo::GuildSkill::GetAnnotation() const {
	return "";
}

void GuildInfo::GuildSpell::ParseSpellNode(DataNode &node) {
	id_ = parse::ReadAsConstant<ESpell>(node.GetValue("id"));
	try {
		spell_type_ = parse::ReadAsConstant<ESpellType>(node.GetValue("type"));
		spell_time_sec_ = kSecsPerRealMin * std::max(0, parse::ReadAsInt(node.GetValue("time")));
	} catch (std::exception &) { }
}

const std::string &GuildInfo::GuildSpell::GetIdAsStr() const {
	return NAME_BY_ITEM<ESpell>(id_);
}

std::string_view GuildInfo::GuildSpell::GetName() const {
	return spell_info[id_].name;
}

bool GuildInfo::GuildSpell::IsAvailable(CharData *ch) const {
	return CanGetSpell(ch, id_) && !IS_SPELL_KNOWN(ch, id_);
}

void GuildInfo::GuildSpell::SetTalent(CharData *ch) const {
	if (spell_type_ == ESpellType::kTemp) {
		temporary_spells::AddSpell(ch, id_, time(nullptr), spell_time_sec_);
	} else {
		SET_BIT(GET_SPELL_TYPE(ch, id_), ESpellType::kKnow);
	}
}

std::string GuildInfo::GuildSpell::GetAnnotation() const {
	if (spell_type_ == ESpellType::kTemp) {
		std::ostringstream out;
		out << GetMessage(EGuildMsg::kTemporary) << " ("
			<< FormatTimeToStr(spell_time_sec_/kSecsPerRealMin) << ")";
		return out.str();
	}
	return "";
}

void GuildInfo::GuildFeat::ParseFeatNode(DataNode &node) {
	id_ = parse::ReadAsConstant<EFeat>(node.GetValue("id"));
}

const std::string &GuildInfo::GuildFeat::GetIdAsStr() const {
	return NAME_BY_ITEM<EFeat>(id_);
}

std::string_view GuildInfo::GuildFeat::GetName() const {
	return feat_info[id_].name;
}

bool GuildInfo::GuildFeat::IsAvailable(CharData *ch) const {
	return CanGetFeat(ch, id_) && !ch->HaveFeat(id_);
}

void GuildInfo::GuildFeat::SetTalent(CharData *ch) const {
	ch->SetFeat(id_);
}

std::string GuildInfo::GuildFeat::GetAnnotation() const {
	return "";
}

/*
 *  Костыльные функции для проверки/снятия валют, поскольку системы валют пока нет.
 */

bool HasEnoughCurrency(CharData *ch, Vnum currency_id, long amount) {
	switch (currency_id) {
		case 0: { // куны
			return ch->get_gold() >= amount;
		}
		case 1: { // слава
			const auto total_glory = GloryConst::get_glory(GET_UNIQUE(ch));
			return total_glory >= amount;
		}
		case 2: { // гривны
			return ch->get_hryvn() >= amount;
		}
		case 3: { // лед
			return ch->get_ice_currency() >= amount;
		}
		case 4: { // ногаты
			return ch->get_nogata() >= amount;
		}
		default:
			return false;
	}
}

void WithdrawCurrency(CharData *ch, Vnum currency_id, long amount) {
	switch (currency_id) {
		case 0: { // куны
			ch->remove_gold(amount);
			break;
		}
		case 1: { // слава
			GloryConst::add_total_spent(amount);
			GloryConst::remove_glory(GET_UNIQUE(ch), amount);
			GloryConst::transfer_log("%s spent %ld const glory in a guild.", GET_NAME(ch), amount);
			break;
		}
		case 2: { // гривны
			ch->sub_hryvn(amount);
			ch->spent_hryvn_sub(amount);
			break;
		}
		case 3: { // лед
			ch->sub_ice_currency(amount);
			break;
		}
		case 4: { // ногаты
			ch->sub_nogata(amount);
			break;
		}
		default:
			return;
	}
}

}
// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
