/**
\authors Created by Sventovit
\date 27.04.2022.
\brief Константы системы заклинаний.
*/

#include "spells_constants.h"
#include "utils/utils.h"

#include <map>
#include <sstream>

std::string GetAffExpiredText(ESpell spell_id) {
	static const std::map<ESpell, std::string> spell_to_text {
		{ESpell::kArmor, "Вы почувствовали себя менее защищенно."},
		{ESpell::kTeleport, "!Teleport!"},
		{ESpell::kBless, "Вы почувствовали себя менее доблестно."},
		{ESpell::kBlindness, "Вы вновь можете видеть."},
		{ESpell::kBurningHands, "!Burning Hands!"},
		{ESpell::kCallLighting, "!Call Lightning"},
		{ESpell::kCharm, "Вы подчиняетесь теперь только себе."},
		{ESpell::kChillTouch, "Вы отметили, что силы вернулись к вам."},
		{ESpell::kClone, "!Clone!"},
		{ESpell::kIceBolts, "!Color Spray!"},
		{ESpell::kControlWeather, "!Control Weather!"},
		{ESpell::kCreateFood, "!Create Food!"},
		{ESpell::kCreateWater, "!Create Water!"},
		{ESpell::kCureBlind, "!Cure Blind!"},
		{ESpell::kCureCritic, "!Cure Critic!"},
		{ESpell::kCureLight, "!Cure Light!"},
		{ESpell::kCurse, "Вы почувствовали себя более уверенно."},
		{ESpell::kDetectAlign, "Вы более не можете определять наклонности."},
		{ESpell::kDetectInvis, "Вы не в состоянии больше видеть невидимых."},
		{ESpell::kDetectMagic, "Вы не в состоянии более определять магию."},
		{ESpell::kDetectPoison, "Вы не в состоянии более определять яды."},
		{ESpell::kDispelEvil, "!Dispel Evil!"},
		{ESpell::kEarthquake, "!Earthquake!"},
		{ESpell::kEnchantWeapon, "!Enchant Weapon!"},
		{ESpell::kEnergyDrain, "!Energy Drain!"},
		{ESpell::kFireball, "!Fireball!"},
		{ESpell::kHarm, "!Harm!"},
		{ESpell::kHeal, "!Heal!"},
		{ESpell::kInvisible, "Вы вновь видимы."},
		{ESpell::kLightingBolt, "!Lightning Bolt!"},
		{ESpell::kLocateObject, "!Locate object!"},
		{ESpell::kMagicMissile, "!Magic Missile!"},
		{ESpell::kPoison, "В вашей крови не осталось ни капельки яда."},
		{ESpell::kProtectFromEvil, "Вы вновь ощущаете страх перед тьмой."},
		{ESpell::kRemoveCurse, "!Remove Curse!"},
		{ESpell::kSanctuary, "Белая аура вокруг вашего тела угасла."},
		{ESpell::kShockingGasp, "!Shocking Grasp!"},
		{ESpell::kSleep, "Вы не чувствуете сонливости."},
		{ESpell::kStrength, "Вы чувствуете себя немного слабее."},
		{ESpell::kSummon, "!Summon!"},
		{ESpell::kPatronage, "Вы утратили покровительство высших сил."},
		{ESpell::kWorldOfRecall, "!Word of Recall!"},
		{ESpell::kRemovePoison, "!Remove Poison!"},
		{ESpell::kSenseLife, "Вы больше не можете чувствовать жизнь."},
		{ESpell::kAnimateDead, "!Animate Dead!"},
		{ESpell::kDispelGood, "!Dispel Good!"},
		{ESpell::kGroupArmor, "!Group Armor!"},
		{ESpell::kGroupHeal, "!Group Heal!"},
		{ESpell::kGroupRecall, "!Group Recall!"},
		{ESpell::kInfravision, "Вы больше не можете видеть ночью."},
		{ESpell::kWaterwalk, "Вы больше не можете ходить по воде."},
		{ESpell::kCureSerious, "!SPELL CURE SERIOUS!"},
		{ESpell::kGroupStrength, "!SPELL GROUP STRENGTH!"},
		{ESpell::kHold, "К вам вернулась способность двигаться."},
		{ESpell::kPowerHold, "!SPELL POWER HOLD!"},
		{ESpell::kMassHold, "!SPELL MASS HOLD!"},
		{ESpell::kFly, "Вы приземлились на землю."},
		{ESpell::kBrokenChains, "Вы вновь стали уязвимы для оцепенения."},
		{ESpell::kNoflee, "Вы опять можете сбежать с поля боя."},
		{ESpell::kCreateLight, "!SPELL CREATE LIGHT!"},
		{ESpell::kDarkness, "Облако тьмы, окружающее вас, спало."},
		{ESpell::kStoneSkin, "Ваша кожа вновь стала мягкой и бархатистой."},
		{ESpell::kCloudly, "Ваши очертания приобрели отчетливость."},
		{ESpell::kSilence, "Теперь вы можете болтать, все что думаете."},
		{ESpell::kLight, "Ваше тело перестало светиться."},
		{ESpell::kChainLighting, "!SPELL CHAIN LIGHTNING!"},
		{ESpell::kFireBlast, "!SPELL FIREBLAST!"},
		{ESpell::kGodsWrath, "!SPELL IMPLOSION!"},
		{ESpell::kWeaknes, "Силы вернулись к вам."},
		{ESpell::kGroupInvisible, "!SPELL GROUP INVISIBLE!"},
		{ESpell::kShadowCloak, "Ваша теневая мантия замерцала и растаяла."},
		{ESpell::kAcid, "!SPELL ACID!"},
		{ESpell::kRepair, "!SPELL REPAIR!"},
		{ESpell::kEnlarge, "Ваши размеры стали прежними."},
		{ESpell::kFear, "!SPELL FEAR!"},
		{ESpell::kSacrifice, "!SPELL SACRIFICE!"},
		{ESpell::kWeb, "Магическая сеть, покрывавшая вас, исчезла."},
		{ESpell::kBlink, "Вы перестали мигать."},
		{ESpell::kRemoveHold, "!SPELL REMOVE HOLD!"},
		{ESpell::kCamouflage, "Вы стали вновь похожи сами на себя."},
		{ESpell::kPowerBlindness, "!SPELL POWER BLINDNESS!"},
		{ESpell::kMassBlindness, "!SPELL MASS BLINDNESS!"},
		{ESpell::kPowerSilence, "!SPELL POWER SIELENCE!"},
		{ESpell::kExtraHits, "!SPELL EXTRA HITS!"},
		{ESpell::kResurrection, "!SPELL RESSURECTION!"},
		{ESpell::kMagicShield, "Ваш волшебный щит рассеялся."},
		{ESpell::kForbidden, "Магия, запечатывающая входы, пропала."},
		{ESpell::kMassSilence, "!SPELL MASS SIELENCE!"},
		{ESpell::kRemoveSilence, "!SPELL REMOVE SIELENCE!"},
		{ESpell::kDamageLight, "!SPELL DAMAGE LIGHT!"},
		{ESpell::kDamageSerious, "!SPELL DAMAGE SERIOUS!"},
		{ESpell::kDamageCritic, "!SPELL DAMAGE CRITIC!"},
		{ESpell::kMassCurse, "!SPELL MASS CURSE!"},
		{ESpell::kArmageddon, "!SPELL ARMAGEDDON!"},
		{ESpell::kGroupFly, "!SPELL GROUP FLY!"},
		{ESpell::kGroupBless, "!SPELL GROUP BLESS!"},
		{ESpell::kResfresh, "!SPELL REFRESH!"},
		{ESpell::kStunning, "!SPELL STUNNING!"},
		{ESpell::kHide, "Вы стали заметны окружающим."},
		{ESpell::kSneak, "Ваши передвижения стали заметны."},
		{ESpell::kDrunked, "Кураж прошел. Мама, лучше бы я умер$q вчера."},
		{ESpell::kAbstinent, "А головка ваша уже не болит."},
		{ESpell::kFullFeed, "Вам снова захотелось жареного, да с дымком."},
		{ESpell::kColdWind, "Вы согрелись и подвижность вернулась к вам."},
		{ESpell::kBattle, "К вам вернулась способность нормально сражаться."},
		{ESpell::kHaemorrhage, "Ваши кровоточащие раны затянулись."},
		{ESpell::kCourage, "Вы успокоились."},
		{ESpell::kWaterbreath, "Вы более не способны дышать водой."},
		{ESpell::kSlowdown, "Медлительность исчезла."},
		{ESpell::kHaste, "Вы стали более медлительны."},
		{ESpell::kMassSlow, "!SPELL MASS SLOW!"},
		{ESpell::kGroupHaste, "!SPELL MASS HASTE!"},
		{ESpell::kGodsShield, "Голубой кокон вокруг вашего тела угас."},
		{ESpell::kFever, "Лихорадка прекратилась."},
		{ESpell::kCureFever, "!SPELL CURE PLAQUE!"},
		{ESpell::kAwareness, "Вы стали менее внимательны."},
		{ESpell::kReligion, "Вы утратили расположение Богов."},
		{ESpell::kAirShield, "Ваш воздушный щит исчез."},
		{ESpell::kPortal, "!PORTAL!"},
		{ESpell::kDispellMagic, "!DISPELL MAGIC!"},
		{ESpell::kSummonKeeper, "!SUMMON KEEPER!"},
		{ESpell::kFastRegeneration, "Живительная сила покинула вас."},
		{ESpell::kCreateWeapon, "!CREATE WEAPON!"},
		{ESpell::kFireShield, "Огненный щит вокруг вашего тела исчез."},
		{ESpell::kRelocate, "!RELOCATE!"},
		{ESpell::kSummonFirekeeper, "!SUMMON FIREKEEPER!"},
		{ESpell::kIceShield, "Ледяной щит вокруг вашего тела исчез."},
		{ESpell::kIceStorm, "Ваши мышцы оттаяли и вы снова можете двигаться."},
		{ESpell::kLessening, "Ваши размеры вновь стали прежними."},
		{ESpell::kShineFlash, "!SHINE LIGHT!"},
		{ESpell::kMadness, "Безумие отпустило вас."},
		{ESpell::kGroupMagicGlass, "!GROUP MAGICGLASS!"},
		{ESpell::kCloudOfArrows, "Облако стрел вокруг вас рассеялось."},
		{ESpell::kVacuum, "Пустота вокруг вас исчезла."},
		{ESpell::kMeteorStorm, "Последний громовой камень грянул в землю и все стихло."},
		{ESpell::kStoneHands, "Ваши руки вернулись к прежнему состоянию."},
		{ESpell::kMindless, "Ваш разум просветлел."},
		{ESpell::kPrismaticAura, "Призматическая аура вокруг вашего тела угасла."},
		{ESpell::kEviless, "Силы зла оставили вас."},
		{ESpell::kAirAura, "Воздушная аура вокруг вас исчезла."},
		{ESpell::kFireAura, "Огненная аура вокруг вас исчезла."},
		{ESpell::kIceAura, "Ледяная аура вокруг вас исчезла."},
		{ESpell::kShock, "!SHOCK!"},
		{ESpell::kMagicGlass, "Вы вновь чувствительны к магическим поражениям."},
		{ESpell::kGroupSanctuary, "!SPELL GROUP SANCTUARY!"},
		{ESpell::kGroupPrismaticAura, "!SPELL GROUP PRISMATICAURA!"},
		{ESpell::kDeafness, "Вы вновь можете слышать."},
		{ESpell::kPowerDeafness, "!SPELL_POWER_DEAFNESS!"},
		{ESpell::kRemoveDeafness, "!SPELL_REMOVE_DEAFNESS!"},
		{ESpell::kMassDeafness, "!SPELL_MASS_DEAFNESS!"},
		{ESpell::kDustStorm, "!SPELL_DUSTSTORM!"},
		{ESpell::kEarthfall, "!SPELL_EARTHFALL!"},
		{ESpell::kSonicWave, "!SPELL_SONICWAVE!"},
		{ESpell::kHolystrike, "!SPELL_HOLYSTRIKE!"},
		{ESpell::kSumonAngel, "!SPELL_SPELL_ANGEL!"},
		{ESpell::kMassFear, "!SPELL_SPELL_MASS_FEAR!"},
		{ESpell::kFascination, "Ваша красота куда-то пропала."},
		{ESpell::kCrying, "Ваша душа успокоилась."},
		{ESpell::kOblivion, "!SPELL_OBLIVION!"},
		{ESpell::kBurdenOfTime, "!SPELL_BURDEN_OF_TIME!"},
		{ESpell::kGroupRefresh, "!SPELL_GROUP_REFRESH!"},
		{ESpell::kPeaceful, "Смирение в вашей душе вдруг куда-то исчезло."},
		{ESpell::kMagicBattle, "К вам вернулась способность нормально сражаться."},
		{ESpell::kBerserk, "Неистовство оставило вас."},
		{ESpell::kStoneBones, "!stone bones!"},
		{ESpell::kRoomLight, "Колдовской свет угас."},
		{ESpell::kDeadlyFog, "Порыв ветра развеял туман смерти."},
		{ESpell::kThunderstorm, "Ветер прогнал грозовые тучи."},
		{ESpell::kLightWalk, "Ваши следы вновь стали заметны."},
		{ESpell::kFailure, "Удача вновь вернулась к вам."},
		{ESpell::kClanPray, "Магические чары ослабели со временем и покинули вас."},
		{ESpell::kGlitterDust, "Покрывавшая вас блестящая пыль осыпалась и растаяла в воздухе."},
		{ESpell::kScream, "Леденящий душу испуг отпустил вас."},
		{ESpell::kCatGrace, "Ваши движения утратили прежнюю колдовскую ловкость."},
		{ESpell::kBullBody, "Ваше телосложение вновь стало обычным."},
		{ESpell::kSnakeWisdom, "Вы утратили навеянную магией мудрость."},
		{ESpell::kGimmicry, "Навеянная магией хитрость покинула вас."},
		{ESpell::kWarcryOfChallenge, "!SPELL_WC_OF_CHALLENGE!"},
		{ESpell::kWarcryOfMenace, ""},
		{ESpell::kWarcryOfRage, "!SPELL_WC_OF_RAGE!"},
		{ESpell::kWarcryOfMadness, ""},
		{ESpell::kWarcryOfThunder, "!SPELL_WC_OF_THUNDER!"},
		{ESpell::kWarcryOfDefence, "Действие клича 'призыв к обороне' закончилось."},
		{ESpell::kWarcryOfBattle, "Действие клича битвы закончилось."},
		{ESpell::kWarcryOfPower, "Действие клича мощи закончилось."},
		{ESpell::kWarcryOfBless, "Действие клича доблести закончилось."},
		{ESpell::kWarcryOfCourage, "Действие клича отваги закончилось."},
		{ESpell::kRuneLabel, "Магические письмена на земле угасли."},
		{ESpell::kAconitumPoison, "В вашей крови не осталось ни капельки яда аконита."},
		{ESpell::kScopolaPoison, "В вашей крови не осталось ни капельки яда скополии."},
		{ESpell::kBelenaPoison, "В вашей крови не осталось ни капельки яда белены."},
		{ESpell::kDaturaPoison, "В вашей крови не осталось ни капельки яда дурмана."},
		{ESpell::kTimerRestore, "SPELL_TIMER_REPAIR"},
		{ESpell::kCombatLuck, "!SPELL_CombatLuck!"},
		{ESpell::kBandage, "Вы аккуратно перевязали свои раны."},
		{ESpell::kNoBandage, "Вы снова можете перевязывать свои раны."},
		{ESpell::kCapable, "!SPELL_CAPABLE!"},
		{ESpell::kStrangle, "Удушье отпустило вас, и вы вздохнули полной грудью."},
		{ESpell::kRecallSpells, "Вам стало не на чем концентрироваться."},
		{ESpell::kHypnoticPattern, "Плывший в воздухе огненный узор потускнел и растаял струйками дыма."},
		{ESpell::kSolobonus, "Одна из наград прекратила действовать."},
		{ESpell::kVampirism, "!SPELL_VAMPIRE!"},
		{ESpell::kRestoration, "!SPELLS_RESTORATION!"},
		{ESpell::kDeathAura, "Силы нави покинули вас."},
		{ESpell::kRecovery, "!SPELL_RECOVERY!"},
		{ESpell::kMassRecovery, "!SPELL_MASS_RECOVERY!"},
		{ESpell::kAuraOfEvil, "Аура зла больше не помогает вам."},
		{ESpell::kMentalShadow, "!SPELL_MENTAL_SHADOW!"},
		{ESpell::kBlackTentacles, "Жуткие черные руки побледнели и расплылись зловонной дымкой."},
		{ESpell::kWhirlwind, "!SPELL_WHIRLWIND!"},
		{ESpell::kIndriksTeeth, "Каменные зубы исчезли, возвратив способность двигаться."},
		{ESpell::kAcidArrow, "!SPELL_MELFS_ACID_ARROW!"},
		{ESpell::kThunderStone, "!SPELL_THUNDERSTONE!"},
		{ESpell::kClod, "!SPELL_CLOD!"},
		{ESpell::kExpedient, "Эффект боевого приема завершился."},
		{ESpell::kSightOfDarkness, "!SPELL SIGHT OF DARKNESS!"},
		{ESpell::kGroupSincerity, "!SPELL GENERAL SINCERITY!"},
		{ESpell::kMagicalGaze, "!SPELL MAGICAL GAZE!"},
		{ESpell::kAllSeeingEye, "!SPELL ALL SEEING EYE!"},
		{ESpell::kEyeOfGods, "!SPELL EYE OF GODS!"},
		{ESpell::kBreathingAtDepth, "!SPELL BREATHING AT DEPTH!"},
		{ESpell::kGeneralRecovery, "!SPELL GENERAL RECOVERY!"},
		{ESpell::kCommonMeal, "!SPELL COMMON MEAL!"},
		{ESpell::kStoneWall, "!SPELL STONE WALL!"},
		{ESpell::kSnakeEyes, "!SPELL SNAKE EYES!"},
		{ESpell::kEarthAura, "Матушка земля забыла про Вас."},
		{ESpell::kGroupProtectFromEvil, "Вы вновь ощущаете страх перед тьмой."},
		{ESpell::kArrowsFire, "!NONE"},
		{ESpell::kArrowsWater, "!NONE"},
		{ESpell::kArrowsEarth, "!NONE"},
		{ESpell::kArrowsAir, "!NONE"},
		{ESpell::kArrowsDeath, "!NONE"},
		{ESpell::kPaladineInspiration, "*Боевое воодушевление угасло, а с ним и вся жажда подвигов!"},
		{ESpell::kDexterity, "Вы стали менее шустрым."},
		{ESpell::kGroupBlink, "!NONE"},
		{ESpell::kGroupCloudly, "!NONE"},
		{ESpell::kGroupAwareness, "!NONE"},
		{ESpell::kWarcryOfExperience, "Действие клича 'обучение' закончилось."},
		{ESpell::kWarcryOfLuck, "Действие клича 'везение' закончилось."},
		{ESpell::kWarcryOfPhysdamage, "Действие клича 'точность' закончилось."},
		{ESpell::kMassFailure, "Удача снова повернулась к вам лицом... и залепила пощечину."},
		{ESpell::kSnare, "Покрывавшие вас сети колдовской западни растаяли."},
		{ESpell::kQUest, "Наложенные на вас чары рассеялись."},
		{ESpell::kExpedientFail, "Вы восстановили равновесие."},
		{ESpell::kLowerEffectiveness, ""},
		{ESpell::kNoInjure, ""},
		{ESpell::kNoCharge, ""},
		{ESpell::kConfuse, ""},
		{ESpell::kDazzle, ""},
		{ESpell::kGreatHeal, "!Great Heal!"},
		{ESpell::kFrenzy, "Жажда крови отступила и Вы пришли в себя."},
		{ESpell::kPortalTimer, "Пентаграмма медленно растаяла."}
	};

	if (!spell_to_text.contains(spell_id)) {
		std::stringstream log_text;
		log_text << "!нет сообщения при спадении аффекта под номером: " << static_cast<int>(spell_id) << "!";
		return log_text.str();
	}

	return spell_to_text.at(spell_id);
}

