/**
\authors Created by Sventovit
\date 14.05.2022.
\brief Модуль механики учителей умений/заклинаний/способностей.
*/

//#include "guilds.h"

#include <fmt/format.h>

#include "engine/ui/color.h"
#include "glory_const.h"
#include "gameplay/magic/magic_utils.h"
#include "gameplay/magic/magic_temp_spells.h"
#include "engine/db/global_objects.h"
#include "engine/structs/info_container.h"   // info_container::kUndefinedVnum

typedef int special_f(CharData *, void *, int, char *);
extern void ASSIGNMASTER(MobVnum mob, special_f, int learn_info);

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

// issue.thing-names: Vedun editable surface for guilds. Guilds are keyed by an integer vnum (no
// enum, no `id` attribute), so the element id handed to/from the editor is the vnum as a string;
// text_id + name go in the label so a guild can also be found by either of those.
std::string GuildsLoader::EditableWhat() const {
	return "guild";
}

std::vector<cfg_manager::EditableElement> GuildsLoader::ListElements() const {
	std::vector<cfg_manager::EditableElement> out;
	for (const auto &guild : MUD::Guilds()) {
		if (guild.GetId() < 0) {   // skip the kUndefined sentinel
			continue;
		}
		out.push_back({std::to_string(guild.GetId()), guild.GetTextId() + " " + guild.GetName()});
	}
	return out;
}

cfg_manager::ValidationResult GuildsLoader::Validate(parser_wrapper::DataNode &doc) const {
	if (MUD::Guilds().Validate(doc.Children())) {
		return {true, ""};
	}
	return {false, "Guild data failed to parse (see syslog for the offending guild)."};
}

parser_wrapper::DataNode GuildsLoader::FindElementNode(parser_wrapper::DataNode root,
														  const std::string &id) const {
	// A <guild> carries no `id` attribute; its key is the integer `vnum`. Iterate ALL children with
	// a name check rather than Children("guild"): a node copied out of a keyed range would otherwise
	// carry that range's filter, breaking its own later Children() (it would stop after the 1st child).
	for (auto &child : root.Children()) {
		if (std::string(child.GetName()) == "guild" && id == child.GetValue("vnum")) {
			return child;
		}
	}
	return parser_wrapper::DataNode{};
}

std::string GuildsLoader::CanonicalElementId(const std::string &id) const {
	// Only a non-negative integer is a valid (new) guild key. Existing guilds are already resolved
	// by ListElements (by vnum, text_id or name) before this create-path hook is consulted.
	if (id.empty()) {
		return "";
	}
	for (const char c : id) {
		if (c < '0' || c > '9') {
			return "";
		}
	}
	return id;
}

parser_wrapper::DataNode GuildsLoader::CreateElementNode(parser_wrapper::DataNode root,
															const std::string &id) const {
	auto node = root.AddChild("guild");
	node.SetValue("vnum", id);
	node.SetValue("text_id", "kUndefined");
	node.SetValue("name", "новая гильдия");
	return node;
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

	if (node.GoToChild("currency")) {
		try {
			guild_info->default_currency_vnum_ = MUD::Currencies().FindAvailableItem(parse::ReadAsStr(node.GetValue("val"))).GetId();
		} catch (std::exception &e) {
			err_log("guild currency error (%s) in guild '%s'.", e.what(), guild_info->GetName().c_str());
		}
		node.GoToParent();
	}
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
	}
	// Talents without their own currency inherit the guild-wide default; if the guild sets none either,
	// fall back to gold so the price still renders (legacy behaviour).
	const Vnum guild_currency = info->default_currency_vnum_ != info_container::kUndefinedVnum
		? info->default_currency_vnum_
		: currencies::kGoldVnum;
	for (auto &talent : info->learning_talents_) {
		talent->ApplyDefaultCurrency(guild_currency);
	}
}

