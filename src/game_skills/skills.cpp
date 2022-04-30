/*************************************************************************
*   File: skills.cpp                                   Part of Bylins    *
*   Skills functions here                                                *
*                                                                        *
*  $Author$                                                       *
*  $Date$                                          *
*  $Revision$                                                     *
************************************************************************ */

#include "obj_prototypes.h"
#include "structs/global_objects.h"
#include "handler.h"
#include "color.h"
#include "utils/random.h"

const int kSkillCapOnZeroRemort = 80;
const int kSkillCapBonusPerRemort = 5;;

const int kNoviceSkillThreshold = 75;
const short kSkillDiceSize = 100;
const short kSkillCriticalFailure = 6;
const short kSkillCriticalSuccess = 95;
const short kSkillDegreeDivider = 5;
const int kMinSkillDegree = 0;
const int kMaxSkillDegree = 10;
const double kSkillWeight = 1.0;
const double kNoviceSkillWeight = 0.75;
const double kParameterWeight = 3.0;
const double kBonusWeight = 1.0;
const double kSaveWeight = 1.0;

const short kDummyKnight = 390;
const short kDummyShield = 391;
const short kDummyWeapon = 392;

const long kMinImprove = 0L;

enum class ELuckTestResult {
	kLuckTestFail = 0,
	kLuckTestSuccess,
	kLuckTestCriticalSuccess,
};

ELuckTestResult MakeLuckTest(CharData *ch, CharData *vict);
void SendSkillRollMsg(CharData *ch, CharData *victim, ESkill skill_id,
					  int actor_rate, int victim_rate, int threshold, int roll, SkillRollResult &result);

class WeapForAct {
 public:
	enum WeapType {
		EWT_UNDEFINED,
		EWT_PROTOTYPE_SHARED_PTR,
		EWT_OBJECT_RAW_PTR,    // Anton Gorev (09/28/2016): We need to get rid of raw pointers in the future
		EWT_STRING
	};

	class WeaponTypeException : public std::exception {
	 public:
		explicit WeaponTypeException(const char *what) : m_what(what) {}

		virtual const char *what() const throw() { return m_what.c_str(); }

	 private:
		std::string m_what;
	};

	WeapForAct() : m_type(EWT_UNDEFINED), m_prototype_raw_ptr(nullptr) {}
	WeapForAct(const WeapForAct &from);
	void set_damage_string(const int damage);
	WeapForAct &operator=(const CObjectPrototype::shared_ptr &prototype_shared_ptr);
	WeapForAct &operator=(ObjData *prototype_raw_ptr);

	auto type() const { return m_type; }
	const auto &get_prototype_shared_ptr() const;
	auto get_prototype_raw_ptr() const;
	const auto get_object_ptr() const;
	const auto &get_string() const;

 private:
	using kick_type_t = std::vector<const char *>;

	WeapForAct &operator=(const WeapForAct &);

	bool check_type(const WeapType type) const { return check_type(type, true); }
	bool check_type(const WeapType type, const bool raise_exception) const;

	WeapType m_type;
	ObjData::shared_ptr m_prototype_shared_ptr;
	ObjData *m_prototype_raw_ptr;
	std::string m_string;

	static const kick_type_t
		s_kick_type;    // Anton Gorev (09/28/2016): As I know, it is a duplicate. We need to reuse kick types from other place.
};

WeapForAct::WeapForAct(const WeapForAct &from) :
	m_type(from.m_type),
	m_prototype_shared_ptr(from.m_prototype_shared_ptr),
	m_prototype_raw_ptr(from.m_prototype_raw_ptr),
	m_string(from.m_string) {
}

void WeapForAct::set_damage_string(const int damage) {
	m_type = EWT_STRING;
	if (damage <= 5) {
		m_string = s_kick_type[0];
	} else if (damage <= 11) {
		m_string = s_kick_type[1];
	} else if (damage <= 26) {
		m_string = s_kick_type[2];
	} else if (damage <= 35) {
		m_string = s_kick_type[3];
	} else if (damage <= 45) {
		m_string = s_kick_type[4];
	} else if (damage <= 56) {
		m_string = s_kick_type[5];
	} else if (damage <= 96) {
		m_string = s_kick_type[6];
	} else if (damage <= 136) {
		m_string = s_kick_type[7];
	} else if (damage <= 176) {
		m_string = s_kick_type[8];
	} else if (damage <= 216) {
		m_string = s_kick_type[9];
	} else if (damage <= 256) {
		m_string = s_kick_type[10];
	} else if (damage <= 296) {
		m_string = s_kick_type[11];
	} else if (damage <= 400) {
		m_string = s_kick_type[12];
	} else if (damage <= 800) {
		m_string = s_kick_type[13];
	} else {
		m_string = s_kick_type[14];
	}
}

const auto &WeapForAct::get_prototype_shared_ptr() const {
	check_type(EWT_PROTOTYPE_SHARED_PTR);
	return m_prototype_shared_ptr;
}

auto WeapForAct::get_prototype_raw_ptr() const {
	check_type(EWT_OBJECT_RAW_PTR);
	return m_prototype_raw_ptr;
}

const auto WeapForAct::get_object_ptr() const {
	if (check_type(EWT_OBJECT_RAW_PTR, false)) {
		return m_prototype_raw_ptr;
	} else if (check_type(EWT_PROTOTYPE_SHARED_PTR, false)) {
		return m_prototype_shared_ptr.get();
	}

	std::stringstream ss;
	ss << "Requested object ptr but actual weapon type is [" << m_type << "]";
	throw WeaponTypeException(ss.str().c_str());
}

const auto &WeapForAct::get_string() const {
	check_type(EWT_STRING);
	return m_string;
}

bool WeapForAct::check_type(const WeapType type, const bool raise_exception) const {
	if (type != m_type) {
		if (raise_exception) {
			std::stringstream ss;
			ss << "Type of weapon [" << m_type << "] does not match to expected [" << type << "]";
			throw WeaponTypeException(ss.str().c_str());
		}

		return false;
	}

	return true;
}

WeapForAct &WeapForAct::operator=(ObjData *prototype_raw_ptr) {
	m_type = EWT_OBJECT_RAW_PTR;
	m_prototype_raw_ptr = prototype_raw_ptr;
	return *this;
}

WeapForAct &WeapForAct::operator=(const CObjectPrototype::shared_ptr &prototype_shared_ptr) {
	m_type = EWT_PROTOTYPE_SHARED_PTR;
	m_prototype_shared_ptr.reset();
	if (prototype_shared_ptr) {
		m_prototype_shared_ptr.reset(new ObjData(*prototype_shared_ptr));
	}
	return *this;
}

const WeapForAct::kick_type_t WeapForAct::s_kick_type =
// силы пинка. полностью соответствуют наносимым поврждениям обычного удара
	{
		"легонько ",        //  1..5
		"слегка ",        // 6..11
		"",            // 12..26
		"сильно ",        // 27..35
		"очень сильно ",    // 36..45
		"чрезвычайно сильно ",    // 46..55
		"БОЛЬНО ",        // 56..96
		"ОЧЕНЬ БОЛЬНО ",    // 97..136
		"ЧРЕЗВЫЧАЙНО БОЛЬНО ",    // 137..176
		"НЕВЫНОСИМО БОЛЬНО ",    // 177..216
		"ЖЕСТОКО ",    // 217..256
		"УЖАСНО ",// 257..296
		"УБИЙСТВЕННО ",     // 297..400
		"ИЗУВЕРСКИ ", // 400+
		"СМЕРТЕЛЬНО " // 800+
	};

struct brief_shields {
	brief_shields(CharData *ch_, CharData *vict_, const WeapForAct &weap_, std::string add_);

	void act_to_char(const char *msg);
	void act_to_vict(const char *msg);
	void act_to_room(const char *msg);

	void act_with_exception_handling(const char *msg, int type) const;

	CharData *ch;
	CharData *vict;
	WeapForAct weap;
	std::string add;
	// флаг отражаемого дамага, который надо глушить в режиме EPrf::BRIEF_SHIELDS
	bool reflect;

 private:
	void act_no_add(const char *msg, int type);
	void act_add(const char *msg, int type);
};

brief_shields::brief_shields(CharData *ch_, CharData *vict_, const WeapForAct &weap_, std::string add_)
	: ch(ch_), vict(vict_), weap(weap_), add(add_), reflect(false) {
}

void brief_shields::act_to_char(const char *msg) {
	if (!reflect
		|| (reflect
			&& !PRF_FLAGGED(ch, EPrf::kBriefShields))) {
		if (!add.empty()
			&& PRF_FLAGGED(ch, EPrf::kBriefShields)) {
			act_add(msg, kToChar);
		} else {
			act_no_add(msg, kToChar);
		}
	}
}

void brief_shields::act_to_vict(const char *msg) {
	if (!reflect
		|| (reflect
			&& !PRF_FLAGGED(vict, EPrf::kBriefShields))) {
		if (!add.empty()
			&& PRF_FLAGGED(vict, EPrf::kBriefShields)) {
			act_add(msg, kToVict | kToSleep);
		} else {
			act_no_add(msg, kToVict | kToSleep);
		}
	}
}

void brief_shields::act_to_room(const char *msg) {
	if (add.empty()) {
		if (reflect) {
			act_no_add(msg, kToNotVict | kToArenaListen | kToNoBriefShields);
		} else {
			act_no_add(msg, kToNotVict | kToArenaListen);
		}
	} else {
		act_no_add(msg, kToNotVict | kToArenaListen | kToNoBriefShields);
		if (!reflect) {
			act_add(msg, kToNotVict | kToArenaListen | kToBriefShields);
		}
	}
}