// GetCastPhrase удалён (issue #3304): фразы заклинаний перенесены в
// lib/cfg/spell_msg.xml (kCastPhraseHeathen/kCastPhraseChristian),
// выбор фразы - через MUD::SpellMessages() в SaySpell().

typedef std::map<ESpell, std::string> ESpell_name_by_value_t;
typedef std::map<const std::string, ESpell> ESpell_value_by_name_t;
ESpell_name_by_value_t ESpell_name_by_value;
ESpell_value_by_name_t ESpell_value_by_name;
void init_ESpell_ITEM_NAMES() {
	ESpell_value_by_name.clear();
	ESpell_name_by_value.clear();

	ESpell_name_by_value[ESpell::kUndefined] = "kUndefined";
	ESpell_name_by_value[ESpell::kArmor] = "kArmor";
	ESpell_name_by_value[ESpell::kTeleport] = "kTeleport";
	ESpell_name_by_value[ESpell::kBless] = "kBless";
	ESpell_name_by_value[ESpell::kBlindness] = "kBlindness";
	ESpell_name_by_value[ESpell::kBurningHands] = "kBurningHands";
	ESpell_name_by_value[ESpell::kCallLighting] = "kCallLighting";
	ESpell_name_by_value[ESpell::kCharm] = "kCharm";
	ESpell_name_by_value[ESpell::kChillTouch] = "kChillTouch";
	ESpell_name_by_value[ESpell::kClone] = "kClone";
	ESpell_name_by_value[ESpell::kIceBolts] = "kIceBolts";
	ESpell_name_by_value[ESpell::kControlWeather] = "kControlWeather";
	ESpell_name_by_value[ESpell::kCreateFood] = "kCreateFood";
	ESpell_name_by_value[ESpell::kCreateWater] = "kCreateWater";
	ESpell_name_by_value[ESpell::kCureBlind] = "kCureBlind";
	ESpell_name_by_value[ESpell::kCureCritic] = "kCureCritic";
	ESpell_name_by_value[ESpell::kCureLight] = "kCureLight";
	ESpell_name_by_value[ESpell::kCurse] = "kCurse";
	ESpell_name_by_value[ESpell::kDetectAlign] = "kDetectAlign";
	ESpell_name_by_value[ESpell::kDetectInvis] = "kDetectInvis";
	ESpell_name_by_value[ESpell::kDetectMagic] = "kDetectMagic";
	ESpell_name_by_value[ESpell::kDetectPoison] = "kDetectPoison";
	ESpell_name_by_value[ESpell::kDispelEvil] = "kDispelEvil";
	ESpell_name_by_value[ESpell::kEarthquake] = "kEarthquake";
	ESpell_name_by_value[ESpell::kEnchantWeapon] = "kEnchantWeapon";
	ESpell_name_by_value[ESpell::kEnergyDrain] = "kEnergyDrain";
	ESpell_name_by_value[ESpell::kFireball] = "kFireball";
	ESpell_name_by_value[ESpell::kHarm] = "kHarm";
	ESpell_name_by_value[ESpell::kHeal] = "kHeal";
	ESpell_name_by_value[ESpell::kInvisible] = "kInvisible";
	ESpell_name_by_value[ESpell::kLightingBolt] = "kLightingBolt";
	ESpell_name_by_value[ESpell::kLocateObject] = "kLocateObject";
	ESpell_name_by_value[ESpell::kMagicMissile] = "kMagicMissile";
	ESpell_name_by_value[ESpell::kPoison] = "kPoison";
	ESpell_name_by_value[ESpell::kProtectFromEvil] = "kProtectFromEvil";
	ESpell_name_by_value[ESpell::kRemoveCurse] = "kRemoveCurse";
	ESpell_name_by_value[ESpell::kSanctuary] = "kSanctuary";
	ESpell_name_by_value[ESpell::kShockingGasp] = "kShockingGasp";
	ESpell_name_by_value[ESpell::kSleep] = "kSleep";
	ESpell_name_by_value[ESpell::kStrength] = "kStrength";
	ESpell_name_by_value[ESpell::kSummon] = "kSummon";
	ESpell_name_by_value[ESpell::kPatronage] = "kPatronage";
	ESpell_name_by_value[ESpell::kWorldOfRecall] = "kWorldOfRecall";
	ESpell_name_by_value[ESpell::kRemovePoison] = "kRemovePoison";
	ESpell_name_by_value[ESpell::kSenseLife] = "kSenseLife";
	ESpell_name_by_value[ESpell::kAnimateDead] = "kAnimateDead";
	ESpell_name_by_value[ESpell::kDispelGood] = "kDispelGood";
	ESpell_name_by_value[ESpell::kGroupArmor] = "kGroupArmor";
	ESpell_name_by_value[ESpell::kGroupHeal] = "kGroupHeal";
	ESpell_name_by_value[ESpell::kGroupRecall] = "kGroupRecall";
	ESpell_name_by_value[ESpell::kInfravision] = "kInfravision";
	ESpell_name_by_value[ESpell::kWaterwalk] = "kWaterwalk";
	ESpell_name_by_value[ESpell::kCureSerious] = "kCureSerious";
	ESpell_name_by_value[ESpell::kGroupStrength] = "kGroupStrength";
	ESpell_name_by_value[ESpell::kHold] = "kHold";
	ESpell_name_by_value[ESpell::kPowerHold] = "kPowerHold";
	ESpell_name_by_value[ESpell::kMassHold] = "kMassHold";
	ESpell_name_by_value[ESpell::kFly] = "kFly";
	ESpell_name_by_value[ESpell::kBrokenChains] = "kBrokenChains";
	ESpell_name_by_value[ESpell::kNoflee] = "kNoflee";
	ESpell_name_by_value[ESpell::kCreateLight] = "kCreateLight";
	ESpell_name_by_value[ESpell::kDarkness] = "kDarkness";
	ESpell_name_by_value[ESpell::kStoneSkin] = "kStoneSkin";
	ESpell_name_by_value[ESpell::kCloudly] = "kCloudly";
	ESpell_name_by_value[ESpell::kSilence] = "kSilence";
	ESpell_name_by_value[ESpell::kLight] = "kLight";
	ESpell_name_by_value[ESpell::kChainLighting] = "kChainLighting";
	ESpell_name_by_value[ESpell::kFireBlast] = "kFireBlast";
	ESpell_name_by_value[ESpell::kGodsWrath] = "kGodsWrath";
	ESpell_name_by_value[ESpell::kWeaknes] = "kWeaknes";
	ESpell_name_by_value[ESpell::kGroupInvisible] = "kGroupInvisible";
	ESpell_name_by_value[ESpell::kShadowCloak] = "kShadowCloak";
	ESpell_name_by_value[ESpell::kAcid] = "kAcid";
	ESpell_name_by_value[ESpell::kRepair] = "kRepair";
	ESpell_name_by_value[ESpell::kEnlarge] = "kEnlarge";
	ESpell_name_by_value[ESpell::kFear] = "kFear";
	ESpell_name_by_value[ESpell::kSacrifice] = "kSacrifice";
	ESpell_name_by_value[ESpell::kWeb] = "kWeb";
	ESpell_name_by_value[ESpell::kBlink] = "kBlink";
	ESpell_name_by_value[ESpell::kRemoveHold] = "kRemoveHold";
	ESpell_name_by_value[ESpell::kCamouflage] = "kCamouflage";
	ESpell_name_by_value[ESpell::kPowerBlindness] = "kPowerBlindness";
	ESpell_name_by_value[ESpell::kMassBlindness] = "kMassBlindness";
	ESpell_name_by_value[ESpell::kPowerSilence] = "kPowerSilence";
	ESpell_name_by_value[ESpell::kExtraHits] = "kExtraHits";
	ESpell_name_by_value[ESpell::kResurrection] = "kResurrection";
	ESpell_name_by_value[ESpell::kMagicShield] = "kMagicShield";
	ESpell_name_by_value[ESpell::kForbidden] = "kForbidden";
	ESpell_name_by_value[ESpell::kMassSilence] = "kMassSilence";
	ESpell_name_by_value[ESpell::kRemoveSilence] = "kRemoveSilence";
	ESpell_name_by_value[ESpell::kDamageLight] = "kDamageLight";
	ESpell_name_by_value[ESpell::kDamageSerious] = "kDamageSerious";
	ESpell_name_by_value[ESpell::kDamageCritic] = "kDamageCritic";
	ESpell_name_by_value[ESpell::kMassCurse] = "kMassCurse";
	ESpell_name_by_value[ESpell::kArmageddon] = "kArmageddon";
	ESpell_name_by_value[ESpell::kGroupFly] = "kGroupFly";
	ESpell_name_by_value[ESpell::kGroupBless] = "kGroupBless";
	ESpell_name_by_value[ESpell::kResfresh] = "kResfresh";
	ESpell_name_by_value[ESpell::kStunning] = "kStunning";
	ESpell_name_by_value[ESpell::kHide] = "kHide";
	ESpell_name_by_value[ESpell::kSneak] = "kSneak";
	ESpell_name_by_value[ESpell::kDrunked] = "kDrunked";
	ESpell_name_by_value[ESpell::kAbstinent] = "kAbstinent";
	ESpell_name_by_value[ESpell::kFullFeed] = "kFullFeed";
	ESpell_name_by_value[ESpell::kColdWind] = "kColdWind";
	ESpell_name_by_value[ESpell::kBattle] = "kBattle";
	ESpell_name_by_value[ESpell::kHaemorrhage] = "kHaemorrhage";
	ESpell_name_by_value[ESpell::kCourage] = "kCourage";
	ESpell_name_by_value[ESpell::kWaterbreath] = "kWaterbreath";
	ESpell_name_by_value[ESpell::kSlowdown] = "kSlowdown";
	ESpell_name_by_value[ESpell::kHaste] = "kHaste";
	ESpell_name_by_value[ESpell::kMassSlow] = "kMassSlow";
	ESpell_name_by_value[ESpell::kGroupHaste] = "kGroupHaste";
	ESpell_name_by_value[ESpell::kGodsShield] = "kGodsShield";
	ESpell_name_by_value[ESpell::kFever] = "kFever";
	ESpell_name_by_value[ESpell::kCureFever] = "kCureFever";
	ESpell_name_by_value[ESpell::kAwareness] = "kAwareness";
	ESpell_name_by_value[ESpell::kReligion] = "kReligion";
	ESpell_name_by_value[ESpell::kAirShield] = "kAirShield";
	ESpell_name_by_value[ESpell::kPortal] = "kPortal";
	ESpell_name_by_value[ESpell::kDispellMagic] = "kDispellMagic";
	ESpell_name_by_value[ESpell::kSummonKeeper] = "kSummonKeeper";
	ESpell_name_by_value[ESpell::kFastRegeneration] = "kFastRegeneration";
	ESpell_name_by_value[ESpell::kCreateWeapon] = "kCreateWeapon";
	ESpell_name_by_value[ESpell::kFireShield] = "kFireShield";
	ESpell_name_by_value[ESpell::kRelocate] = "kRelocate";
	ESpell_name_by_value[ESpell::kSummonFirekeeper] = "kSummonFirekeeper";
	ESpell_name_by_value[ESpell::kIceShield] = "kIceShield";
	ESpell_name_by_value[ESpell::kIceStorm] = "kIceStorm";
	ESpell_name_by_value[ESpell::kLessening] = "kLessening";
	ESpell_name_by_value[ESpell::kShineFlash] = "kShineFlash";
	ESpell_name_by_value[ESpell::kMadness] = "kMadness";
	ESpell_name_by_value[ESpell::kGroupMagicGlass] = "kGroupMagicGlass";
	ESpell_name_by_value[ESpell::kCloudOfArrows] = "kCloudOfArrows";
	ESpell_name_by_value[ESpell::kVacuum] = "kVacuum";
	ESpell_name_by_value[ESpell::kMeteorStorm] = "kMeteorStorm";
	ESpell_name_by_value[ESpell::kStoneHands] = "kStoneHands";
	ESpell_name_by_value[ESpell::kMindless] = "kMindless";
	ESpell_name_by_value[ESpell::kPrismaticAura] = "kPrismaticAura";
	ESpell_name_by_value[ESpell::kEviless] = "kEviless";
	ESpell_name_by_value[ESpell::kAirAura] = "kAirAura";
	ESpell_name_by_value[ESpell::kFireAura] = "kFireAura";
	ESpell_name_by_value[ESpell::kIceAura] = "kIceAura";
	ESpell_name_by_value[ESpell::kShock] = "kShock";
	ESpell_name_by_value[ESpell::kMagicGlass] = "kMagicGlass";
	ESpell_name_by_value[ESpell::kGroupSanctuary] = "kGroupSanctuary";
	ESpell_name_by_value[ESpell::kGroupPrismaticAura] = "kGroupPrismaticAura";
	ESpell_name_by_value[ESpell::kDeafness] = "kDeafness";
	ESpell_name_by_value[ESpell::kPowerDeafness] = "kPowerDeafness";
	ESpell_name_by_value[ESpell::kRemoveDeafness] = "kRemoveDeafness";
	ESpell_name_by_value[ESpell::kMassDeafness] = "kMassDeafness";
	ESpell_name_by_value[ESpell::kDustStorm] = "kDustStorm";
	ESpell_name_by_value[ESpell::kEarthfall] = "kEarthfall";
	ESpell_name_by_value[ESpell::kSonicWave] = "kSonicWave";
	ESpell_name_by_value[ESpell::kHolystrike] = "kHolystrike";
	ESpell_name_by_value[ESpell::kSumonAngel] = "kSumonAngel";
	ESpell_name_by_value[ESpell::kMassFear] = "kMassFear";
	ESpell_name_by_value[ESpell::kFascination] = "kFascination";
	ESpell_name_by_value[ESpell::kCrying] = "kCrying";
	ESpell_name_by_value[ESpell::kOblivion] = "kOblivion";
	ESpell_name_by_value[ESpell::kBurdenOfTime] = "kBurdenOfTime";
	ESpell_name_by_value[ESpell::kGroupRefresh] = "kGroupRefresh";
	ESpell_name_by_value[ESpell::kPeaceful] = "kPeaceful";
	ESpell_name_by_value[ESpell::kMagicBattle] = "kMagicBattle";
	ESpell_name_by_value[ESpell::kBerserk] = "kBerserk";
	ESpell_name_by_value[ESpell::kStoneBones] = "kStoneBones";
	ESpell_name_by_value[ESpell::kRoomLight] = "kRoomLight";
	ESpell_name_by_value[ESpell::kDeadlyFog] = "kDeadlyFog";
	ESpell_name_by_value[ESpell::kThunderstorm] = "kThunderstorm";
	ESpell_name_by_value[ESpell::kLightWalk] = "kLightWalk";
	ESpell_name_by_value[ESpell::kFailure] = "kFailure";
	ESpell_name_by_value[ESpell::kClanPray] = "kClanPray";
	ESpell_name_by_value[ESpell::kGlitterDust] = "kGlitterDust";
	ESpell_name_by_value[ESpell::kScream] = "kScream";
	ESpell_name_by_value[ESpell::kCatGrace] = "kCatGrace";
	ESpell_name_by_value[ESpell::kBullBody] = "kBullBody";
	ESpell_name_by_value[ESpell::kSnakeWisdom] = "kSnakeWisdom";
	ESpell_name_by_value[ESpell::kGimmicry] = "kGimmicry";
	ESpell_name_by_value[ESpell::kWarcryOfChallenge] = "kWarcryOfChallenge";
	ESpell_name_by_value[ESpell::kWarcryOfMenace] = "kWarcryOfMenace";
	ESpell_name_by_value[ESpell::kWarcryOfRage] = "kWarcryOfRage";
	ESpell_name_by_value[ESpell::kWarcryOfMadness] = "kWarcryOfMadness";
	ESpell_name_by_value[ESpell::kWarcryOfThunder] = "kWarcryOfThunder";
	ESpell_name_by_value[ESpell::kWarcryOfDefence] = "kWarcryOfDefence";
	ESpell_name_by_value[ESpell::kWarcryOfBattle] = "kWarcryOfBattle";
	ESpell_name_by_value[ESpell::kWarcryOfPower] = "kWarcryOfPower";
	ESpell_name_by_value[ESpell::kWarcryOfBless] = "kWarcryOfBless";
	ESpell_name_by_value[ESpell::kWarcryOfCourage] = "kWarcryOfCourage";
	ESpell_name_by_value[ESpell::kWarcryOfExperience] = "kWarcryOfExperience";
	ESpell_name_by_value[ESpell::kWarcryOfLuck] = "kWarcryOfLuck";
	ESpell_name_by_value[ESpell::kWarcryOfPhysdamage] = "kWarcryOfPhysdamage";
	ESpell_name_by_value[ESpell::kRuneLabel] = "kRuneLabel";
	ESpell_name_by_value[ESpell::kAconitumPoison] = "kAconitumPoison";
	ESpell_name_by_value[ESpell::kScopolaPoison] = "kScopolaPoison";
	ESpell_name_by_value[ESpell::kBelenaPoison] = "kBelenaPoison";
	ESpell_name_by_value[ESpell::kDaturaPoison] = "kDaturaPoison";
	ESpell_name_by_value[ESpell::kTimerRestore] = "kTimerRestore";
	ESpell_name_by_value[ESpell::kCombatLuck] = "kCombatLuck";
	ESpell_name_by_value[ESpell::kBandage] = "kBandage";
	ESpell_name_by_value[ESpell::kNoBandage] = "kNoBandage";
	ESpell_name_by_value[ESpell::kCapable] = "kCapable";
	ESpell_name_by_value[ESpell::kStrangle] = "kStrangle";
	ESpell_name_by_value[ESpell::kRecallSpells] = "kRecallSpells";
	ESpell_name_by_value[ESpell::kHypnoticPattern] = "kHypnoticPattern";
	ESpell_name_by_value[ESpell::kSolobonus] = "kSolobonus";
	ESpell_name_by_value[ESpell::kVampirism] = "kVampirism";
	ESpell_name_by_value[ESpell::kRestoration] = "kRestoration";
	ESpell_name_by_value[ESpell::kDeathAura] = "kDeathAura";
	ESpell_name_by_value[ESpell::kRecovery] = "kRecovery";
	ESpell_name_by_value[ESpell::kMassRecovery] = "kMassRecovery";
	ESpell_name_by_value[ESpell::kAuraOfEvil] = "kAuraOfEvil";
	ESpell_name_by_value[ESpell::kMentalShadow] = "kMentalShadow";
	ESpell_name_by_value[ESpell::kBlackTentacles] = "kBlackTentacles";
	ESpell_name_by_value[ESpell::kWhirlwind] = "kWhirlwind";
	ESpell_name_by_value[ESpell::kIndriksTeeth] = "kIndriksTeeth";
	ESpell_name_by_value[ESpell::kAcidArrow] = "kAcidArrow";
	ESpell_name_by_value[ESpell::kThunderStone] = "kThunderStone";
	ESpell_name_by_value[ESpell::kClod] = "kClod";
	ESpell_name_by_value[ESpell::kExpedient] = "kExpedient";
	ESpell_name_by_value[ESpell::kSightOfDarkness] = "kSightOfDarkness";
	ESpell_name_by_value[ESpell::kGroupSincerity] = "kGroupSincerity";
	ESpell_name_by_value[ESpell::kMagicalGaze] = "kMagicalGaze";
	ESpell_name_by_value[ESpell::kAllSeeingEye] = "kAllSeeingEye";
	ESpell_name_by_value[ESpell::kEyeOfGods] = "kEyeOfGods";
	ESpell_name_by_value[ESpell::kBreathingAtDepth] = "kBreathingAtDepth";
	ESpell_name_by_value[ESpell::kGeneralRecovery] = "kGeneralRecovery";
	ESpell_name_by_value[ESpell::kCommonMeal] = "kCommonMeal";
	ESpell_name_by_value[ESpell::kStoneWall] = "kStoneWall";
	ESpell_name_by_value[ESpell::kSnakeEyes] = "kSnakeEyes";
	ESpell_name_by_value[ESpell::kEarthAura] = "kEarthAura";
	ESpell_name_by_value[ESpell::kGroupProtectFromEvil] = "kGroupProtectFromEvil";
	ESpell_name_by_value[ESpell::kArrowsFire] = "kArrowsFire";
	ESpell_name_by_value[ESpell::kArrowsWater] = "kArrowsWater";
	ESpell_name_by_value[ESpell::kArrowsEarth] = "kArrowsEarth";
	ESpell_name_by_value[ESpell::kArrowsAir] = "kArrowsAir";
	ESpell_name_by_value[ESpell::kArrowsDeath] = "kArrowsDeath";
	ESpell_name_by_value[ESpell::kPaladineInspiration] = "kPaladineInspiration";
	ESpell_name_by_value[ESpell::kDexterity] = "kDexterity";
	ESpell_name_by_value[ESpell::kGroupBlink] = "kGroupBlink";
	ESpell_name_by_value[ESpell::kGroupCloudly] = "kGroupCloudly";
	ESpell_name_by_value[ESpell::kGroupAwareness] = "kGroupAwareness";
	ESpell_name_by_value[ESpell::kMassFailure] = "kMassFailure";
	ESpell_name_by_value[ESpell::kSnare] = "kSnare";
	ESpell_name_by_value[ESpell::kExpedientFail] = "kExpedientFail";
	ESpell_name_by_value[ESpell::kLowerEffectiveness] = "kLowerEffectiveness";
	ESpell_name_by_value[ESpell::kConfuse] = "kConfuse";
	ESpell_name_by_value[ESpell::kNoInjure] = "kNoInjure";
	ESpell_name_by_value[ESpell::kFireBreath] = "kFireBreath";
	ESpell_name_by_value[ESpell::kGasBreath] = "kGasBreath";
	ESpell_name_by_value[ESpell::kFrostBreath] = "kFrostBreath";
	ESpell_name_by_value[ESpell::kAcidBreath] = "kAcidBreath";
	ESpell_name_by_value[ESpell::kLightingBreath] = "kLightingBreath";
	ESpell_name_by_value[ESpell::kIdentify] = "kIdentify";
	ESpell_name_by_value[ESpell::kFullIdentify] = "kFullIdentify";
	ESpell_name_by_value[ESpell::kQUest] = "kQUest";
	ESpell_name_by_value[ESpell::kPortalTimer] = "kPortalTimer";
	ESpell_name_by_value[ESpell::kNoCharge] = "kNoCharge";
	ESpell_name_by_value[ESpell::kDazzle] = "kDazzle";
	ESpell_name_by_value[ESpell::kGreatHeal] = "kGreatHeal";
	ESpell_name_by_value[ESpell::kFrenzy] = "kFrenzy";

	for (const auto &i : ESpell_name_by_value) {
		ESpell_value_by_name[i.second] = i.first;
	}

	// "kDefault" is an alias for kUndefined: the default sheaf id in spell_msg.xml
	// (issue #3304). kUndefined keeps its canonical name "kUndefined" for NAME_BY_ITEM.
	ESpell_value_by_name["kDefault"] = ESpell::kUndefined;
}