const std::string &GuildInfo::GetMsg(EMsg msg_id) {
	// issue.thing-names: guild messages live in guild_msg.xml (the default sheaf of the vnum-keyed
	// guild message container), not a hardcoded table.
	return MUD::GuildMessages().GetMessage(info_container::kUndefinedVnum, msg_id);
}

int GuildInfo::DoGuildLearn(CharData *ch, void *me, int /*cmd*/, char *argument) {
	if (ch->IsNpc()) {
		return 0;
	}
	// dispatched via do_specproc(kGuild); the command word is always учить/practice.

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
	const auto &guild = MUD::Guild(guild_vnum);

	if (guild.GetId() < 0) {
		act(GetMsg(EMsg::kDischarge), false, ch, nullptr, trainer, kToChar);
		err_log("try to call DoGuildLearn wuthout assigned guild vnum.");
		return 0;
	}

	std::string params{argument};
	utils::Trim(params);
	guild.Process(trainer, ch, params);
	return 1;
}

void GuildInfo::AssignToTrainers() const {
	for (const auto trainer_vnum: trainers_) {
		ASSIGNMASTER(trainer_vnum, DoGuildLearn, GetId());
	}
};

void GuildInfo::Process(CharData *trainer, CharData *ch, std::string &argument) const {
	if (argument.empty()) {
		DisplayMenu(trainer, ch);
		return;
	}

	act(GetMsg(EMsg::kInquiry), false, ch, nullptr, trainer, kToRoom);

	if (utils::IsAbbr(argument, "все") || utils::IsAbbr(argument, "all")) {
		LearnAll(trainer, ch);
		return;
	}
	if (argument.size() > 100) {
		SendMsgToChar("Превышена максимальная длина строки.", ch);
		return;
	}
	try {
		std::size_t talent_num = std::stoi(argument);
		LearnWithTalentNum(trainer, ch, talent_num);
	} catch (std::exception &) {
		LearnWithTalentName(trainer, ch, argument);
	}
};

void GuildInfo::DisplayMenu(CharData *trainer, CharData *ch) const {
	std::ostringstream out;
	auto count{0};
	table_wrapper::Table table;
	for (const auto &talent : learning_talents_) {
		if (talent->IsUnlearnable(ch)) {
			continue;
		}

		++count;
		table << (kColorCyn + std::to_string(count) + kColorNrm + ")" + kColorGrn)
			  << talent->GetTalentTypeName()
			  << ("'" + static_cast<std::string>(talent->GetName()) + "'" + kColorNrm);

		auto price = talent->CalcPrice(ch);
		if (price) {
			table << price << talent->GetPriceCurrencyStr(price);
		} else {
			table << "--" << GetMsg(EMsg::kFree);
		}
		table << talent->GetAnnotation(ch);
		table << table_wrapper::kEndRow;
	}

	act(GetMsg(EMsg::kAskToChar), false, ch, nullptr, trainer, kToChar);
	act(GetMsg(EMsg::kAskToRoom), false, ch, nullptr, trainer, kToRoom);
	if (count) {
		act(GetMsg(EMsg::kGreeting), false, ch, nullptr, trainer, kToChar);
		table_wrapper::DecorateNoBorderTable(ch, table);
		table_wrapper::PrintTableToStream(out, table);
		out << "\n";
		SendMsgToChar(out.str(), ch);
	} else {
		act(GetMsg(EMsg::kListEmpty), false, ch, nullptr, trainer, kToChar);
		act(GetMsg(EMsg::kListEmpty), false, ch, nullptr, trainer, kToRoom);
	}
}

void GuildInfo::LearnWithTalentNum(CharData *trainer, CharData *ch, std::size_t talent_num) const {
	if (learning_talents_.empty()) {
		return;   // issue #3631: нечему учить -- иначе clamp(1, size=0) с lo>hi
	}
	talent_num = std::clamp(talent_num, size_t(1), learning_talents_.size());

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

	act(GetMsg(EMsg::kCannotToChar), false, ch, nullptr, trainer, kToChar);
	act(GetMsg(EMsg::kCannotToRoom), false, ch, nullptr, trainer, kToRoom);
}