void brief_shields::act_with_exception_handling(const char *msg, const int type) const {
	try {
		const auto weapon_type = weap.type();
		switch (weapon_type) {
			case WeapForAct::EWT_STRING: act(msg, false, ch, nullptr, vict, type, weap.get_string());
				break;

			case WeapForAct::EWT_OBJECT_RAW_PTR:
			case WeapForAct::EWT_PROTOTYPE_SHARED_PTR: act(msg, false, ch, weap.get_object_ptr(), vict, type);
				break;

			default: act(msg, false, ch, nullptr, vict, type);
		}
	}
	catch (const WeapForAct::WeaponTypeException &e) {
		mudlog(e.what(), BRF, kLvlBuilder, ERRLOG, true);
	}
}

void brief_shields::act_no_add(const char *msg, int type) {
	act_with_exception_handling(msg, type);
}

void brief_shields::act_add(const char *msg, int type) {
	char buf_[kMaxInputLength];
	snprintf(buf_, sizeof(buf_), "%s%s", msg, add.c_str());
	act_with_exception_handling(buf_, type);
}

const WeapForAct init_weap(CharData *ch, int dam, int attacktype) {
	// Нижеследующий код повергает в ужас
	// ААА, ШОЭТО*ЛЯ?! Что ЭТО вообще делает?
	WeapForAct weap;
	int weap_i = 0;

	switch (attacktype) {
		case to_underlying(ESkill::kBackstab) + kTypeHit: weap = GET_EQ(ch, EEquipPos::kWield);
			if (!weap.get_prototype_raw_ptr()) {
				weap_i = real_object(kDummyKnight);
				if (0 <= weap_i) {
					weap = obj_proto[weap_i];
				}
			}
			break;

		case to_underlying(ESkill::kThrow) + kTypeHit: weap = GET_EQ(ch, EEquipPos::kWield);
			if (!weap.get_prototype_raw_ptr()) {
				weap_i = real_object(kDummyKnight);
				if (0 <= weap_i) {
					weap = obj_proto[weap_i];
				}
			}
			break;

		case to_underlying(ESkill::kBash) + kTypeHit: weap = GET_EQ(ch, EEquipPos::kShield);
			if (!weap.get_prototype_raw_ptr()) {
				weap_i = real_object(kDummyShield);
				if (0 <= weap_i) {
					weap = obj_proto[weap_i];
				}
			}
			break;

		case to_underlying(ESkill::kKick) + kTypeHit:
			// weap - текст силы удара
			weap.set_damage_string(dam);
			break;

		case kTypeHit: break;

		default: weap_i = real_object(kDummyWeapon);
			if (0 <= weap_i) {
				weap = obj_proto[weap_i];
			}
	}

	return weap;
}

typedef std::map<ESkill, std::string> ESkill_name_by_value_t;
typedef std::map<const std::string, ESkill> ESkill_value_by_name_t;
ESkill_name_by_value_t ESkill_name_by_value;
ESkill_value_by_name_t ESkill_value_by_name;

void init_ESkill_ITEM_NAMES() {
	ESkill_name_by_value.clear();
	ESkill_value_by_name.clear();

	ESkill_name_by_value[ESkill::kUndefined] = "kUndefined";
	ESkill_name_by_value[ESkill::kIncorrect] = "kIncorrect";
	ESkill_name_by_value[ESkill::kAny] = "kAny";
	ESkill_name_by_value[ESkill::kGlobalCooldown] = "kGlobalCooldown";
	ESkill_name_by_value[ESkill::kProtect] = "kProtect";
	ESkill_name_by_value[ESkill::kIntercept] = "kIntercept";
	ESkill_name_by_value[ESkill::kLeftHit] = "kLeftHit";
	ESkill_name_by_value[ESkill::kHammer] = "kHammer";
	ESkill_name_by_value[ESkill::kOverwhelm] = "kOverwhelm";
	ESkill_name_by_value[ESkill::kPoisoning] = "kPoisoning";
	ESkill_name_by_value[ESkill::kSense] = "kSense";
	ESkill_name_by_value[ESkill::kRiding] = "kRiding";
	ESkill_name_by_value[ESkill::kHideTrack] = "kHideTrack";
	ESkill_name_by_value[ESkill::kReligion] = "kReligion";
	ESkill_name_by_value[ESkill::kSkinning] = "kSkinning";
	ESkill_name_by_value[ESkill::kMultiparry] = "kMultiparry";
	ESkill_name_by_value[ESkill::kCutting] = "kCutting";
	ESkill_name_by_value[ESkill::kReforging] = "kReforging";
	ESkill_name_by_value[ESkill::kLeadership] = "kLeadership";
	ESkill_name_by_value[ESkill::kPunctual] = "kPunctual";
	ESkill_name_by_value[ESkill::kAwake] = "kAwake";
	ESkill_name_by_value[ESkill::kIdentify] = "kIdentify";
	ESkill_name_by_value[ESkill::kHearing] = "kHearing";
	ESkill_name_by_value[ESkill::kCreatePotion] = "kCreatePotion";
	ESkill_name_by_value[ESkill::kCreateScroll] = "kCreateScroll";
	ESkill_name_by_value[ESkill::kCreateWand] = "kCreateWand";
	ESkill_name_by_value[ESkill::kPry] = "kPry";
	ESkill_name_by_value[ESkill::kArmoring] = "kArmoring";
	ESkill_name_by_value[ESkill::kHangovering] = "kHangovering";
	ESkill_name_by_value[ESkill::kFirstAid] = "kFirstAid";
	ESkill_name_by_value[ESkill::kCampfire] = "kCampfire";
	ESkill_name_by_value[ESkill::kCreateBow] = "kCreateBow";
	ESkill_name_by_value[ESkill::kThrow] = "kThrow";
	ESkill_name_by_value[ESkill::kBackstab] = "kBackstab";
	ESkill_name_by_value[ESkill::kBash] = "kBash";
	ESkill_name_by_value[ESkill::kHide] = "kHide";
	ESkill_name_by_value[ESkill::kKick] = "kKick";
	ESkill_name_by_value[ESkill::kPickLock] = "kPickLock";
	ESkill_name_by_value[ESkill::kPunch] = "kPunch";
	ESkill_name_by_value[ESkill::kRescue] = "kRescue";
	ESkill_name_by_value[ESkill::kSneak] = "kSneak";
	ESkill_name_by_value[ESkill::kSteal] = "kSteal";
	ESkill_name_by_value[ESkill::kTrack] = "kTrack";
	ESkill_name_by_value[ESkill::kClubs] = "kClubs";
	ESkill_name_by_value[ESkill::kAxes] = "kAxes";
	ESkill_name_by_value[ESkill::kLongBlades] = "kLongBlades";
	ESkill_name_by_value[ESkill::kShortBlades] = "kShortBlades";
	ESkill_name_by_value[ESkill::kNonstandart] = "kNonstandart";
	ESkill_name_by_value[ESkill::kTwohands] = "kTwohands";
	ESkill_name_by_value[ESkill::kPicks] = "kPicks";
	ESkill_name_by_value[ESkill::kSpades] = "kSpades";
	ESkill_name_by_value[ESkill::kSideAttack] = "kSideAttack";
	ESkill_name_by_value[ESkill::kDisarm] = "kDisarm";
	ESkill_name_by_value[ESkill::kParry] = "kParry";
	ESkill_name_by_value[ESkill::kMorph] = "kMorph";
	ESkill_name_by_value[ESkill::kBows] = "kBows";
	ESkill_name_by_value[ESkill::kAddshot] = "kAddshot";
	ESkill_name_by_value[ESkill::kDisguise] = "kDisguise";
	ESkill_name_by_value[ESkill::kDodge] = "kDodge";
	ESkill_name_by_value[ESkill::kShieldBlock] = "kShieldBlock";
	ESkill_name_by_value[ESkill::kLooking] = "kLooking";
	ESkill_name_by_value[ESkill::kUndercut] = "kUndercut";
	ESkill_name_by_value[ESkill::kRepair] = "kRepair";
	ESkill_name_by_value[ESkill::kSharpening] = "kSharpening";
	ESkill_name_by_value[ESkill::kCourage] = "kCourage";
	ESkill_name_by_value[ESkill::kJinx] = "kJinx";
	ESkill_name_by_value[ESkill::kNoParryHit] = "kNoParryHit";
	ESkill_name_by_value[ESkill::kTownportal] = "kTownportal";
	ESkill_name_by_value[ESkill::kMakeStaff] = "kMakeStaff";
	ESkill_name_by_value[ESkill::kMakeBow] = "kMakeBow";
	ESkill_name_by_value[ESkill::kMakeWeapon] = "kMakeWeapon";
	ESkill_name_by_value[ESkill::kMakeArmor] = "kMakeArmor";
	ESkill_name_by_value[ESkill::kMakeJewel] = "kMakeJewel";
	ESkill_name_by_value[ESkill::kMakeWear] = "kMakeWear";
	ESkill_name_by_value[ESkill::kMakePotion] = "kMakePotion";
	ESkill_name_by_value[ESkill::kDigging] = "kDigging";
	ESkill_name_by_value[ESkill::kJewelry] = "kJewelry";
	ESkill_name_by_value[ESkill::kWarcry] = "kWarcry";
	ESkill_name_by_value[ESkill::kTurnUndead] = "kTurnUndead";
	ESkill_name_by_value[ESkill::kIronwind] = "kIronwind";
	ESkill_name_by_value[ESkill::kStrangle] = "kStrangle";
	ESkill_name_by_value[ESkill::kAirMagic] = "kAirMagic";
	ESkill_name_by_value[ESkill::kFireMagic] = "kFireMagic";
	ESkill_name_by_value[ESkill::kWaterMagic] = "kWaterMagic";
	ESkill_name_by_value[ESkill::kEarthMagic] = "kEarthMagic";
	ESkill_name_by_value[ESkill::kLightMagic] = "kLightMagic";
	ESkill_name_by_value[ESkill::kDarkMagic] = "kDarkMagic";
	ESkill_name_by_value[ESkill::kMindMagic] = "kMindMagic";
	ESkill_name_by_value[ESkill::kLifeMagic] = "kLifeMagic";
	ESkill_name_by_value[ESkill::kMakeAmulet] = "kMakeAmulet";
	ESkill_name_by_value[ESkill::kStun] = "kStun";

	for (const auto &i: ESkill_name_by_value) {
		ESkill_value_by_name[i.second] = i.first;
	}
}