template<>
const std::string &NAME_BY_ITEM<ESpell>(const ESpell item) {
	if (ESpell_name_by_value.empty()) {
		init_ESpell_ITEM_NAMES();
	}
	return ESpell_name_by_value.at(item);
}

template<>
ESpell ITEM_BY_NAME(const std::string &name) {
	if (ESpell_name_by_value.empty()) {
		init_ESpell_ITEM_NAMES();
	}
	return ESpell_value_by_name.at(name);
}

const ESpell &operator++(ESpell &s) {
	s = static_cast<ESpell>(to_underlying(s) + 1);
	return s;
}

std::ostream& operator<<(std::ostream &os, const ESpell &s){
	os << to_underlying(s) << " (" << NAME_BY_ITEM<ESpell>(s) << ")";
	return os;
};

typedef std::map<EElement, std::string> EElement_name_by_value_t;
typedef std::map<const std::string, EElement> EElement_value_by_name_t;
EElement_name_by_value_t EElement_name_by_value;
EElement_value_by_name_t EElement_value_by_name;
void init_EElement_ITEM_NAMES() {
	EElement_value_by_name.clear();
	EElement_name_by_value.clear();

	EElement_name_by_value[EElement::kUndefined] = "kUndefined";
	EElement_name_by_value[EElement::kAir] = "kAir";
	EElement_name_by_value[EElement::kFire] = "kFire";
	EElement_name_by_value[EElement::kWater] = "kWater";
	EElement_name_by_value[EElement::kEarth] = "kEarth";
	EElement_name_by_value[EElement::kLight] = "kLight";
	EElement_name_by_value[EElement::kDark] = "kDark";
	EElement_name_by_value[EElement::kMind] = "kMind";
	EElement_name_by_value[EElement::kLife] = "kLife";

	for (const auto &i : EElement_name_by_value) {
		EElement_value_by_name[i.second] = i.first;
	}
}

