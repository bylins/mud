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
		{kSpellArmor, "Вы почувствовали себя менее защищенно."},
		{kSpellTeleport, "!Teleport!"},
		{kSpellBless, "Вы почувствовали себя менее доблестно."},
		{kSpellBlindness, "Вы вновь можете видеть."},
		{kSpellBurningHands, "!Burning Hands!"},
		{kSpellCallLighting, "!Call Lightning"},
		{kSpellCharm, "Вы подчиняетесь теперь только себе."},
		{kSpellChillTouch, "Вы отметили, что силы вернулись к вам."},
		{kSpellClone, "!Clone!"},
		{kSpellIceBolts, "!Color Spray!"},
		{kSpellControlWeather, "!Control Weather!"},
		{kSpellCreateFood, "!Create Food!"},
		{kSpellCreateWater, "!Create Water!"},
		{kSpellCureBlind, "!Cure Blind!"},
		{kSpellCureCritic, "!Cure Critic!"},
		{kSpellCureLight, "!Cure Light!"},
		{kSpellCurse, "Вы почувствовали себя более уверенно."},
		{kSpellDetectAlign, "Вы более не можете определять наклонности."},
		{kSpellDetectInvis, "Вы не в состоянии больше видеть невидимых."},
		{kSpellDetectMagic, "Вы не в состоянии более определять магию."},
		{kSpellDetectPoison, "Вы не в состоянии более определять яды."},
		{kSpellDispelEvil, "!Dispel Evil!"},
		{kSpellEarthquake, "!Earthquake!"},
		{kSpellEnchantWeapon, "!Enchant Weapon!"},
		{kSpellEnergyDrain, "!Energy Drain!"},
		{kSpellFireball, "!Fireball!"},
		{kSpellHarm, "!Harm!"},
		{kSpellHeal, "!Heal!"},
		{kSpellInvisible, "Вы вновь видимы."},
		{kSpellLightingBolt, "!Lightning Bolt!"},
		{kSpellLocateObject, "!Locate object!"},
		{kSpellMagicMissile, "!Magic Missile!"},
		{kSpellPoison, "В вашей крови не осталось ни капельки яда."},
		{kSpellProtectFromEvil, "Вы вновь ощущаете страх перед тьмой."},
		{kSpellRemoveCurse, "!Remove Curse!"},
		{kSpellSanctuary, "Белая аура вокруг вашего тела угасла."},
		{kSpellShockingGasp, "!Shocking Grasp!"},
		{kSpellSleep, "Вы не чувствуете сонливости."},
		{kSpellStrength, "Вы чувствуете себя немного слабее."},
		{kSpellSummon, "!Summon!"},
		{kSpellPatronage, "Вы утратили покровительство высших сил."},
		{kSpellWorldOfRecall, "!Word of Recall!"},
		{kSpellRemovePoison, "!Remove Poison!"},
		{kSpellSenseLife, "Вы больше не можете чувствовать жизнь."},
		{kSpellAnimateDead, "!Animate Dead!"},
		{kSpellDispelGood, "!Dispel Good!"},
		{kSpellGroupArmor, "!Group Armor!"},
		{kSpellGroupHeal, "!Group Heal!"},
		{kSpellGroupRecall, "!Group Recall!"},
		{kSpellInfravision, "Вы больше не можете видеть ночью."},
		{kSpellWaterwalk, "Вы больше не можете ходить по воде."},
		{kSpellCureSerious, "!SPELL CURE SERIOUS!"},
		{kSpellGroupStrength, "!SPELL GROUP STRENGTH!"},
		{kSpellHold, "К вам вернулась способность двигаться."},
		{kSpellPowerHold, "!SPELL POWER HOLD!"},
		{kSpellMassHold, "!SPELL MASS HOLD!"},
		{kSpellFly, "Вы приземлились на землю."},
		{kSpellBrokenChains, "Вы вновь стали уязвимы для оцепенения."},
		{kSpellNoflee, "Вы опять можете сбежать с поля боя."},
		{kSpellCreateLight, "!SPELL CREATE LIGHT!"},
		{kSpellDarkness, "Облако тьмы, окружающее вас, спало."},
		{kSpellStoneSkin, "Ваша кожа вновь стала мягкой и бархатистой."},
		{kSpellCloudly, "Ваши очертания приобрели отчетливость."},
		{kSpellSllence, "Теперь вы можете болтать, все что думаете."},
		{kSpellLight, "Ваше тело перестало светиться."},
		{kSpellChainLighting, "!SPELL CHAIN LIGHTNING!"},
		{kSpellFireBlast, "!SPELL FIREBLAST!"},
		{kSpellGodsWrath, "!SPELL IMPLOSION!"},
		{kSpellWeaknes, "Силы вернулись к вам."},
		{kSpellGroupInvisible, "!SPELL GROUP INVISIBLE!"},
		{kSpellShadowCloak, "Ваша теневая мантия замерцала и растаяла."},
		{kSpellAcid, "!SPELL ACID!"},
		{kSpellRepair, "!SPELL REPAIR!"},
		{kSpellEnlarge, "Ваши размеры стали прежними."},
		{kSpellFear, "!SPELL FEAR!"},
		{kSpellSacrifice, "!SPELL SACRIFICE!"},
		{kSpellWeb, "Магическая сеть, покрывавшая вас, исчезла."},
		{kSpellBlink, "Вы перестали мигать."},
		{kSpellRemoveHold, "!SPELL REMOVE HOLD!"},
		{kSpellCamouflage, "Вы стали вновь похожи сами на себя."},
		{kSpellPowerBlindness, "!SPELL POWER BLINDNESS!"},
		{kSpellMassBlindness, "!SPELL MASS BLINDNESS!"},
		{kSpellPowerSilence, "!SPELL POWER SIELENCE!"},
		{kSpellExtraHits, "!SPELL EXTRA HITS!"},
		{kSpellResurrection, "!SPELL RESSURECTION!"},
		{kSpellMagicShield, "Ваш волшебный щит рассеялся."},
		{kSpellForbidden, "Магия, запечатывающая входы, пропала."},
		{kSpellMassSilence, "!SPELL MASS SIELENCE!"},
		{kSpellRemoveSilence, "!SPELL REMOVE SIELENCE!"},
		{kSpellDamageLight, "!SPELL DAMAGE LIGHT!"},
		{kSpellDamageSerious, "!SPELL DAMAGE SERIOUS!"},
		{kSpellDamageCritic, "!SPELL DAMAGE CRITIC!"},
		{kSpellMassCurse, "!SPELL MASS CURSE!"},
		{kSpellArmageddon, "!SPELL ARMAGEDDON!"},
		{kSpellGroupFly, "!SPELL GROUP FLY!"},
		{kSpellGroupBless, "!SPELL GROUP BLESS!"},
		{kSpellResfresh, "!SPELL REFRESH!"},
		{kSpellStunning, "!SPELL STUNNING!"},
		{kSpellHide, "Вы стали заметны окружающим."},
		{kSpellSneak, "Ваши передвижения стали заметны."},
		{kSpellDrunked, "Кураж прошел. Мама, лучше бы я умер$q вчера."},
		{kSpellAbstinent, "А головка ваша уже не болит."},
		{kSpellFullFeed, "Вам снова захотелось жареного, да с дымком."},
		{kSpellColdWind, "Вы согрелись и подвижность вернулась к вам."},
		{kSpellBattle, "К вам вернулась способность нормально сражаться."},
		{kSpellHaemorrhage, "Ваши кровоточащие раны затянулись."},
		{kSpellCourage, "Вы успокоились."},
		{kSpellWaterbreath, "Вы более не способны дышать водой."},
		{kSpellSlowdown, "Медлительность исчезла."},
		{kSpellHaste, "Вы стали более медлительны."},
		{kSpellMassSlow, "!SPELL MASS SLOW!"},
		{kSpellGroupHaste, "!SPELL MASS HASTE!"},
		{kSpellGodsShield, "Голубой кокон вокруг вашего тела угас."},
		{kSpellFever, "Лихорадка прекратилась."},
		{kSpellCureFever, "!SPELL CURE PLAQUE!"},
		{kSpellAwareness, "Вы стали менее внимательны."},
		{kSpellReligion, "Вы утратили расположение Богов."},
		{kSpellAirShield, "Ваш воздушный щит исчез."},
		{kSpellPortal, "!PORTAL!"},
		{kSpellDispellMagic, "!DISPELL MAGIC!"},
		{kSpellSummonKeeper, "!SUMMON KEEPER!"},
		{kSpellFastRegeneration, "Живительная сила покинула вас."},
		{kSpellCreateWeapon, "!CREATE WEAPON!"},
		{kSpellFireShield, "Огненный щит вокруг вашего тела исчез."},
		{kSpellRelocate, "!RELOCATE!"},
		{kSpellSummonFirekeeper, "!SUMMON FIREKEEPER!"},
		{kSpellIceShield, "Ледяной щит вокруг вашего тела исчез."},
		{kSpellIceStorm, "Ваши мышцы оттаяли и вы снова можете двигаться."},
		{kSpellLessening, "Ваши размеры вновь стали прежними."},
		{kSpellShineFlash, "!SHINE LIGHT!"},
		{kSpellMadness, "Безумие боя отпустило вас."},
		{kSpellGroupMagicGlass, "!GROUP MAGICGLASS!"},
		{kSpellCloudOfArrows, "Облако стрел вокруг вас рассеялось."},
		{kSpellVacuum, "!VACUUM!"},
		{kSpellMeteorStorm, "Последний громовой камень грянул в землю и все стихло."},
		{kSpellStoneHands, "Ваши руки вернулись к прежнему состоянию."},
		{kSpellMindless, "Ваш разум просветлел."},
		{kSpellPrismaticAura, "Призматическая аура вокруг вашего тела угасла."},
		{kSpellEviless, "Силы зла оставили вас."},
		{kSpellAirAura, "Воздушная аура вокруг вас исчезла."},
		{kSpellFireAura, "Огненная аура вокруг вас исчезла."},
		{kSpellIceAura, "Ледяная аура вокруг вас исчезла."},
		{kSpellShock, "!SHOCK!"},
		{kSpellMagicGlass, "Вы вновь чувствительны к магическим поражениям."},
		{kSpellGroupSanctuary, "!SPELL GROUP SANCTUARY!"},
		{kSpellGroupPrismaticAura, "!SPELL GROUP PRISMATICAURA!"},
		{kSpellDeafness, "Вы вновь можете слышать."},
		{kSpellPowerDeafness, "!SPELL_POWER_DEAFNESS!"},
		{kSpellRemoveDeafness, "!SPELL_REMOVE_DEAFNESS!"},
		{kSpellMassDeafness, "!SPELL_MASS_DEAFNESS!"},
		{kSpellDustStorm, "!SPELL_DUSTSTORM!"},
		{kSpellEarthfall, "!SPELL_EARTHFALL!"},
		{kSpellSonicWave, "!SPELL_SONICWAVE!"},
		{kSpellHolystrike, "!SPELL_HOLYSTRIKE!"},
		{kSpellSumonAngel, "!SPELL_SPELL_ANGEL!"},
		{kSpellMassFear, "!SPELL_SPELL_MASS_FEAR!"},
		{kSpellFascination, "Ваша красота куда-то пропала."},
		{kSpellCrying, "Ваша душа успокоилась."},
		{kSpellOblivion, "!SPELL_OBLIVION!"},
		{kSpellBurdenOfTime, "!SPELL_BURDEN_OF_TIME!"},
		{kSpellGroupRefresh, "!SPELL_GROUP_REFRESH!"},
		{kSpellPeaceful, "Смирение в вашей душе вдруг куда-то исчезло."},
		{kSpellMagicBattle, "К вам вернулась способность нормально сражаться."},
		{kSpellBerserk, "Неистовство оставило вас."},
		{kSpellStoneBones, "!stone bones!"},
		{kSpellRoomLight, "Колдовской свет угас."},
		{kSpellPoosinedFog, "Порыв ветра развеял ядовитый туман."},
		{kSpellThunderstorm, "Ветер прогнал грозовые тучи."},
		{kSpellLightWalk, "Ваши следы вновь стали заметны."},
		{kSpellFailure, "Удача вновь вернулась к вам."},
		{kSpellClanPray, "Магические чары ослабели со временем и покинули вас."},
		{kSpellGlitterDust, "Покрывавшая вас блестящая пыль осыпалась и растаяла в воздухе."},
		{kSpellScream, "Леденящий душу испуг отпустил вас."},
		{kSpellCatGrace, "Ваши движения утратили прежнюю колдовскую ловкость."},
		{kSpellBullBody, "Ваше телосложение вновь стало обычным."},
		{kSpellSnakeWisdom, "Вы утратили навеянную магией мудрость."},
		{kSpellGimmicry, "Навеянная магией хитрость покинула вас."},
		{kSpellWarcryOfChallenge, "!SPELL_WC_OF_CHALLENGE!"},
		{kSpellWarcryOfMenace, ""},
		{kSpellWarcryOfRage, "!SPELL_WC_OF_RAGE!"},
		{kSpellWarcryOfMadness, ""},
		{kSpellWarcryOfThunder, "!SPELL_WC_OF_THUNDER!"},
		{kSpellWarcryOfDefence, "Действие клича 'призыв к обороне' закончилось."},
		{kSpellWarcryOfBattle, "Действие клича битвы закончилось."},
		{kSpellWarcryOfPower, "Действие клича мощи закончилось."},
		{kSpellWarcryOfBless, "Действие клича доблести закончилось."},
		{kSpellWarcryOfCourage, "Действие клича отваги закончилось."},
		{kSpellRuneLabel, "Магические письмена на земле угасли."},
		{kSpellAconitumPoison, "В вашей крови не осталось ни капельки яда."},
		{kSpellScopolaPoison, "В вашей крови не осталось ни капельки яда."},
		{kSpellBelenaPoison, "В вашей крови не осталось ни капельки яда."},
		{kSpellDaturaPoison, "В вашей крови не осталось ни капельки яда."},
		{kSpellTimerRestore, "SPELL_TIMER_REPAIR"},
		{kSpellLucky, "!SPELL_LACKY!"},
		{kSpellBandage, "Вы аккуратно перевязали свои раны."},
		{kSpellNoBandage, "Вы снова можете перевязывать свои раны."},
		{kSpellCapable, "!SPELL_CAPABLE!"},
		{kSpellStrangle, "Удушье отпустило вас, и вы вздохнули полной грудью."},
		{kSpellRecallSpells, "Вам стало не на чем концентрироваться."},
		{kSpellHypnoticPattern, "Плывший в воздухе огненный узор потускнел и растаял струйками дыма."},
		{kSpellSolobonus, "Одна из наград прекратила действовать."},
		{kSpellVampirism, "!SPELL_VAMPIRE!"},
		{kSpellRestoration, "!SPELLS_RESTORATION!"},
		{kSpellDeathAura, "Силы нави покинули вас."},
		{kSpellRecovery, "!SPELL_RECOVERY!"},
		{kSpellMassRecovery, "!SPELL_MASS_RECOVERY!"},
		{kSpellAuraOfEvil, "Аура зла больше не помогает вам."},
		{kSpellMentalShadow, "!SPELL_MENTAL_SHADOW!"},
		{kSpellBlackTentacles, "Жуткие черные руки побледнели и расплылись зловонной дымкой."},
		{kSpellWhirlwind, "!SPELL_WHIRLWIND!"},
		{kSpellIndriksTeeth, "Каменные зубы исчезли, возвратив способность двигаться."},
		{kSpellAcidArrow, "!SPELL_MELFS_ACID_ARROW!"},
		{kSpellThunderStone, "!SPELL_THUNDERSTONE!"},
		{kSpellClod, "!SPELL_CLOD!"},
		{kSpellExpedient, "Эффект боевого приема завершился."},
		{kSpellSightOfDarkness, "!SPELL SIGHT OF DARKNESS!"},
		{kSpellGroupSincerity, "!SPELL GENERAL SINCERITY!"},
		{kSpellMagicalGaze, "!SPELL MAGICAL GAZE!"},
		{kSpellAllSeeingEye, "!SPELL ALL SEEING EYE!"},
		{kSpellEyeOfGods, "!SPELL EYE OF GODS!"},
		{kSpellBreathingAtDepth, "!SPELL BREATHING AT DEPTH!"},
		{kSpellGeneralRecovery, "!SPELL GENERAL RECOVERY!"},
		{kSpellCommonMeal, "!SPELL COMMON MEAL!"},
		{kSpellStoneWall, "!SPELL STONE WALL!"},
		{kSpellSnakeEyes, "!SPELL SNAKE EYES!"},
		{kSpellEarthAura, "Матушка земля забыла про Вас."},
		{kSpellGroupProtectFromEvil, "Вы вновь ощущаете страх перед тьмой."},
		{kSpellArrowsFire, "!NONE"},
		{kSpellArrowsWater, "!NONE"},
		{kSpellArrowsEarth, "!NONE"},
		{kSpellArrowsAir, "!NONE"},
		{kSpellArrowsDeath, "!NONE"},
		{kSpellPaladineInspiration, "*Боевое воодушевление угасло, а с ним и вся жажда подвигов!"},
		{kSpellDexterity, "Вы стали менее шустрым."},
		{kSpellGroupBlink, "!NONE"},
		{kSpellGroupCloudly, "!NONE"},
		{kSpellGroupAwareness, "!NONE"},
		{kSpellWatctyOfExpirence, "Действие клича 'обучение' закончилось."},
		{kSpellWarcryOfLuck, "Действие клича 'везение' закончилось."},
		{kSpellWarcryOfPhysdamage, "Действие клича 'точность' закончилось."},
		{kSpellMassFailure, "Удача снова повернулась к вам лицом... и залепила пощечину."},
		{kSpellSnare, "Покрывавшие вас сети колдовской западни растаяли."},
		{kSpellQUest, "Наложенные на вас чары рассеялись."},
		{kSpellExpedientFail, "Вы восстановили равновесие."}
	};

	if (!spell_to_text.contains(spell_id)) {
		std::stringstream log_text;
		log_text << "!нет сообщения при спадении аффекта под номером: " << static_cast<int>(spell_id) << "!";
		return log_text.str();
	}

	return spell_to_text.at(spell_id);
}