template<>
ESkill ITEM_BY_NAME(const std::string &name) {
	if (ESkill_name_by_value.empty()) {
		init_ESkill_ITEM_NAMES();
	}
	return ESkill_value_by_name.at(name);
}

template<>
const std::string &NAME_BY_ITEM<ESkill>(const ESkill item) {
	if (ESkill_name_by_value.empty()) {
		init_ESkill_ITEM_NAMES();
	}
	return ESkill_name_by_value.at(item);
}

///
/// \param add = "", строка для добавления после основного сообщения (краткий режим щитов)
///
int SendSkillMessages(int dam, CharData *ch, CharData *vict, int attacktype, std::string add) {
	int i, j, nr;
	struct AttackMsgSet *msg;

	//log("[SKILL MESSAGE] Message for skill %d",attacktype);
	for (i = 0; i < kMaxMessages; i++) {
		if (fight_messages[i].attack_type == attacktype) {
			nr = RollDices(1, fight_messages[i].number_of_attacks);
			// log("[SKILL MESSAGE] %d(%d)",fight_messages[i].number_of_attacks,nr);
			for (j = 1, msg = fight_messages[i].msg_set; (j < nr) && msg; j++) {
				msg = msg->next;
			}

			const auto weap = init_weap(ch, dam, attacktype);
			brief_shields brief(ch, vict, weap, add);
			if (attacktype == kSpellFireShield
				|| attacktype == kSpellMagicGlass) {
				brief.reflect = true;
			}

			if (!vict->IsNpc() && (GetRealLevel(vict) >= kLvlImmortal) && !PLR_FLAGGED((ch), EPlrFlag::kWriting)) {
				switch (attacktype) {
					case to_underlying(ESkill::kBackstab) + kTypeHit:
					case to_underlying(ESkill::kThrow) + kTypeHit:
					case to_underlying(ESkill::kBash) + kTypeHit:
					case to_underlying(ESkill::kKick) + kTypeHit: SendMsgToChar("&W&q", ch);
						break;

					default: SendMsgToChar("&y&q", ch);
						break;
				}
				brief.act_to_char(msg->god_msg.attacker_msg);
				SendMsgToChar("&Q&n", ch);
				brief.act_to_vict(msg->god_msg.victim_msg);
				brief.act_to_room(msg->god_msg.room_msg);
			} else if (dam != 0) {
				if (GET_POS(vict) == EPosition::kDead) {
					SendMsgToChar("&Y&q", ch);
					brief.act_to_char(msg->die_msg.attacker_msg);
					SendMsgToChar("&Q&n", ch);
					SendMsgToChar("&R&q", vict);
					brief.act_to_vict(msg->die_msg.victim_msg);
					SendMsgToChar("&Q&n", vict);
					brief.act_to_room(msg->die_msg.room_msg);
				} else {
					SendMsgToChar("&Y&q", ch);
					brief.act_to_char(msg->hit_msg.attacker_msg);
					SendMsgToChar("&Q&n", ch);
					SendMsgToChar("&R&q", vict);
					brief.act_to_vict(msg->hit_msg.victim_msg);
					SendMsgToChar("&Q&n", vict);
					brief.act_to_room(msg->hit_msg.room_msg);
				}
			} else if (ch != vict)    // Dam == 0
			{
				switch (attacktype) {
					case to_underlying(ESkill::kBackstab) + kTypeHit:
					case to_underlying(ESkill::kThrow) + kTypeHit:
					case to_underlying(ESkill::kBash) + kTypeHit:
					case to_underlying(ESkill::kKick) + kTypeHit: SendMsgToChar("&W&q", ch);
						break;

					default: SendMsgToChar("&y&q", ch);
						break;
				}
				brief.act_to_char(msg->miss_msg.attacker_msg);
				SendMsgToChar("&Q&n", ch);
				SendMsgToChar("&r&q", vict);
				brief.act_to_vict(msg->miss_msg.victim_msg);
				SendMsgToChar("&Q&n", vict);
				brief.act_to_room(msg->miss_msg.room_msg);
			}
			return (1);
		}
	}
	return (0);
}

int CalculateVictimRate(CharData *ch, const ESkill skill_id, CharData *vict) {
	if (!vict) {
		return 0;
	}

	int rate = 0;

	switch (skill_id) {

		case ESkill::kBackstab: {
			if ((GET_POS(vict) >= EPosition::kFight) && AFF_FLAGGED(vict, EAffect::kAwarness)) {
				rate += 30;
			}
			rate += GET_REAL_DEX(vict);
			//rate -= size_app[GET_POS_SIZE(vict)].ac;
			break;
		}

		case ESkill::kBash: {
			if (GET_POS(vict) < EPosition::kFight && GET_POS(vict) >= EPosition::kSleep) {
				rate -= 20;
			}
			if (PRF_FLAGGED(vict, EPrf::kAwake)) {
				rate -= CalculateSkillAwakeModifier(ch, vict);
			}
			break;
		}

		case ESkill::kHide: {
			if (AWAKE(vict)) {
				rate -= int_app[GET_REAL_INT(vict)].observation;
			}
			break;
		}

		case ESkill::kKick: {
			//rate += size_app[GET_POS_SIZE(vict)].interpolate;
			rate += GET_REAL_CON(vict);
			if (PRF_FLAGGED(vict, EPrf::kAwake)) {
				rate += CalculateSkillAwakeModifier(ch, vict);
			}
			break;
		}

		case ESkill::kSneak: {
			if (AWAKE(vict)) {
				rate -= int_app[GET_REAL_INT(vict)].observation;
			}
			break;
		}

		case ESkill::kSteal: {
			if (AWAKE(vict)) {
				rate -= int_app[GET_REAL_INT(vict)].observation;
			}
			break;
		}

		case ESkill::kTrack: {
			rate += GET_REAL_CON(vict) / 2;
			break;
		}

		case ESkill::kMultiparry:
		case ESkill::kParry: {
			rate = 100;
			break;
		}

		case ESkill::kIntercept: {
			rate -= dex_bonus(GET_REAL_DEX(vict));
			rate -= size_app[GET_POS_SIZE(vict)].interpolate;
			break;
		}

		case ESkill::kProtect: {
			rate = 50;
			break;
		}

		case ESkill::kDisarm: {
			rate -= dex_bonus(GET_REAL_STR(ch));
			if (GET_EQ(vict, EEquipPos::kBoths))
				rate -= 10;
			if (PRF_FLAGGED(vict, EPrf::kAwake))
				rate -= CalculateSkillAwakeModifier(ch, vict);
			break;
		}

		case ESkill::kDisguise: {
			if (AWAKE(vict))
				rate -= int_app[GET_REAL_INT(vict)].observation;
			break;
		}

		case ESkill::kDodge: {
			rate -= dex_bonus(GET_REAL_DEX(vict));
			break;
		}

		case ESkill::kUndercut: {
			if (AWAKE(vict) || AFF_FLAGGED(vict, EAffect::kAwarness) || MOB_FLAGGED(vict, EMobFlag::kMobAwake))
				rate -= 20;
			if (PRF_FLAGGED(vict, EPrf::kAwake))
				rate -= CalculateSkillAwakeModifier(ch, vict);
			break;
		}

		case ESkill::kHammer: {
			if (vict->IsNpc())
				rate -= size_app[GET_POS_SIZE(vict)].shocking / 2;
			else
				rate -= size_app[GET_POS_SIZE(vict)].shocking;
			break;
		}

		case ESkill::kOverwhelm: {
			rate -= GET_REAL_CON(vict);
			break;
		}

		case ESkill::kPunctual: {
			rate -= int_app[GET_REAL_INT(vict)].observation;
			break;
		}

		case ESkill::kAwake: {
			rate -= int_app[GET_REAL_INT(vict)].observation;
		}
			break;

		case ESkill::kPry: {
			if (CAN_SEE(vict, ch) && AWAKE(vict)) {
				rate -= int_app[GET_REAL_INT(ch)].observation;
			}
			break;
		}

		case ESkill::kStrangle: {
			if (CAN_SEE(ch, vict) && PRF_FLAGGED(vict, EPrf::kAwake)) {
				rate -= CalculateSkillAwakeModifier(ch, vict);
			}
			break;
		}

		case ESkill::kStun: {
			if (PRF_FLAGGED(vict, EPrf::kAwake))
				rate -= CalculateSkillAwakeModifier(ch, vict);
			break;
		}
		default: {
			rate = 0;
			break;
		}
	}

	if (!MUD::Skills()[skill_id].autosuccess) {
		rate -= static_cast<int>(round(GET_SAVE(vict, MUD::Skills()[skill_id].save_type) * kSaveWeight));
	}

	return rate;
}

