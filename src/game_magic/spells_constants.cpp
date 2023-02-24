/**
\authors Created by Sventovit
\date 27.04.2022.
\brief Константы системы заклинаний.
*/

#include "spells_constants.h"

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
		{ESpell::kAconitumPoison, "В вашей крови не осталось ни капельки яда."},
		{ESpell::kScopolaPoison, "В вашей крови не осталось ни капельки яда."},
		{ESpell::kBelenaPoison, "В вашей крови не осталось ни капельки яда."},
		{ESpell::kDaturaPoison, "В вашей крови не осталось ни капельки яда."},
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
		{ESpell::kPortalTimer, "Пентаграмма медленно растаяла."}
	};

	if (!spell_to_text.contains(spell_id)) {
		std::stringstream log_text;
		log_text << "!нет сообщения при спадении аффекта под номером: " << static_cast<int>(spell_id) << "!";
		return log_text.str();
	}

	return spell_to_text.at(spell_id);
}

std::optional<CastPhraseList> GetCastPhrase(ESpell spell_id) {
	// маппинг заклинания в текстовую пару [для язычника, для христианина]
	static const std::map<ESpell, CastPhraseList> cast_to_text {
		{ESpell::kArmor, {"буде во прибежище", "... Он - помощь наша и защита наша."}},
		{ESpell::kTeleport, {"несите ветры", "... дух поднял меня и перенес меня."}},
		{ESpell::kBless, {"истягну умь крепостию", "... даст блага просящим у Него."}},
		{ESpell::kBlindness, {"Чтоб твои зенки вылезли!", "... поразит тебя Господь слепотою."}},
		{ESpell::kBurningHands, {"узри огонь!", "... простер руку свою к огню."}},
		{ESpell::kCallLighting, {"Разрази тебя Перун!", "... и путь для громоносной молнии."}},
		{ESpell::kCharm, {"умь полонить", "... слушай пастыря сваего, и уразумей."}},
		{ESpell::kChillTouch, {"хладну персты воскладаше", "... которые черны от льда."}},
		{ESpell::kClone, {"пусть будет много меня", "... и плодились, и весьма умножились."}},
		{ESpell::kIceBolts, {"хлад и мраз исторгнути", "... и из воды делается лед."}},
		{ESpell::kControlWeather, {"стихия подкоряшися", "... власть затворить небо, чтобы не шел дождь."}},
		{ESpell::kCreateFood, {"будовати снедь", "... это хлеб, который Господь дал вам в пищу."}},
		{ESpell::kCreateWater, {"напоиши влагой", "... и потекло много воды."}},
		{ESpell::kCureBlind, {"зряще узрите", "... и прозрят из тьмы и мрака глаза слепых."}},
		{ESpell::kCureCritic, {"гой еси", "... да зарубцуются гноища твои."}},
		{ESpell::kCureLight, {"малейше целити раны", "... да затянутся раны твои."}},
		{ESpell::kCurse, {"порча", "... проклят ты пред всеми скотами."}},
		{ESpell::kDetectAlign, {"узряще норов", "... и отделит одних от других, как пастырь отделяет овец от козлов."}},
		{ESpell::kDetectInvis, {"взор мечетный", "... ибо нет ничего тайного, что не сделалось бы явным."}},
		{ESpell::kDetectMagic, {"зряще ворожбу", "... покажись, ересь богопротивная."}},
		{ESpell::kDetectPoison, {"зряще трутизну", "... по плодам их узнаете их."}},
		{ESpell::kDispelEvil, {"долой нощи", "... грешников преследует зло, а праведникам воздается добром."}},
		{ESpell::kEarthquake, {"земля тутнет", "... в тот же час произошло великое землетрясение."}},
		{ESpell::kEnchantWeapon, {"ницовати стружие", "... укрепи сталь Божьим перстом."}},
		{ESpell::kEnergyDrain, {"преторгоста", "... да иссякнут соки, питающие тело."}},
		{ESpell::kFireball, {"огненну солнце", "... да ниспадет огонь с неба, и пожрет их."}},
		{ESpell::kHarm, {"згола скверна", "... и жестокою болью во всех костях твоих."}},
		{ESpell::kHeal, {"згола гой еси", "... тебе говорю, встань."}},
		{ESpell::kInvisible, {"низовати мечетно", "... ибо видимое временно, а невидимое вечно."}},
		{ESpell::kLightingBolt, {"грянет гром", "... и были громы и молнии."}},
		{ESpell::kLocateObject, {"рища, летая умом под облакы", "... ибо всякий просящий получает, и ищущий находит."}},
		{ESpell::kMagicMissile, {"ворожья стрела", "... остры стрелы Твои."}},
		{ESpell::kPoison, {"трутизна", "... и пошлю на них зубы зверей и яд ползающих по земле."}},
		{ESpell::kProtectFromEvil, {"супостат нощи", "... свет, который в тебе, да убоится тьма."}},
		{ESpell::kRemoveCurse, {"изыде порча", "... да простятся тебе прегрешения твои."}},
		{ESpell::kSanctuary, {"иже во святых", "... буде святым, аки Господь наш свят."}},
		{ESpell::kShockingGasp, {"воскладше огненну персты", "... и дано буде жечь врагов огнем."}},
		{ESpell::kSleep, {"иже дремлет", "... на веки твои тяжесть покладет."}},
		{ESpell::kStrength, {"будет силен", "... и человек разумный укрепляет силу свою."}},
		{ESpell::kSummon, {"кличу-велю", "... и послали за ним и призвали его."}},
		{ESpell::kPatronage, {"ибо будет угоден Богам", "... ибо спасет людей Своих от грехов их."}},
		{ESpell::kWorldOfRecall, {"с глаз долой исчезни", "... ступай с миром."}},
		{ESpell::kRemovePoison, {"изыде трутизна", "... именем Божьим, изгнати сгниенье из тела."}},
		{ESpell::kSenseLife, {"зряще живота", "... ибо нет ничего сокровенного, что не обнаружилось бы."}},
		{ESpell::kAnimateDead, {"живот изо праха створисте", "... и земля извергнет мертвецов."}},
		{ESpell::kDispelGood, {"свет сгинь", "... и тьма свет накроет."}},
		{ESpell::kGroupArmor, {"прибежище други", "... ибо кто Бог, кроме Господа, и кто защита, кроме Бога нашего?"}},
		{ESpell::kGroupHeal, {"други, гой еси", "... вам говорю, встаньте."}},
		{ESpell::kGroupRecall, {"исчезните с глаз моих", "... вам говорю, ступайте с миром."}},
		{ESpell::kInfravision, {"в нощи зряще", "...ибо ни днем, ни ночью сна не знают очи."}},
		{ESpell::kWaterwalk, {"по воде аки по суху", "... поднимись и ввергнись в море."}},
		{ESpell::kCureSerious, {"целите раны", "... да уменьшатся раны твои."}},
		{ESpell::kGroupStrength, {"други сильны", "... и даст нам Господь силу."}},
		{ESpell::kHold, {"аки околел", "... замри."}},
		{ESpell::kPowerHold, {"згола аки околел", "... замри надолго."}},
		{ESpell::kMassHold, {"их окалеть", "... замрите."}},
		{ESpell::kFly, {"летать зегзицею", "... и полетел, и понесся на крыльях ветра."}},
		{ESpell::kBrokenChains, {"вериги и цепи железные изорву", "... и цепи упали с рук его."}},
		{ESpell::kNoflee, {"опуташа в путины железны", "... да поищешь уйти, да не возможешь."}},
		{ESpell::kCreateLight, {"будовати светоч", "... да будет свет."}},
		{ESpell::kDarkness, {"тьмою прикрыты", "... тьма покроет землю."}},
		{ESpell::kStoneSkin, {"буде тверд аки камень", "... твердость ли камней твердость твоя?"}},
		{ESpell::kCloudly, {"мгла покрыла", "... будут как утренний туман."}},
		{ESpell::kSilence, {"типун тебе на язык!", "... да замкнутся уста твои."}},
		{ESpell::kLight, {"буде аки светоч", "... и да воссияет над ним свет!"}},
		{ESpell::kChainLighting, {"глаголят небеса", "... понесутся меткие стрелы молний из облаков."}},
		{ESpell::kFireBlast, {"створисте огненну струя", "... и ввергне их в озеро огненное."}},
		{ESpell::kGodsWrath, {"гнев божиа не минути", "... и воспламенится гнев Господа, и Он скоро истребит тебя."}},
		{ESpell::kWeaknes, {"буде чахнуть", "... и силу могучих ослабляет."}},
		{ESpell::kGroupInvisible, {"други, низовати мечетны",
								"... возвещай всем великую силу Бога. И, сказав сие, они стали невидимы."}},
		{ESpell::kShadowCloak, {"будут тени и туман, и мрак ночной", "... распростираются вечерние тени."}},
		{ESpell::kAcid, {"жги аки смола горячая", "... подобно мучению от скорпиона."}},
		{ESpell::kRepair, {"будь целым, аки прежде", "... заделаю трещины в ней и разрушенное восстановлю."}},
		{ESpell::kEnlarge, {"возросши к небу", "... и плоть выросла."}},
		{ESpell::kFear, {"падоша в тернии", "... убойся того, кто по убиении ввергнет в геенну."}},
		{ESpell::kSacrifice, {"да коснется тебя Чернобог", "... плоть твоя и тело твое будут истощены."}},
		{ESpell::kWeb, {"сети ловчи", "... терны и сети на пути коварного."}},
		{ESpell::kBlink, {"от стрел укрытие и от меча оборона", "...да защитит он себя."}},
		{ESpell::kRemoveHold, {"буде быстр аки прежде", "... встань, и ходи."}},
		{ESpell::kCamouflage, {"", ""}},
		{ESpell::kPowerBlindness, {"згола застить очеса", "... поразит тебя Господь слепотою навечно."}},
		{ESpell::kMassBlindness, {"их очеса непотребны", "... и Он поразил их слепотою."}},
		{ESpell::kPowerSilence, {"згола не прерчет", "... исходящее из уст твоих, да не осквернит слуха."}},
		{ESpell::kExtraHits, {"буде полон здоровья", "... крепкое тело лучше несметного богатства."}},
		{ESpell::kResurrection, {"воскресе из мертвых", "... оживут мертвецы Твои, восстанут мертвые тела!"}},
		{ESpell::kMagicShield, {"и ворога оберегись", "... руками своими да защитит он себя"}},
		{ESpell::kForbidden, {"вороги не войдут", "... ибо положена печать, и никто не возвращается."}},
		{ESpell::kMassSilence, {"их уста непотребны", "... да замкнутся уста ваши."}},
		{ESpell::kRemoveSilence, {"глаголите", "... слова из уст мудрого - благодать."}},
		{ESpell::kDamageLight, {"падош", "... будет чувствовать боль."}},
		{ESpell::kDamageSerious, {"скверна", "... постигнут тебя муки."}},
		{ESpell::kDamageCritic, {"сильна скверна", "... боль и муки схватили."}},
		{ESpell::kMassCurse, {"порча их", "... прокляты вы пред всеми скотами."}},
		{ESpell::kArmageddon, {"суд божиа не минути", "... какою мерою мерите, такою отмерено будет и вам."}},
		{ESpell::kGroupFly, {"крыла им створисте", "... и все летающие по роду их."}},
		{ESpell::kGroupBless, {"други, наполнися ратнаго духа", "... блажены те, слышащие слово Божие."}},
		{ESpell::kResfresh, {"буде свеж", "... не будет у него ни усталого, ни изнемогающего."}},
		{ESpell::kStunning, {"да обратит тебя Чернобог в мертвый камень!", "... и проклял его именем Господним."}},
		{ESpell::kHide, {"", ""}},
		{ESpell::kSneak, {"", ""}},
		{ESpell::kDrunked, {"", ""}},
		{ESpell::kAbstinent, {"", ""}},
		{ESpell::kFullFeed, {"брюхо полно", "... душа больше пищи, и тело - одежды."}},
		{ESpell::kColdWind, {"веют ветры", "... подует северный холодный ветер."}},
		{ESpell::kBattle, {"", ""}},
		{ESpell::kHaemorrhage, {"", ""}},
		{ESpell::kCourage, {"", ""}},
		{ESpell::kWaterbreath, {"не затвори темне березе", "... дух дышит, где хочет."}},
		{ESpell::kSlowdown, {"немочь", "...и помедлил еще семь дней других."}},
		{ESpell::kHaste, {"скор аки ястреб", "... поднимет его ветер и понесет, и он быстро побежит от него."}},
		{ESpell::kMassSlow, {"тернии им", "... загорожу путь их тернами."}},
		{ESpell::kGroupHaste, {"быстры аки ястребов стая", "... и они быстры как серны на горах."}},
		{ESpell::kGodsShield, {"Живый в помощи Вышняго", "... благословен буде Грядый во имя Господне."}},
		{ESpell::kFever, {"нутро снеде", "... и сделаются жестокие и кровавые язвы."}},
		{ESpell::kCureFever, {"Навь, очисти тело", "... хочу, очистись."}},
		{ESpell::kAwareness, {"око недреманно", "... не дам сна очам моим и веждам моим - дремания."}},
		{ESpell::kReligion, {"", ""}},
		{ESpell::kAirShield, {"Стрибог, даруй прибежище", "... защита от ветра и покров от непогоды."}},
		{ESpell::kPortal, {"буде путь короток", "... входите во врата Его."}},
		{ESpell::kDispellMagic, {"изыде ворожба", "... выйди, дух нечистый."}},
		{ESpell::kSummonKeeper, {"Сварог, даруй защитника", "... и благословен защитник мой!"}},
		{ESpell::kFastRegeneration, {"заживет, аки на собаке", "... нет богатства лучше телесного здоровья."}},
		{ESpell::kCreateWeapon, {"будовати стружие", "...вооружите из себя людей на войну"}},
		{ESpell::kFireShield, {"Хорс, даруй прибежище", "... душа горячая, как пылающий огонь."}},
		{ESpell::kRelocate, {"Стрибог, укажи путь...", "... указывай им путь, по которому они должны идти."}},
		{ESpell::kSummonFirekeeper, {"Дажьбог, даруй защитника", "... Ангел Мой с вами, и он защитник душ ваших."}},
		{ESpell::kIceShield, {"Морена, даруй прибежище", "... а снег и лед выдерживали огонь и не таяли."}},
		{ESpell::kIceStorm, {"торже, яко вихор", "... и град, величиною в талант, падет с неба."}},
		{ESpell::kLessening, {"буде мал аки мышь", "... плоть на нем пропадает."}},
		{ESpell::kShineFlash, {"засти очи им", "... свет пламени из средины огня."}},
		{ESpell::kMadness, {"згола яростен", "... и ярость его загорелась в нем."}},
		{ESpell::kGroupMagicGlass, {"гладь воды отразит", "... воздай им по делам их, по злым поступкам их."}},
		{ESpell::kCloudOfArrows, {"и будут стрелы молний, и зарницы в высях",
							   "... соберу на них бедствия и истощу на них стрелы Мои."}},
		{ESpell::kVacuum, {"Сдохни!", "... и услышав слова сии - пал бездыханен."}},
		{ESpell::kMeteorStorm, {"идти дождю стрелами", "... и камни, величиною в талант, падут с неба."}},
		{ESpell::kStoneHands, {"сильны велетов руки", "... рука Моя пребудет с ним, и мышца Моя укрепит его."}},
		{ESpell::kMindless, {"разум аки мутный омут", "... и безумие его с ним."}},
		{ESpell::kPrismaticAura, {"окружен радугой", "... явится радуга в облаке."}},
		{ESpell::kEviless, {"зло творяще", "... и ты воздашь им злом."}},
		{ESpell::kAirAura, {"Мать-земля, даруй защиту.", "... поклон тебе матушка земля."}},
		{ESpell::kFireAura, {"Сварог, даруй защиту.", "... и огонь низводит с неба."}},
		{ESpell::kIceAura, {"Морена, даруй защиту.", "... текущие холодные воды."}},
		{ESpell::kShock, {"будет слеп и глух, аки мертвец", "... кто делает или глухим, или слепым."}},
		{ESpell::kMagicGlass, {"Аз воздам!", "... и воздам каждому из вас."}},
		{ESpell::kGroupSanctuary, {"иже во святых, други", "... будьте святы, аки Господь наш свят."}},
		{ESpell::kGroupPrismaticAura, {"други, буде окружены радугой", "... взгляни на радугу, и прославь Сотворившего ее."}},
		{ESpell::kDeafness, {"оглохни", "... и глухота поразит тебя."}},
		{ESpell::kPowerDeafness, {"да застит уши твои", "... и будь глухим надолго."}},
		{ESpell::kRemoveDeafness, {"слушай глас мой", "... услышь слово Его."}},
		{ESpell::kMassDeafness, {"будьте глухи", "... и не будут слышать уши ваши."}},
		{ESpell::kDustStorm, {"пыль поднимется столбами", "... и пыль поглотит вас."}},
		{ESpell::kEarthfall, {"пусть каменья падут", "... и обрушатся камни с небес."}},
		{ESpell::kSonicWave, {"да невзлюбит тебя воздух", "... и даже воздух покарает тебя."}},
		{ESpell::kHolystrike, {"Велес, упокой мертвых",
							"... и предоставь мертвым погребать своих мертвецов."}},
		{ESpell::kSumonAngel, {"Боги, даруйте защитника", "... дабы уберег он меня от зла."}},
		{ESpell::kMassFear, {"Поврещу сташивые души их в скарядие!", "... и затмил ужас разум их."}},
		{ESpell::kFascination, {"Да пребудет с тобой вся краса мира!", "... и омолодил он, и украсил их."}},
		{ESpell::kCrying, {"Будут слезы твои, аки камень на сердце",
						"... и постигнет твой дух угнетение вечное."}},
		{ESpell::kOblivion, {"будь живот аки буява с шерстнями.",
						  "... опадет на тебя чернь страшная."}},
		{ESpell::kBurdenOfTime, {"Яко небытие нещадно к вам, али время вернулось вспять.",
							  "... и время не властно над ними."}},
		{ESpell::kGroupRefresh, {"Исполняше други силою!",
							  "...да не останется ни обделенного, ни обессиленного."}},
		{ESpell::kPeaceful, {"Избавь речь свою от недобрых слов, а ум - от крамольных мыслей.",
						  "... любите врагов ваших и благотворите ненавидящим вас."}},
		{ESpell::kMagicBattle, {"", ""}},
		{ESpell::kBerserk, {"", ""}},
		{ESpell::kStoneBones, {"Обращу кости их в твердый камень.",
							"...и тот, кто упадет на камень сей, разобьется."}},
		{ESpell::kRoomLight, {"Да буде СВЕТ !!!", "...ибо сказал МОНТЕР !!!"}},
		{ESpell::kDeadlyFog, {"Порча великая туманом обернись!", "...и смерть явись в тумане его."}},
		{ESpell::kThunderstorm, {"Абие велий вихрь деяти!",
							  "...творит молнии при дожде, изводит ветер из хранилищ Своих."}},
		{ESpell::kLightWalk, {"", ""}},
		{ESpell::kFailure, {"аще доля зла и удача немилостива", ".. и несчастен, и жалок, и нищ."}},
		{ESpell::kClanPray, {"", ""}},
		{ESpell::kGlitterDust, {"зрети супостат охабиша", "...и бросали пыль на воздух."}},
		{ESpell::kScream, {"язвень голки уведати", "...но в полночь раздался крик."}},
		{ESpell::kCatGrace, {"ристати споро", "...и не уязвит враг того, кто скор."}},
		{ESpell::kBullBody, {"руци яре ворога супротив", "...и мощь звериная жила в теле его."}},
		{ESpell::kSnakeWisdom, {"веси и зрети стези отай", "...и даровал мудрость ему."}},
		{ESpell::kGimmicry, {"клюка вящего улучити", "...ибо кто познал ум Господень?"}},
		{ESpell::kWarcryOfChallenge, {"Эй, псы шелудивые, керасти плешивые, ослопы беспорточные, мшицы задочные!",
								   "Эй, псы шелудивые, керасти плешивые, ослопы беспорточные, мшицы задочные!"}},
		{ESpell::kWarcryOfMenace, {"Покрошу-изувечу, душу выну и в блины закатаю!",
								"Покрошу-изувечу, душу выну и в блины закатаю!"}},
		{ESpell::kWarcryOfRage, {"Не отступим, други, они наше сало сперли!",
							  "Не отступим, други, они наше сало сперли!"}},
		{ESpell::kWarcryOfMadness, {"Всех убью, а сам$g останусь!", "Всех убью, а сам$g останусь!"}},
		{ESpell::kWarcryOfThunder, {"Шоб вас приподняло, да шлепнуло!!!", "Шоб вас приподняло да шлепнуло!!!"}},
		{ESpell::kWarcryOfDefence, {"В строй други, защитим животами Русь нашу!",
								 "В строй други, защитим животами Русь нашу!"}},
		{ESpell::kWarcryOfBattle, {"Дер-ржать строй, волчьи хвосты!", "Дер-ржать строй, волчьи хвосты!"}},
		{ESpell::kWarcryOfPower, {"Сарынь на кичку!", "Сарынь на кичку!"}},
		{ESpell::kWarcryOfBless, {"Стоять крепко! За нами Киев, Подол и трактир с пивом!!!",
							   "Стоять крепко! За нами Киев, Подол и трактир с пивом!!!"}},
		{ESpell::kWarcryOfCourage, {"Орлы! Будем биться как львы!", "Орлы! Будем биться как львы!"}},
		{ESpell::kRuneLabel, {"...пьсати черты и резы.", "...и Сам отошел от них на вержение камня."}},
		{ESpell::kAconitumPoison, {"трутизна", "... и пошлю на них зубы зверей и яд ползающих по земле."}},
		{ESpell::kScopolaPoison, {"трутизна", "... и пошлю на них зубы зверей и яд ползающих по земле."}},
		{ESpell::kBelenaPoison, {"трутизна", "... и пошлю на них зубы зверей и яд ползающих по земле."}},
		{ESpell::kDaturaPoison, {"трутизна", "... и пошлю на них зубы зверей и яд ползающих по земле."}},
		{ESpell::kTimerRestore, {"", ""}},
		{ESpell::kCombatLuck, {"", ""}},
		{ESpell::kBandage, {"", ""}},
		{ESpell::kNoBandage, {"", ""}},
		{ESpell::kCapable, {"", ""}},
		{ESpell::kStrangle, {"", ""}},
		{ESpell::kRecallSpells, {"", ""}},
		{ESpell::kHypnoticPattern, {"ажбо супостаты блазнити да клюковати",
								 "...и утроба его приготовляет обман."}},
		{ESpell::kSolobonus, {"", ""}},
		{ESpell::kVampirism, {"", ""}},
		{ESpell::kRestoration, {"Да прими вид прежний, якой был.",
							 ".. Воззри на предмет сей Отче и верни ему силу прежнюю."}},
		{ESpell::kDeathAura, {"Надели силою своею Навь, дабы собрать урожай тебе.",
						   "...налякай ворогов наших и покарай их немощью."}},
		{ESpell::kRecovery, {"Обрасти плотью сызнова.", "... прости Господи грехи, верни плоть созданию."}},
		{ESpell::kMassRecovery, {"Обрастите плотью сызнова.",
							  "... прости Господи грехи, верни плоть созданиям."}},
		{ESpell::kAuraOfEvil, {"Возьми личину зла для жатвы славной.", "Надели силой злою во благо."}},
		{ESpell::kMentalShadow, {"Силою мысли защиту будую себе.",
							  "Даруй Отче защиту, силой разума воздвигнутую."}},
		{ESpell::kBlackTentacles, {"Ато егоже руци попасти.",
								"И он не знает, что мертвецы там и что в глубине..."}},
		{ESpell::kWhirlwind, {"Вждати бурю обло створити.", "И поднялась великая буря..."}},
		{ESpell::kIndriksTeeth, {"Идеже индрика зубы супостаты изъмати.",
							  "Есть род, у которого зубы - мечи и челюсти - ножи..."}},
		{ESpell::kAcidArrow, {"Варно сожжет струя!",
						   "...и на коже его сделаются как бы язвы проказы"}},
		{ESpell::kThunderStone, {"Небесе тутнет!", "...и взял оттуда камень, и бросил из пращи."}},
		{ESpell::kClod, {"Онома утес низринется!",
					  "...доколе камень не оторвался от горы без содействия рук."}},
		{ESpell::kExpedient, {"!Применил боевой прием!", "!use battle expedient!"}},
		{ESpell::kSightOfDarkness, {"Что свет, что тьма - глазу одинаково.",
								 "Станьте зрячи в тьме кромешной!"}},
		{ESpell::kGroupSincerity, {"...да не скроются намерения.",
								"И узрим братья намерения окружающих."}},
		{ESpell::kMagicalGaze, {"Узрим же все, что с магией навкруги нас.",
							 "Покажи, Спаситель, магические силы братии."}},
		{ESpell::kAllSeeingEye, {"Все тайное станет явным.",
							  "Не спрячется, не скроется, ни заяц, ни блоха."}},
		{ESpell::kEyeOfGods, {"Осязаемое откройся взору!",
						   "Да не скроется от взора вашего, ни одна живая душа."}},
		{ESpell::kBreathingAtDepth, {"Аки стайка рыбок, плывите вместе.",
								  "Что в воде, что на земле, воздух свежим будет."}},
		{ESpell::kGeneralRecovery, {"...дабы пройти вместе не одну сотню верст",
								 "Сохрани Отче от усталости детей своих!"}},
		{ESpell::kCommonMeal, {"Благодарите богов за хлеб и соль!",
							"...дабы не осталось голодающих на свете белом"}},
		{ESpell::kStoneWall, {"Станем други крепки як николы!", "Укрепим тела наши перед битвой!"}},
		{ESpell::kSnakeEyes, {"Что яд, а что мед. Не обманемся!",
						   "...и самый сильный яд станет вам виден."}},
		{ESpell::kEarthAura, {"Велес, даруй защиту.", "... земля благословенна твоя."}},
		{ESpell::kGroupProtectFromEvil, {"други, супостат нощи",
									  "други, свет который в нас, да убоится тьма."}},
		{ESpell::kArrowsFire, {"!магический выстрел!", "!use battle expedient!"}},
		{ESpell::kArrowsWater, {"!магический выстрел!", "!use battle expedient!"}},
		{ESpell::kArrowsEarth, {"!магический выстрел!", "!use battle expedient!"}},
		{ESpell::kArrowsAir, {"!магический выстрел!", "!use battle expedient!"}},
		{ESpell::kArrowsDeath, {"!магический выстрел!", "!use battle expedient!"}},
		{ESpell::kPaladineInspiration, {"", ""}},
		{ESpell::kDexterity, {"будет ловким", "... и человек разумный укрепляет ловкость свою."}},
		{ESpell::kGroupBlink, {"защити нас от железа разящего", "... ни стрела, ни меч не пронзят печень вашу."}},
		{ESpell::kGroupCloudly, {"огрожу беззакония их туманом",
							  "...да защитит и покроет рассветная пелена тела ваши."}},
		{ESpell::kGroupAwareness, {"буде вежды ваши открыты", "... и забота о ближнем отгоняет сон от очей их."}},
		{ESpell::kWarcryOfExperience, {"найдем новизну в рутине сражений!", "найдем новизну в рутине сражений!"}},
		{ESpell::kWarcryOfLuck, {"и пусть удача будет нашей спутницей!", "и пусть удача будет нашей спутницей!"}},
		{ESpell::kWarcryOfPhysdamage, {"бей в глаз, не порти шкуру", "бей в глаз, не порти шкуру."}},
		{ESpell::kMassFailure, {"...отче Велес, очи отвержеши!",
							 "...надежда тщетна: не упадешь ли от одного взгляда его?"}},
		{ESpell::kSnare, {"Заклинати поврещение в сети заскопиены!",
					   "...будет трапеза их сетью им, и мирное пиршество их - западнею."}},
		{ESpell::kExpedientFail, {"!Провалил боевой прием!",
							   "!Провалил боевой прием!"}}
	};

	if (!cast_to_text.contains(spell_id)) {
		return std::nullopt;
	}
	return cast_to_text.at(spell_id);
}

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
	ESpell_name_by_value[ESpell::kFireBreath] = "kFireBreath";
	ESpell_name_by_value[ESpell::kGasBreath] = "kGasBreath";
	ESpell_name_by_value[ESpell::kFrostBreath] = "kFrostBreath";
	ESpell_name_by_value[ESpell::kAcidBreath] = "kAcidBreath";
	ESpell_name_by_value[ESpell::kLightingBreath] = "kLightingBreath";
	ESpell_name_by_value[ESpell::kIdentify] = "kIdentify";
	ESpell_name_by_value[ESpell::kFullIdentify] = "kFullIdentify";
	ESpell_name_by_value[ESpell::kQUest] = "kQUest";
	ESpell_name_by_value[ESpell::kPortalTimer] = "kPortalTimer";

	for (const auto &i : ESpell_name_by_value) {
		ESpell_value_by_name[i.second] = i.first;
	}
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

	EMagic_name_by_value[EMagic::kMagNone] = "kMagNone";
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
