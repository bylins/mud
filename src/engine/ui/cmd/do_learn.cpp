#include "do_learn.h"

#include "engine/core/handler.h"
#include "gameplay/classes/classes_spell_slots.h"
#include "gameplay/classes/pc_classes.h"
#include "gameplay/magic/spells_info.h"
#include "engine/db/global_objects.h"

namespace do_learn {

class LearningError : public std::exception {};
class LowRemortOrLvl : public std::exception {};
class LearningFail : public std::exception {};
class NotAvailable : public std::exception {};
class AlreadyKnown : public std::runtime_error {
 public:
	AlreadyKnown() = delete;
	explicit AlreadyKnown(const std::string &known_what)
		: std::runtime_error(known_what) {};
};

bool IsLearningFailed(CharData *ch, ObjData *obj) {
	auto addchance = LIKE_ROOM(ch) ? 10 : 0;
	addchance += (GET_OBJ_VAL(obj, 0) == EBook::kSpell) ? 0 : 10;

	if (!obj->has_flag(EObjFlag::KNofail)
		&& number(1, 100) > int_app[POSI(GetRealInt(ch))].spell_aknowlege + addchance) {
		return true;
	}
	return false;
}

void SendSuccessLearningMessage(CharData *ch, ObjData *book, const std::string &talent_name) {
	static const std::string stype0[] = {"несколько секретов", "секрет"};
	static const std::string stype2[] = {"заклинания", "умения", "умения", "рецепта", "способности"};

	std::ostringstream out;
	out << "Вы взяли в руки " << book->get_PName(ECase::kAcc) << " и начали изучать." << "\r\n" <<
		"Постепенно, незнакомые доселе, буквы стали складываться в понятные слова и фразы." << "\r\n" <<
		"Буквально через несколько минут вы узнали " <<
		((GET_OBJ_VAL(book, 0) == EBook::kSkillUpgrade) ? stype0[0] : stype0[1]) <<
		" " << stype2[GET_OBJ_VAL(book, 0)] << " \"" << talent_name << "\"." << "\r\n";
	SendMsgToChar(out.str(), ch);
	out.str(std::string());

	act("$n взял$g в руки $o3 и принял$u изучать.\r\n"
		"Спустя некоторое время $s лицо озарилось пониманием.\r\n"
		"Вы с удивлением увидели, как $o замерцал$G и растаял$G.",
		false, ch, book, nullptr, kToRoom);

	out << "LEARN: Игрок" << ch->get_name() << "выучил " <<
		((GET_OBJ_VAL(book, 0) == EBook::kSkillUpgrade) ? stype0[1] : stype0[0]) << " " <<
		stype2[GET_OBJ_VAL(book, 0)] << " \"" << talent_name << "\"";
	log("%s", out.str().c_str());
}

void LearnSpellBook(CharData *ch, ObjData *obj) {
	if (classes::CalcCircleSlotsAmount(ch, 1) <= 0) {
		SendMsgToChar("Далась вам эта магия! Пошли бы, водочки выпили...\r\n", ch);
		throw LearningError();
	}
	auto spell_id = static_cast<ESpell>(GET_OBJ_VAL(obj, 1));
	if (spell_id <= ESpell::kUndefined || spell_id > ESpell::kLast) {
		SendMsgToChar("МАГИЯ НЕ ОПРЕДЕЛЕНА - сообщите Богам!\r\n", ch);
		throw LearningError();
	}

	if (!CanGetSpell(ch, spell_id, GET_OBJ_VAL(obj, 2))) {
		throw LowRemortOrLvl();
	}
	auto spell_name = MUD::Spell(spell_id).GetName();
	if (GET_SPELL_TYPE(ch, spell_id) & ESpellType::kKnow) {
		throw AlreadyKnown(spell_name);
	}
	if (IsLearningFailed(ch, obj)) {
		throw LearningFail();
	}

	GET_SPELL_TYPE(ch, spell_id) |= ESpellType::kKnow;
	SendSuccessLearningMessage(ch, obj, spell_name);
}

void LearnSkillBook(CharData *ch, ObjData *obj) {
	if (GET_OBJ_VAL(obj, 2) < 1) {
		SendMsgToChar("НЕКОРРЕКТНЫЙ УРОВЕНЬ - сообщите Богам!\r\n", ch);
		throw LearningError();
	}

	auto skill_id = static_cast<ESkill>(GET_OBJ_VAL(obj, 1));
	if (MUD::Skills().IsInvalid(skill_id)) {
		SendMsgToChar("УМЕНИЕ НЕ ОПРЕДЕЛЕНО - сообщите Богам!\r\n", ch);
		throw LearningError();
	}

	auto skill_name = MUD::Skill(skill_id).GetName();
	if (ch->GetSkill(skill_id)) {
		throw AlreadyKnown(skill_name);
	}
	if (!CanGetSkill(ch, skill_id, GET_OBJ_VAL(obj, 2))) {
		throw LowRemortOrLvl();
	}
	if (IsLearningFailed(ch, obj)) {
		throw LearningFail();
	}

	ch->set_skill(skill_id, 5);
	SendSuccessLearningMessage(ch, obj, skill_name);
}

void LearnSkillUpgradeBook(CharData *ch, ObjData *obj) {
	auto skill_id = static_cast<ESkill>(GET_OBJ_VAL(obj, 1));
	if (MUD::Skills().IsInvalid(skill_id)) {
		SendMsgToChar("УМЕНИЕ НЕ ОПРЕДЕЛЕНО - сообщите Богам!\r\n", ch);
		throw LearningError();
	}

	const auto book_skill_cap = GET_OBJ_VAL(obj, 3);
	auto skill_name = MUD::Skill(skill_id).GetName();
	if ((book_skill_cap > 0 && ch->GetMorphSkill(skill_id) >= book_skill_cap) ||
		(book_skill_cap <= 0 && ch->GetMorphSkill(skill_id) >= CalcSkillRemortCap(ch))) {
		throw AlreadyKnown(skill_name);
	}
	if (!CanGetSkill(ch, skill_id, GET_OBJ_VAL(obj, 2))) {
		throw LowRemortOrLvl();
	}
	if (IsLearningFailed(ch, obj)) {
		throw LearningFail();
	}

	SendSuccessLearningMessage(ch, obj, skill_name);
	const auto left_skill_level = ch->GetMorphSkill(skill_id) + GET_OBJ_VAL(obj, 2);
	if (book_skill_cap > 0) {
		ch->set_skill(skill_id, std::min(left_skill_level, GET_OBJ_VAL(obj, 3)));
	} else {
		ch->set_skill(skill_id, std::min(left_skill_level, CalcSkillRemortCap(ch)));
	}
}

void ProcessLearningNotAvailable(CharData *ch, ObjData *book);
void LearnReceiptBook(CharData *ch, ObjData *obj) {
	auto receipt_id = im_get_recipe(GET_OBJ_VAL(obj, 1));
	auto receipt_name = imrecipes[receipt_id].name;

	if (receipt_id < 0) {
		SendMsgToChar("РЕЦЕПТ НЕ ОПРЕДЕЛЕН - сообщите Богам!\r\n", ch);
		throw LearningError();
	}
	if (imrecipes[receipt_id].classknow[(int) ch->GetClass()] != kKnownRecipe) {
		throw NotAvailable();
	}
	im_rskill *receipt_skill = im_get_char_rskill(ch, receipt_id);
	if (receipt_skill) {
		throw AlreadyKnown(receipt_name);
	}
	if (MAX(GET_OBJ_VAL(obj, 2), imrecipes[receipt_id].level) <= GetRealLevel(ch) &&
		imrecipes[receipt_id].remort <= GetRealRemort(ch)) {
		if (imrecipes[receipt_id].level == -1 || imrecipes[receipt_id].remort == -1) {
			SendMsgToChar("Некорректная запись рецепта для вашего класса - сообщите Богам.\r\n", ch);
			throw LowRemortOrLvl();
		}
	}
	if (IsLearningFailed(ch, obj)) {
		throw LearningFail();
	}
	SendSuccessLearningMessage(ch, obj, receipt_name);
	CREATE(receipt_skill, 1);
	receipt_skill->rid = receipt_id;
	receipt_skill->link = GET_RSKILL(ch);
	GET_RSKILL(ch) = receipt_skill;
	receipt_skill->perc = 5;
}

void LearnFeatBook(CharData *ch, ObjData *obj) {
	auto feat_id = static_cast<EFeat>(GET_OBJ_VAL(obj, 1));
	if (MUD::Feat(feat_id).IsUnavailable()) {
		SendMsgToChar("СПОСОБНОСТЬ НЕ ОПРЕДЕЛЕНА - сообщите Богам!\r\n", ch);
		throw LearningError();
	}

	auto feat_name = MUD::Feat(feat_id).GetName();
	if (ch->HaveFeat(feat_id)) {
		throw AlreadyKnown(feat_name);
	}

	if (!CanGetFeat(ch, feat_id)) {
		throw LowRemortOrLvl();
	}

	if (IsLearningFailed(ch, obj)) {
		throw LearningFail();
	}

	SendSuccessLearningMessage(ch, obj, feat_name);
	ch->SetFeat(feat_id);
}

void LearnBook(CharData *ch, ObjData *obj) {
	switch (GET_OBJ_VAL(obj, 0)) {
		case EBook::kSpell: {
			LearnSpellBook(ch, obj);
			break;
		}
		case EBook::kSkill: {
			LearnSkillBook(ch, obj);
			break;
		}
		case EBook::kSkillUpgrade: {
			LearnSkillUpgradeBook(ch, obj);
			break;
		}
		case EBook::kReceipt: {
			LearnReceiptBook(ch, obj);
			break;
		}
		case EBook::kFeat: {
			LearnFeatBook(ch, obj);
			break;
		}
		default: break;
	}
}

bool IncorrectBookType(ObjData *obj) {
	auto book_type = GET_OBJ_VAL(obj, 0);
	return (book_type != EBook::kSpell &&
		book_type != EBook::kSkill &&
		book_type != EBook::kSkillUpgrade &&
		book_type != EBook::kReceipt &&
		book_type != EBook::kFeat);
}

ObjData *FindBook(CharData *ch, char *argument) {
	one_argument(argument, arg);
	if (!*arg) {
		SendMsgToChar("Вы принялись внимательно изучать свои ногти. Да, пора бы и подстричь.\r\n", ch);
		act("$n удивленно уставил$u на свои ногти. Подстриг бы их кто-нибудь $m.",
			false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
		return nullptr;
	}

	auto book = get_obj_in_list_vis(ch, arg, ch->carrying);
	if (!book) {
		SendMsgToChar("А у вас этого нет.\r\n", ch);
		return nullptr;
	}

	if (book->get_type() != EObjType::kBook) {
		act("Вы уставились на $o3, как баран на новые ворота.",
			false, ch, book, nullptr, kToChar);
		act("$n начал$g внимательно изучать устройство $o1.",
			false, ch, book, nullptr, kToRoom);
		return nullptr;
	}

	if (IncorrectBookType(book)) {
		act("НЕВЕРНЫЙ ТИП КНИГИ - сообщите Богам!", false, ch, book, nullptr, kToChar);
		return nullptr;
	}

	return book;
}

void ProcessLowRemortOrLevelException(CharData *ch, ObjData *book) {
	const char *positon = number(0, 1) ? "вон та" : (number(0, 1) ? "вот эта" : "пятая справа");
	const char *what = number(0, 1) ? "жука" : (number(0, 1) ? "бабочку" : "русалку");
	const char
		*whom = book->get_sex() == EGender::kFemale ? "нее" : (book->get_sex() == EGender::kPoly ? "них" : "него");
	sprintf(buf,
			"- \"Какие интересные буковки! Особенно %s, похожая на %s\".\r\n"
			"Полюбовавшись еще несколько минут на сию красоту, вы с чувством выполненного\r\n"
			"долга закрыли %s. До %s вы еще не доросли.\r\n",
			positon, what, book->get_PName(ECase::kAcc).c_str(), whom);
	SendMsgToChar(buf, ch);
	act("$n с интересом осмотрел$g $o3, крякнул$g от досады и положил$g обратно.",
		false, ch, book, nullptr, kToRoom);
}

void ProcessLearningFailException(CharData *ch, ObjData *book) {
	sprintf(buf, "Вы взяли в руки %s и начали изучать.\r\n"
				 "Непослушные буквы никак не хотели выстраиваться в понятные и доступные фразы.\r\n"
				 "Промучившись несколько минут, вы бросили это унылое занятие, с удивлением отметив исчезновение %s.\r\n",
			book->get_PName(ECase::kAcc).c_str(), book->get_PName(ECase::kGen).c_str());
	SendMsgToChar(buf, ch);
	act("$n взял$g в руки $o3 и принял$u изучать, но промучившись несколько минут, бросил$g это занятие.\r\n"
		"Вы с удивлением увидели, как $o замерцал$G и растаял$G.",
		false, ch, book, nullptr, kToRoom);
	ExtractObjFromWorld(book);
}

void ProcessLearningNotAvailable(CharData *ch, ObjData *book) {
	sprintf(buf, "Вы взяли в руки %s и начали изучать.\r\n"
			"Какая-то бормотуха получится, решили вы.\r\n",
			book->get_PName(ECase::kAcc).c_str());
	SendMsgToChar(buf, ch);
	act("$n взял$g в руки $o3 и принял$u изучать, но почесав макушку, бросил$g это занятие.\r\n",
			false, ch, book, nullptr, kToRoom);
}

void ProcessAlreadyKnownException(CharData *ch, ObjData *book, const AlreadyKnown &exception) {
	static const std::string talent_type[] = {"заклинание", "умение", "умение", "рецепт", "рецепт", "способность"};

	std::ostringstream out;
	out << "Вы открыли " << book->get_PName(ECase::kAcc) << " и принялись с интересом изучать." << "\r\n" <<
		"Каким же было разочарование, когда ";
	if (GET_OBJ_VAL(book, 0) == EBook::kSkillUpgrade) {
		out << "изучив " << book->get_PName(ECase::kAcc).c_str() << " от корки до корки вы так и не узнали ничего нового.";
	} else {
		out << "прочитав " <<
			(number(0, 1) ? "несколько страниц, " : number(0, 1) ? "пару строк," : "почти до конца,") <<
			"\r\n" << "вы поняли, что это " << talent_type[GET_OBJ_VAL(book, 0)] <<
			" \"" << exception.what() << "\".";
	}
	out << "\r\n";
	SendMsgToChar(out.str(), ch);

	act("$n с интересом принял$u читать $o3.\r\n"
		"Постепенно $s интерес начал угасать, и $e, плюясь, сунул$g $o3 обратно.",
		false, ch, book, nullptr, kToRoom);
}

void ProcessExceptions(CharData *ch, ObjData *book) {
	try {
		throw;
	} catch (const LowRemortOrLvl &e) {
		ProcessLowRemortOrLevelException(ch, book);
	} catch (const LearningFail &e) {
		ProcessLearningFailException(ch, book);
	} catch (const AlreadyKnown &e) {
		ProcessAlreadyKnownException(ch, book, e);
	} catch (const NotAvailable &e) {
		ProcessLearningNotAvailable(ch, book);
	} catch (const LearningError &e) {
		// We don't need to do something in this case.
	} catch (...) {
		log("Unprocessed exception has been thrown in DoLearn!");
	}
}

} // namespace do_learn

void DoLearn(CharData *ch, char *argument, int/* cmd*/, int /*subcmd*/) {
	if (ch->IsNpc()) {
		return;
	}

	auto book = do_learn::FindBook(ch, argument);
	if (!book) {
		return;
	}

	try {
		do_learn::LearnBook(ch, book);
		ExtractObjFromWorld(book);
	} catch (const std::exception &e) {
		do_learn::ProcessExceptions(ch, book);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