template<>
const std::string &NAME_BY_ITEM<EElement>(const EElement item) {
	if (EElement_name_by_value.empty()) {
		init_EElement_ITEM_NAMES();
	}
	return EElement_name_by_value.at(item);
}

template<>
EElement ITEM_BY_NAME(const std::string &name) {
	if (EElement_name_by_value.empty()) {
		init_EElement_ITEM_NAMES();
	}
	return EElement_value_by_name.at(name);
}

typedef std::map<ESpellType, std::string> ESpellType_name_by_value_t;
typedef std::map<const std::string, ESpellType> ESpellType_value_by_name_t;
ESpellType_name_by_value_t ESpellType_name_by_value;
ESpellType_value_by_name_t ESpellType_value_by_name;
void init_ESpellType_ITEM_NAMES() {
	ESpellType_value_by_name.clear();
	ESpellType_name_by_value.clear();

	ESpellType_name_by_value[ESpellType::kKnow] = "kKnow";
	ESpellType_name_by_value[ESpellType::kTemp] = "kTemp";
/*	ESpellType_name_by_value[ESpellType::kRunes] = "kRunes";
	ESpellType_name_by_value[ESpellType::kItemCast] = "kItemCast";
	ESpellType_name_by_value[ESpellType::kPotionCast] = "kPotionCast";
	ESpellType_name_by_value[ESpellType::kWandCast] = "kWandCast";*/

	for (const auto &i : ESpellType_name_by_value) {
		ESpellType_value_by_name[i.second] = i.first;
	}
}