// \TODO По-хорошему, нужна отдельно функция, которая считает голый скилл с бонусами от перков
// И отдельно - бонус от статов и всего прочего. Однако не вижу сейчас смысла этим заниматься,
// потому что по-хорошему половина этих параметров должна находиться в описании соответствующей абилки
// которого не имеется и которое сейчас вводить не время.
int CalculateSkillRate(CharData *ch, const ESkill skill_id, CharData *vict) {
	int base_percent = ch->get_skill(skill_id);
	int parameter_bonus = 0; // бонус от ключевого параметра
	int bonus = 0; // бонус от дополнительных параметров.
	int size = 0; // бонусы/штрафы размера (не спрашивайте...)

	if (base_percent <= 0) {
		return 0;
	}

	if (!ch->IsNpc() && !ch->affected.empty()) {
		for (const auto &aff: ch->affected) {
			if (aff->location == EApply::kPlague) {
				base_percent -= number(ch->get_skill(skill_id) * 0.4, ch->get_skill(skill_id) * 0.05);
			}
		}
	}

	if (!ch->IsNpc()) {
		size = (MAX(0, GET_REAL_SIZE(ch) - 50));
	} else {
		size = size_app[GET_POS_SIZE(ch)].interpolate / 2;
	}

	switch (skill_id) {

		case ESkill::kHideTrack: {
			parameter_bonus += GET_REAL_INT(ch);
			bonus += IsAbleToUseFeat(ch, EFeat::kStealthy) ? 5 : 0;
			break;
		}

		case ESkill::kBackstab: {
			parameter_bonus += GET_REAL_DEX(ch);
			if (IsAwakeOthers(ch) || IsEquipInMetall(ch)) {
				bonus += -50;
			}
			if (vict) {
				if (!CAN_SEE(vict, ch)) {
					bonus += 20;
				}
				if (GET_POS(vict) < EPosition::kFight) {
					bonus += (20 * (EPosition::kFight - GET_POS(vict)));
				}
			}
			break;
		}

		case ESkill::kBash: {
			parameter_bonus += dex_bonus(GET_REAL_DEX(ch));
			bonus = size + (GET_EQ(ch, EEquipPos::kShield) ?
							weapon_app[MIN(35, MAX(0, GET_OBJ_WEIGHT(GET_EQ(ch, EEquipPos::kShield))))].bashing : 0);
			if (PRF_FLAGGED(ch, EPrf::kAwake)) {
				bonus = -50;
			}
			break;
		}

		case ESkill::kHide: {
			parameter_bonus += dex_bonus(GET_REAL_DEX(ch));
			bonus = size_app[GET_POS_SIZE(ch)].ac + (IsAbleToUseFeat(ch, EFeat::kStealthy) ? 5 : 0);
			if (IsAwakeOthers(ch) || IsEquipInMetall(ch)) {
				bonus -= 50;
			}
			if (is_dark(ch->in_room)) {
				bonus += 25;
			}
			if (SECT(ch->in_room) == ESector::kInside) {
				bonus += 20;
			} else if (SECT(ch->in_room) == ESector::kCity) {
				bonus -= 15;
			} else if (SECT(ch->in_room) == ESector::kForest) {
				bonus += 20;
			} else if (SECT(ch->in_room) == ESector::kHills
				|| SECT(ch->in_room) == ESector::kMountain) {
				bonus += 10;
			}
			break;
		}

		case ESkill::kKick: {
			if (!ch->IsOnHorse() && vict->IsOnHorse()) {
				base_percent = 0;
			} else {
				parameter_bonus += GET_REAL_STR(ch);
				if (AFF_FLAGGED(vict, EAffect::kHold)) {
					bonus += 50;
				}
			}
			break;
		}

		case ESkill::kPickLock: {
			parameter_bonus = dex_bonus(GET_REAL_DEX(ch));
			bonus += IsAbleToUseFeat(ch, EFeat::kNimbleFingers) ? 5 : 0;
			break;
		}

		case ESkill::kRescue: {
			parameter_bonus = dex_bonus(GET_REAL_DEX(ch));
			break;
		}

		case ESkill::kSneak: {
			parameter_bonus = dex_bonus(GET_REAL_DEX(ch));
			bonus += IsAbleToUseFeat(ch, EFeat::kStealthy) ? 10 : 0;
			if (IsAwakeOthers(ch) || IsEquipInMetall(ch))
				bonus -= 50;
			if (SECT(ch->in_room) == ESector::kCity)
				bonus -= 10;
			if (is_dark(ch->in_room)) {
				bonus += 20;
			}
			if (vict) {
				if (!CAN_SEE(vict, ch))
					bonus += 25;
			}
			break;
		}

		case ESkill::kSteal: {
			parameter_bonus = dex_bonus(GET_REAL_DEX(ch));
			bonus += (IsAbleToUseFeat(ch, EFeat::kNimbleFingers) ? 5 : 0);
			if (IsAwakeOthers(ch) || IsEquipInMetall(ch))
				bonus -= 50;
			if (is_dark(ch->in_room))
				bonus += 20;
			if (vict) {
				if (!CAN_SEE(vict, ch))
					bonus += 25;
				if (AWAKE(vict)) {
					if (AFF_FLAGGED(vict, EAffect::kAwarness))
						bonus -= 30;
				}
			}
			break;
		}

		case ESkill::kTrack: {
			parameter_bonus = int_app[GET_REAL_INT(ch)].observation;
			bonus += IsAbleToUseFeat(ch, EFeat::kTracker) ? 10 : 0;
			if (SECT(ch->in_room) == ESector::kForest
				|| SECT(ch->in_room) == ESector::kField) {
				bonus += 10;
			}
			if (SECT(ch->in_room) == ESector::kUnderwater) {
				bonus -= 30;
			}
			bonus = GetComplexSkillModifier(ch, ESkill::kUndefined, GAPPLY_SKILL_SUCCESS, bonus);
			if (SECT(ch->in_room) == ESector::kWaterSwim
				|| SECT(ch->in_room) == ESector::kWaterNoswim
				|| SECT(ch->in_room) == ESector::kOnlyFlying
				|| SECT(ch->in_room) == ESector::kUnderwater
				|| SECT(ch->in_room) == ESector::kSecret
				|| ROOM_FLAGGED(ch->in_room, ERoomFlag::kNoTrack)) {
				parameter_bonus = 0;
				bonus = -100;
			}

			break;
		}

		case ESkill::kSense: {
			parameter_bonus += int_app[GET_REAL_INT(ch)].observation;
			bonus += IsAbleToUseFeat(ch, EFeat::kTracker) ? 20 : 0;
			break;
		}

		case ESkill::kMultiparry:
		case ESkill::kParry: {
			parameter_bonus += dex_bonus(GET_REAL_DEX(ch));
			if (GET_AF_BATTLE(ch, kEafAwake)) {
				bonus += ch->get_skill(ESkill::kAwake);
			}
			if (GET_EQ(ch, EEquipPos::kHold)
				&& GET_OBJ_TYPE(GET_EQ(ch, EEquipPos::kHold)) == EObjType::kWeapon) {
				bonus += weapon_app[MAX(0, MIN(50, GET_OBJ_WEIGHT(GET_EQ(ch, EEquipPos::kHold))))].parrying;
			}
			break;
		}

		case ESkill::kShieldBlock: {
			parameter_bonus += dex_bonus(GET_REAL_DEX(ch));
			bonus += GET_EQ(ch, EEquipPos::kShield) ?
					 MIN(10, MAX(0, GET_OBJ_WEIGHT(GET_EQ(ch, EEquipPos::kShield)) - 20)) : 0;
			break;
		}

		case ESkill::kIntercept: {
			parameter_bonus += dex_bonus(GET_REAL_DEX(ch));
			if (vict) {
				bonus += size_app[GET_POS_SIZE(vict)].interpolate;
			}
			break;
		}

		case ESkill::kProtect: {
			parameter_bonus = dex_bonus(GET_REAL_DEX(ch)) + size;
			break;
		}

		case ESkill::kBows: {
			parameter_bonus = dex_bonus(GET_REAL_DEX(ch));
			break;
		}

		case ESkill::kLooking:[[fallthrough]];
		case ESkill::kHearing: {
			parameter_bonus += int_app[GET_REAL_INT(ch)].observation;
			break;
		}

		case ESkill::kDisarm: {
			parameter_bonus += dex_bonus(GET_REAL_DEX(ch)) + dex_bonus(GET_REAL_STR(ch));
			break;
		}

		case ESkill::kAddshot: {
			if (IsEquipInMetall(ch)) {
				bonus -= 20;
			}
			break;
		}

		case ESkill::kNoParryHit: {
			parameter_bonus = dex_bonus(GET_REAL_DEX(ch));
			break;
		}

		case ESkill::kDisguise: {
			parameter_bonus = dex_bonus(GET_REAL_DEX(ch)) - size_app[GET_POS_SIZE(ch)].ac;
			bonus += (IsAbleToUseFeat(ch, EFeat::kStealthy) ? 5 : 0);
			if (IsAwakeOthers(ch))
				bonus -= 100;
			if (is_dark(ch->in_room))
				bonus += 15;
			if (SECT(ch->in_room) == ESector::kCity)
				bonus -= 15;
			else if (SECT(ch->in_room) == ESector::kForest)
				bonus += 10;
			else if (SECT(ch->in_room) == ESector::kHills
				|| SECT(ch->in_room) == ESector::kMountain)
				bonus += 5;
			if (IsEquipInMetall(ch))
				bonus -= 30;

			break;
		}

		case ESkill::kDodge: {
			parameter_bonus = -size_app[GET_POS_SIZE(ch)].ac + dex_bonus(GET_REAL_DEX(ch));
			if (IsEquipInMetall(ch))
				bonus -= 40;
			break;
		}

		case ESkill::kUndercut: {
			parameter_bonus = dex_bonus(GET_REAL_DEX(ch));
			if (IsEquipInMetall(ch)) {
				bonus -= 10;
			}
			if (vict) {
				if (!CAN_SEE(vict, ch))
					bonus += 10;
				if (GET_POS(vict) < EPosition::kSit)
					bonus -= 50;
			}
			break;
		}

		case ESkill::kHammer: {
			bonus = size + dex_bonus(GET_REAL_STR(ch));
			break;
		}

		case ESkill::kOverwhelm: {
			bonus = dex_bonus(GET_REAL_STR(ch));
			if (GET_EQ(ch, EEquipPos::kWield)) {
				bonus += weapon_app[GET_OBJ_WEIGHT(GET_EQ(ch, EEquipPos::kWield))].shocking;
			} else {
				if (GET_EQ(ch, EEquipPos::kBoths)) {
					bonus += weapon_app[GET_OBJ_WEIGHT(GET_EQ(ch, EEquipPos::kBoths))].shocking;
				}
			}
			break;
		}

		case ESkill::kPoisoning: break;
		case ESkill::kLeadership: {
			parameter_bonus += cha_app[GET_REAL_CHA(ch)].leadership;
			bonus += IsAbleToUseFeat(ch, EFeat::kMagneticPersonality) ? 5 : 0;
			break;
		}

		case ESkill::kPunctual: {
			parameter_bonus = dex_bonus(GET_REAL_INT(ch));
			if (GET_EQ(ch, EEquipPos::kWield))
				bonus += MAX(18, GET_OBJ_WEIGHT(GET_EQ(ch, EEquipPos::kWield))) - 18
					+ MAX(25, GET_OBJ_WEIGHT(GET_EQ(ch, EEquipPos::kWield))) - 25
					+ MAX(30, GET_OBJ_WEIGHT(GET_EQ(ch, EEquipPos::kWield))) - 30;
			if (GET_EQ(ch, EEquipPos::kHold))
				bonus += MAX(18, GET_OBJ_WEIGHT(GET_EQ(ch, EEquipPos::kHold))) - 18
					+ MAX(25, GET_OBJ_WEIGHT(GET_EQ(ch, EEquipPos::kHold))) - 25
					+ MAX(30, GET_OBJ_WEIGHT(GET_EQ(ch, EEquipPos::kHold))) - 30;
			if (GET_EQ(ch, EEquipPos::kBoths))
				bonus += MAX(25, GET_OBJ_WEIGHT(GET_EQ(ch, EEquipPos::kBoths))) - 25
					+ MAX(30, GET_OBJ_WEIGHT(GET_EQ(ch, EEquipPos::kBoths))) - 30;
			break;
		}

		case ESkill::kAwake: {
			parameter_bonus = dex_bonus(GET_REAL_DEX(ch));
			bonus += int_app[GET_REAL_INT(ch)].observation;
			/* if (real_dex < INT_APP_SIZE) {
				bonus = int_app[real_dex].observation;
			} else {
				log("SYSERR: Global buffer overflow for ESkill::kAwake. Requested real_dex is %zd, but maximal allowed is %zd.",
					real_dex, INT_APP_SIZE);
			} */
		}
			break;

		case ESkill::kIdentify: {
			parameter_bonus = int_app[GET_REAL_INT(ch)].observation;
			bonus += (IsAbleToUseFeat(ch, EFeat::kConnoiseur) ? 20 : 0);
			break;
		}

		case ESkill::kPry: {
			parameter_bonus = cha_app[GET_REAL_CHA(ch)].illusive;
			if (vict) {
				if (!CAN_SEE(vict, ch)) {
					bonus += 50;
				}
			}
			break;
		}

		case ESkill::kHangovering: {
			parameter_bonus = GET_REAL_CON(ch) / 2;
			bonus += (IsAbleToUseFeat(ch, EFeat::kDrunkard) ? 20 : 0);
			break;
		}

		case ESkill::kFirstAid: {
			bonus = (IsAbleToUseFeat(ch, EFeat::kHealer) ? 10 : 0);
			break;
		}

		case ESkill::kCampfire: {
			if (get_room_sky(ch->in_room) == kSkyRaining)
				bonus -= 50;
			else if (get_room_sky(ch->in_room) != kSkyLightning)
				bonus -= number(10, 25);
			break;
		}

		case ESkill::kRiding: {
			bonus = cha_app[GET_REAL_CHA(ch)].leadership;
			break;
		}

		case ESkill::kStrangle: {
			bonus = dex_bonus(GET_REAL_DEX(ch));
			if (AFF_FLAGGED(vict, EAffect::kHold)) {
				bonus += 30;
			} else {
				if (!CAN_SEE(ch, vict))
					bonus += 20;
			}
			break;
		}

		case ESkill::kStun: {
			parameter_bonus = dex_bonus(GET_REAL_STR(ch));
			if (GET_EQ(ch, EEquipPos::kWield))
				bonus += weapon_app[GET_OBJ_WEIGHT(GET_EQ(ch, EEquipPos::kWield))].shocking;
			else if (GET_EQ(ch, EEquipPos::kBoths))
				bonus += weapon_app[GET_OBJ_WEIGHT(GET_EQ(ch, EEquipPos::kBoths))].shocking;
			break;
		}
		default: break;
	}

	if (AFF_FLAGGED(ch, EAffect::kNoobRegen)) {
		bonus += 5;
	}

	double rate = 0;
	if (MakeLuckTest(ch, vict) != ELuckTestResult::kLuckTestFail) {
		rate = round(std::max(0, base_percent - kNoviceSkillThreshold) * kSkillWeight
						 + std::min(kNoviceSkillThreshold, base_percent) * kNoviceSkillWeight
						 + bonus * kBonusWeight
						 + parameter_bonus * kParameterWeight);
	}

	return static_cast<int>(rate);
}