// TODO:refactor and replate int spell by ESpell
std::optional<CastPhraseList> GetCastPhrase(ESpell spell_id) {
	// маппинг заклинания в текстовую пару [для язычника, для христианина]
	static const std::map<ESpell, CastPhraseList> cast_to_text {
		{kSpellArmor, {"буде во прибежище", "... Он - помощь наша и защита наша."}},
		{kSpellTeleport, {"несите ветры", "... дух поднял меня и перенес меня."}},
		{kSpellBless, {"истягну умь крепостию", "... даст блага просящим у Него."}},
		{kSpellBlindness, {"Чтоб твои зенки вылезли!", "... поразит тебя Господь слепотою."}},
		{kSpellBurningHands, {"узри огонь!", "... простер руку свою к огню."}},
		{kSpellCallLighting, {"Разрази тебя Перун!", "... и путь для громоносной молнии."}},
		{kSpellCharm, {"умь полонить", "... слушай пастыря сваего, и уразумей."}},
		{kSpellChillTouch, {"хладну персты воскладаше", "... которые черны от льда."}},
		{kSpellClone, {"пусть будет много меня", "... и плодились, и весьма умножились."}},
		{kSpellIceBolts, {"хлад и мраз исторгнути", "... и из воды делается лед."}},
		{kSpellControlWeather, {"стихия подкоряшися", "... власть затворить небо, чтобы не шел дождь."}},
		{kSpellCreateFood, {"будовати снедь", "... это хлеб, который Господь дал вам в пищу."}},
		{kSpellCreateWater, {"напоиши влагой", "... и потекло много воды."}},
		{kSpellCureBlind, {"зряще узрите", "... и прозрят из тьмы и мрака глаза слепых."}},
		{kSpellCureCritic, {"гой еси", "... да зарубцуются гноища твои."}},
		{kSpellCureLight, {"малейше целити раны", "... да затянутся раны твои."}},
		{kSpellCurse, {"порча", "... проклят ты пред всеми скотами."}},
		{kSpellDetectAlign, {"узряще норов", "... и отделит одних от других, как пастырь отделяет овец от козлов."}},
		{kSpellDetectInvis, {"взор мечетный", "... ибо нет ничего тайного, что не сделалось бы явным."}},
		{kSpellDetectMagic, {"зряще ворожбу", "... покажись, ересь богопротивная."}},
		{kSpellDetectPoison, {"зряще трутизну", "... по плодам их узнаете их."}},
		{kSpellDispelEvil, {"долой нощи", "... грешников преследует зло, а праведникам воздается добром."}},
		{kSpellEarthquake, {"земля тутнет", "... в тот же час произошло великое землетрясение."}},
		{kSpellEnchantWeapon, {"ницовати стружие", "... укрепи сталь Божьим перстом."}},
		{kSpellEnergyDrain, {"преторгоста", "... да иссякнут соки, питающие тело."}},
		{kSpellFireball, {"огненну солнце", "... да ниспадет огонь с неба, и пожрет их."}},
		{kSpellHarm, {"згола скверна", "... и жестокою болью во всех костях твоих."}},
		{kSpellHeal, {"згола гой еси", "... тебе говорю, встань."}},
		{kSpellInvisible, {"низовати мечетно", "... ибо видимое временно, а невидимое вечно."}},
		{kSpellLightingBolt, {"грянет гром", "... и были громы и молнии."}},
		{kSpellLocateObject, {"рища, летая умом под облакы", "... ибо всякий просящий получает, и ищущий находит."}},
		{kSpellMagicMissile, {"ворожья стрела", "... остры стрелы Твои."}},
		{kSpellPoison, {"трутизна", "... и пошлю на них зубы зверей и яд ползающих по земле."}},
		{kSpellProtectFromEvil, {"супостат нощи", "... свет, который в тебе, да убоится тьма."}},
		{kSpellRemoveCurse, {"изыде порча", "... да простятся тебе прегрешения твои."}},
		{kSpellSanctuary, {"иже во святых", "... буде святым, аки Господь наш свят."}},
		{kSpellShockingGasp, {"воскладше огненну персты", "... и дано буде жечь врагов огнем."}},
		{kSpellSleep, {"иже дремлет", "... на веки твои тяжесть покладет."}},
		{kSpellStrength, {"будет силен", "... и человек разумный укрепляет силу свою."}},
		{kSpellSummon, {"кличу-велю", "... и послали за ним и призвали его."}},
		{kSpellPatronage, {"ибо будет угоден Богам", "... ибо спасет людей Своих от грехов их."}},
		{kSpellWorldOfRecall, {"с глаз долой исчезни", "... ступай с миром."}},
		{kSpellRemovePoison, {"изыде трутизна", "... именем Божьим, изгнати сгниенье из тела."}},
		{kSpellSenseLife, {"зряще живота", "... ибо нет ничего сокровенного, что не обнаружилось бы."}},
		{kSpellAnimateDead, {"живот изо праха створисте", "... и земля извергнет мертвецов."}},
		{kSpellDispelGood, {"свет сгинь", "... и тьма свет накроет."}},
		{kSpellGroupArmor, {"прибежище други", "... ибо кто Бог, кроме Господа, и кто защита, кроме Бога нашего?"}},
		{kSpellGroupHeal, {"други, гой еси", "... вам говорю, встаньте."}},
		{kSpellGroupRecall, {"исчезните с глаз моих", "... вам говорю, ступайте с миром."}},
		{kSpellInfravision, {"в нощи зряще", "...ибо ни днем, ни ночью сна не знают очи."}},
		{kSpellWaterwalk, {"по воде аки по суху", "... поднимись и ввергнись в море."}},
		{kSpellCureSerious, {"целите раны", "... да уменьшатся раны твои."}},
		{kSpellGroupStrength, {"други сильны", "... и даст нам Господь силу."}},
		{kSpellHold, {"аки околел", "... замри."}},
		{kSpellPowerHold, {"згола аки околел", "... замри надолго."}},
		{kSpellMassHold, {"их окалеть", "... замрите."}},
		{kSpellFly, {"летать зегзицею", "... и полетел, и понесся на крыльях ветра."}},
		{kSpellBrokenChains, {"вериги и цепи железные изорву", "... и цепи упали с рук его."}},
		{kSpellNoflee, {"опуташа в путины железны", "... да поищешь уйти, да не возможешь."}},
		{kSpellCreateLight, {"будовати светоч", "... да будет свет."}},
		{kSpellDarkness, {"тьмою прикрыты", "... тьма покроет землю."}},
		{kSpellStoneSkin, {"буде тверд аки камень", "... твердость ли камней твердость твоя?"}},
		{kSpellCloudly, {"мгла покрыла", "... будут как утренний туман."}},
		{kSpellSllence, {"типун тебе на язык!", "... да замкнутся уста твои."}},
		{kSpellLight, {"буде аки светоч", "... и да воссияет над ним свет!"}},
		{kSpellChainLighting, {"глаголят небеса", "... понесутся меткие стрелы молний из облаков."}},
		{kSpellFireBlast, {"створисте огненну струя", "... и ввергне их в озеро огненное."}},
		{kSpellGodsWrath, {"гнев божиа не минути", "... и воспламенится гнев Господа, и Он скоро истребит тебя."}},
		{kSpellWeaknes, {"буде чахнуть", "... и силу могучих ослабляет."}},
		{kSpellGroupInvisible, {"други, низовати мечетны",
								"... возвещай всем великую силу Бога. И, сказав сие, они стали невидимы."}},
		{kSpellShadowCloak, {"будут тени и туман, и мрак ночной", "... распростираются вечерние тени."}},
		{kSpellAcid, {"жги аки смола горячая", "... подобно мучению от скорпиона."}},
		{kSpellRepair, {"будь целым, аки прежде", "... заделаю трещины в ней и разрушенное восстановлю."}},
		{kSpellEnlarge, {"возросши к небу", "... и плоть выросла."}},
		{kSpellFear, {"падоша в тернии", "... убойся того, кто по убиении ввергнет в геенну."}},
		{kSpellSacrifice, {"да коснется тебя Чернобог", "... плоть твоя и тело твое будут истощены."}},
		{kSpellWeb, {"сети ловчи", "... терны и сети на пути коварного."}},
		{kSpellBlink, {"от стрел укрытие и от меча оборона", "...да защитит он себя."}},
		{kSpellRemoveHold, {"буде быстр аки прежде", "... встань, и ходи."}},
		{kSpellCamouflage, {"", ""}},
		{kSpellPowerBlindness, {"згола застить очеса", "... поразит тебя Господь слепотою навечно."}},
		{kSpellMassBlindness, {"их очеса непотребны", "... и Он поразил их слепотою."}},
		{kSpellPowerSilence, {"згола не прерчет", "... исходящее из уст твоих, да не осквернит слуха."}},
		{kSpellExtraHits, {"буде полон здоровья", "... крепкое тело лучше несметного богатства."}},
		{kSpellResurrection, {"воскресе из мертвых", "... оживут мертвецы Твои, восстанут мертвые тела!"}},
		{kSpellMagicShield, {"и ворога оберегись", "... руками своими да защитит он себя"}},
		{kSpellForbidden, {"вороги не войдут", "... ибо положена печать, и никто не возвращается."}},
		{kSpellMassSilence, {"их уста непотребны", "... да замкнутся уста ваши."}},
		{kSpellRemoveSilence, {"глаголите", "... слова из уст мудрого - благодать."}},
		{kSpellDamageLight, {"падош", "... будет чувствовать боль."}},
		{kSpellDamageSerious, {"скверна", "... постигнут тебя муки."}},
		{kSpellDamageCritic, {"сильна скверна", "... боль и муки схватили."}},
		{kSpellMassCurse, {"порча их", "... прокляты вы пред всеми скотами."}},
		{kSpellArmageddon, {"суд божиа не минути", "... какою мерою мерите, такою отмерено будет и вам."}},
		{kSpellGroupFly, {"крыла им створисте", "... и все летающие по роду их."}},
		{kSpellGroupBless, {"други, наполнися ратнаго духа", "... блажены те, слышащие слово Божие."}},
		{kSpellResfresh, {"буде свеж", "... не будет у него ни усталого, ни изнемогающего."}},
		{kSpellStunning, {"да обратит тебя Чернобог в мертвый камень!", "... и проклял его именем Господним."}},
		{kSpellHide, {"", ""}},
		{kSpellSneak, {"", ""}},
		{kSpellDrunked, {"", ""}},
		{kSpellAbstinent, {"", ""}},
		{kSpellFullFeed, {"брюхо полно", "... душа больше пищи, и тело - одежды."}},
		{kSpellColdWind, {"веют ветры", "... подует северный холодный ветер."}},
		{kSpellBattle, {"", ""}},
		{kSpellHaemorrhage, {"", ""}},
		{kSpellCourage, {"", ""}},
		{kSpellWaterbreath, {"не затвори темне березе", "... дух дышит, где хочет."}},
		{kSpellSlowdown, {"немочь", "...и помедлил еще семь дней других."}},
		{kSpellHaste, {"скор аки ястреб", "... поднимет его ветер и понесет, и он быстро побежит от него."}},
		{kSpellMassSlow, {"тернии им", "... загорожу путь их тернами."}},
		{kSpellGroupHaste, {"быстры аки ястребов стая", "... и они быстры как серны на горах."}},
		{kSpellGodsShield, {"Живый в помощи Вышняго", "... благословен буде Грядый во имя Господне."}},
		{kSpellFever, {"нутро снеде", "... и сделаются жестокие и кровавые язвы."}},
		{kSpellCureFever, {"Навь, очисти тело", "... хочу, очистись."}},
		{kSpellAwareness, {"око недреманно", "... не дам сна очам моим и веждам моим - дремания."}},
		{kSpellReligion, {"", ""}},
		{kSpellAirShield, {"Стрибог, даруй прибежище", "... защита от ветра и покров от непогоды."}},
		{kSpellPortal, {"буде путь короток", "... входите во врата Его."}},
		{kSpellDispellMagic, {"изыде ворожба", "... выйди, дух нечистый."}},
		{kSpellSummonKeeper, {"Сварог, даруй защитника", "... и благословен защитник мой!"}},
		{kSpellFastRegeneration, {"заживет, аки на собаке", "... нет богатства лучше телесного здоровья."}},
		{kSpellCreateWeapon, {"будовати стружие", "...вооружите из себя людей на войну"}},
		{kSpellFireShield, {"Хорс, даруй прибежище", "... душа горячая, как пылающий огонь."}},
		{kSpellRelocate, {"Стрибог, укажи путь...", "... указывай им путь, по которому они должны идти."}},
		{kSpellSummonFirekeeper, {"Дажьбог, даруй защитника", "... Ангел Мой с вами, и он защитник душ ваших."}},
		{kSpellIceShield, {"Морена, даруй прибежище", "... а снег и лед выдерживали огонь и не таяли."}},
		{kSpellIceStorm, {"торже, яко вихор", "... и град, величиною в талант, падет с неба."}},
		{kSpellLessening, {"буде мал аки мышь", "... плоть на нем пропадает."}},
		{kSpellShineFlash, {"засти очи им", "... свет пламени из средины огня."}},
		{kSpellMadness, {"згола яростен", "... и ярость его загорелась в нем."}},
		{kSpellGroupMagicGlass, {"гладь воды отразит", "... воздай им по делам их, по злым поступкам их."}},
		{kSpellCloudOfArrows, {"и будут стрелы молний, и зарницы в высях",
							   "... соберу на них бедствия и истощу на них стрелы Мои."}},
		{kSpellVacuum, {"Умри!", "... и услышав слова сии - пал бездыханен."}},
		{kSpellMeteorStorm, {"идти дождю стрелами", "... и камни, величиною в талант, падут с неба."}},
		{kSpellStoneHands, {"сильны велетов руки", "... рука Моя пребудет с ним, и мышца Моя укрепит его."}},
		{kSpellMindless, {"разум аки мутный омут", "... и безумие его с ним."}},
		{kSpellPrismaticAura, {"окружен радугой", "... явится радуга в облаке."}},
		{kSpellEviless, {"зло творяще", "... и ты воздашь им злом."}},
		{kSpellAirAura, {"Мать-земля, даруй защиту.", "... поклон тебе матушка земля."}},
		{kSpellFireAura, {"Сварог, даруй защиту.", "... и огонь низводит с неба."}},
		{kSpellIceAura, {"Морена, даруй защиту.", "... текущие холодные воды."}},
		{kSpellShock, {"будет слеп и глух, аки мертвец", "... кто делает или глухим, или слепым."}},
		{kSpellMagicGlass, {"Аз воздам!", "... и воздам каждому из вас."}},
		{kSpellGroupSanctuary, {"иже во святых, други", "... будьте святы, аки Господь наш свят."}},
		{kSpellGroupPrismaticAura, {"други, буде окружены радугой", "... взгляни на радугу, и прославь Сотворившего ее."}},
		{kSpellDeafness, {"оглохни", "... и глухота поразит тебя."}},
		{kSpellPowerDeafness, {"да застит уши твои", "... и будь глухим надолго."}},
		{kSpellRemoveDeafness, {"слушай глас мой", "... услышь слово Его."}},
		{kSpellMassDeafness, {"будьте глухи", "... и не будут слышать уши ваши."}},
		{kSpellDustStorm, {"пыль поднимется столбами", "... и пыль поглотит вас."}},
		{kSpellEarthfall, {"пусть каменья падут", "... и обрушатся камни с небес."}},
		{kSpellSonicWave, {"да невзлюбит тебя воздух", "... и даже воздух покарает тебя."}},
		{kSpellHolystrike, {"Велес, упокой мертвых",
							"... и предоставь мертвым погребать своих мертвецов."}},
		{kSpellSumonAngel, {"Боги, даруйте защитника", "... дабы уберег он меня от зла."}},
		{kSpellMassFear, {"Поврещу сташивые души их в скарядие!", "... и затмил ужас разум их."}},
		{kSpellFascination, {"Да пребудет с тобой вся краса мира!", "... и омолодил он, и украсил их."}},
		{kSpellCrying, {"Будут слезы твои, аки камень на сердце",
						"... и постигнет твой дух угнетение вечное."}},
		{kSpellOblivion, {"будь живот аки буява с шерстнями.",
						  "... опадет на тебя чернь страшная."}},
		{kSpellBurdenOfTime, {"Яко небытие нещадно к вам, али время вернулось вспять.",
							  "... и время не властно над ними."}},
		{kSpellGroupRefresh, {"Исполняше други силою!",
							  "...да не останется ни обделенного, ни обессиленного."}},
		{kSpellPeaceful, {"Избавь речь свою от недобрых слов, а ум - от крамольных мыслей.",
						  "... любите врагов ваших и благотворите ненавидящим вас."}},
		{kSpellMagicBattle, {"", ""}},
		{kSpellBerserk, {"", ""}},
		{kSpellStoneBones, {"Обращу кости их в твердый камень.",
							"...и тот, кто упадет на камень сей, разобьется."}},
		{kSpellRoomLight, {"Да буде СВЕТ !!!", "...ибо сказал МОНТЕР !!!"}},
		{kSpellPoosinedFog, {"Порчу воздух !!!", "...и зловонное дыхание его."}},
		{kSpellThunderstorm, {"Абие велий вихрь деяти!",
							  "...творит молнии при дожде, изводит ветер из хранилищ Своих."}},
		{kSpellLightWalk, {"", ""}},
		{kSpellFailure, {"аще доля зла и удача немилостива", ".. и несчастен, и жалок, и нищ."}},
		{kSpellClanPray, {"", ""}},
		{kSpellGlitterDust, {"зрети супостат охабиша", "...и бросали пыль на воздух."}},
		{kSpellScream, {"язвень голки уведати", "...но в полночь раздался крик."}},
		{kSpellCatGrace, {"ристати споро", "...и не уязвит враг того, кто скор."}},
		{kSpellBullBody, {"руци яре ворога супротив", "...и мощь звериная жила в теле его."}},
		{kSpellSnakeWisdom, {"веси и зрети стези отай", "...и даровал мудрость ему."}},
		{kSpellGimmicry, {"клюка вящего улучити", "...ибо кто познал ум Господень?"}},
		{kSpellWarcryOfChallenge, {"Эй, псы шелудивые, керасти плешивые, ослопы беспорточные, мшицы задочные!",
								   "Эй, псы шелудивые, керасти плешивые, ослопы беспорточные, мшицы задочные!"}},
		{kSpellWarcryOfMenace, {"Покрошу-изувечу, душу выну и в блины закатаю!",
								"Покрошу-изувечу, душу выну и в блины закатаю!"}},
		{kSpellWarcryOfRage, {"Не отступим, други, они наше сало сперли!",
							  "Не отступим, други, они наше сало сперли!"}},
		{kSpellWarcryOfMadness, {"Всех убью, а сам$g останусь!", "Всех убью, а сам$g останусь!"}},
		{kSpellWarcryOfThunder, {"Шоб вас приподняло, да шлепнуло!!!", "Шоб вас приподняло да шлепнуло!!!"}},
		{kSpellWarcryOfDefence, {"В строй други, защитим животами Русь нашу!",
								 "В строй други, защитим животами Русь нашу!"}},
		{kSpellWarcryOfBattle, {"Дер-ржать строй, волчьи хвосты!", "Дер-ржать строй, волчьи хвосты!"}},
		{kSpellWarcryOfPower, {"Сарынь на кичку!", "Сарынь на кичку!"}},
		{kSpellWarcryOfBless, {"Стоять крепко! За нами Киев, Подол и трактир с пивом!!!",
							   "Стоять крепко! За нами Киев, Подол и трактир с пивом!!!"}},
		{kSpellWarcryOfCourage, {"Орлы! Будем биться как львы!", "Орлы! Будем биться как львы!"}},
		{kSpellRuneLabel, {"...пьсати черты и резы.", "...и Сам отошел от них на вержение камня."}},
		{kSpellAconitumPoison, {"трутизна", "... и пошлю на них зубы зверей и яд ползающих по земле."}},
		{kSpellScopolaPoison, {"трутизна", "... и пошлю на них зубы зверей и яд ползающих по земле."}},
		{kSpellBelenaPoison, {"трутизна", "... и пошлю на них зубы зверей и яд ползающих по земле."}},
		{kSpellDaturaPoison, {"трутизна", "... и пошлю на них зубы зверей и яд ползающих по земле."}},
		{kSpellTimerRestore, {"", ""}},
		{kSpellLucky, {"", ""}},
		{kSpellBandage, {"", ""}},
		{kSpellNoBandage, {"", ""}},
		{kSpellCapable, {"", ""}},
		{kSpellStrangle, {"", ""}},
		{kSpellRecallSpells, {"", ""}},
		{kSpellHypnoticPattern, {"ажбо супостаты блазнити да клюковати",
								 "...и утроба его приготовляет обман."}},
		{kSpellSolobonus, {"", ""}},
		{kSpellVampirism, {"", ""}},
		{kSpellRestoration, {"Да прими вид прежний, якой был.",
							 ".. Воззри на предмет сей Отче и верни ему силу прежнюю."}},
		{kSpellDeathAura, {"Надели силою своею Навь, дабы собрать урожай тебе.",
						   "...налякай ворогов наших и покарай их немощью."}},
		{kSpellRecovery, {"Обрасти плотью сызнова.", "... прости Господи грехи, верни плоть созданию."}},
		{kSpellMassRecovery, {"Обрастите плотью сызнова.",
							  "... прости Господи грехи, верни плоть созданиям."}},
		{kSpellAuraOfEvil, {"Возьми личину зла для жатвы славной.", "Надели силой злою во благо."}},
		{kSpellMentalShadow, {"Силою мысли защиту будую себе.",
							  "Даруй Отче защиту, силой разума воздвигнутую."}},
		{kSpellBlackTentacles, {"Ато егоже руци попасти.",
								"И он не знает, что мертвецы там и что в глубине..."}},
		{kSpellWhirlwind, {"Вждати бурю обло створити.", "И поднялась великая буря..."}},
		{kSpellIndriksTeeth, {"Идеже индрика зубы супостаты изъмати.",
							  "Есть род, у которого зубы - мечи и челюсти - ножи..."}},
		{kSpellAcidArrow, {"Варно сожжет струя!",
						   "...и на коже его сделаются как бы язвы проказы"}},
		{kSpellThunderStone, {"Небесе тутнет!", "...и взял оттуда камень, и бросил из пращи."}},
		{kSpellClod, {"Онома утес низринется!",
					  "...доколе камень не оторвался от горы без содействия рук."}},
		{kSpellExpedient, {"!Применил боевой прием!", "!use battle expedient!"}},
		{kSpellSightOfDarkness, {"Что свет, что тьма - глазу одинаково.",
								 "Станьте зрячи в тьме кромешной!"}},
		{kSpellGroupSincerity, {"...да не скроются намерения.",
								"И узрим братья намерения окружающих."}},
		{kSpellMagicalGaze, {"Узрим же все, что с магией навкруги нас.",
							 "Покажи, Спаситель, магические силы братии."}},
		{kSpellAllSeeingEye, {"Все тайное станет явным.",
							  "Не спрячется, не скроется, ни заяц, ни блоха."}},
		{kSpellEyeOfGods, {"Осязаемое откройся взору!",
						   "Да не скроется от взора вашего, ни одна живая душа."}},
		{kSpellBreathingAtDepth, {"Аки стайка рыбок, плывите вместе.",
								  "Что в воде, что на земле, воздух свежим будет."}},
		{kSpellGeneralRecovery, {"...дабы пройти вместе не одну сотню верст",
								 "Сохрани Отче от усталости детей своих!"}},
		{kSpellCommonMeal, {"Благодарите богов за хлеб и соль!",
							"...дабы не осталось голодающих на свете белом"}},
		{kSpellStoneWall, {"Станем други крепки як николы!", "Укрепим тела наши перед битвой!"}},
		{kSpellSnakeEyes, {"Что яд, а что мед. Не обманемся!",
						   "...и самый сильный яд станет вам виден."}},
		{kSpellEarthAura, {"Велес, даруй защиту.", "... земля благословенна твоя."}},
		{kSpellGroupProtectFromEvil, {"други, супостат нощи",
									  "други, свет который в нас, да убоится тьма."}},
		{kSpellArrowsFire, {"!магический выстрел!", "!use battle expedient!"}},
		{kSpellArrowsWater, {"!магический выстрел!", "!use battle expedient!"}},
		{kSpellArrowsEarth, {"!магический выстрел!", "!use battle expedient!"}},
		{kSpellArrowsAir, {"!магический выстрел!", "!use battle expedient!"}},
		{kSpellArrowsDeath, {"!магический выстрел!", "!use battle expedient!"}},
		{kSpellPaladineInspiration, {"", ""}},
		{kSpellDexterity, {"будет ловким", "... и человек разумный укрепляет ловкость свою."}},
		{kSpellGroupBlink, {"защити нас от железа разящего", "... ни стрела, ни меч не пронзят печень вашу."}},
		{kSpellGroupCloudly, {"огрожу беззакония их туманом",
							  "...да защитит и покроет рассветная пелена тела ваши."}},
		{kSpellGroupAwareness, {"буде вежды ваши открыты", "... и забота о ближнем отгоняет сон от очей их."}},
		{kSpellWatctyOfExpirence, {"найдем новизну в рутине сражений!", "найдем новизну в рутине сражений!"}},
		{kSpellWarcryOfLuck, {"и пусть удача будет нашей спутницей!", "и пусть удача будет нашей спутницей!"}},
		{kSpellWarcryOfPhysdamage, {"бей в глаз, не порти шкуру", "бей в глаз, не порти шкуру."}},
		{kSpellMassFailure, {"...отче Велес, очи отвержеши!",
							 "...надежда тщетна: не упадешь ли от одного взгляда его?"}},
		{kSpellSnare, {"Заклинати поврещение в сети заскопиены!",
					   "...будет трапеза их сетью им, и мирное пиршество их - западнею."}},
		{kSpellExpedientFail, {"!Провалил боевой прием!",
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
	ESpell_name_by_value[ESpell::kSpellArmor] = "kSpellArmor";
	ESpell_name_by_value[ESpell::kSpellTeleport] = "kSpellTeleport";
	ESpell_name_by_value[ESpell::kSpellBless] = "kSpellBless";
	ESpell_name_by_value[ESpell::kSpellBlindness] = "kSpellBlindness";
	ESpell_name_by_value[ESpell::kSpellBurningHands] = "kSpellBurningHands";
	ESpell_name_by_value[ESpell::kSpellCallLighting] = "kSpellCallLighting";
	ESpell_name_by_value[ESpell::kSpellCharm] = "kSpellCharm";
	ESpell_name_by_value[ESpell::kSpellChillTouch] = "kSpellChillTouch";
	ESpell_name_by_value[ESpell::kSpellClone] = "kSpellClone";
	ESpell_name_by_value[ESpell::kSpellIceBolts] = "kSpellIceBolts";
	ESpell_name_by_value[ESpell::kSpellControlWeather] = "kSpellControlWeather";
	ESpell_name_by_value[ESpell::kSpellCreateFood] = "kSpellCreateFood";
	ESpell_name_by_value[ESpell::kSpellCreateWater] = "kSpellCreateWater";
	ESpell_name_by_value[ESpell::kSpellCureBlind] = "kSpellCureBlind";
	ESpell_name_by_value[ESpell::kSpellCureCritic] = "kSpellCureCritic";
	ESpell_name_by_value[ESpell::kSpellCureLight] = "kSpellCureLight";
	ESpell_name_by_value[ESpell::kSpellCurse] = "kSpellCurse";
	ESpell_name_by_value[ESpell::kSpellDetectAlign] = "kSpellDetectAlign";
	ESpell_name_by_value[ESpell::kSpellDetectInvis] = "kSpellDetectInvis";
	ESpell_name_by_value[ESpell::kSpellDetectMagic] = "kSpellDetectMagic";
	ESpell_name_by_value[ESpell::kSpellDetectPoison] = "kSpellDetectPoison";
	ESpell_name_by_value[ESpell::kSpellDispelEvil] = "kSpellDispelEvil";
	ESpell_name_by_value[ESpell::kSpellEarthquake] = "kSpellEarthquake";
	ESpell_name_by_value[ESpell::kSpellEnchantWeapon] = "kSpellEnchantWeapon";
	ESpell_name_by_value[ESpell::kSpellEnergyDrain] = "kSpellEnergyDrain";
	ESpell_name_by_value[ESpell::kSpellFireball] = "kSpellFireball";
	ESpell_name_by_value[ESpell::kSpellHarm] = "kSpellHarm";
	ESpell_name_by_value[ESpell::kSpellHeal] = "kSpellHeal";
	ESpell_name_by_value[ESpell::kSpellInvisible] = "kSpellInvisible";
	ESpell_name_by_value[ESpell::kSpellLightingBolt] = "kSpellLightingBolt";
	ESpell_name_by_value[ESpell::kSpellLocateObject] = "kSpellLocateObject";
	ESpell_name_by_value[ESpell::kSpellMagicMissile] = "kSpellMagicMissile";
	ESpell_name_by_value[ESpell::kSpellPoison] = "kSpellPoison";
	ESpell_name_by_value[ESpell::kSpellProtectFromEvil] = "kSpellProtectFromEvil";
	ESpell_name_by_value[ESpell::kSpellRemoveCurse] = "kSpellRemoveCurse";
	ESpell_name_by_value[ESpell::kSpellSanctuary] = "kSpellSanctuary";
	ESpell_name_by_value[ESpell::kSpellShockingGasp] = "kSpellShockingGasp";
	ESpell_name_by_value[ESpell::kSpellSleep] = "kSpellSleep";
	ESpell_name_by_value[ESpell::kSpellStrength] = "kSpellStrength";
	ESpell_name_by_value[ESpell::kSpellSummon] = "kSpellSummon";
	ESpell_name_by_value[ESpell::kSpellPatronage] = "kSpellPatronage";
	ESpell_name_by_value[ESpell::kSpellWorldOfRecall] = "kSpellWorldOfRecall";
	ESpell_name_by_value[ESpell::kSpellRemovePoison] = "kSpellRemovePoison";
	ESpell_name_by_value[ESpell::kSpellSenseLife] = "kSpellSenseLife";
	ESpell_name_by_value[ESpell::kSpellAnimateDead] = "kSpellAnimateDead";
	ESpell_name_by_value[ESpell::kSpellDispelGood] = "kSpellDispelGood";
	ESpell_name_by_value[ESpell::kSpellGroupArmor] = "kSpellGroupArmor";
	ESpell_name_by_value[ESpell::kSpellGroupHeal] = "kSpellGroupHeal";
	ESpell_name_by_value[ESpell::kSpellGroupRecall] = "kSpellGroupRecall";
	ESpell_name_by_value[ESpell::kSpellInfravision] = "kSpellInfravision";
	ESpell_name_by_value[ESpell::kSpellWaterwalk] = "kSpellWaterwalk";
	ESpell_name_by_value[ESpell::kSpellCureSerious] = "kSpellCureSerious";
	ESpell_name_by_value[ESpell::kSpellGroupStrength] = "kSpellGroupStrength";
	ESpell_name_by_value[ESpell::kSpellHold] = "kSpellHold";
	ESpell_name_by_value[ESpell::kSpellPowerHold] = "kSpellPowerHold";
	ESpell_name_by_value[ESpell::kSpellMassHold] = "kSpellMassHold";
	ESpell_name_by_value[ESpell::kSpellFly] = "kSpellFly";
	ESpell_name_by_value[ESpell::kSpellBrokenChains] = "kSpellBrokenChains";
	ESpell_name_by_value[ESpell::kSpellNoflee] = "kSpellNoflee";
	ESpell_name_by_value[ESpell::kSpellCreateLight] = "kSpellCreateLight";
	ESpell_name_by_value[ESpell::kSpellDarkness] = "kSpellDarkness";
	ESpell_name_by_value[ESpell::kSpellStoneSkin] = "kSpellStoneSkin";
	ESpell_name_by_value[ESpell::kSpellCloudly] = "kSpellCloudly";
	ESpell_name_by_value[ESpell::kSpellSllence] = "kSpellSllence";
	ESpell_name_by_value[ESpell::kSpellLight] = "kSpellLight";
	ESpell_name_by_value[ESpell::kSpellChainLighting] = "kSpellChainLighting";
	ESpell_name_by_value[ESpell::kSpellFireBlast] = "kSpellFireBlast";
	ESpell_name_by_value[ESpell::kSpellGodsWrath] = "kSpellGodsWrath";
	ESpell_name_by_value[ESpell::kSpellWeaknes] = "kSpellWeaknes";
	ESpell_name_by_value[ESpell::kSpellGroupInvisible] = "kSpellGroupInvisible";
	ESpell_name_by_value[ESpell::kSpellShadowCloak] = "kSpellShadowCloak";
	ESpell_name_by_value[ESpell::kSpellAcid] = "kSpellAcid";
	ESpell_name_by_value[ESpell::kSpellRepair] = "kSpellRepair";
	ESpell_name_by_value[ESpell::kSpellEnlarge] = "kSpellEnlarge";
	ESpell_name_by_value[ESpell::kSpellFear] = "kSpellFear";
	ESpell_name_by_value[ESpell::kSpellSacrifice] = "kSpellSacrifice";
	ESpell_name_by_value[ESpell::kSpellWeb] = "kSpellWeb";
	ESpell_name_by_value[ESpell::kSpellBlink] = "kSpellBlink";
	ESpell_name_by_value[ESpell::kSpellRemoveHold] = "kSpellRemoveHold";
	ESpell_name_by_value[ESpell::kSpellCamouflage] = "kSpellCamouflage";
	ESpell_name_by_value[ESpell::kSpellPowerBlindness] = "kSpellPowerBlindness";
	ESpell_name_by_value[ESpell::kSpellMassBlindness] = "kSpellMassBlindness";
	ESpell_name_by_value[ESpell::kSpellPowerSilence] = "kSpellPowerSilence";
	ESpell_name_by_value[ESpell::kSpellExtraHits] = "kSpellExtraHits";
	ESpell_name_by_value[ESpell::kSpellResurrection] = "kSpellResurrection";
	ESpell_name_by_value[ESpell::kSpellMagicShield] = "kSpellMagicShield";
	ESpell_name_by_value[ESpell::kSpellForbidden] = "kSpellForbidden";
	ESpell_name_by_value[ESpell::kSpellMassSilence] = "kSpellMassSilence";
	ESpell_name_by_value[ESpell::kSpellRemoveSilence] = "kSpellRemoveSilence";
	ESpell_name_by_value[ESpell::kSpellDamageLight] = "kSpellDamageLight";
	ESpell_name_by_value[ESpell::kSpellDamageSerious] = "kSpellDamageSerious";
	ESpell_name_by_value[ESpell::kSpellDamageCritic] = "kSpellDamageCritic";
	ESpell_name_by_value[ESpell::kSpellMassCurse] = "kSpellMassCurse";
	ESpell_name_by_value[ESpell::kSpellArmageddon] = "kSpellArmageddon";
	ESpell_name_by_value[ESpell::kSpellGroupFly] = "kSpellGroupFly";
	ESpell_name_by_value[ESpell::kSpellGroupBless] = "kSpellGroupBless";
	ESpell_name_by_value[ESpell::kSpellResfresh] = "kSpellResfresh";
	ESpell_name_by_value[ESpell::kSpellStunning] = "kSpellStunning";
	ESpell_name_by_value[ESpell::kSpellHide] = "kSpellHide";
	ESpell_name_by_value[ESpell::kSpellSneak] = "kSpellSneak";
	ESpell_name_by_value[ESpell::kSpellDrunked] = "kSpellDrunked";
	ESpell_name_by_value[ESpell::kSpellAbstinent] = "kSpellAbstinent";
	ESpell_name_by_value[ESpell::kSpellFullFeed] = "kSpellFullFeed";
	ESpell_name_by_value[ESpell::kSpellColdWind] = "kSpellColdWind";
	ESpell_name_by_value[ESpell::kSpellBattle] = "kSpellBattle";
	ESpell_name_by_value[ESpell::kSpellHaemorrhage] = "kSpellHaemorragis";
	ESpell_name_by_value[ESpell::kSpellCourage] = "kSpellCourage";
	ESpell_name_by_value[ESpell::kSpellWaterbreath] = "kSpellWaterbreath";
	ESpell_name_by_value[ESpell::kSpellSlowdown] = "kSpellSlowdown";
	ESpell_name_by_value[ESpell::kSpellHaste] = "kSpellHaste";
	ESpell_name_by_value[ESpell::kSpellMassSlow] = "kSpellMassSlow";
	ESpell_name_by_value[ESpell::kSpellGroupHaste] = "kSpellGroupHaste";
	ESpell_name_by_value[ESpell::kSpellGodsShield] = "kSpellGodsShield";
	ESpell_name_by_value[ESpell::kSpellFever] = "kSpellFever";
	ESpell_name_by_value[ESpell::kSpellCureFever] = "kSpellCureFever";
	ESpell_name_by_value[ESpell::kSpellAwareness] = "kSpellAwareness";
	ESpell_name_by_value[ESpell::kSpellReligion] = "kSpellReligion";
	ESpell_name_by_value[ESpell::kSpellAirShield] = "kSpellAirShield";
	ESpell_name_by_value[ESpell::kSpellPortal] = "kSpellPortal";
	ESpell_name_by_value[ESpell::kSpellDispellMagic] = "kSpellDispellMagic";
	ESpell_name_by_value[ESpell::kSpellSummonKeeper] = "kSpellSummonKeeper";
	ESpell_name_by_value[ESpell::kSpellFastRegeneration] = "kSpellFastRegeneration";
	ESpell_name_by_value[ESpell::kSpellCreateWeapon] = "kSpellCreateWeapon";
	ESpell_name_by_value[ESpell::kSpellFireShield] = "kSpellFireShield";
	ESpell_name_by_value[ESpell::kSpellRelocate] = "kSpellRelocate";
	ESpell_name_by_value[ESpell::kSpellSummonFirekeeper] = "kSpellSummonFirekeeper";
	ESpell_name_by_value[ESpell::kSpellIceShield] = "kSpellIceShield";
	ESpell_name_by_value[ESpell::kSpellIceStorm] = "kSpellIceStorm";
	ESpell_name_by_value[ESpell::kSpellLessening] = "kSpellLessening";
	ESpell_name_by_value[ESpell::kSpellShineFlash] = "kSpellShineFlash";
	ESpell_name_by_value[ESpell::kSpellMadness] = "kSpellMadness";
	ESpell_name_by_value[ESpell::kSpellGroupMagicGlass] = "kSpellGroupMagicGlass";
	ESpell_name_by_value[ESpell::kSpellCloudOfArrows] = "kSpellCloudOfArrows";
	ESpell_name_by_value[ESpell::kSpellVacuum] = "kSpellVacuum";
	ESpell_name_by_value[ESpell::kSpellMeteorStorm] = "kSpellMeteorStorm";
	ESpell_name_by_value[ESpell::kSpellStoneHands] = "kSpellStoneHands";
	ESpell_name_by_value[ESpell::kSpellMindless] = "kSpellMindless";
	ESpell_name_by_value[ESpell::kSpellPrismaticAura] = "kSpellPrismaticAura";
	ESpell_name_by_value[ESpell::kSpellEviless] = "kSpellEviless";
	ESpell_name_by_value[ESpell::kSpellAirAura] = "kSpellAirAura";
	ESpell_name_by_value[ESpell::kSpellFireAura] = "kSpellFireAura";
	ESpell_name_by_value[ESpell::kSpellIceAura] = "kSpellIceAura";
	ESpell_name_by_value[ESpell::kSpellShock] = "kSpellShock";
	ESpell_name_by_value[ESpell::kSpellMagicGlass] = "kSpellMagicGlass";
	ESpell_name_by_value[ESpell::kSpellGroupSanctuary] = "kSpellGroupSanctuary";
	ESpell_name_by_value[ESpell::kSpellGroupPrismaticAura] = "kSpellGroupPrismaticAura";
	ESpell_name_by_value[ESpell::kSpellDeafness] = "kSpellDeafness";
	ESpell_name_by_value[ESpell::kSpellPowerDeafness] = "kSpellPowerDeafness";
	ESpell_name_by_value[ESpell::kSpellRemoveDeafness] = "kSpellRemoveDeafness";
	ESpell_name_by_value[ESpell::kSpellMassDeafness] = "kSpellMassDeafness";
	ESpell_name_by_value[ESpell::kSpellDustStorm] = "kSpellDustStorm";
	ESpell_name_by_value[ESpell::kSpellEarthfall] = "kSpellEarthfall";
	ESpell_name_by_value[ESpell::kSpellSonicWave] = "kSpellSonicWave";
	ESpell_name_by_value[ESpell::kSpellHolystrike] = "kSpellHolystrike";
	ESpell_name_by_value[ESpell::kSpellSumonAngel] = "kSpellSumonAngel";
	ESpell_name_by_value[ESpell::kSpellMassFear] = "kSpellMassFear";
	ESpell_name_by_value[ESpell::kSpellFascination] = "kSpellFascination";
	ESpell_name_by_value[ESpell::kSpellCrying] = "kSpellCrying";
	ESpell_name_by_value[ESpell::kSpellOblivion] = "kSpellOblivion";
	ESpell_name_by_value[ESpell::kSpellBurdenOfTime] = "kSpellBurdenOfTime";
	ESpell_name_by_value[ESpell::kSpellGroupRefresh] = "kSpellGroupRefresh";
	ESpell_name_by_value[ESpell::kSpellPeaceful] = "kSpellPeaceful";
	ESpell_name_by_value[ESpell::kSpellMagicBattle] = "kSpellMagicBattle";
	ESpell_name_by_value[ESpell::kSpellBerserk] = "kSpellBerserk";
	ESpell_name_by_value[ESpell::kSpellStoneBones] = "kSpellStoneBones";
	ESpell_name_by_value[ESpell::kSpellRoomLight] = "kSpellRoomLight";
	ESpell_name_by_value[ESpell::kSpellPoosinedFog] = "kSpellPoosinedFog";
	ESpell_name_by_value[ESpell::kSpellThunderstorm] = "kSpellThunderstorm";
	ESpell_name_by_value[ESpell::kSpellLightWalk] = "kSpellLightWalk";
	ESpell_name_by_value[ESpell::kSpellFailure] = "kSpellFailure";
	ESpell_name_by_value[ESpell::kSpellClanPray] = "kSpellClanPray";
	ESpell_name_by_value[ESpell::kSpellGlitterDust] = "kSpellGlitterDust";
	ESpell_name_by_value[ESpell::kSpellScream] = "kSpellScream";
	ESpell_name_by_value[ESpell::kSpellCatGrace] = "kSpellCatGrace";
	ESpell_name_by_value[ESpell::kSpellBullBody] = "kSpellBullBody";
	ESpell_name_by_value[ESpell::kSpellSnakeWisdom] = "kSpellSnakeWisdom";
	ESpell_name_by_value[ESpell::kSpellGimmicry] = "kSpellGimmicry";
	ESpell_name_by_value[ESpell::kSpellWarcryOfChallenge] = "kSpellWarcryOfChallenge";
	ESpell_name_by_value[ESpell::kSpellWarcryOfMenace] = "kSpellWarcryOfMenace";
	ESpell_name_by_value[ESpell::kSpellWarcryOfRage] = "kSpellWarcryOfRage";
	ESpell_name_by_value[ESpell::kSpellWarcryOfMadness] = "kSpellWarcryOfMadness";
	ESpell_name_by_value[ESpell::kSpellWarcryOfThunder] = "kSpellWarcryOfThunder";
	ESpell_name_by_value[ESpell::kSpellWarcryOfDefence] = "kSpellWarcryOfDefence";
	ESpell_name_by_value[ESpell::kSpellWarcryOfBattle] = "kSpellWarcryOfBattle";
	ESpell_name_by_value[ESpell::kSpellWarcryOfPower] = "kSpellWarcryOfPower";
	ESpell_name_by_value[ESpell::kSpellWarcryOfBless] = "kSpellWarcryOfBless";
	ESpell_name_by_value[ESpell::kSpellWarcryOfCourage] = "kSpellWarcryOfCourage";
	ESpell_name_by_value[ESpell::kSpellRuneLabel] = "kSpellRuneLabel";
	ESpell_name_by_value[ESpell::kSpellAconitumPoison] = "kSpellAconitumPoison";
	ESpell_name_by_value[ESpell::kSpellScopolaPoison] = "kSpellScopolaPoison";
	ESpell_name_by_value[ESpell::kSpellBelenaPoison] = "kSpellBelenaPoison";
	ESpell_name_by_value[ESpell::kSpellDaturaPoison] = "kSpellDaturaPoison";
	ESpell_name_by_value[ESpell::kSpellTimerRestore] = "kSpellTimerRestore";
	ESpell_name_by_value[ESpell::kSpellLucky] = "kSpellLucky";
	ESpell_name_by_value[ESpell::kSpellBandage] = "kSpellBandage";
	ESpell_name_by_value[ESpell::kSpellNoBandage] = "kSpellNoBandage";
	ESpell_name_by_value[ESpell::kSpellCapable] = "kSpellCapable";
	ESpell_name_by_value[ESpell::kSpellStrangle] = "kSpellStrangle";
	ESpell_name_by_value[ESpell::kSpellRecallSpells] = "kSpellRecallSpells";
	ESpell_name_by_value[ESpell::kSpellHypnoticPattern] = "kSpellHypnoticPattern";
	ESpell_name_by_value[ESpell::kSpellSolobonus] = "kSpellSolobonus";
	ESpell_name_by_value[ESpell::kSpellVampirism] = "kSpellVampirism";
	ESpell_name_by_value[ESpell::kSpellRestoration] = "kSpellRestoration";
	ESpell_name_by_value[ESpell::kSpellDeathAura] = "kSpellDeathAura";
	ESpell_name_by_value[ESpell::kSpellRecovery] = "kSpellRecovery";
	ESpell_name_by_value[ESpell::kSpellMassRecovery] = "kSpellMassRecovery";
	ESpell_name_by_value[ESpell::kSpellAuraOfEvil] = "kSpellAuraOfEvil";
	ESpell_name_by_value[ESpell::kSpellMentalShadow] = "kSpellMentalShadow";
	ESpell_name_by_value[ESpell::kSpellBlackTentacles] = "kSpellBlackTentacles";
	ESpell_name_by_value[ESpell::kSpellWhirlwind] = "kSpellWhirlwind";
	ESpell_name_by_value[ESpell::kSpellIndriksTeeth] = "kSpellIndriksTeeth";
	ESpell_name_by_value[ESpell::kSpellAcidArrow] = "kSpellAcidArrow";
	ESpell_name_by_value[ESpell::kSpellThunderStone] = "kSpellThunderStone";
	ESpell_name_by_value[ESpell::kSpellClod] = "kSpellClod";
	ESpell_name_by_value[ESpell::kSpellExpedient] = "kSpellExpedient";
	ESpell_name_by_value[ESpell::kSpellSightOfDarkness] = "kSpellSightOfDarkness";
	ESpell_name_by_value[ESpell::kSpellGroupSincerity] = "kSpellGroupSincerity";
	ESpell_name_by_value[ESpell::kSpellMagicalGaze] = "kSpellMagicalGaze";
	ESpell_name_by_value[ESpell::kSpellAllSeeingEye] = "kSpellAllSeeingEye";
	ESpell_name_by_value[ESpell::kSpellEyeOfGods] = "kSpellEyeOfGods";
	ESpell_name_by_value[ESpell::kSpellBreathingAtDepth] = "kSpellBreathingAtDepth";
	ESpell_name_by_value[ESpell::kSpellGeneralRecovery] = "kSpellGeneralRecovery";
	ESpell_name_by_value[ESpell::kSpellCommonMeal] = "kSpellCommonMeal";
	ESpell_name_by_value[ESpell::kSpellStoneWall] = "kSpellStoneWall";
	ESpell_name_by_value[ESpell::kSpellSnakeEyes] = "kSpellSnakeEyes";
	ESpell_name_by_value[ESpell::kSpellEarthAura] = "kSpellEarthAura";
	ESpell_name_by_value[ESpell::kSpellGroupProtectFromEvil] = "kSpellGroupProtectFromEvil";
	ESpell_name_by_value[ESpell::kSpellArrowsFire] = "kSpellArrowsFire";
	ESpell_name_by_value[ESpell::kSpellArrowsWater] = "kSpellArrowsWater";
	ESpell_name_by_value[ESpell::kSpellArrowsEarth] = "kSpellArrowsEarth";
	ESpell_name_by_value[ESpell::kSpellArrowsAir] = "kSpellArrowsAir";
	ESpell_name_by_value[ESpell::kSpellArrowsDeath] = "kSpellArrowsDeath";
	ESpell_name_by_value[ESpell::kSpellPaladineInspiration] = "kSpellPaladineInspiration";
	ESpell_name_by_value[ESpell::kSpellDexterity] = "kSpellDexterity";
	ESpell_name_by_value[ESpell::kSpellGroupBlink] = "kSpellGroupBlink";
	ESpell_name_by_value[ESpell::kSpellGroupCloudly] = "kSpellGroupCloudly";
	ESpell_name_by_value[ESpell::kSpellGroupAwareness] = "kSpellGroupAwareness";
	ESpell_name_by_value[ESpell::kSpellMassFailure] = "kSpellMassFailure";
	ESpell_name_by_value[ESpell::kSpellSnare] = "kSpellSnare";
	ESpell_name_by_value[ESpell::kSpellExpedientFail] = "kSpellExpedientFail";

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

std::ostream& operator<<(std::ostream & os, ESpell &s){
	os << to_underlying(s) << " (" << NAME_BY_ITEM<ESpell>(s) << ")";
	return os;
};

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