template<>
const std::string &NAME_BY_ITEM<ESpellType>(const ESpellType item) {
	if (ESpellType_name_by_value.empty()) {
		init_ESpellType_ITEM_NAMES();
	}
	return ESpellType_name_by_value.at(item);
}

template<>
ESpellType ITEM_BY_NAME(const std::string &name) {
	if (ESpellType_name_by_value.empty()) {
		init_ESpellType_ITEM_NAMES();
	}
	return ESpellType_value_by_name.at(name);
}

typedef std::map<EMagic, std::string> EMagic_name_by_value_t;
typedef std::map<const std::string, EMagic> EMagic_value_by_name_t;
EMagic_name_by_value_t EMagic_name_by_value;
EMagic_value_by_name_t EMagic_value_by_name;
void init_EMagic_ITEM_NAMES() {
	EMagic_value_by_name.clear();
	EMagic_name_by_value.clear();

	EMagic_name_by_value[EMagic::kMagDamage] = "kMagDamage";
	EMagic_name_by_value[EMagic::kMagAffects] = "kMagAffects";
	EMagic_name_by_value[EMagic::kMagUnaffects] = "kMagUnaffects";
	EMagic_name_by_value[EMagic::kMagPoints] = "kMagPoints";
	EMagic_name_by_value[EMagic::kMagAlterObjs] = "kMagAlterObjs";
	EMagic_name_by_value[EMagic::kMagGroups] = "kMagGroups";
	EMagic_name_by_value[EMagic::kMagMasses] = "kMagMasses";
	EMagic_name_by_value[EMagic::kMagAreas] = "kMagAreas";
	EMagic_name_by_value[EMagic::kMagSummons] = "kMagSummons";
	EMagic_name_by_value[EMagic::kMagCreations] = "kMagCreations";
	EMagic_name_by_value[EMagic::kMagManual] = "kMagManual";
	EMagic_name_by_value[EMagic::kMagWarcry] = "kMagWarcry";
	EMagic_name_by_value[EMagic::kMagNeedControl] = "kMagNeedControl";
	EMagic_name_by_value[EMagic::kMagCharRelocate] = "kMagCharRelocate";
	EMagic_name_by_value[EMagic::kNpcDamagePc] = "kNpcDamagePc";
	EMagic_name_by_value[EMagic::kNpcDamagePcMinhp] = "kNpcDamagePcMinhp";
	EMagic_name_by_value[EMagic::kNpcAffectPc] = "kNpcAffectPc";
	EMagic_name_by_value[EMagic::kNpcAffectPcCaster] = "kNpcAffectPcCaster";
	EMagic_name_by_value[EMagic::kNpcAffectNpc] = "kNpcAffectNpc";
	EMagic_name_by_value[EMagic::kNpcUnaffectNpc] = "kNpcUnaffectNpc";
	EMagic_name_by_value[EMagic::kNpcUnaffectNpcCaster] = "kNpcUnaffectNpcCaster";
	EMagic_name_by_value[EMagic::kNpcDummy] = "kNpcDummy";
	EMagic_name_by_value[EMagic::kMagRoom] = "kMagRoom";
	EMagic_name_by_value[EMagic::kMagCasterInroom] = "kMagCasterInroom";
	EMagic_name_by_value[EMagic::kMagCasterInworld] = "kMagCasterInworld";
	EMagic_name_by_value[EMagic::kMagCasterAnywhere] = "kMagCasterAnywhere";
	EMagic_name_by_value[EMagic::kMagCasterInworldDelay] = "kMagCasterInworldDelay";

	for (const auto &i : EMagic_name_by_value) {
		EMagic_value_by_name[i.second] = i.first;
	}
}