ELuckTestResult MakeLuckTest(CharData *ch, CharData *vict) {
	int luck = ch->calc_morale();

	if (AFF_FLAGGED(ch, EAffect::kDeafness)) {
		luck -= 20;
	}
	if (vict && IsAbleToUseFeat(vict, EFeat::kWarriorSpirit)) {
		luck -= 10;
	}

	const int prob = number(0, 999);
	if (luck < 0) {
		luck = luck * 10;
	}
	const int bonus_limit = MIN(150, luck * 10);
	int fail_limit = MIN(990, 950 + luck * 10 / 6);
	if (luck >= 50) {
		fail_limit = 999;
	}
	if (prob >= fail_limit) {   // Абсолютный фейл 4.9 процента
		return ELuckTestResult::kLuckTestFail;
	} else if (prob < bonus_limit) {
		return ELuckTestResult::kLuckTestCriticalSuccess;
	}
	return ELuckTestResult::kLuckTestSuccess;
}

SkillRollResult MakeSkillTest(CharData *ch, ESkill skill_id, CharData *vict) {
	int actor_rate = CalculateSkillRate(ch, skill_id, vict);
	int victim_rate = CalculateVictimRate(ch, skill_id, vict);

	int success_threshold = std::max(0, actor_rate - victim_rate - MUD::Skills()[skill_id].difficulty);
	int dice_roll = number(1, kSkillDiceSize);

	// \TODO Механика критфейла и критуспеха пока не реализована.
	SkillRollResult result;
	result.success = dice_roll <= success_threshold;
	result.critical = (dice_roll > kSkillCriticalSuccess) || (dice_roll < kSkillCriticalFailure);
	result.degree = std::abs(dice_roll - success_threshold) / kSkillDegreeDivider;
	result.degree = std::clamp(result.degree, kMinSkillDegree, kMaxSkillDegree);

	SendSkillRollMsg(ch, vict, skill_id, actor_rate, victim_rate, success_threshold, dice_roll, result);
	return result;
}

void SendSkillRollMsg(CharData *ch, CharData *victim, ESkill skill_id,
	int actor_rate, int victim_rate, int threshold, int roll, SkillRollResult &result) {
	std::stringstream buffer;
	buffer << KICYN
		   << "Skill: '" << MUD::Skills()[skill_id].name << "'"
		   << " Rate: " << actor_rate
		   << " Victim: " << victim->get_name()
		   << " V.Rate: " << victim_rate
		   << " Difficulty: " << MUD::Skills()[skill_id].difficulty
		   << " Threshold: " << threshold
		   << " Roll: " << roll
		   << " Success: " << (result.success ? "&Gyes&C" : "&Rno&C")
		   << " Crit: " << (result.critical ? "yes" : "no")
		   << " Degree: " << result.degree
		   << KNRM << std::endl;
	ch->send_to_TC(false, true, true, buffer.str().c_str());
}

// \TODO Не забыть убрать после ребаланса умений
void SendSkillBalanceMsg(CharData *ch, const std::string &skill_name, int percent, int prob, bool success) {
	std::stringstream buffer;
	buffer << KICYN
		   << "Skill: " << skill_name
		   << " Percent: " << percent
		   << " Prob: " << prob
		   << " Success: " << (success ? "yes" : "no")
		   << KNRM << std::endl;
	ch->send_to_TC(false, true, true, buffer.str().c_str());
}