void GuildInfo::LearnWithTalentName(CharData *trainer, CharData *ch, const std::string &talent_name) const {
	auto result = std::find_if(learning_talents_.begin(), learning_talents_.end(),
							   [ch, &talent_name](const TalentPtr &talent) {
								   if (talent->IsUnlearnable(ch)) {
									   return false;
								   }
								   if (utils::IsEquivalent(talent_name,
													static_cast<std::string>(talent->GetName()))) {
									   return true;
								   }
								   return false;
							   });

	if (result != learning_talents_.end()) {
		LearnSingle(trainer, ch, *result);
	} else {
		act(GetMsg(EMsg::kListEmpty), false, ch, nullptr, trainer, kToChar);
		act(GetMsg(EMsg::kListEmpty), false, ch, nullptr, trainer, kToRoom);
	}
}

void GuildInfo::LearnSingle(CharData *trainer, CharData *ch, const TalentPtr &talent) {
	if (ProcessPayment(trainer, ch, talent)) {
		act(GetMsg(EMsg::kLearnToChar), false, ch, nullptr, trainer, kToChar);
		act(GetMsg(EMsg::kLearnToRoom), false, ch, nullptr, trainer, kToRoom);
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
				act(GetMsg(EMsg::kLearnToChar), false, ch, nullptr, trainer, kToChar);
				act(GetMsg(EMsg::kLearnToRoom), false, ch, nullptr, trainer, kToRoom);
				need_msg = false;
			}
			Learn(trainer, ch, talent);
		} else {
			return;
		}
		++spell_count;
	}

	if (!skill_feat_count && !spell_count) {
		act(GetMsg(EMsg::kNothingLearned), false, ch, nullptr, trainer, kToChar);
		act(GetMsg(EMsg::kDidNotTeach), false, ch, nullptr, trainer, kToRoom);
	} else if (skill_feat_count && !spell_count) {
		act(GetMsg(EMsg::kAllSkills), false, ch, nullptr, trainer, kToChar);
		act(GetMsg(EMsg::kAllSkills), false, ch, nullptr, trainer, kToRoom);
	}
}

void GuildInfo::Learn(CharData *trainer, CharData *ch, const TalentPtr &talent) {
	if (talent->IsLearningFailed()) {
		act(GetMsg(EMsg::kFailToChar), false, ch, nullptr, trainer, kToChar);
		act(GetMsg(EMsg::kFailToRoom), false, ch, nullptr, trainer, kToRoom);
	} else {
		auto out = fmt::format(fmt::runtime(GetMsg(EMsg::kTalentEarned)),
							   fmt::arg("talent_type", talent->GetTalentTypeName()),
							   fmt::arg("color", kColorBoldYel),
							   fmt::arg("name", talent->GetName()),
							   fmt::arg("nocolor", kColorNrm));
		act(out, false, ch, nullptr, trainer, kToChar);
		talent->SetTalent(ch);
	}
};

bool GuildInfo::ProcessPayment(CharData *trainer, CharData *ch, const TalentPtr &talent) {
	if (!talent->TakePayment(ch)) {
		act(GetMsg(EMsg::kIsInsolvent), false, ch, nullptr, trainer, kToChar);
		act(GetMsg(EMsg::kIsInsolvent), false, ch, nullptr, trainer, kToRoom);
		return false;
	}

	auto price = talent->CalcPrice(ch);
	if (price > 0) {
		auto description = MUD::Currency(talent->GetCurrencyId()).GetObjName(price, grammar::ECase::kAcc);
		act(fmt::format(fmt::runtime(GetMsg(EMsg::kYouGiveMoney)), fmt::arg("money", description)),
			false, ch, nullptr, trainer, kToChar);
		act(fmt::format(fmt::runtime(GetMsg(EMsg::kSomeoneGivesMoney)), fmt::arg("money", description)),
			false, ch, nullptr, trainer, kToRoom);
	}

	return true;
}

