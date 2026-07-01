#if !defined FIGHT_CONSTANTS_HPP_
#define FIGHT_CONSTANTS_HPP_

namespace fight {
enum AttackType {
	kMainHand = 1,	//Основная атака
	kOffHand = 2,	//Доп.атака
	kMobAdd = 3		//Доп.атака моба
};

const int kZeroDmg = 0;
const int kLethalDmg = 11;

enum DmgType {
	kUndefDmg,	// для совместимости, надо убирать
	kPhysDmg,	// физический урон
	kMagicDmg,	// магический урон, дальше смотрим тип
	kPoisonDmg,	// чтобы использовать вместо UNDEF
	kPureDmg	// например, чистый урон SLOW DT
};

// Damage source for fight messages (issue #3311): the weapon attack type or, for
// server-inflicted damage, the trap / bleeding kind. The underlying value plus
// kTypeHit equals the record number in lib/misc/messages (kHit+kTypeHit==400 ...
// kSting+kTypeHit==416; kDeathTrap+kTypeHit==495 ... kBleeding+kTypeHit==499). The
enum class EDamageSource {
	kUndefined = -1,
	// Weapon attack types (were the anonymous fight enum type_hit ... type_sting).
	kHit,		// was type_hit
	kSkin,		// was type_skin
	kWhip,		// was type_whip
	kSlash,		// was type_slash
	kBite,		// was type_bite
	kBludgeon,	// was type_bludgeon
	kCrush,		// was type_crush
	kPound,		// was type_pound
	kClaw,		// was type_claw
	kMaul,		// was type_maul
	kThrash,	// was type_thrash
	kPierce,	// was type_pierce
	kBlast,		// was type_blast
	kPunch,		// was type_punch
	kStab,		// was type_stab
	kPick,		// was type_pick
	kSting,		// was type_sting
	// Magic-melee breath attacks: a breathing mob deals elemental damage typed
	// kMagicDmg; the EElement on the Damage picks the resist channel. Keyed into
	// hit_msg.xml like any other attack type (replaces the old kService breath
	// pseudo-spells + CastDamage workaround).
	kFireBreath,
	kGasBreath,
	kFrostBreath,
	kAcidBreath,
	kLightingBreath,
	// Server-inflicted damage sources (lib/misc/messages 495..499; no damager,
	// so only the to-victim / to-room messages are present).
	kTriggerDeath = 95,		// kTypeTriggerdeath (495): DG mdamage/wdamage/odamage
	kTunnelDeath,			// kTypeTunnerldeath (496): tunnel / van-room death
	kUnderwaterDeathTrap,	// kTypeWaterdeath (497): underwater death trap (drowning)
	kSlowDeathTrap,			// kTypeRoomdeath (498): slow death trap
	kSuffering,				// kTypeSuffering (499): suffering / bleeding out
};

enum {
	kIgnoreSanct,			// игнор санки
	kIgnorePrism,			// игнор призмы
	kIgnoreArmor,			// игнор брони
	kHalfIgnoreArmor,		// ополовинивание учитываемой брони
	kIgnoreAbsorbe,			// игнор поглощения
	kNoFleeDmg,				// от этого урона не нужно убегать (яд и т.п.)
	kCritHit,				// крит удар
	kCritLuck,				// крит удача
	kIgnoreFireShield,		// игнор возврата дамаги от огненного щита
	kMagicReflect,			// дамаг идет от магического зеркала или звукового барьера
	kVictimFireShield,		// жертва имеет огненный щит
	kVictimAirShield,		// жертва имеет воздушный щит
	kVictimIceShield,		// жертва имеет ледяной щит
	kDrawBriefFireShield,	// было отражение от огненного щита в кратком режиме
	kDrawBriefAirShield,	// было отражение от воздушного щита в кратком режиме
	kDrawBriefIceShield,	// было отражение от ледяного щита в кратком режиме
	kDrawBriefMagMirror,	// была обратка от маг. зеркала
	kIgnoreBlink,			// игнор мигания
	kShieldApplied,			// issue.damage-change: одна из защит (щитов) уже поглотила часть этого удара
	kPunctualCrit,			// issue.damage-change: удар точным стилем (dam_critic>0) -- пробивает лед. щит

	kHitFlagsNum			// кол-во флагов
};

} // namespace fight

#endif // FIGHT_CONSTANTS_HPP_