int CalcCurrentSkill(CharData *ch, const ESkill skill_id, CharData *vict) {
	if (MUD::Skills().IsInvalid(skill_id)) {
		return 0;
	}

	int base_percent = ch->get_skill(skill_id);
	int total_percent = 0;
	int victim_sav = 0; // савис жертвы,
	int victim_modi = 0; // другие модификаторы, влияющие на прохождение
	int bonus = 0; // бонус от дополнительных параметров.
	int size = 0; // бонусы/штрафы размера (не спрашивайте...)
	bool ignore_luck = false; // Для скиллов, не учитывающих удачу

	if (base_percent <= 0) {
		return 0;
	}

	if (!ch->IsNpc() && !ch->affected.empty()) {
		for (const auto &aff: ch->affected) {
			if (aff->location == EApply::kPlague) {
				base_percent -= number(ch->get_skill(skill_id) * 0.4, ch->get_skill(skill_id) * 0.05);
			}
		}
	}

	base_percent += int_app[GET_REAL_INT(ch)].to_skilluse;
	if (!ch->IsNpc()) {
		size = (MAX(0, GET_REAL_SIZE(ch) - 50));
	} else {
		size = size_app[GET_POS_SIZE(ch)].interpolate / 2;
	}

	switch (skill_id) {

		case ESkill::kHideTrack: {
			bonus = (IsAbleToUseFeat(ch, EFeat::kStealthy) ? 5 : 0);
			break;
		}

		case ESkill::kBackstab: {
			victim_sav = -GET_REAL_SAVING_REFLEX(vict);
			bonus = dex_bonus(GET_REAL_DEX(ch)) * 2;
			if (IsAwakeOthers(ch)
				|| IsEquipInMetall(ch)) {
				bonus -= 50;
			}

			if (vict) {
				if (!CAN_SEE(vict, ch)) {
					bonus += 25;
				}

				if (GET_POS(vict) < EPosition::kFight) {
					bonus += (20 * (EPosition::kFight - GET_POS(vict)));
				} else if (AFF_FLAGGED(vict, EAffect::kAwarness)) {
					victim_modi -= 30;
				}
				victim_modi += size_app[GET_POS_SIZE(vict)].ac;
				victim_modi -= dex_bonus(GET_REAL_DEX(vict));
			}
			break;
		}

		case ESkill::kBash: {
			victim_sav = -GET_REAL_SAVING_REFLEX(vict);
			bonus = size
				+ dex_bonus(GET_REAL_DEX(ch))
				+ (GET_EQ(ch, EEquipPos::kShield)
				   ? weapon_app[MIN(35, MAX(0, GET_OBJ_WEIGHT(GET_EQ(ch, EEquipPos::kShield))))].bashing
				   : 0);
			if (vict) {
				if (GET_POS(vict) < EPosition::kFight && GET_POS(vict) > EPosition::kSleep) {
					victim_modi -= 20;
				}
				if (PRF_FLAGGED(vict, EPrf::kAwake)) {
					victim_modi -= CalculateSkillAwakeModifier(ch, vict);
				}
			}
			break;
		}

		case ESkill::kHide: {
			bonus =
				dex_bonus(GET_REAL_DEX(ch)) - size_app[GET_POS_SIZE(ch)].ac + (IsAbleToUseFeat(ch, EFeat::kStealthy) ? 5 : 0);
			if (IsAwakeOthers(ch) || IsEquipInMetall(ch)) {
				bonus -= 50;
			}

			if (is_dark(ch->in_room)) {
				bonus += 25;
			}

			if (SECT(ch->in_room) == ESector::kInside) {
				bonus += 20;
			} else if (SECT(ch->in_room) == ESector::kCity) {
				bonus -= 15;
			} else if (SECT(ch->in_room) == ESector::kForest) {
				bonus += 20;
			} else if (SECT(ch->in_room) == ESector::kHills
				|| SECT(ch->in_room) == ESector::kMountain) {
				bonus += 10;
			}

			if (vict) {
				if (AWAKE(vict)) {
					victim_modi -= int_app[GET_REAL_INT(vict)].observation;
				}
			}
			break;
		}

		case ESkill::kKick: {
			victim_sav = -GET_REAL_SAVING_STABILITY(vict);
			bonus = dex_bonus(GET_REAL_DEX(ch)) + dex_bonus(GET_REAL_STR(ch));
			if (vict) {
				victim_modi += size_app[GET_POS_SIZE(vict)].interpolate;
				victim_modi -= GET_REAL_CON(vict);
				if (PRF_FLAGGED(vict, EPrf::kAwake)) {
					victim_modi -= CalculateSkillAwakeModifier(ch, vict);
				}
			}
			break;
		}

		case ESkill::kPickLock: {
			bonus = dex_bonus(GET_REAL_DEX(ch)) + (IsAbleToUseFeat(ch, EFeat::kNimbleFingers) ? 5 : 0);
			break;
		}

		case ESkill::kPunch: {
			victim_sav = -GET_REAL_SAVING_REFLEX(vict);
			break;
		}

		case ESkill::kRescue: {
			bonus = dex_bonus(GET_REAL_DEX(ch));
			break;
		}

		case ESkill::kSneak: {
			bonus = dex_bonus(GET_REAL_DEX(ch))
				+ (IsAbleToUseFeat(ch, EFeat::kStealthy) ? 10 : 0);

			if (IsAwakeOthers(ch) || IsEquipInMetall(ch))
				bonus -= 50;

			if (SECT(ch->in_room) == ESector::kCity)
				bonus -= 10;
			if (is_dark(ch->in_room))
				bonus += 20;

			if (vict) {
				if (GetRealLevel(vict) > 35)
					bonus -= 50;
				if (!CAN_SEE(vict, ch))
					bonus += 25;
				if (AWAKE(vict)) {
					victim_modi -= int_app[GET_REAL_INT(vict)].observation;
				}
			}
			break;
		}

		case ESkill::kSteal: {
			bonus = dex_bonus(GET_REAL_DEX(ch))
				+ (IsAbleToUseFeat(ch, EFeat::kNimbleFingers) ? 5 : 0);

			if (IsAwakeOthers(ch) || IsEquipInMetall(ch))
				bonus -= 50;
			if (is_dark(ch->in_room))
				bonus += 20;

			if (vict) {
				victim_sav = -GET_REAL_SAVING_REFLEX(vict);
				if (!CAN_SEE(vict, ch))
					bonus += 25;
				if (AWAKE(vict)) {
					victim_modi -= int_app[GET_REAL_INT(vict)].observation;
					if (AFF_FLAGGED(vict, EAffect::kAwarness))
						bonus -= 30;
				}
			}
			break;
		}

		case ESkill::kTrack: {
			ignore_luck = true;
			total_percent =
				base_percent + int_app[GET_REAL_INT(ch)].observation + (IsAbleToUseFeat(ch, EFeat::kTracker) ? 10 : 0);

			if (SECT(ch->in_room) == ESector::kForest || SECT(ch->in_room) == ESector::kField) {
				total_percent += 10;
			}

			total_percent = GetComplexSkillModifier(ch, ESkill::kUndefined, GAPPLY_SKILL_SUCCESS, total_percent);

			if (SECT(ch->in_room) == ESector::kWaterSwim
				|| SECT(ch->in_room) == ESector::kWaterNoswim
				|| SECT(ch->in_room) == ESector::kOnlyFlying
				|| SECT(ch->in_room) == ESector::kUnderwater
				|| SECT(ch->in_room) == ESector::kSecret
				|| ROOM_FLAGGED(ch->in_room, ERoomFlag::kNoTrack))
				total_percent = 0;

			if (vict) {
				victim_modi += GET_REAL_CON(vict) / 2;
			}
			break;
		}

		case ESkill::kSense: {
			bonus = int_app[GET_REAL_INT(ch)].observation
				+ (IsAbleToUseFeat(ch, EFeat::kTracker) ? 20 : 0);
			break;
		}

		case ESkill::kMultiparry:
		case ESkill::kParry: {
			victim_sav = dex_bonus(GET_REAL_DEX(vict));
			bonus = dex_bonus(GET_REAL_DEX(ch));
			if (GET_AF_BATTLE(ch, kEafAwake)) {
				bonus += ch->get_skill(ESkill::kAwake);
			}

			if (GET_EQ(ch, EEquipPos::kHold)
				&& GET_OBJ_TYPE(GET_EQ(ch, EEquipPos::kHold)) == EObjType::kWeapon) {
				bonus += weapon_app[MAX(0, MIN(50, GET_OBJ_WEIGHT(GET_EQ(ch, EEquipPos::kHold))))].parrying;
			}
			victim_modi = 100;
			break;
		}

		case ESkill::kShieldBlock: {
			int shield_mod =
				GET_EQ(ch, EEquipPos::kShield) ? MIN(10, MAX(0, GET_OBJ_WEIGHT(GET_EQ(ch, EEquipPos::kShield)) - 20)) : 0;
			int dex_mod = MAX(0, (GET_REAL_DEX(ch) - 20) / 3);
			bonus = dex_mod + shield_mod;
			break;
		}

		case ESkill::kIntercept: {
			victim_sav = dex_bonus(GET_REAL_DEX(vict));
			bonus = dex_bonus(GET_REAL_DEX(ch)) +
				size_app[GET_POS_SIZE(vict)].interpolate;

			if (vict) {
				victim_modi -= dex_bonus(GET_REAL_DEX(vict));
				victim_modi -= size_app[GET_POS_SIZE(vict)].interpolate;
			}
			break;
		}

		case ESkill::kProtect: {
			bonus = dex_bonus(GET_REAL_DEX(ch)) + size;
			victim_modi = 50;
			break;
		}

		case ESkill::kBows: {
			bonus = dex_bonus(GET_REAL_DEX(ch));
			break;
		}

		case ESkill::kTwohands: break;
		case ESkill::kLongBlades: break;
		case ESkill::kSpades: break;
		case ESkill::kShortBlades: break;
		case ESkill::kClubs: break;
		case ESkill::kPicks: break;
		case ESkill::kNonstandart: break;
		case ESkill::kAxes: break;
		case ESkill::kSideAttack: {
			victim_sav = -GET_REAL_SAVING_REFLEX(vict) + dex_bonus(GET_REAL_DEX(ch)); //equal
			break;
		}

		case ESkill::kLooking:[[fallthrough]];
		case ESkill::kHearing: {
			bonus = int_app[GET_REAL_INT(ch)].observation;
			break;
		}

		case ESkill::kDisarm: {
			victim_sav = -GET_REAL_SAVING_REFLEX(vict);
			bonus = dex_bonus(GET_REAL_DEX(ch)) + dex_bonus(GET_REAL_STR(ch));
			if (vict) {
				victim_modi -= dex_bonus(GET_REAL_STR(ch));
				if (GET_EQ(vict, EEquipPos::kBoths))
					victim_modi -= 10;
				if (PRF_FLAGGED(vict, EPrf::kAwake))
					victim_modi -= CalculateSkillAwakeModifier(ch, vict);
			}
			break;
		}

		case ESkill::kAddshot: {
			if (IsEquipInMetall(ch))
				bonus -= 5;
			ignore_luck = true;
			break;
		}

		case ESkill::kNoParryHit: {
			bonus = dex_bonus(GET_REAL_DEX(ch));
			break;
		}

		case ESkill::kDisguise: {
			bonus = dex_bonus(GET_REAL_DEX(ch)) - size_app[GET_POS_SIZE(ch)].ac
				+ (IsAbleToUseFeat(ch, EFeat::kStealthy) ? 5 : 0);

			if (IsAwakeOthers(ch))
				bonus -= 100;

			if (is_dark(ch->in_room))
				bonus += 15;

			if (SECT(ch->in_room) == ESector::kCity)
				bonus -= 15;
			else if (SECT(ch->in_room) == ESector::kForest)
				bonus += 10;
			else if (SECT(ch->in_room) == ESector::kHills
				|| SECT(ch->in_room) == ESector::kMountain)
				bonus += 5;
			if (IsEquipInMetall(ch))
				bonus -= 30;

			if (vict) {
				if (AWAKE(vict))
					victim_modi -= int_app[GET_REAL_INT(vict)].observation;
			}
			break;
		}

		case ESkill::kDodge: {
			bonus = -size_app[GET_POS_SIZE(ch)].ac +
				dex_bonus(GET_REAL_DEX(ch));

			if (IsEquipInMetall(ch))
				bonus -= 40;

			if (vict) {
				victim_modi -= dex_bonus(GET_REAL_DEX(vict));
			}
			break;
		}

		case ESkill::kUndercut: {
			victim_sav = -GET_REAL_SAVING_REFLEX(vict);
			bonus = dex_bonus(GET_REAL_DEX(ch)) + ((dex_bonus(GET_REAL_DEX(ch)) * 5) / 10)
				+ size_app[GET_POS_SIZE(ch)].ac; // тест х3 признан вредительским
			if (IsEquipInMetall(ch))
				bonus -= 10;
			if (vict) {
				if (!CAN_SEE(vict, ch))
					bonus += 10;
				if (GET_POS(vict) < EPosition::kSit)
					bonus -= 50;
				if (AWAKE(vict) || AFF_FLAGGED(vict, EAffect::kAwarness) || MOB_FLAGGED(vict, EMobFlag::kMobAwake))
					victim_modi -= 20;
				if (PRF_FLAGGED(vict, EPrf::kAwake))
					victim_modi -= CalculateSkillAwakeModifier(ch, vict);
			}
			break;
		}

		case ESkill::kRepair: break;
		case ESkill::kSharpening: break;
		case ESkill::kWarcry: break;
		case ESkill::kCourage: break;
		case ESkill::kLeftHit: break;
		case ESkill::kHammer: {
			victim_sav = -GET_REAL_SAVING_STABILITY(vict);
			bonus = size + dex_bonus(GET_REAL_STR(ch));

			if (vict->IsNpc())
				victim_modi -= (size_app[GET_POS_SIZE(vict)].shocking) / 2;
			else
				victim_modi -= size_app[GET_POS_SIZE(vict)].shocking;
			break;
		}

		case ESkill::kOverwhelm: {
			victim_sav = -GET_REAL_SAVING_STABILITY(vict);
			bonus = dex_bonus(GET_REAL_STR(ch));
			if (GET_EQ(ch, EEquipPos::kWield))
				bonus +=
					weapon_app[GET_OBJ_WEIGHT(GET_EQ(ch, EEquipPos::kWield))].shocking;
			else if (GET_EQ(ch, EEquipPos::kBoths))
				bonus +=
					weapon_app[GET_OBJ_WEIGHT(GET_EQ(ch, EEquipPos::kBoths))].shocking;

			if (vict) {
				victim_modi -= GET_REAL_CON(vict);
			}
			break;
		}

		case ESkill::kPoisoning: break;
		case ESkill::kLeadership: {
			bonus = cha_app[GET_REAL_CHA(ch)].leadership;
			break;
		}

		case ESkill::kPunctual: {
			victim_sav = -GET_REAL_SAVING_CRITICAL(vict);
			bonus = dex_bonus(GET_REAL_INT(ch));
			if (GET_EQ(ch, EEquipPos::kWield))
				bonus += MAX(18, GET_OBJ_WEIGHT(GET_EQ(ch, EEquipPos::kWield))) - 18
					+ MAX(25, GET_OBJ_WEIGHT(GET_EQ(ch, EEquipPos::kWield))) - 25
					+ MAX(30, GET_OBJ_WEIGHT(GET_EQ(ch, EEquipPos::kWield))) - 30;
			if (GET_EQ(ch, EEquipPos::kHold))
				bonus += MAX(18, GET_OBJ_WEIGHT(GET_EQ(ch, EEquipPos::kHold))) - 18
					+ MAX(25, GET_OBJ_WEIGHT(GET_EQ(ch, EEquipPos::kHold))) - 25
					+ MAX(30, GET_OBJ_WEIGHT(GET_EQ(ch, EEquipPos::kHold))) - 30;
			if (GET_EQ(ch, EEquipPos::kBoths))
				bonus += MAX(25, GET_OBJ_WEIGHT(GET_EQ(ch, EEquipPos::kBoths))) - 25
					+ MAX(30, GET_OBJ_WEIGHT(GET_EQ(ch, EEquipPos::kBoths))) - 30;
			if (vict) {
				victim_modi -= int_app[GET_REAL_INT(vict)].observation;
			}
			break;
		}

		case ESkill::kAwake: {
			const size_t real_dex = static_cast<size_t>(GET_REAL_DEX(ch));
			if (real_dex < INT_APP_SIZE) {
				bonus = int_app[real_dex].observation;

				if (vict) {
					victim_modi -= int_app[GET_REAL_INT(vict)].observation;
				}
			} else {
				log("SYSERR: Global buffer overflow for ESkill::kAwake. Requested real_dex is %zd, but maximal allowed is %zd.",
					real_dex, INT_APP_SIZE);
			}
		}
			break;

		case ESkill::kIdentify: {
			bonus = int_app[GET_REAL_INT(ch)].observation
				+ (IsAbleToUseFeat(ch, EFeat::kConnoiseur) ? 20 : 0);
			break;
		}

		case ESkill::kCreatePotion:
		case ESkill::kCreateScroll:
		case ESkill::kCreateWand: break;

		case ESkill::kPry: {
			bonus = cha_app[GET_REAL_CHA(ch)].illusive;
			if (vict) {
				if (!CAN_SEE(vict, ch))
					bonus += 50;
				else if (AWAKE(vict))
					victim_modi -= int_app[GET_REAL_INT(ch)].observation;
			}
			break;
		}

		case ESkill::kArmoring: break;

		case ESkill::kHangovering: {
			bonus = -GET_REAL_CON(ch) / 2
				+ (IsAbleToUseFeat(ch, EFeat::kDrunkard) ? 20 : 0);
			break;
		}

		case ESkill::kFirstAid: {
			bonus = (IsAbleToUseFeat(ch, EFeat::kHealer) ? 10 : 0);
			break;
		}

		case ESkill::kCampfire: {
			if (get_room_sky(ch->in_room) == kSkyRaining)
				bonus -= 50;
			else if (get_room_sky(ch->in_room) != kSkyLightning)
				bonus -= number(10, 25);
			break;
		}

		case ESkill::kRiding: {
			bonus = cha_app[GET_REAL_CHA(ch)].leadership;
			break;
		}

		case ESkill::kMorph: break;
		case ESkill::kStrangle: {
			victim_sav = -GET_REAL_SAVING_REFLEX(vict);
			bonus = dex_bonus(GET_REAL_DEX(ch));
			if (AFF_FLAGGED(vict, EAffect::kHold)) {
				bonus += (base_percent + bonus) / 2;
			} else {
				if (!CAN_SEE(ch, vict))
					bonus += (base_percent + bonus) / 5;
				if (PRF_FLAGGED(vict, EPrf::kAwake))
					victim_modi -= CalculateSkillAwakeModifier(ch, vict);
			}
			break;
		}

		case ESkill::kStun: {
			victim_sav = -GET_REAL_SAVING_STABILITY(vict);
			if (!vict->IsNpc())
				victim_sav *= 2;
			else
				victim_sav -= GetRealLevel(vict);

			bonus = dex_bonus(GET_REAL_STR(ch));
			if (GET_EQ(ch, EEquipPos::kWield))
				bonus +=
					weapon_app[GET_OBJ_WEIGHT(GET_EQ(ch, EEquipPos::kWield))].shocking;
			else if (GET_EQ(ch, EEquipPos::kBoths))
				bonus +=
					weapon_app[GET_OBJ_WEIGHT(GET_EQ(ch, EEquipPos::kBoths))].shocking;

			if (PRF_FLAGGED(vict, EPrf::kAwake))
				victim_modi -= CalculateSkillAwakeModifier(ch, vict);
			break;
		}
		default: break;
	}
	std::string LuckTempStr = "Удача игнор";
	if (!ignore_luck) {
		switch (MakeLuckTest(ch, vict)) {
			case ELuckTestResult::kLuckTestSuccess: 
				LuckTempStr = "Удача прошла";
				break;
			case ELuckTestResult::kLuckTestFail: 
				LuckTempStr = "Удача фэйл";
				base_percent = 0;
				bonus = 0;
				break;
			case ELuckTestResult::kLuckTestCriticalSuccess: 
				base_percent = MUD::Skills()[skill_id].cap;
				LuckTempStr = "Удача крит";
				break;
		}
	}

	auto percent = base_percent + bonus + victim_sav + victim_modi/2;
	total_percent = std::clamp(percent, 0, MUD::Skills()[skill_id].cap);

	if (PRF_FLAGGED(ch, EPrf::kAwake) && (skill_id == ESkill::kBash)) {
		total_percent /= 2;
	}

	if (GET_GOD_FLAG(ch, EGf::kGodsLike) || (vict && GET_GOD_FLAG(vict, EGf::kGodscurse))) {
		total_percent = MUD::Skills()[skill_id].cap;
	} else if (GET_GOD_FLAG(ch, EGf::kGodscurse)) {
		total_percent = 0;
	}

	ch->send_to_TC(false, true, true,
			"&CTarget: %s, skill: %s, base_percent: %d, bonus: %d, victim_save: %d, victim_modi: %d, total_percent: %d, удача: %s&n\r\n",
			vict ? GET_NAME(vict) : "NULL",
			MUD::Skills()[skill_id].GetName(),
			base_percent,
			bonus,
			victim_sav,
			victim_modi / 2,
			total_percent,
			LuckTempStr.c_str());
	return (total_percent);
}