void GuildInfo::Print(CharData *ch, std::ostringstream &buffer) const {
/*	buffer << fmt::format("Print guild:\n Vnum: {grn}{}{nrm} \n TextId: {grn}{}{nrm}\n Name: {grn}{}{nrm}\n",
				    GetId(), GetTextId(), name_, fmt::arg("grn", KGRN), fmt::arg("nrm", KNRM));*/

	buffer << "Print guild:" << "\n"
		   << " Vnum: " << kColorGrn << GetId() << kColorNrm << "\n"
		   << " TextId: " << kColorGrn << GetTextId() << kColorNrm << "\n"
		   << " Name: " << kColorGrn << name_ << kColorNrm << "\n";

	if (!trainers_.empty()) {
		buffer << " Trainers vnums: " << kColorGrn;
		for (const auto vnum: trainers_) {
			buffer << vnum << ", ";
		}
		buffer.seekp(-2, std::ios_base::end);
		buffer << "." << kColorNrm << "\n";
	}

	if (!learning_talents_.empty()) {
		buffer << " Trained talents: " << "\n";
		table_wrapper::Table table;
		table << table_wrapper::kHeader
			<< "Id" << "Name" << "Currency" << "Annotation" << "Fail" << "Class" << table_wrapper::kEndRow;
		for (const auto &talent: learning_talents_) {
			table << talent->GetIdAsStr()
				<< talent->GetName()
				<< MUD::Currency(talent->GetCurrencyId()).GetPluralName()
				<< talent->GetAnnotation(ch)
				<< talent->GetFailChance()
				<< talent->GetClassesList()
				<< table_wrapper::kEndRow;
		}
		table_wrapper::DecorateNoBorderTable(ch, table);
		table_wrapper::PrintTableToStream(buffer, table);
	}

	buffer << "\n";
}

bool GuildInfo::IGuildTalent::IsLearnable(CharData *ch) const {
	if (ch->IsNpc() || currency_vnum_ == info_container::kUndefinedVnum) {
		return false;
	}
	return ((trained_classes_.empty() || trained_classes_.contains(ch->GetClass())) && IsAvailable(ch));
}

std::string GuildInfo::IGuildTalent::GetClassesList() const {
	if (!trained_classes_.empty()) {
		auto out = fmt::memory_buffer();
		for (const auto class_id: trained_classes_) {
			if (out.size()) {
				fmt::format_to(std::back_inserter(out), ", {}", NAME_BY_ITEM(class_id));
			} else {
				fmt::format_to(std::back_inserter(out), "{}", NAME_BY_ITEM(class_id));
			}
		}
		fmt::format_to(std::back_inserter(out), ".");
		return to_string(out);
	} else {
		return "all";
	}
}