template<>
const std::string &NAME_BY_ITEM<EMagic>(const EMagic item) {
	if (EMagic_name_by_value.empty()) {
		init_EMagic_ITEM_NAMES();
	}
	return EMagic_name_by_value.at(item);
}

template<>
EMagic ITEM_BY_NAME(const std::string &name) {
	if (EMagic_name_by_value.empty()) {
		init_EMagic_ITEM_NAMES();
	}
	return EMagic_value_by_name.at(name);
}

typedef std::map<ETarget, std::string> ETarget_name_by_value_t;
typedef std::map<const std::string, ETarget> ETarget_value_by_name_t;
ETarget_name_by_value_t ETarget_name_by_value;
ETarget_value_by_name_t ETarget_value_by_name;
void init_ETarget_ITEM_NAMES() {
	ETarget_value_by_name.clear();
	ETarget_name_by_value.clear();

	ETarget_name_by_value[ETarget::kTarNone] = "kTarNone";
	ETarget_name_by_value[ETarget::kTarIgnore] = "kTarIgnore";
	ETarget_name_by_value[ETarget::kTarCharRoom] = "kTarCharRoom";
	ETarget_name_by_value[ETarget::kTarCharWorld] = "kTarCharWorld";
	ETarget_name_by_value[ETarget::kTarFightSelf] = "kTarFightSelf";
	ETarget_name_by_value[ETarget::kTarFightVict] = "kTarFightVict";
	ETarget_name_by_value[ETarget::kTarSelfOnly] = "kTarSelfOnly";
	ETarget_name_by_value[ETarget::kTarNotSelf] = "kTarNotSelf";
	ETarget_name_by_value[ETarget::kTarObjInv] = "kTarObjInv";
	ETarget_name_by_value[ETarget::kTarObjRoom] = "kTarObjRoom";
	ETarget_name_by_value[ETarget::kTarObjWorld] = "kTarObjWorld";
	ETarget_name_by_value[ETarget::kTarObjEquip] = "kTarObjEquip";
	ETarget_name_by_value[ETarget::kTarRoomThis] = "kTarRoomThis";
	ETarget_name_by_value[ETarget::kTarRoomDir] = "kTarRoomDir";
	ETarget_name_by_value[ETarget::kTarRoomWorld] = "kTarRoomWorld";
	ETarget_name_by_value[ETarget::kTarAllyOnly] = "kTarAllyOnly";

	for (const auto &i : ETarget_name_by_value) {
		ETarget_value_by_name[i.second] = i.first;
	}
}

template<>
const std::string &NAME_BY_ITEM<ETarget>(const ETarget item) {
	if (ETarget_name_by_value.empty()) {
		init_ETarget_ITEM_NAMES();
	}
	return ETarget_name_by_value.at(item);
}

template<>
ETarget ITEM_BY_NAME(const std::string &name) {
	if (ETarget_name_by_value.empty()) {
		init_ETarget_ITEM_NAMES();
	}
	return ETarget_value_by_name.at(name);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