void ImproveSkill(CharData *ch, const ESkill skill, int success, CharData *victim) {
	const int trained_skill = ch->get_trained_skill(skill);

	if (trained_skill <= 0 || trained_skill >= CalcSkillMinCap(ch, skill)) {
		return;
	}

	if (ch->IsNpc() || ch->agrobd) {
		return;
	}

	if ((skill == ESkill::kSteal) && (!victim->IsNpc())) {
		return;
	}

	if (victim &&
		(MOB_FLAGGED(victim, EMobFlag::kNoSkillTrain)
			|| MOB_FLAGGED(victim, EMobFlag::kMounting)
			|| !OK_GAIN_EXP(ch, victim)
			|| (victim->get_master() && !victim->get_master()->IsNpc()))) {
		return;
	}

	if (ch->in_room == kNowhere
		|| ROOM_FLAGGED(ch->in_room, ERoomFlag::kPeaceful)
		|| ROOM_FLAGGED(ch->in_room, ERoomFlag::kArena)
		|| ROOM_FLAGGED(ch->in_room, ERoomFlag::kHouse)
		|| ROOM_FLAGGED(ch->in_room, ERoomFlag::kHouseEntry)) {
		return;
	}

	// Если чар нуб, то до 50% скиллы качаются гораздо быстрее
	int INT_PLAYER = (ch->get_trained_skill(skill) < 51
		&& (AFF_FLAGGED(ch, EAffect::kNoobRegen))) ? 50 : GET_REAL_INT(ch);

	int div = int_app[INT_PLAYER].improve;
	if ((ch)->get_class() >= ECharClass::kFirst && (ch)->get_class() <= ECharClass::kLast) {
		div += MUD::Classes()[(ch)->get_class()].GetImprove(skill) / 100;
	}

	int prob = success ? 20000 : 15000;
	prob /= std::max(1, div);
	prob -= 5 * wis_bonus(GET_REAL_WIS(ch), WIS_MAX_SKILLS);
	prob += number(1, trained_skill * 5);

	int skill_is = number(1, MAX(1, prob));
	if ((victim && skill_is <= GET_REAL_INT(ch) * GetRealLevel(victim) / GetRealLevel(ch))
		|| (!victim && skill_is <= GET_REAL_INT(ch))) {
		if (success) {
			sprintf(buf, "%sВы повысили уровень умения \"%s\".%s\r\n",
					CCICYN(ch, C_NRM), MUD::Skills()[skill].GetName(), CCNRM(ch, C_NRM));
		} else {
			sprintf(buf, "%sПоняв свои ошибки, вы повысили уровень умения \"%s\".%s\r\n",
					CCICYN(ch, C_NRM), MUD::Skills()[skill].GetName(), CCNRM(ch, C_NRM));
		}
		SendMsgToChar(buf, ch);
		ch->set_morphed_skill(skill, (trained_skill + number(1, 2)));
		if (!IS_IMMORTAL(ch)) {
			ch->set_morphed_skill(skill,
								  (MIN(kSkillCapOnZeroRemort + GET_REAL_REMORT(ch) * 5, ch->get_trained_skill(skill))));
		}
		if (victim && victim->IsNpc()) {
			MOB_FLAGS(victim).set(EMobFlag::kNoSkillTrain);
		}
	}
}