GuildInfo::IGuildTalent::IGuildTalent(ETalent talent_type, DataNode &node) {

	talent_type_ = talent_type;

	try {
		fail_chance_ = std::clamp(parse::ReadAsInt(node.GetValue("fail")), 0, 100);
	} catch (std::exception &e) {
		err_log("wrong fail chance format (%s).", e.what());
	}

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
			// An absent (or kUndefined/invalid) currency leaves currency_vnum_ at kUndefinedVnum so the
			// talent inherits the guild-wide default (applied by ApplyDefaultCurrency); only an explicit,
			// valid currency overrides it.
			const char *currency_text_id = parse::ReadAsStr(node.GetValue("currency"));
			if (currency_text_id != nullptr && *currency_text_id != '\0') {
				const auto id = MUD::Currencies().FindAvailableItem(currency_text_id).GetId();
				if (id >= 0) {
					currency_vnum_ = id;
				}
			}
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

std::string GuildInfo::IGuildTalent::GetPriceCurrencyStr(long price) const {
	if (currency_vnum_ != info_container::kUndefinedVnum) {
		return MUD::Currency(currency_vnum_).GetNameWithAmount(price);
	} else {
		return GetMsg(EMsg::kError);
	}
}


bool GuildInfo::IGuildTalent::TakePayment(CharData *ch) const {
	auto price = CalcPrice(ch);

	if (currencies::GetHand(*ch, currency_vnum_) >= price) {
		currencies::RemoveHand(*ch, currency_vnum_, price);
		return true;
	}

	return false;
}

bool GuildInfo::IGuildTalent::IsLearningFailed() const {
	auto roll = number(1, 100);
	return  roll <= fail_chance_;
}

void GuildInfo::GuildSkill::ParseSkillNode(DataNode &node) {
	id_ = parse::ReadAsConstant<ESkill>(node.GetValue("id"));
	if (node.GoToChild("upgrade")) {
		try {
			practices_ = std::max(1, parse::ReadAsInt(node.GetValue("practices")));
			min_skill_ = std::max(0, parse::ReadAsInt(node.GetValue("min")));
			max_skill_ = std::max(1, parse::ReadAsInt(node.GetValue("max")));
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
	return MUD::Skill(id_).name;
}

int GuildInfo::GuildSkill::CalcGuildSkillCap(CharData *ch) const {
	return std::min(GetTrainedSkill(ch, id_) + practices_,
					std::min(CalcSkillHardCap(ch, id_), max_skill_));
}

int GuildInfo::GuildSkill::CalcPracticesQuantity(CharData *ch) const {
	return std::clamp(CalcGuildSkillCap(ch) - GetTrainedSkill(ch, id_), 1, std::max(1, practices_));   // issue #3631: hi>=lo
}

long GuildInfo::GuildSkill::CalcPrice(CharData *buyer) const {
	return GuildInfo::IGuildTalent::CalcPrice(buyer)*CalcPracticesQuantity(buyer);
};

bool GuildInfo::GuildSkill::IsAvailable(CharData *ch) const {
	auto skill = GetTrainedSkill(ch, id_);
	return CanGetSkill(ch, id_) && skill >= min_skill_ &&
		skill < max_skill_ && skill < CalcSkillHardCap(ch, id_);
}

void GuildInfo::GuildSkill::SetTalent(CharData *ch) const {
	SetSkill(ch, id_, GetTrainedSkill(ch, id_) + CalcPracticesQuantity(ch));
}

std::string GuildInfo::GuildSkill::GetAnnotation(CharData *ch) const {
	return fmt::format("{}-{} (+{})", min_skill_, max_skill_, CalcPracticesQuantity(ch));
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
	return MUD::Spell(id_).GetName();
}

bool GuildInfo::GuildSpell::IsAvailable(CharData *ch) const {
	return CanGetSpell(ch, id_) && !IS_SPELL_SET(ch, id_, ESpellType::kKnow);
}

void GuildInfo::GuildSpell::SetTalent(CharData *ch) const {
	if (spell_type_ == ESpellType::kTemp) {
		auto spell_duration = spell_time_sec_ + temporary_spells::GetSpellLeftTime(ch, id_);
		temporary_spells::AddSpell(ch, id_, time(nullptr), spell_duration);
	} else {
		SET_BIT(GET_SPELL_TYPE(ch, id_), ESpellType::kKnow);
	}
}

std::string GuildInfo::GuildSpell::GetAnnotation(CharData * /*ch*/) const {
	if (spell_type_ == ESpellType::kTemp) {
		return fmt::format("{} ({})",
						   GetMsg(EMsg::kTemporary), FormatTimeToStr(spell_time_sec_/kSecsPerRealMin));
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
	return MUD::Feat(id_).GetName();
}

bool GuildInfo::GuildFeat::IsAvailable(CharData *ch) const {
	return CanGetFeat(ch, id_) && !ch->HaveFeat(id_);
}

void GuildInfo::GuildFeat::SetTalent(CharData *ch) const {
	ch->SetFeat(id_);
}

std::string GuildInfo::GuildFeat::GetAnnotation(CharData * /*ch*/) const {
	return "";
}

/*
 *  Костыльные функции для проверки/снятия валют, поскольку системы валют пока нет.
 */

}
// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