void TrainSkill(CharData *ch, const ESkill skill, bool success, CharData *vict) {
	if (!ch->IsNpc()) {
		if (skill != ESkill::kSideAttack
			&& ch->get_trained_skill(skill) > 0
			&& (!vict
				|| (vict->IsNpc()
					&& !MOB_FLAGGED(vict, EMobFlag::kProtect)
					&& !MOB_FLAGGED(vict, EMobFlag::kNoSkillTrain)
					&& !AFF_FLAGGED(vict, EAffect::kCharmed)
					&& !IS_HORSE(vict)))) {
			ImproveSkill(ch, skill, success, vict);
		}
	} else if (!IS_CHARMICE(ch)) {
		if (ch->get_skill(skill) > 0
			&& GET_REAL_INT(ch) <= number(0, 1000 - 20 * GET_REAL_WIS(ch))
			&& ch->get_skill(skill) < MUD::Skills()[skill].difficulty) {
			ch->set_skill(skill, ch->get_trained_skill(skill) + 1);
		}
	}
}

/**
* Расчет влияния осторожки у victim против умений killer.
* В данный момент учитывается случай 'игрок против игрока', где осторожка считается как скилл/2
*/
int CalculateSkillAwakeModifier(CharData *killer, CharData *victim) {
	int result = 0;
	if (!killer || !victim) {
		log("SYSERROR: zero character in CalculateSkillAwakeModifier.");
	} else if (killer->IsNpc() || victim->IsNpc()) {
		result = victim->get_skill(ESkill::kAwake);
	} else {
		result = victim->get_skill(ESkill::kAwake) / 2;
	}
	return result;
}

//Определим мин уровень для изучения скилла из книги
//req_lvl - требуемый уровень из книги
int GetSkillMinLevel(CharData *ch, ESkill skill, int req_lvl) {
	int min_lvl = MAX(req_lvl, MUD::Classes()[ch->get_class()].GetMinLevel(skill))
		- std::max(0, GET_REAL_REMORT(ch) / MUD::Classes()[ch->get_class()].GetSkillLevelDecrement());
	return MAX(1, min_lvl);
};

/*
 * функция определяет, может ли персонаж илучить скилл
 * \TODO нужно перенести в класс "инфа о классе персонажа" (как и требования по классу, собственно)
 */
int GetSkillMinLevel(CharData *ch, ESkill skill) {
	int min_lvl = MUD::Classes()[ch->get_class()].GetMinLevel(skill)
		- std::max(0, GET_REAL_REMORT(ch) / MUD::Classes()[ch->get_class()].GetSkillLevelDecrement());
	return MAX(1, min_lvl);
};

bool IsAbleToGetSkill(CharData *ch, ESkill skill, int req_lvl) {
	if (GET_REAL_REMORT(ch) < MUD::Classes()[ch->get_class()].GetMinRemort(skill) ||
		MUD::Classes()[ch->get_class()].HasntSkill(skill)) {
		return false;
	}
	if (GetRealLevel(ch) < GetSkillMinLevel(ch, skill, req_lvl)) {
		return false;
	}
	return true;
}

bool IsAbleToGetSkill(CharData *ch, ESkill skill) {
	if (GET_REAL_REMORT(ch) < MUD::Classes()[ch->get_class()].GetMinRemort(skill)
		|| MUD::Classes()[ch->get_class()].HasntSkill(skill)) {
		return false;
	}
	if (GetRealLevel(ch) < GetSkillMinLevel(ch, skill)) {
		return false;
	}

	return true;
}

int CalcSkillRemortCap(const CharData *ch) {
	return kSkillCapOnZeroRemort + GET_REAL_REMORT(ch) * kSkillCapBonusPerRemort;
}

int CalcSkillWisdomCap(const CharData *ch) {
	return std::min(CalcSkillRemortCap(ch), wis_bonus(GET_REAL_WIS(ch), WIS_MAX_LEARN_L20) * GetRealLevel(ch) / 20);
}

int CalcSkillHardCap(const CharData *ch, const ESkill skill) {
	return std::min(CalcSkillRemortCap(ch), MUD::Skills()[skill].cap);
}

int CalcSkillMinCap(const CharData *ch, const ESkill skill) {
	return std::min(CalcSkillWisdomCap(ch), MUD::Skills()[skill].cap);
}

const ESkill &operator++(ESkill &s) {
	s = static_cast<ESkill>(to_underlying(s) + 1);
	return s;
}

std::ostream& operator<<(std::ostream & os, ESkill &s){
	os << to_underlying(s) << " (" << NAME_BY_ITEM<ESkill>(s) << ")";
	return os;
};

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
