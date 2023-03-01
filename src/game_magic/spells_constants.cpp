/**
\authors Created by Sventovit
\date 27.04.2022.
\brief Константы системы заклинаний.
*/

#include "spells_constants.h"
#include "entities/entities_constants.h"

#include <map>
#include <sstream>

using SpellMsgSet = std::map<ESpellMsg, std::string>;

const std::string &GetSpellMsg(ESpell spell_id, ESpellMsg msg_id) {
	static const std::unordered_map<ESpell, SpellMsgSet> spells_msg =
		{{ESpell::kArmor, {
			{ESpellMsg::kCastPoly, "буде во прибежище"},
			{ESpellMsg::kCastChrist, "... Он - помощь наша и защита наша."},
			{ESpellMsg::kAffExpired, "Вы почувствовали себя менее защищенно."},
			{ESpellMsg::kAffImposedForVict, "Вы почувствовали вокруг себя невидимую защиту."},
			{ESpellMsg::kAffImposedForRoom, "Вокруг $n1 вспыхнул белый щит и тут же погас."}}},
		 {ESpell::kTeleport, {
			 {ESpellMsg::kCastPoly, "несите ветры"},
			 {ESpellMsg::kCastChrist, "... дух поднял меня и перенес меня."},
			 {ESpellMsg::kAffExpired, "!Teleport!"}}},
		 {ESpell::kBless, {
			 {ESpellMsg::kCastPoly, "истягну умь крепостию"},
			 {ESpellMsg::kCastChrist, "... даст блага просящим у Него."},
			 {ESpellMsg::kAffExpired, "Вы почувствовали себя менее доблестно."},
			 {ESpellMsg::kAffImposedForVict, "Боги одарили вас своей улыбкой."},
			 {ESpellMsg::kAffImposedForRoom, "$n осветил$u на миг неземным светом."}}},
		 {ESpell::kBlindness, {
			 {ESpellMsg::kCastPoly, "Чтоб твои зенки вылезли!"},
			 {ESpellMsg::kCastChrist, "... поразит тебя Господь слепотою."},
			 {ESpellMsg::kAffExpired, "Вы вновь можете видеть."},
			 {ESpellMsg::kAffImposedForRoom, "$n0 ослеп$q!"},
			 {ESpellMsg::kAffImposedForVict, "Вы ослепли!"}}},
		 {ESpell::kBurningHands, {
			 {ESpellMsg::kCastPoly, "узри огонь!"},
			 {ESpellMsg::kCastChrist, "... простер руку свою к огню."},
			 {ESpellMsg::kAreaForChar, "С ваших ладоней сорвался поток жаркого пламени."},
			 {ESpellMsg::kAreaForRoom, "$n0 испустил$g поток жаркого багрового пламени!"},
			 {ESpellMsg::kAffExpired, "!Burning Hands!"}}},
		 {ESpell::kCallLighting, {
			 {ESpellMsg::kCastPoly, "Разрази тебя Перун!"},
			 {ESpellMsg::kCastChrist, "... и путь для громоносной молнии."},
			 {ESpellMsg::kAffExpired, "!Call Lightning"},
			 {ESpellMsg::kAffImposedForVict, "Взрыв шаровой молнии $N1 отдался в вашей голове громким звоном."},
			 {ESpellMsg::kAffImposedForRoom, "$n зашатал$u, пытаясь прийти в себя от взрыва шаровой молнии."}}},
		 {ESpell::kCharm, {
			 {ESpellMsg::kCastPoly, "умь полонить"},
			 {ESpellMsg::kCastChrist, "... слушай пастыря сваего, и уразумей."},
			 {ESpellMsg::kAffExpired, "Вы подчиняетесь теперь только себе."},
			 {ESpellMsg::kAffImposedForVict, "Ваша воля склонилась перед $N4."}}},
		 {ESpell::kChillTouch, {
			 {ESpellMsg::kCastPoly, "хладну персты воскладаше"},
			 {ESpellMsg::kCastChrist, "... которые черны от льда."},
			 {ESpellMsg::kAffExpired, "Вы отметили, что силы вернулись к вам."},
			 {ESpellMsg::kAffImposedForVict, "Вы почувствовали себя слабее!"},
			 {ESpellMsg::kAffImposedForRoom, "Боевой пыл $n1 несколько остыл."}}},
		 {ESpell::kClone, {
			 {ESpellMsg::kCastPoly, "пусть будет много меня"},
			 {ESpellMsg::kCastChrist, "... и плодились, и весьма умножились."},
			 {ESpellMsg::kAffExpired, "!Clone!"}}},
		 {ESpell::kIceBolts, {
			 {ESpellMsg::kCastPoly, "хлад и мраз исторгнути"},
			 {ESpellMsg::kCastChrist, "... и из воды делается лед."},
			 {ESpellMsg::kAreaForChar, "Из ваших рук вылетел сноп ледяных стрел."},
			 {ESpellMsg::kAreaForRoom, "$n0 метнул$g во врагов сноп ледяных стрел."},
			 {ESpellMsg::kAffExpired, "!Color Spray!"}}},
		 {ESpell::kControlWeather, {
			 {ESpellMsg::kCastPoly, "стихия подкоряшися"},
			 {ESpellMsg::kCastChrist, "... власть затворить небо, чтобы не шел дождь."},
			 {ESpellMsg::kAffExpired, "!Control Weather!"}}},
		 {ESpell::kCreateFood, {
			 {ESpellMsg::kCastPoly, "будовати снедь"},
			 {ESpellMsg::kCastChrist, "... это хлеб, который Господь дал вам в пищу."},
			 {ESpellMsg::kAffExpired, "!Create Food!"}}},
		 {ESpell::kCreateWater, {
			 {ESpellMsg::kCastPoly, "напоиши влагой"},
			 {ESpellMsg::kCastChrist, "... и потекло много воды."},
			 {ESpellMsg::kAffExpired, "!Create Water!"}}},
		 {ESpell::kCureBlind, {
			 {ESpellMsg::kCastPoly, "зряще узрите"},
			 {ESpellMsg::kCastChrist, "... и прозрят из тьмы и мрака глаза слепых."},
			 {ESpellMsg::kAffExpired, "!Cure Blind!"}}},
		 {ESpell::kCureCritic, {
			 {ESpellMsg::kCastPoly, "гой еси"},
			 {ESpellMsg::kCastChrist, "... да зарубцуются гноища твои."},
			 {ESpellMsg::kAffExpired, "!Cure Critic!"}}},
		 {ESpell::kCureLight, {
			 {ESpellMsg::kCastPoly, "малейше целити раны"},
			 {ESpellMsg::kCastChrist, "... да затянутся раны твои."},
			 {ESpellMsg::kAffExpired, "!Cure Light!"}}},
		 {ESpell::kCurse, {
			 {ESpellMsg::kCastPoly, "порча"},
			 {ESpellMsg::kCastChrist, "... проклят ты пред всеми скотами."},
			 {ESpellMsg::kAffExpired, "Вы почувствовали себя более уверенно."},
			 {ESpellMsg::kAffImposedForRoom, "Красное сияние вспыхнуло над $n4 и тут же погасло!"},
			 {ESpellMsg::kAffImposedForVict, "Боги сурово поглядели на вас."}}},
		 {ESpell::kDetectAlign, {
			 {ESpellMsg::kCastPoly, "узряще норов"},
			 {ESpellMsg::kCastChrist, "... и отделит одних от других, как пастырь отделяет овец от козлов."},
			 {ESpellMsg::kAffExpired, "Вы более не можете определять наклонности."},
			 {ESpellMsg::kAffImposedForVict, "Ваши глаза приобрели зеленый оттенок."},
			 {ESpellMsg::kAffImposedForRoom, "Глаза $n1 приобрели зеленый оттенок."}}},
		 {ESpell::kDetectInvis, {
			 {ESpellMsg::kCastPoly, "взор мечетный"},
			 {ESpellMsg::kCastChrist, "... ибо нет ничего тайного, что не сделалось бы явным."},
			 {ESpellMsg::kAffExpired, "Вы не в состоянии больше видеть невидимых."},
			 {ESpellMsg::kAffImposedForVict, "Ваши глаза приобрели золотистый оттенок."},
			 {ESpellMsg::kAffImposedForRoom, "Глаза $n1 приобрели золотистый оттенок."}}},
		 {ESpell::kDetectMagic, {
			 {ESpellMsg::kCastPoly, "зряще ворожбу"},
			 {ESpellMsg::kCastChrist, "... покажись, ересь богопротивная."},
			 {ESpellMsg::kAffExpired, "Вы не в состоянии более определять магию."},
			 {ESpellMsg::kAffImposedForVict, "Ваши глаза приобрели желтый оттенок."},
			 {ESpellMsg::kAffImposedForRoom, "Глаза $n1 приобрели желтый оттенок."}}},
		 {ESpell::kDetectPoison, {
			 {ESpellMsg::kCastPoly, "зряще трутизну"},
			 {ESpellMsg::kCastChrist, "... по плодам их узнаете их."},
			 {ESpellMsg::kAffExpired, "Вы не в состоянии более определять яды."},
			 {ESpellMsg::kAffImposedForVict, "Ваши глаза приобрели карий оттенок."},
			 {ESpellMsg::kAffImposedForRoom, "Глаза $n1 приобрели карий оттенок."}}},
		 {ESpell::kDispelEvil, {
			 {ESpellMsg::kCastPoly, "долой нощи"},
			 {ESpellMsg::kCastChrist, "... грешников преследует зло, а праведникам воздается добром."},
			 {ESpellMsg::kAffExpired, "!Dispel Evil!"}}},
		 {ESpell::kEarthquake, {
			 {ESpellMsg::kCastPoly, "земля тутнет"},
			 {ESpellMsg::kCastChrist, "... в тот же час произошло великое землетрясение."},
			 {ESpellMsg::kAreaForChar, "Вы опустили руки, и земля начала дрожать вокруг вас!"},
			 {ESpellMsg::kAreaForRoom, "$n опустил$g руки, и земля задрожала!"},
			 {ESpellMsg::kAffExpired, "!Earthquake!"}}},
		 {ESpell::kEnchantWeapon, {
			 {ESpellMsg::kCastPoly, "ницовати стружие"},
			 {ESpellMsg::kCastChrist, "... укрепи сталь Божьим перстом."},
			 {ESpellMsg::kComponentUse, "Вы подготовили дополнительные компоненты для зачарования.\r\n"},
			 {ESpellMsg::kComponentMissing, "Вы были уверены что положили его в этот карман...\r\n"},
			 {ESpellMsg::kComponentExhausted, "$o вспыхнул голубоватым светом, когда его вставили в предмет.\r\n"},
			 {ESpellMsg::kAffExpired, "!Enchant Weapon!"}}},
		 {ESpell::kEnergyDrain, {
			 {ESpellMsg::kCastPoly, "преторгоста"},
			 {ESpellMsg::kCastChrist, "... да иссякнут соки, питающие тело."},
			 {ESpellMsg::kAffExpired, "!Energy Drain!"},
			 {ESpellMsg::kAffImposedForVict, "Вы почувствовали себя слабее!"},
			 {ESpellMsg::kAffImposedForRoom, "$n стал$g немного слабее."}}},
		 {ESpell::kFireball, {
			 {ESpellMsg::kCastPoly, "огненну солнце"},
			 {ESpellMsg::kCastChrist, "... да ниспадет огонь с неба, и пожрет их."},
			 {ESpellMsg::kAffExpired, "!Fireball!"}}},
		 {ESpell::kHarm, {
			 {ESpellMsg::kCastPoly, "згола скверна"},
			 {ESpellMsg::kCastChrist, "... и жестокою болью во всех костях твоих."},
			 {ESpellMsg::kAffExpired, "!Harm!"}}},
		 {ESpell::kHeal, {
			 {ESpellMsg::kCastPoly, "згола гой еси"},
			 {ESpellMsg::kCastChrist, "... тебе говорю, встань."},
			 {ESpellMsg::kAffExpired, "!Heal!"}}},
		 {ESpell::kInvisible, {
			 {ESpellMsg::kCastPoly, "низовати мечетно"},
			 {ESpellMsg::kCastChrist, "... ибо видимое временно, а невидимое вечно."},
			 {ESpellMsg::kAffExpired, "Вы вновь видимы."},
			 {ESpellMsg::kAffImposedForVict, "Вы стали невидимы для окружающих."},
			 {ESpellMsg::kAffImposedForRoom, "$n медленно растворил$u в пустоте."}}},
		 {ESpell::kLightingBolt, {
			 {ESpellMsg::kCastPoly, "грянет гром"},
			 {ESpellMsg::kCastChrist, "... и были громы и молнии."},
			 {ESpellMsg::kAffExpired, "!Lightning Bolt!"}}},
		 {ESpell::kLocateObject, {
			 {ESpellMsg::kCastPoly, "рища, летая умом под облакы"},
			 {ESpellMsg::kCastChrist, "... ибо всякий просящий получает, и ищущий находит."},
			 {ESpellMsg::kAffExpired, "!Locate object!"}}},
		 {ESpell::kMagicMissile, {
			 {ESpellMsg::kCastPoly, "ворожья стрела"},
			 {ESpellMsg::kCastChrist, "... остры стрелы Твои."},
			 {ESpellMsg::kAffExpired, "!Magic Missile!"}}},
		 {ESpell::kPoison, {
			 {ESpellMsg::kCastPoly, "трутизна"},
			 {ESpellMsg::kCastChrist, "... и пошлю на них зубы зверей и яд ползающих по земле."},
			 {ESpellMsg::kAffExpired, "В вашей крови не осталось ни капельки яда."},
			 {ESpellMsg::kAffImposedForVict, "Вы почувствовали себя отравленным."},
			 {ESpellMsg::kAffImposedForRoom, "$n позеленел$g от действия яда."}}},
		 {ESpell::kProtectFromEvil, {
			 {ESpellMsg::kCastPoly, "супостат нощи"},
			 {ESpellMsg::kCastChrist, "... свет, который в тебе, да убоится тьма."},
			 {ESpellMsg::kAffExpired, "Вы вновь ощущаете страх перед тьмой."},
			 {ESpellMsg::kAffImposedForVict, "Вы подавили в себе страх к тьме."},
			 {ESpellMsg::kAffImposedForRoom, "$n подавил$g в себе страх к тьме."}}},
		 {ESpell::kRemoveCurse, {
			 {ESpellMsg::kCastPoly, "изыде порча"},
			 {ESpellMsg::kCastChrist, "... да простятся тебе прегрешения твои."},
			 {ESpellMsg::kAffExpired, "!Remove Curse!"}}},
		 {ESpell::kSanctuary, {
			 {ESpellMsg::kCastPoly, "иже во святых"},
			 {ESpellMsg::kCastChrist, "... буде святым, аки Господь наш свят."},
			 {ESpellMsg::kAffExpired, "Белая аура вокруг вашего тела угасла."},
			 {ESpellMsg::kAffImposedForVict, "Белая аура мгновенно окружила вас."},
			 {ESpellMsg::kAffImposedForRoom, "Белая аура покрыла $n3 с головы до пят."}}},
		 {ESpell::kShockingGasp, {
			 {ESpellMsg::kCastPoly, "воскладше огненну персты"},
			 {ESpellMsg::kCastChrist, "... и дано буде жечь врагов огнем."},
			 {ESpellMsg::kAffExpired, "!Shocking Grasp!"}}},
		 {ESpell::kSleep, {
			 {ESpellMsg::kCastPoly, "иже дремлет"},
			 {ESpellMsg::kCastChrist, "... на веки твои тяжесть покладет."},
			 {ESpellMsg::kAffExpired, "Вы не чувствуете сонливости."}}},
		 {ESpell::kStrength, {
			 {ESpellMsg::kCastPoly, "будет силен"},
			 {ESpellMsg::kCastChrist, "... и человек разумный укрепляет силу свою."},
			 {ESpellMsg::kAffExpired, "Вы чувствуете себя немного слабее."},
			 {ESpellMsg::kAffImposedForVict, "Вы почувствовали себя сильнее."},
			 {ESpellMsg::kAffImposedForRoom, "Мышцы $n1 налились силой."}}},
		 {ESpell::kSummon, {
			 {ESpellMsg::kCastPoly, "кличу-велю"},
			 {ESpellMsg::kCastChrist, "... и послали за ним и призвали его."},
			 {ESpellMsg::kAffExpired, "!Summon!"}}},
		 {ESpell::kPatronage, {
			 {ESpellMsg::kCastPoly, "ибо будет угоден Богам"},
			 {ESpellMsg::kCastChrist, "... ибо спасет людей Своих от грехов их."},
			 {ESpellMsg::kAffExpired, "Вы утратили покровительство высших сил."},
			 {ESpellMsg::kAffImposedForVict, "Исходящий с небес свет на мгновение озарил вас."},
			 {ESpellMsg::kAffImposedForRoom, "Исходящий с небес свет на мгновение озарил $n3."}}},
		 {ESpell::kWorldOfRecall, {
			 {ESpellMsg::kCastPoly, "с глаз долой исчезни"},
			 {ESpellMsg::kCastChrist, "... ступай с миром."},
			 {ESpellMsg::kAffExpired, "!Word of Recall!"}}},
		 {ESpell::kRemovePoison, {
			 {ESpellMsg::kCastPoly, "изыде трутизна"},
			 {ESpellMsg::kCastChrist, "... именем Божьим, изгнати сгниенье из тела."},
			 {ESpellMsg::kAffExpired, "!Remove Poison!"}}},
		 {ESpell::kSenseLife, {
			 {ESpellMsg::kCastPoly, "зряще живота"},
			 {ESpellMsg::kCastChrist, "... ибо нет ничего сокровенного, что не обнаружилось бы."},
			 {ESpellMsg::kAffExpired, "Вы больше не можете чувствовать жизнь."},
			 {ESpellMsg::kAffImposedForVict, "Вы способны разглядеть даже микроба."},
			 {ESpellMsg::kAffImposedForRoom, "$n0 начал$g замечать любые движения."}}},
		 {ESpell::kAnimateDead, {
			 {ESpellMsg::kCastPoly, "живот изо праха створисте"},
			 {ESpellMsg::kCastChrist, "... и земля извергнет мертвецов."},
			 {ESpellMsg::kAffExpired, "!Animate Dead!"}}},
		 {ESpell::kDispelGood, {
			 {ESpellMsg::kCastPoly, "свет сгинь"},
			 {ESpellMsg::kCastChrist, "... и тьма свет накроет."},
			 {ESpellMsg::kAffExpired, "!Dispel Good!"}}},
		 {ESpell::kGroupArmor, {
			 {ESpellMsg::kCastPoly, "прибежище други"},
			 {ESpellMsg::kCastChrist, "... ибо кто Бог, кроме Господа, и кто защита, кроме Бога нашего?"},
			 {ESpellMsg::kAreaForChar, "Вы создали защитную сферу, которая окутала вас и пространство рядом с вами.\r\n"},
			 {ESpellMsg::kAffExpired, "Вы почувствовали себя менее защищенно."},
			 {ESpellMsg::kAffImposedForVict, "Вы почувствовали вокруг себя невидимую защиту."},
			 {ESpellMsg::kAffImposedForRoom, "Вокруг $n1 вспыхнул белый щит и тут же погас."}}},
		 {ESpell::kGroupHeal, {
			 {ESpellMsg::kCastPoly, "други, гой еси"},
			 {ESpellMsg::kCastChrist, "... вам говорю, встаньте."},
			 {ESpellMsg::kAreaForChar, "Вы подняли голову вверх и ощутили яркий свет, ласково бегущий по вашему телу.\r\n"},
			 {ESpellMsg::kAffExpired, "!Group Heal!"}}},
		 {ESpell::kGroupRecall, {
			 {ESpellMsg::kCastPoly, "исчезните с глаз моих"},
			 {ESpellMsg::kCastChrist, "... вам говорю, ступайте с миром."},
			 {ESpellMsg::kAreaForChar, "Вы выкрикнули заклинание и хлопнули в ладоши.\r\n"},
			 {ESpellMsg::kAreaForRoom, "$n0 выкрикнул$g заклинание."},
			 {ESpellMsg::kAffExpired, "!Group Recall!"}}},
		 {ESpell::kInfravision, {
			 {ESpellMsg::kCastPoly, "в нощи зряще"},
			 {ESpellMsg::kCastChrist, "...ибо ни днем, ни ночью сна не знают очи."},
			 {ESpellMsg::kAffExpired, "Вы больше не можете видеть ночью."},
			 {ESpellMsg::kAffImposedForVict, "Ваши глаза приобрели красный оттенок."},
			 {ESpellMsg::kAffImposedForRoom, "Глаза $n1 приобрели красный оттенок."}}},
		 {ESpell::kWaterwalk, {
			 {ESpellMsg::kCastPoly, "по воде аки по суху"},
			 {ESpellMsg::kCastChrist, "... поднимись и ввергнись в море."},
			 {ESpellMsg::kAffExpired, "Вы больше не можете ходить по воде."},
			 {ESpellMsg::kAffImposedForVict, "На рыбалку вы можете отправляться без лодки."}}},
		 {ESpell::kCureSerious, {
			 {ESpellMsg::kCastPoly, "целите раны"},
			 {ESpellMsg::kCastChrist, "... да уменьшатся раны твои."},
			 {ESpellMsg::kAffExpired, "!SPELL CURE SERIOUS!"}}},
		 {ESpell::kGroupStrength, {
			 {ESpellMsg::kCastPoly, "други сильны"},
			 {ESpellMsg::kCastChrist, "... и даст нам Господь силу."},
			 {ESpellMsg::kAreaForChar, "Вы наделили союзников силой медведя.\r\n"},
			 {ESpellMsg::kAffExpired, "Вы чувствуете себя немного слабее."},
			 {ESpellMsg::kAffImposedForVict, "Вы почувствовали себя сильнее."},
			 {ESpellMsg::kAffImposedForRoom, "Мышцы $n1 налились силой."}}},
		 {ESpell::kHold, {
			 {ESpellMsg::kCastPoly, "аки околел"},
			 {ESpellMsg::kCastChrist, "... замри."},
			 {ESpellMsg::kAffExpired, "К вам вернулась способность двигаться."},
			 {ESpellMsg::kAffImposedForRoom, "$n0 замер$q на месте!"},
			 {ESpellMsg::kAffImposedForVict, "Вы замерли на месте, не в силах пошевельнуться."}}},
		 {ESpell::kPowerHold, {
			 {ESpellMsg::kCastPoly, "згола аки околел"},
			 {ESpellMsg::kCastChrist, "... замри надолго."},
			 {ESpellMsg::kAffExpired, "Подвижность вернулась к вам."},
			 {ESpellMsg::kAffImposedForRoom, "$n0 замер$q на месте!"},
			 {ESpellMsg::kAffImposedForVict, "Вы замерли на месте, не в силах пошевельнуться."}}},
		 {ESpell::kMassHold, {
			 {ESpellMsg::kCastPoly, "их окалеть"},
			 {ESpellMsg::kCastChrist, "... замрите."},
			 {ESpellMsg::kAreaForChar, "Вы сжали зубы от боли, когда из вашего тела вырвалось множество невидимых каменных лучей."},
			 {ESpellMsg::kAreaForVict, "В вас попал каменный луч, исходящий от $n1."},
			 {ESpellMsg::kAffExpired, "К вам вернулась способность двигаться."},
			 {ESpellMsg::kAffImposedForRoom, "$n0 замер$q на месте!"},
			 {ESpellMsg::kAffImposedForVict, "Вы замерли на месте, не в силах пошевельнуться."}}},
		 {ESpell::kFly, {
			 {ESpellMsg::kCastPoly, "летать зегзицею"},
			 {ESpellMsg::kCastChrist, "... и полетел, и понесся на крыльях ветра."},
			 {ESpellMsg::kAffExpired, "Вы приземлились на землю."},
			 {ESpellMsg::kAffImposedForRoom, "$n0 медленно поднял$u в воздух."},
			 {ESpellMsg::kAffImposedForVict, "Вы медленно поднялись в воздух."}}},
		 {ESpell::kBrokenChains, {
			 {ESpellMsg::kCastPoly, "вериги и цепи железные изорву"},
			 {ESpellMsg::kCastChrist, "... и цепи упали с рук его."},
			 {ESpellMsg::kAreaForChar, "Вы подняли руки к небу и оно осветилось яркими вспышками!"},
			 {ESpellMsg::kAreaForRoom, "$n поднял$g руки к небу и оно осветилось яркими вспышками!"},
			 {ESpellMsg::kAffExpired, "Вы вновь стали уязвимы для оцепенения."},
			 {ESpellMsg::kAffImposedForRoom, "Ярко-синий ореол вспыхнул вокруг $n1 и тут же угас."},
			 {ESpellMsg::kAffImposedForVict, "Волна ярко-синего света омыла вас с головы до ног."}}},
		 {ESpell::kNoflee, {
			 {ESpellMsg::kCastPoly, "опуташа в путины железны"},
			 {ESpellMsg::kCastChrist, "... да поищешь уйти, да не возможешь."},
			 {ESpellMsg::kAffExpired, "Вы опять можете сбежать с поля боя."},
			 {ESpellMsg::kAffImposedForRoom, "$n0 теперь прикован$a к $N2."},
			 {ESpellMsg::kAffImposedForVict, "Вы не сможете покинуть $N3."}}},
		 {ESpell::kCreateLight, {
			 {ESpellMsg::kCastPoly, "будовати светоч"},
			 {ESpellMsg::kCastChrist, "... да будет свет."},
			 {ESpellMsg::kAffExpired, "!SPELL CREATE LIGHT!"}}},
		 {ESpell::kDarkness, {
			 {ESpellMsg::kCastPoly, "тьмою прикрыты"},
			 {ESpellMsg::kCastChrist, "... тьма покроет землю."},
			 {ESpellMsg::kAffExpired, "Облако тьмы, окружающее вас, спало."},
			 {ESpellMsg::kAffImposedForVict, "Ваши глаза приобрели красный оттенок."},
			 {ESpellMsg::kAffImposedForRoom, "Глаза $n1 приобрели красный оттенок."}}},
		 {ESpell::kStoneSkin, {
			 {ESpellMsg::kCastPoly, "буде тверд аки камень"},
			 {ESpellMsg::kCastChrist, "... твердость ли камней твердость твоя?"},
			 {ESpellMsg::kAffExpired, "Ваша кожа вновь стала мягкой и бархатистой."},
			 {ESpellMsg::kAffImposedForVict, "Вы стали менее чувствительны к ударам."},
			 {ESpellMsg::kAffImposedForRoom, "Кожа $n1 покрылась каменными пластинами."}}},
		 {ESpell::kCloudly, {
			 {ESpellMsg::kCastPoly, "мгла покрыла"},
			 {ESpellMsg::kCastChrist, "... будут как утренний туман."},
			 {ESpellMsg::kAffExpired, "Ваши очертания приобрели отчетливость."},
			 {ESpellMsg::kAffImposedForVict, "Ваше тело стало прозрачным, как туман."},
			 {ESpellMsg::kAffImposedForRoom, "Очертания $n1 расплылись и стали менее отчетливыми."}}},
		 {ESpell::kSilence, {
			 {ESpellMsg::kCastPoly, "типун тебе на язык!"},
			 {ESpellMsg::kCastChrist, "... да замкнутся уста твои."},
			 {ESpellMsg::kAffExpired, "Теперь вы можете болтать, все что думаете."},
			 {ESpellMsg::kAffImposedForRoom, "$n0 прикусил$g язык!"},
			 {ESpellMsg::kAffImposedForVict, "Вы не в состоянии вымолвить ни слова."}}},
		 {ESpell::kLight, {
			 {ESpellMsg::kCastPoly, "буде аки светоч"},
			 {ESpellMsg::kCastChrist, "... и да воссияет над ним свет!"},
			 {ESpellMsg::kAffExpired, "Ваше тело перестало светиться."},
			 {ESpellMsg::kAffImposedForRoom, "$n0 начал$g светиться ярким светом."},
			 {ESpellMsg::kAffImposedForVict, "Вы засветились, освещая комнату."}}},
		 {ESpell::kChainLighting, {
			 {ESpellMsg::kCastPoly, "глаголят небеса"},
			 {ESpellMsg::kCastChrist, "... понесутся меткие стрелы молний из облаков."},
			 {ESpellMsg::kAffExpired, "!SPELL CHAIN LIGHTNING!"}}},
		 {ESpell::kFireBlast, {
			 {ESpellMsg::kCastPoly, "створисте огненну струя"},
			 {ESpellMsg::kCastChrist, "... и ввергне их в озеро огненное."},
			 {ESpellMsg::kAreaForChar, "Вы вызвали потоки подземного пламени!"},
			 {ESpellMsg::kAreaForRoom, "$n0 вызвал$g потоки пламени из глубин земли!"},
			 {ESpellMsg::kAffExpired, "!SPELL FIREBLAST!"}}},
		 {ESpell::kGodsWrath, {
			 {ESpellMsg::kCastPoly, "гнев божиа не минути"},
			 {ESpellMsg::kCastChrist, "... и воспламенится гнев Господа, и Он скоро истребит тебя."},
			 {ESpellMsg::kAffExpired, "!SPELL IMPLOSION!"}}},
		 {ESpell::kWeaknes, {
			 {ESpellMsg::kCastPoly, "буде чахнуть"},
			 {ESpellMsg::kCastChrist, "... и силу могучих ослабляет."},
			 {ESpellMsg::kAffExpired, "Силы вернулись к вам."},
			 {ESpellMsg::kAffImposedForVict, "Вы почувствовали себя слабее!"},
			 {ESpellMsg::kAffImposedForRoom, "$n стал$g немного слабее."}}},
		 {ESpell::kGroupInvisible, {
			 {ESpellMsg::kCastPoly, "други, низовати мечетны."},
			 {ESpellMsg::kCastChrist, "... возвещай всем великую силу Бога. И, сказав сие, они стали невидимы."},
			 {ESpellMsg::kAreaForChar, "Вы вызвали прозрачный туман, поглотивший все дружественное вам.\r\n"},
			 {ESpellMsg::kAffExpired, "Вы вновь видимы."},
			 {ESpellMsg::kAffImposedForVict, "Вы стали невидимы для окружающих."},
			 {ESpellMsg::kAffImposedForRoom, "$n медленно растворил$u в пустоте."},}},
		 {ESpell::kShadowCloak, {
			 {ESpellMsg::kCastPoly, "будут тени и туман, и мрак ночной"},
			 {ESpellMsg::kCastChrist, "... распростираются вечерние тени."},
			 {ESpellMsg::kAffExpired, "Ваша теневая мантия замерцала и растаяла."},
			 {ESpellMsg::kAffImposedForVict, "Густые тени окутали вас."},
			 {ESpellMsg::kAffImposedForRoom, "$n скрыл$u в густой тени."}}},
		 {ESpell::kAcid, {
			 {ESpellMsg::kCastPoly, "жги аки смола горячая"},
			 {ESpellMsg::kCastChrist, "... подобно мучению от скорпиона."},
			 {ESpellMsg::kAffExpired, "!SPELL ACID!"}}},
		 {ESpell::kRepair, {
			 {ESpellMsg::kCastPoly, "будь целым, аки прежде"},
			 {ESpellMsg::kCastChrist, "... заделаю трещины в ней и разрушенное восстановлю."},
			 {ESpellMsg::kAffExpired, "!SPELL REPAIR!"}}},
		 {ESpell::kEnlarge, {
			 {ESpellMsg::kCastPoly, "возросши к небу"},
			 {ESpellMsg::kCastChrist, "... и плоть выросла."},
			 {ESpellMsg::kAffExpired, "Ваши размеры стали прежними."},
			 {ESpellMsg::kAffImposedForVict, "Вы стали крупнее."},
			 {ESpellMsg::kAffImposedForRoom, "$n начал$g расти, как на дрожжах."}}},
		 {ESpell::kFear, {
			 {ESpellMsg::kCastPoly, "падоша в тернии"},
			 {ESpellMsg::kCastChrist, "... убойся того, кто по убиении ввергнет в геенну."},
			 {ESpellMsg::kAffExpired, "!SPELL FEAR!"}}},
		 {ESpell::kSacrifice, {
			 {ESpellMsg::kCastPoly, "да коснется тебя Чернобог"},
			 {ESpellMsg::kCastChrist, "... плоть твоя и тело твое будут истощены."},
			 {ESpellMsg::kAffExpired, "!SPELL SACRIFICE!"}}},
		 {ESpell::kWeb, {
			 {ESpellMsg::kCastPoly, "сети ловчи"},
			 {ESpellMsg::kCastChrist, "... терны и сети на пути коварного."},
			 {ESpellMsg::kAffExpired, "Магическая сеть, покрывавшая вас, исчезла."},
			 {ESpellMsg::kAffImposedForRoom, "$n3 покрыла невидимая паутина, сковывая $s движения!"},
			 {ESpellMsg::kAffImposedForVict, "Вас покрыла невидимая паутина!"}}},
		 {ESpell::kBlink, {
			 {ESpellMsg::kCastPoly, "от стрел укрытие и от меча оборона"},
			 {ESpellMsg::kCastChrist, "...да защитит он себя."},
			 {ESpellMsg::kAffExpired, "Вы перестали мигать."},
			 {ESpellMsg::kAffImposedForRoom, "$n начал$g мигать."},
			 {ESpellMsg::kAffImposedForVict, "Вы начали мигать."}}},
		 {ESpell::kRemoveHold, {
			 {ESpellMsg::kCastPoly, "буде быстр аки прежде"},
			 {ESpellMsg::kCastChrist, "... встань, и ходи."},
			 {ESpellMsg::kAffExpired, "!SPELL REMOVE HOLD!"}}},
		 {ESpell::kCamouflage, {
			 {ESpellMsg::kAffExpired, "Вы стали вновь похожи сами на себя."}}},
		 {ESpell::kPowerBlindness, {
			 {ESpellMsg::kCastPoly, "згола застить очеса"},
			 {ESpellMsg::kCastChrist, "... поразит тебя Господь слепотою навечно."},
			 {ESpellMsg::kAffExpired, "!SPELL POWER BLINDNESS!"}}},
		 {ESpell::kMassBlindness, {
			 {ESpellMsg::kCastPoly, "их очеса непотребны"},
			 {ESpellMsg::kCastChrist, "... и Он поразил их слепотою."},
			 {ESpellMsg::kAreaForChar, "У вас над головой возникла яркая вспышка, которая ослепила все живое."},
			 {ESpellMsg::kAreaForRoom, "Вдруг над головой $n1 возникла яркая вспышка."},
			 {ESpellMsg::kAreaForVict, "Вы невольно взглянули на вспышку света, вызванную $n4, и ваши глаза заслезились."},
			 {ESpellMsg::kAffExpired, "Вы вновь можете видеть."},
			 {ESpellMsg::kAffImposedForRoom, "$n0 ослеп$q!"},
			 {ESpellMsg::kAffImposedForVict, "Вы ослепли!"}}},
		 {ESpell::kPowerSilence, {
			 {ESpellMsg::kCastPoly, "згола не прерчет"},
			 {ESpellMsg::kCastChrist, "... исходящее из уст твоих, да не осквернит слуха."},
			 {ESpellMsg::kAffExpired, "Теперь вы можете болтать, все что думаете."},
			 {ESpellMsg::kAffImposedForRoom, "$n0 прикусил$g язык!"},
			 {ESpellMsg::kAffImposedForVict, "Вы не в состоянии вымолвить ни слова."}}},
		 {ESpell::kExtraHits, {
			 {ESpellMsg::kCastPoly, "буде полон здоровья"},
			 {ESpellMsg::kCastChrist, "... крепкое тело лучше несметного богатства."},
			 {ESpellMsg::kAffExpired, "!SPELL EXTRA HITS!"}}},
		 {ESpell::kResurrection, {
			 {ESpellMsg::kCastPoly, "воскресе из мертвых"},
			 {ESpellMsg::kCastChrist, "... оживут мертвецы Твои, восстанут мертвые тела!"},
			 {ESpellMsg::kAffExpired, "!SPELL RESSURECTION!"}}},
		 {ESpell::kMagicShield, {
			 {ESpellMsg::kCastPoly, "и ворога оберегись"},
			 {ESpellMsg::kCastChrist, "... руками своими да защитит он себя"},
			 {ESpellMsg::kAffExpired, "Ваш волшебный щит рассеялся."},
			 {ESpellMsg::kAffImposedForRoom, "Сверкающий щит вспыхнул вокруг $n1 и угас."},
			 {ESpellMsg::kAffImposedForVict, "Сверкающий щит вспыхнул вокруг вас и угас."}}},
		 {ESpell::kForbidden, {
			 {ESpellMsg::kCastPoly, "вороги не войдут"},
			 {ESpellMsg::kCastChrist, "... ибо положена печать, и никто не возвращается."},
			 {ESpellMsg::kAffExpired, "Магия, запечатывающая входы, пропала."}}},
		 {ESpell::kMassSilence, {
			 {ESpellMsg::kCastPoly, "их уста непотребны"},
			 {ESpellMsg::kCastChrist, "... да замкнутся уста ваши."},
			 {ESpellMsg::kAreaForChar, "Поведя вокруг грозным взглядом, вы заставили всех замолчать."},
			 {ESpellMsg::kAreaForVict, "Вы встретились взглядом с $n4, и у вас появилось ощущение, что горлу чего-то не хватает."},
			 {ESpellMsg::kAffExpired, "Теперь вы можете болтать, все что думаете."},
			 {ESpellMsg::kAffImposedForRoom, "$n0 прикусил$g язык!"},
			 {ESpellMsg::kAffImposedForVict, "Вы не в состоянии вымолвить ни слова."}}},
		 {ESpell::kRemoveSilence, {
			 {ESpellMsg::kCastPoly, "глаголите"},
			 {ESpellMsg::kCastChrist, "... слова из уст мудрого - благодать."},
			 {ESpellMsg::kAffExpired, "!SPELL REMOVE SIELENCE!"}}},
		 {ESpell::kDamageLight, {
			 {ESpellMsg::kCastPoly, "падош"},
			 {ESpellMsg::kCastChrist, "... будет чувствовать боль."},
			 {ESpellMsg::kAffExpired, "!SPELL DAMAGE LIGHT!"}}},
		 {ESpell::kDamageSerious, {
			 {ESpellMsg::kCastPoly, "скверна"},
			 {ESpellMsg::kCastChrist, "... постигнут тебя муки."},
			 {ESpellMsg::kAffExpired, "!SPELL DAMAGE SERIOUS!"}}},
		 {ESpell::kDamageCritic, {
			 {ESpellMsg::kCastPoly, "сильна скверна"},
			 {ESpellMsg::kCastChrist, "... боль и муки схватили."},
			 {ESpellMsg::kAffExpired, "!SPELL DAMAGE CRITIC!"}}},
		 {ESpell::kMassCurse, {
			 {ESpellMsg::kCastPoly, "порча их"},
			 {ESpellMsg::kCastChrist, "... прокляты вы пред всеми скотами."},
			 {ESpellMsg::kAreaForChar, "Медленно оглянувшись, вы прошептали древние слова."},
			 {ESpellMsg::kAreaForVict, "$n злобно посмотрел$g на вас и начал$g шептать древние слова."},
			 {ESpellMsg::kAffExpired, "Вы почувствовали себя более уверенно."},
			 {ESpellMsg::kAffImposedForRoom, "Красное сияние вспыхнуло над $n4 и тут же погасло!"},
			 {ESpellMsg::kAffImposedForVict, "Боги сурово поглядели на вас."}}},
		 {ESpell::kArmageddon, {
			 {ESpellMsg::kCastPoly, "суд божиа не минути"},
			 {ESpellMsg::kCastChrist, "... какою мерою мерите, такою отмерено будет и вам."},
			 {ESpellMsg::kAreaForChar, "Вы сплели руки в замысловатом жесте, и все потускнело!"},
			 {ESpellMsg::kAreaForRoom, "$n сплел$g руки в замысловатом жесте, и все потускнело!"},
			 {ESpellMsg::kAffExpired, "!SPELL ARMAGEDDON!"}}},
		 {ESpell::kGroupFly, {
			 {ESpellMsg::kCastPoly, "крыла им створисте"},
			 {ESpellMsg::kCastChrist, "... и все летающие по роду их."},
			 {ESpellMsg::kAreaForChar, "Ваше заклинание вызвало белое облако, которое разделилось, подхватывая вас и товарищей.\r\n"},
			 {ESpellMsg::kAffExpired, "Вы приземлились на землю."},
			 {ESpellMsg::kAffImposedForRoom, "$n0 медленно поднял$u в воздух."},
			 {ESpellMsg::kAffImposedForVict, "Вы медленно поднялись в воздух."}}},
		 {ESpell::kGroupBless, {
			 {ESpellMsg::kCastPoly, "други, наполнися ратнаго духа"},
			 {ESpellMsg::kCastChrist, "... блажены те, слышащие слово Божие."},
			 {ESpellMsg::kAreaForChar, "Прикрыв глаза, вы прошептали таинственную молитву.\r\n"},
			 {ESpellMsg::kAffExpired, "Вы почувствовали себя менее доблестно."},
			 {ESpellMsg::kAffImposedForVict, "Боги одарили вас своей улыбкой."},
			 {ESpellMsg::kAffImposedForRoom, "$n осветил$u на миг неземным светом."}}},
		 {ESpell::kResfresh, {
			 {ESpellMsg::kCastPoly, "буде свеж"},
			 {ESpellMsg::kCastChrist, "... не будет у него ни усталого, ни изнемогающего."},
			 {ESpellMsg::kAffExpired, "!SPELL REFRESH!"}}},
		 {ESpell::kStunning, {
			 {ESpellMsg::kCastPoly, "да обратит тебя Чернобог в мертвый камень!"},
			 {ESpellMsg::kCastChrist, "... и проклял его именем Господним."},
			 {ESpellMsg::kAffExpired, "Каменное проклятие отпустило вас."}}},
		 {ESpell::kHide, {
			 {ESpellMsg::kAffExpired, "Вы стали заметны окружающим."}}},
		 {ESpell::kSneak, {
			 {ESpellMsg::kAffExpired, "Ваши передвижения стали заметны."}}},
		 {ESpell::kDrunked, {
			 {ESpellMsg::kAffExpired, "Кураж прошел. Мама, лучше бы я умер$q вчера."}}},
		 {ESpell::kAbstinent, {
			 {ESpellMsg::kAffExpired, "А головка ваша уже не болит."}}},
		 {ESpell::kFullFeed, {
			 {ESpellMsg::kCastPoly, "брюхо полно"},
			 {ESpellMsg::kCastChrist, "... душа больше пищи, и тело - одежды."},
			 {ESpellMsg::kAffExpired, "Вам снова захотелось жареного, да с дымком."}}},
		 {ESpell::kColdWind, {
			 {ESpellMsg::kCastPoly, "веют ветры"},
			 {ESpellMsg::kCastChrist, "... подует северный холодный ветер."},
			 {ESpellMsg::kAffExpired, "Вы согрелись и подвижность вернулась к вам."},
			 {ESpellMsg::kAffImposedForVict, "Вы покрылись серебристым инеем."},
			 {ESpellMsg::kAffImposedForRoom, "$n покрыл$u красивым серебристым инеем."}}},
		 {ESpell::kBattle, {
			 {ESpellMsg::kAffExpired, "К вам вернулась способность нормально сражаться."}}},
		 {ESpell::kHaemorrhage, {
			 {ESpellMsg::kAffExpired, "Ваши кровоточащие раны затянулись."}}},
		 {ESpell::kCourage, {
			 {ESpellMsg::kAffExpired, "Вы успокоились."}}},
		 {ESpell::kWaterbreath, {
			 {ESpellMsg::kCastPoly, "не затвори темне березе"},
			 {ESpellMsg::kCastChrist, "... дух дышит, где хочет."},
			 {ESpellMsg::kAffExpired, "Вы более не способны дышать водой."},
			 {ESpellMsg::kAffImposedForVict, "У вас выросли жабры."},
			 {ESpellMsg::kAffImposedForRoom, "У $n1 выросли жабры."}}},
		 {ESpell::kSlowdown, {
			 {ESpellMsg::kCastPoly, "немочь"},
			 {ESpellMsg::kCastChrist, "...и помедлил еще семь дней других."},
			 {ESpellMsg::kAffExpired, "Медлительность исчезла."},
			 {ESpellMsg::kAffImposedForRoom, "Движения $n1 заметно замедлились."},
			 {ESpellMsg::kAffImposedForVict, "Ваши движения заметно замедлились."}}},
		 {ESpell::kHaste, {
			 {ESpellMsg::kCastPoly, "скор аки ястреб"},
			 {ESpellMsg::kCastChrist, "... поднимет его ветер и понесет, и он быстро побежит от него."},
			 {ESpellMsg::kAffExpired, "Вы стали более медлительны."},
			 {ESpellMsg::kAffImposedForVict, "Вы начали двигаться быстрее."},
			 {ESpellMsg::kAffImposedForRoom, "$n начал$g двигаться заметно быстрее."}}},
		 {ESpell::kMassSlow, {
			 {ESpellMsg::kCastPoly, "тернии им"},
			 {ESpellMsg::kCastChrist, "... загорожу путь их тернами."},
			 {ESpellMsg::kAreaForChar, "Положив ладони на землю, вы вызвали цепкие корни,\r\nопутавшие существ, стоящих рядом с вами."},
			 {ESpellMsg::kAreaForVict, "$n вызвал$g цепкие корни, опутавшие ваши ноги."},
			 {ESpellMsg::kAffExpired, "Медлительность исчезла."},
			 {ESpellMsg::kAffImposedForRoom, "Движения $n1 заметно замедлились."},
			 {ESpellMsg::kAffImposedForVict, "Ваши движения заметно замедлились."}}},
		 {ESpell::kGroupHaste, {
			 {ESpellMsg::kCastPoly, "быстры аки ястребов стая"},
			 {ESpellMsg::kCastChrist, "... и они быстры как серны на горах."},
			 {ESpellMsg::kAreaForChar, "Разведя руки в стороны, вы ощутили всю мощь стихии ветра.\r\n"},
			 {ESpellMsg::kAffExpired, "Вы стали более медлительны."},
			 {ESpellMsg::kAffImposedForVict, "Вы начали двигаться быстрее."},
			 {ESpellMsg::kAffImposedForRoom, "$n начал$g двигаться заметно быстрее."}}},
		 {ESpell::kGodsShield, {
			 {ESpellMsg::kCastPoly, "Живый в помощи Вышняго"},
			 {ESpellMsg::kCastChrist, "... благословен буде Грядый во имя Господне."},
			 {ESpellMsg::kAffExpired, "Голубой кокон вокруг вашего тела угас."},
			 {ESpellMsg::kAffImposedForVict, "Вас покрыл голубой кокон."},
			 {ESpellMsg::kAffImposedForRoom, "$n покрыл$u сверкающим коконом."}}},
		 {ESpell::kFever, {
			 {ESpellMsg::kCastPoly, "нутро снеде"},
			 {ESpellMsg::kCastChrist, "... и сделаются жестокие и кровавые язвы."},
			 {ESpellMsg::kAffExpired, "Лихорадка прекратилась."},
			 {ESpellMsg::kAffImposedForVict, "Вас скрутило в жестокой лихорадке."},
			 {ESpellMsg::kAffImposedForRoom, "$n3 скрутило в жестокой лихорадке."}}},
		 {ESpell::kCureFever, {
			 {ESpellMsg::kCastPoly, "Навь, очисти тело"},
			 {ESpellMsg::kCastChrist, "... хочу, очистись."},
			 {ESpellMsg::kAffExpired, "!SPELL CURE PLAQUE!"}}},
		 {ESpell::kAwareness, {
			 {ESpellMsg::kCastPoly, "око недреманно"},
			 {ESpellMsg::kCastChrist, "... не дам сна очам моим и веждам моим - дремания."},
			 {ESpellMsg::kAffExpired, "Вы стали менее внимательны."},
			 {ESpellMsg::kAffImposedForVict, "Вы стали более внимательны к окружающему."},
			 {ESpellMsg::kAffImposedForRoom, "$n начал$g внимательно осматриваться по сторонам."}}},
		 {ESpell::kReligion, {
			 {ESpellMsg::kAffExpired, "Вы утратили расположение Богов."}}},
		 {ESpell::kAirShield, {
			 {ESpellMsg::kCastPoly, "Стрибог, даруй прибежище"},
			 {ESpellMsg::kCastChrist, "... защита от ветра и покров от непогоды."},
			 {ESpellMsg::kAffExpired, "Ваш воздушный щит исчез."},
			 {ESpellMsg::kAffImposedForVict, "Вас окутал воздушный щит."},
			 {ESpellMsg::kAffImposedForRoom, "$n3 окутал воздушный щит."}}},
		 {ESpell::kPortal, {
			 {ESpellMsg::kCastPoly, "буде путь короток"},
			 {ESpellMsg::kCastChrist, "... входите во врата Его."},
			 {ESpellMsg::kAffExpired, "!PORTAL!"}}},
		 {ESpell::kDispellMagic, {
			 {ESpellMsg::kCastPoly, "изыде ворожба"},
			 {ESpellMsg::kCastChrist, "... выйди, дух нечистый."},
			 {ESpellMsg::kAffExpired, "!DISPELL MAGIC!"}}},
		 {ESpell::kSummonKeeper, {
			 {ESpellMsg::kCastPoly, "Сварог, даруй защитника"},
			 {ESpellMsg::kCastChrist, "... и благословен защитник мой!"},
			 {ESpellMsg::kAffExpired, "!SUMMON KEEPER!"}}},
		 {ESpell::kFastRegeneration, {
			 {ESpellMsg::kCastPoly, "заживет, аки на собаке"},
			 {ESpellMsg::kCastChrist, "... нет богатства лучше телесного здоровья."},
			 {ESpellMsg::kAffExpired, "Живительная сила покинула вас."},
			 {ESpellMsg::kAffImposedForVict, "Вас наполнила живительная сила."},
			 {ESpellMsg::kAffImposedForRoom, "$n расцвел$g на ваших глазах."}}},
		 {ESpell::kCreateWeapon, {
			 {ESpellMsg::kCastPoly, "будовати стружие"},
			 {ESpellMsg::kCastChrist, "...вооружите из себя людей на войну"},
			 {ESpellMsg::kAffExpired, "!CREATE WEAPON!"}}},
		 {ESpell::kFireShield, {
			 {ESpellMsg::kCastPoly, "Хорс, даруй прибежище"},
			 {ESpellMsg::kCastChrist, "... душа горячая, как пылающий огонь."},
			 {ESpellMsg::kAffExpired, "Огненный щит вокруг вашего тела исчез."},
			 {ESpellMsg::kAffImposedForVict, "Вас окутал огненный щит."},
			 {ESpellMsg::kAffImposedForRoom, "$n3 окутал огненный щит."}}},
		 {ESpell::kRelocate, {
			 {ESpellMsg::kCastPoly, "Стрибог, укажи путь..."},
			 {ESpellMsg::kCastChrist, "... указывай им путь, по которому они должны идти."},
			 {ESpellMsg::kAffExpired, "!RELOCATE!"}}},
		 {ESpell::kSummonFirekeeper, {
			 {ESpellMsg::kCastPoly, "Дажьбог, даруй защитника"},
			 {ESpellMsg::kCastChrist, "... Ангел Мой с вами, и он защитник душ ваших."},
			 {ESpellMsg::kAffExpired, "!SUMMON FIREKEEPER!"}}},
		 {ESpell::kIceShield, {
			 {ESpellMsg::kCastPoly, "Морена, даруй прибежище"},
			 {ESpellMsg::kCastChrist, "... а снег и лед выдерживали огонь и не таяли."},
			 {ESpellMsg::kAffExpired, "Ледяной щит вокруг вашего тела исчез."},
			 {ESpellMsg::kAffImposedForVict, "Вас окутал ледяной щит."},
			 {ESpellMsg::kAffImposedForRoom, "$n3 окутал ледяной щит."}}},
		 {ESpell::kIceStorm, {
			 {ESpellMsg::kCastPoly, "торже, яко вихор"},
			 {ESpellMsg::kCastChrist, "... и град, величиною в талант, падет с неба."},
			 {ESpellMsg::kAreaForChar, "Вы воздели руки к небу, и тысячи мелких льдинок хлынули вниз!"},
			 {ESpellMsg::kAreaForRoom, "$n воздел$g руки к небу, и тысячи мелких льдинок хлынули вниз!"},
			 {ESpellMsg::kAffExpired, "Ваши мышцы оттаяли и вы снова можете двигаться."},
			 {ESpellMsg::kAffImposedForRoom, "$n3 оглушило."},
			 {ESpellMsg::kAffImposedForVict, "Вас оглушило."}}},
		 {ESpell::kLessening, {
			 {ESpellMsg::kCastPoly, "буде мал аки мышь"},
			 {ESpellMsg::kCastChrist, "... плоть на нем пропадает."},
			 {ESpellMsg::kAffExpired, "Ваши размеры вновь стали прежними."},
			 {ESpellMsg::kAffImposedForVict, "Вы стали мельче."},
			 {ESpellMsg::kAffImposedForRoom, "$n скукожил$u."}}},
		 {ESpell::kShineFlash, {
			 {ESpellMsg::kCastPoly, "засти очи им"},
			 {ESpellMsg::kCastChrist, "... свет пламени из средины огня."},
			 {ESpellMsg::kAffExpired, "Вы прозрели."},
			 {ESpellMsg::kAffImposedForRoom, "$n0 ослеп$q!"},
			 {ESpellMsg::kAffImposedForVict, "Вы ослепли!"}}},
		 {ESpell::kMadness, {
			 {ESpellMsg::kCastPoly, "згола яростен"},
			 {ESpellMsg::kCastChrist, "... и ярость его загорелась в нем."},
			 {ESpellMsg::kAffExpired, "Безумие отпустило вас."}}},
		 {ESpell::kGroupMagicGlass, {
			 {ESpellMsg::kCastPoly, "гладь воды отразит"},
			 {ESpellMsg::kCastChrist, "... воздай им по делам их, по злым поступкам их."},
			 {ESpellMsg::kAreaForChar, "Вы произнесли несколько резких слов, и все вокруг засеребрилось.\r\n"},
			 {ESpellMsg::kAffExpired, "Зеркало магии истаяло."},
			 {ESpellMsg::kAffImposedForVict, "Вас покрыло зеркало магии."},
			 {ESpellMsg::kAffImposedForRoom, "$n3 покрыла зеркальная пелена."}}},
		 {ESpell::kCloudOfArrows, {
			 {ESpellMsg::kCastPoly, "и будут стрелы молний, и зарницы в высях"},
			 {ESpellMsg::kCastChrist, "... соберу на них бедствия и истощу на них стрелы Мои."},
			 {ESpellMsg::kAffExpired, "Облако стрел вокруг вас рассеялось."},
			 {ESpellMsg::kAffImposedForVict, "Вас окружило облако летающих огненных стрел."},
			 {ESpellMsg::kAffImposedForRoom, "$n3 окружило облако летающих огненных стрел."}}},
		 {ESpell::kVacuum, {
			 {ESpellMsg::kCastPoly, "Сдохни!"},
			 {ESpellMsg::kCastChrist, "... и услышав слова сии - пал бездыханен."},
			 {ESpellMsg::kAffExpired, "Пустота вокруг вас исчезла."}}},
		 {ESpell::kMeteorStorm, {
			 {ESpellMsg::kCastPoly, "идти дождю стрелами"},
			 {ESpellMsg::kCastChrist, "... и камни, величиною в талант, падут с неба."},
			 {ESpellMsg::kAffExpired, "Последний громовой камень грянул в землю и все стихло."}}},
		 {ESpell::kStoneHands, {
			 {ESpellMsg::kCastPoly, "сильны велетов руки"},
			 {ESpellMsg::kCastChrist, "... рука Моя пребудет с ним, и мышца Моя укрепит его."},
			 {ESpellMsg::kAffExpired, "Ваши руки вернулись к прежнему состоянию."},
			 {ESpellMsg::kAffImposedForVict, "Ваши руки задубели."},
			 {ESpellMsg::kAffImposedForRoom, "Руки $n1 задубели."}}},
		 {ESpell::kMindless, {
			 {ESpellMsg::kCastPoly, "разум аки мутный омут"},
			 {ESpellMsg::kCastChrist, "... и безумие его с ним."},
			 {ESpellMsg::kAffExpired, "Ваш разум просветлел."},
			 {ESpellMsg::kAffImposedForVict, "Ваш разум помутился!"},
			 {ESpellMsg::kAffImposedForRoom, "$n0 стал$g слаб$g на голову!"}}},
		 {ESpell::kPrismaticAura, {
			 {ESpellMsg::kCastPoly, "окружен радугой"},
			 {ESpellMsg::kCastChrist, "... явится радуга в облаке."},
			 {ESpellMsg::kAffExpired, "Призматическая аура вокруг вашего тела угасла."},
			 {ESpellMsg::kAffImposedForVict, "Вас покрыла призматическая аура."},
			 {ESpellMsg::kAffImposedForRoom, "$n3 покрыла призматическая аура."}}},
		 {ESpell::kEviless, {
			 {ESpellMsg::kCastPoly, "зло творяще"},
			 {ESpellMsg::kCastChrist, "... и ты воздашь им злом."},
			 {ESpellMsg::kAreaForChar, "Вы запросили помощи у Чернобога. Долг перед темными силами стал чуточку больше.."},
			 {ESpellMsg::kAreaForRoom, "Внезапно появившееся чёрное облако скрыло $n3 на мгновение от вашего взгляда."},
			 {ESpellMsg::kAffExpired, "Силы зла оставили вас."},
			 {ESpellMsg::kAffImposedForVict, "Черное облако покрыло вас."},
			 {ESpellMsg::kAffImposedForRoom, "Черное облако покрыло $n3 с головы до пят."}}},
		 {ESpell::kAirAura, {
			 {ESpellMsg::kCastPoly, "Мать-земля, даруй защиту."},
			 {ESpellMsg::kCastChrist, "... поклон тебе матушка земля."},
			 {ESpellMsg::kAreaForChar, "Силы воздуха пришли к вам на помощь и защитили вас.\r\n"},
			 {ESpellMsg::kAffExpired, "Воздушная аура вокруг вас исчезла."},
			 {ESpellMsg::kAffImposedForVict, "Вас окружила воздушная аура."},
			 {ESpellMsg::kAffImposedForRoom, "$n3 окружила воздушная аура."}}},
		 {ESpell::kFireAura, {
			 {ESpellMsg::kCastPoly, "Сварог, даруй защиту."},
			 {ESpellMsg::kCastChrist, "... и огонь низводит с неба."},
			 {ESpellMsg::kAreaForChar, "Силы огня пришли к вам на помощь и защитили вас.\r\n"},
			 {ESpellMsg::kAffExpired, "Огненная аура вокруг вас исчезла."},
			 {ESpellMsg::kAffImposedForVict, "Вас окружила огненная аура."},
			 {ESpellMsg::kAffImposedForRoom, "$n3 окружила огненная аура."}}},
		 {ESpell::kIceAura, {
			 {ESpellMsg::kCastPoly, "Морена, даруй защиту."},
			 {ESpellMsg::kCastChrist, "... текущие холодные воды."},
			 {ESpellMsg::kAreaForChar, "Силы холода пришли к вам на помощь и защитили вас.\r\n"},
			 {ESpellMsg::kAffExpired, "Ледяная аура вокруг вас исчезла."},
			 {ESpellMsg::kAffImposedForVict, "$n3 окружила ледяная аура."},
			 {ESpellMsg::kAffImposedForRoom, "Вас окружила ледяная аура."}}},
		 {ESpell::kShock, {
			 {ESpellMsg::kCastPoly, "будет слеп и глух, аки мертвец"},
			 {ESpellMsg::kCastChrist, "... кто делает или глухим, или слепым."},
			 {ESpellMsg::kAreaForChar, "Яркая вспышка слетела с кончиков ваших пальцев и с оглушительным грохотом взорвалась в воздухе."},
			 {ESpellMsg::kAreaForRoom, "Выпущенная $n1 яркая вспышка с оглушительным грохотом взорвалась в воздухе."},
			 {ESpellMsg::kAffExpired, "Ваше сознание прояснилось."},
			 {ESpellMsg::kAffImposedForRoom, "$n1 в шоке!"},
			 {ESpellMsg::kAffImposedForVict, "Вас повергло в шок."}}},
		 {ESpell::kMagicGlass, {
			 {ESpellMsg::kCastPoly, "Аз воздам!"},
			 {ESpellMsg::kCastChrist, "... и воздам каждому из вас."},
			 {ESpellMsg::kAffExpired, "Вы вновь чувствительны к магическим поражениям."},
			 {ESpellMsg::kAffImposedForVict, "Вас покрыло зеркало магии."},
			 {ESpellMsg::kAffImposedForRoom, "$n3 покрыла зеркальная пелена."}}},
		 {ESpell::kGroupSanctuary, {
			 {ESpellMsg::kCastPoly, "иже во святых, други"},
			 {ESpellMsg::kCastChrist, "... будьте святы, аки Господь наш свят."},
			 {ESpellMsg::kAreaForChar, "Вы подняли руки к небу и произнесли священную молитву.\r\n"},
			 {ESpellMsg::kAffExpired, "Белая аура вокруг вашего тела угасла."},
			 {ESpellMsg::kAffImposedForVict, "Белая аура мгновенно окружила вас."},
			 {ESpellMsg::kAffImposedForRoom, "Белая аура покрыла $n3 с головы до пят."}}},
		 {ESpell::kGroupPrismaticAura, {
			 {ESpellMsg::kCastPoly, "други, буде окружены радугой"},
			 {ESpellMsg::kCastChrist, "... взгляни на радугу, и прославь Сотворившего ее."},
			 {ESpellMsg::kAreaForChar, "Силы духа, призванные вами, окутали вас и окружающих голубоватым сиянием.\r\n"},
			 {ESpellMsg::kAffExpired, "Призматическая аура вокруг вашего тела угасла."},
			 {ESpellMsg::kAffImposedForVict, "Вас покрыла призматическая аура."},
			 {ESpellMsg::kAffImposedForRoom, "$n3 покрыла призматическая аура."}}},
		 {ESpell::kDeafness, {
			 {ESpellMsg::kCastPoly, "оглохни"},
			 {ESpellMsg::kCastChrist, "... и глухота поразит тебя."},
			 {ESpellMsg::kAffExpired, "Вы вновь можете слышать."},
			 {ESpellMsg::kAffImposedForRoom, "$n0 оглох$q!"},
			 {ESpellMsg::kAffImposedForVict, "Вы оглохли."}}},
		 {ESpell::kPowerDeafness, {
			 {ESpellMsg::kCastPoly, "да застит уши твои"},
			 {ESpellMsg::kCastChrist, "... и будь глухим надолго."},
			 {ESpellMsg::kAffExpired, "Вы вновь можете слышать."},
			 {ESpellMsg::kAffImposedForRoom, "$n0 оглох$q!"},
			 {ESpellMsg::kAffImposedForVict, "Вы оглохли."}}},
		 {ESpell::kRemoveDeafness, {
			 {ESpellMsg::kCastPoly, "слушай глас мой"},
			 {ESpellMsg::kCastChrist, "... услышь слово Его."},
			 {ESpellMsg::kAffExpired, "!SPELL_REMOVE_DEAFNESS!"},
			 {ESpellMsg::kAffImposedForRoom, "$n0 оглох$q!"},
			 {ESpellMsg::kAffImposedForVict, "Вы оглохли."}}},
		 {ESpell::kMassDeafness, {
			 {ESpellMsg::kCastPoly, "будьте глухи"},
			 {ESpellMsg::kCastChrist, "... и не будут слышать уши ваши."},
			 {ESpellMsg::kAreaForChar, "Вы нахмурились, склонив голову, и громкий хлопок сотряс воздух."},
			 {ESpellMsg::kAreaForRoom, "Как только $n0 склонил$g голову, раздался оглушающий хлопок."},
			 {ESpellMsg::kAffExpired, "Вы вновь можете слышать."},
			 {ESpellMsg::kAffImposedForRoom, "$n0 оглох$q!"},
			 {ESpellMsg::kAffImposedForVict, "Вы оглохли."}}},
		 {ESpell::kDustStorm, {
			 {ESpellMsg::kCastPoly, "пыль поднимется столбами"},
			 {ESpellMsg::kCastChrist, "... и пыль поглотит вас."},
			 {ESpellMsg::kAreaForChar, "Вы взмахнули руками и вызвали огромное пылевое облако, скрывшее все вокруг."},
			 {ESpellMsg::kAreaForRoom, "Вас поглотила пылевая буря, вызванная $n4."},
			 {ESpellMsg::kAffExpired, "Вы протерли глаза от пыли."},
			 {ESpellMsg::kAffImposedForRoom, "$n0 ослеп$q!"},
			 {ESpellMsg::kAffImposedForVict, "Вы ослепли!"}}},
		 {ESpell::kEarthfall, {
			 {ESpellMsg::kCastPoly, "пусть каменья падут"},
			 {ESpellMsg::kCastChrist, "... и обрушатся камни с небес."},
			 {ESpellMsg::kAreaForChar, "Вы высоко подбросили комок земли и он, увеличиваясь на глазах, обрушился вниз."},
			 {ESpellMsg::kAreaForRoom, "$n высоко подбросил$g комок земли, который, увеличиваясь на глазах, стал падать вниз."},
			 {ESpellMsg::kAffExpired, "Громкий звон в голове несколько поутих."},
			 {ESpellMsg::kAffImposedForRoom, "$n3 оглушило."},
			 {ESpellMsg::kAffImposedForVict, "Вас оглушило."}}},
		 {ESpell::kSonicWave, {
			 {ESpellMsg::kCastPoly, "да невзлюбит тебя воздух"},
			 {ESpellMsg::kCastChrist, "... и даже воздух покарает тебя."},
			 {ESpellMsg::kAreaForChar, "Вы оттолкнули от себя воздух руками, и он плотным кольцом стремительно двинулся во все стороны!"},
			 {ESpellMsg::kAreaForRoom, "$n махнул$g руками, и огромное кольцо сжатого воздуха распостранилось во все стороны!"},
			 {ESpellMsg::kAffExpired, "!SPELL_SONICWAVE!"}}},
		 {ESpell::kHolystrike, {
			 {ESpellMsg::kCastPoly, "Велес, упокой мертвых"},
			 {ESpellMsg::kCastChrist, "... и предоставь мертвым погребать своих мертвецов."},
			 {ESpellMsg::kAffExpired, "!SPELL_HOLYSTRIKE!"},
			 {ESpellMsg::kAffImposedForRoom, "$n0 замер$q на месте!"},
			 {ESpellMsg::kAffImposedForVict, "Вы замерли на месте, не в силах пошевельнуться."}}},
		 {ESpell::kSumonAngel, {
			 {ESpellMsg::kCastPoly, "Боги, даруйте защитника"},
			 {ESpellMsg::kCastChrist, "... дабы уберег он меня от зла."},
			 {ESpellMsg::kAffExpired, "!SPELL_SPELL_ANGEL!"}}},
		 {ESpell::kMassFear, {
			 {ESpellMsg::kCastPoly, "Поврещу сташивые души их в скарядие!"},
			 {ESpellMsg::kCastChrist, "... и затмил ужас разум их."},
			 {ESpellMsg::kAreaForChar, "Вы оглядели комнату устрашающим взглядом, заставив всех содрогнуться."},
			 {ESpellMsg::kAreaForRoom, "$n0 оглядел$g комнату устрашающим взглядом."},
			 {ESpellMsg::kAffExpired, "!SPELL_SPELL_MASS_FEAR!"}}},
		 {ESpell::kFascination, {
			 {ESpellMsg::kCastPoly, "Да пребудет с тобой вся краса мира!"},
			 {ESpellMsg::kCastChrist, "... и омолодил он, и украсил их."},
			 {ESpellMsg::kComponentUse, "Вы взяли череп летучей мыши в левую руку.\r\n"},
			 {ESpellMsg::kComponentMissing, "Батюшки светы! А помаду-то я дома забыл$g.\r\n"},
			 {ESpellMsg::kComponentExhausted, "$o рассыпался в ваших руках от неловкого движения.\r\n"},
			 {ESpellMsg::kAffExpired, "Ваша красота куда-то пропала."},
			 {ESpellMsg::kAffImposedForVict, "Вы попытались вспомнить уроки старой цыганки, что учила вас людям головы морочить."},
			 {ESpellMsg::kAffImposedForRoom, "$n0 достал$g из маленькой сумочки какие-то вонючие порошки и отвернул$u, бормоча под нос \r\n\"..так это на ресницы надо, кажется... Эх, только бы не перепутать...\" \r\n"}}},
		 {ESpell::kCrying, {
			 {ESpellMsg::kCastPoly, "Будут слезы твои, аки камень на сердце"},
			 {ESpellMsg::kCastChrist, "... и постигнет твой дух угнетение вечное."},
			 {ESpellMsg::kAffExpired, "Ваша душа успокоилась."},
			 {ESpellMsg::kAffImposedForRoom, "$n0 издал$g протяжный стон."},
			 {ESpellMsg::kAffImposedForVict, "Вы впали в уныние."}}},
		 {ESpell::kOblivion, {
			 {ESpellMsg::kCastPoly, "будь живот аки буява с шерстнями."},
			 {ESpellMsg::kCastChrist, "... опадет на тебя чернь страшная."},
			 {ESpellMsg::kAffExpired, "Тёмная вода забвения схлынула из вашего раузма."},
			 {ESpellMsg::kAffImposedForRoom, "Облако забвения окружило $n3."},
			 {ESpellMsg::kAffImposedForVict, "Ваш разум помутился."}}},
		 {ESpell::kBurdenOfTime, {
			 {ESpellMsg::kCastPoly, "Яко небытие нещадно к вам, али время вернулось вспять."},
			 {ESpellMsg::kCastChrist, "... и время не властно над ними."},
			 {ESpellMsg::kAreaForChar, "Вы скрестили руки на груди, вызвав яркую вспышку синего света."},
			 {ESpellMsg::kAreaForRoom, "$n0 скрестил$g руки на груди, вызвав яркую вспышку синего света."},
			 {ESpellMsg::kAffExpired, "Иновременное небытие больше не властно над вами."},
			 {ESpellMsg::kAffImposedForRoom, "Облако забвения окружило $n3."},
			 {ESpellMsg::kAffImposedForVict, "Ваш разум помутился."}}},
		 {ESpell::kGroupRefresh, {
			 {ESpellMsg::kCastPoly, "Исполняше други силою!"},
			 {ESpellMsg::kCastChrist, "...да не останется ни обделенного, ни обессиленного."},
			 {ESpellMsg::kAreaForChar, "Ваша магия наполнила воздух зеленоватым сиянием.\r\n"},
			 {ESpellMsg::kAffExpired, "!SPELL_GROUP_REFRESH!"}}},
		 {ESpell::kPeaceful, {
			 {ESpellMsg::kCastPoly, "Избавь речь свою от недобрых слов, а ум - от крамольных мыслей."},
			 {ESpellMsg::kCastChrist, "... любите врагов ваших и благотворите ненавидящим вас."},
			 {ESpellMsg::kAffExpired, "Смирение в вашей душе вдруг куда-то исчезло."},
			 {ESpellMsg::kAffImposedForRoom, "Взгляд $n1 потускнел, а сам он успокоился."},
			 {ESpellMsg::kAffImposedForVict, "Ваша душа очистилась от зла и странно успокоилась."}}},
		 {ESpell::kMagicBattle, {
			 {ESpellMsg::kAffExpired, "К вам вернулась способность нормально сражаться."}}},
		 {ESpell::kBerserk, {
			 {ESpellMsg::kAffExpired, "Неистовство оставило вас."}}},
		 {ESpell::kStoneBones, {
			 {ESpellMsg::kCastPoly, "Обращу кости их в твердый камень."},
			 {ESpellMsg::kCastChrist, "...и тот, кто упадет на камень сей, разобьется."},
			 {ESpellMsg::kAffExpired, "Читить вредно!"},
			 {ESpellMsg::kAffImposedForVict, "Читим-с?! Нехорошо! Инцидент запротоколирован."},
			 {ESpellMsg::kAffImposedForRoom, "Кости $n1 обрели твердость кремня."}}},
		 {ESpell::kRoomLight, {
			 {ESpellMsg::kCastPoly, "Да буде СВЕТ !!!"},
			 {ESpellMsg::kCastChrist, "...ибо сказал МОНТЕР !!!"},
			 {ESpellMsg::kAffExpired, "Колдовской свет угас."}}},
		 {ESpell::kDeadlyFog, {
			 {ESpellMsg::kCastPoly, "Порча великая туманом обернись!"},
			 {ESpellMsg::kCastChrist, "...и смерть явись в тумане его."},
			 {ESpellMsg::kAffExpired, "Порыв ветра развеял туман смерти."}}},
		 {ESpell::kThunderstorm, {
			 {ESpellMsg::kCastPoly, "Абие велий вихрь деяти!"},
			 {ESpellMsg::kCastChrist, "...творит молнии при дожде, изводит ветер из хранилищ Своих."},
			 {ESpellMsg::kAffExpired, "Ветер прогнал грозовые тучи."}}},
		 {ESpell::kLightWalk, {
			 {ESpellMsg::kAffExpired, "Ваши следы вновь стали заметны."}}},
		 {ESpell::kFailure, {
			 {ESpellMsg::kCastPoly, "аще доля зла и удача немилостива"},
			 {ESpellMsg::kCastChrist, ".. и несчастен, и жалок, и нищ."},
			 {ESpellMsg::kAreaForChar, "Вы простерли руки над головой, вызвав череду раскатов грома."},
			 {ESpellMsg::kAreaForRoom, "$n0 вызвал$g череду раскатов грома, заставивших все вокруг содрогнуться."},
			 {ESpellMsg::kAffExpired, "Удача вновь вернулась к вам."},
			 {ESpellMsg::kAffImposedForRoom, "Тяжелое бурое облако сгустилось над $n4."},
			 {ESpellMsg::kAffImposedForVict, "Тяжелые тучи сгустились над вами, и вы почувствовали, что удача покинула вас."}}},
		 {ESpell::kClanPray, {
			 {ESpellMsg::kAffExpired, "Магические чары ослабели со временем и покинули вас."}}},
		 {ESpell::kGlitterDust, {
			 {ESpellMsg::kCastPoly, "зрети супостат охабиша"},
			 {ESpellMsg::kCastChrist, "...и бросали пыль на воздух."},
			 {ESpellMsg::kAreaForChar, "Вы слегка прищелкнули пальцами, и вокруг сгустилось облако блестящей пыли."},
			 {ESpellMsg::kAreaForRoom, "$n0 сотворил$g облако блестящей пыли, медленно осевшее на землю."},
			 {ESpellMsg::kAffExpired, "Покрывавшая вас блестящая пыль осыпалась и растаяла в воздухе."},
			 {ESpellMsg::kAffImposedForRoom, "Облако ярко блестящей пыли накрыло $n3."},
			 {ESpellMsg::kAffImposedForVict, "Липкая блестящая пыль покрыла вас с головы до пят."}}},
		 {ESpell::kScream, {
			 {ESpellMsg::kCastPoly, "язвень голки уведати"},
			 {ESpellMsg::kCastChrist, "...но в полночь раздался крик."},
			 {ESpellMsg::kAreaForChar, "Вы испустили кошмарный вопль, едва не разорвавший вам горло."},
			 {ESpellMsg::kAreaForRoom, "$n0 испустил$g кошмарный вопль, отдавшийся в вашей душе замогильным холодом."},
			 {ESpellMsg::kAffExpired, "Леденящий душу испуг отпустил вас."},
			 {ESpellMsg::kAffImposedForRoom, "$n0 побледнел$g и задрожал$g от страха."},
			 {ESpellMsg::kAffImposedForVict, "Страх сжал ваше сердце ледяными когтями."}}},
		 {ESpell::kCatGrace, {
			 {ESpellMsg::kCastPoly, "ристати споро"},
			 {ESpellMsg::kCastChrist, "...и не уязвит враг того, кто скор."},
			 {ESpellMsg::kAffExpired, "Ваши движения утратили прежнюю колдовскую ловкость."},
			 {ESpellMsg::kAffImposedForVict, "Ваши движения обрели невиданную ловкость."},
			 {ESpellMsg::kAffImposedForRoom, "Движения $n1 обрели невиданную ловкость."}}},
		 {ESpell::kBullBody, {
			 {ESpellMsg::kCastPoly, "руци яре ворога супротив"},
			 {ESpellMsg::kCastChrist, "...и мощь звериная жила в теле его."},
			 {ESpellMsg::kAffExpired, "Ваше телосложение вновь стало обычным."},
			 {ESpellMsg::kAffImposedForVict, "Ваше тело налилось звериной мощью."},
			 {ESpellMsg::kAffImposedForRoom, "Плечи $n1 раздались вширь, а тело налилось звериной мощью."}}},
		 {ESpell::kSnakeWisdom, {
			 {ESpellMsg::kCastPoly, "веси и зрети стези отай"},
			 {ESpellMsg::kCastChrist, "...и даровал мудрость ему."},
			 {ESpellMsg::kAffExpired, "Вы утратили навеянную магией мудрость."},
			 {ESpellMsg::kAffImposedForVict, "Шелест змеиной чешуи коснулся вашего сознания, и вы стали мудрее."},
			 {ESpellMsg::kAffImposedForRoom, "$n спокойно и мудро посмотрел$g вокруг."}}},
		 {ESpell::kGimmicry, {
			 {ESpellMsg::kCastPoly, "клюка вящего улучити"},
			 {ESpellMsg::kCastChrist, "...ибо кто познал ум Господень?"},
			 {ESpellMsg::kAffExpired, "Навеянная магией хитрость покинула вас."},
			 {ESpellMsg::kAffImposedForVict, "Вы почувствовали, что для вашего ума более нет преград."},
			 {ESpellMsg::kAffImposedForRoom, "$n хитро прищурил$u и поглядел$g по сторонам."}}},
		 {ESpell::kWarcryOfChallenge, {
			 {ESpellMsg::kCastPoly, "Эй, псы шелудивые, керасти плешивые, ослопы беспорточные, мшицы задочные!"},
			 {ESpellMsg::kCastChrist, "Эй, псы шелудивые, керасти плешивые, ослопы беспорточные, мшицы задочные!"},
			 {ESpellMsg::kAreaForRoom, "Вы не стерпели насмешки, и бросились на $n1!"},
			 {ESpellMsg::kAffExpired, "!SPELL_WC_OF_CHALLENGE!"}}},
		 {ESpell::kWarcryOfMenace, {
			 {ESpellMsg::kCastPoly, "Покрошу-изувечу, душу выну и в блины закатаю!"},
			 {ESpellMsg::kCastChrist, "Покрошу-изувечу, душу выну и в блины закатаю!"},
			 {ESpellMsg::kAffExpired, "Похоже, к вам вернулась удача."},
			 {ESpellMsg::kAffImposedForVict, "Похоже, сегодня не ваш день."},
			 {ESpellMsg::kAffImposedForRoom, "Удача покинула $n3."}}},
		 {ESpell::kWarcryOfRage, {
			 {ESpellMsg::kCastPoly, "Не отступим, други, они наше сало сперли!"},
			 {ESpellMsg::kCastChrist, "Не отступим, други, они наше сало сперли!"},
			 {ESpellMsg::kAffExpired, "!SPELL_WC_OF_RAGE!"},
			 {ESpellMsg::kAffImposedForRoom, "$n0 оглох$q!"},
			 {ESpellMsg::kAffImposedForVict, "Вы оглохли."}}},
		 {ESpell::kWarcryOfMadness, {
			 {ESpellMsg::kCastPoly, "Всех убью, а сам$g останусь!"},
			 {ESpellMsg::kCastChrist, "Всех убью, а сам$g останусь!"},
			 {ESpellMsg::kAffExpired, "Рассудок вернулся к вам."},
			 {ESpellMsg::kAffImposedForRoom, "$n0 потерял$g рассудок."},
			 {ESpellMsg::kAffImposedForVict, "Вас обуяло безумие!"}}},
		 {ESpell::kWarcryOfThunder, {
			 {ESpellMsg::kCastPoly, "Шоб вас приподняло, да шлепнуло!!!"},
			 {ESpellMsg::kCastChrist, "Шоб вас приподняло да шлепнуло!!!"},
			 {ESpellMsg::kAffExpired, "!SPELL_WC_OF_THUNDER!"},
			 {ESpellMsg::kAffImposedForRoom, "$n0 оглох$q!"},
			 {ESpellMsg::kAffImposedForVict, "Вы оглохли."}}},
		 {ESpell::kWarcryOfDefence, {
			 {ESpellMsg::kCastPoly, "В строй други, защитим животами Русь нашу!"},
			 {ESpellMsg::kCastChrist, "В строй други, защитим животами Русь нашу!"},
			 {ESpellMsg::kAffExpired, "Действие клича 'призыв к обороне' закончилось."}}},
		 {ESpell::kWarcryOfBattle, {
			 {ESpellMsg::kCastPoly, "Дер-ржать строй, волчьи хвосты!"},
			 {ESpellMsg::kCastChrist, "Дер-ржать строй, волчьи хвосты!"},
			 {ESpellMsg::kAffExpired, "Действие клича битвы закончилось."}}},
		 {ESpell::kWarcryOfPower, {
			 {ESpellMsg::kCastPoly, "Сарынь на кичку!"},
			 {ESpellMsg::kCastChrist, "Сарынь на кичку!"},
			 {ESpellMsg::kAffExpired, "Действие клича мощи закончилось."}}},
		 {ESpell::kWarcryOfBless, {
			 {ESpellMsg::kCastPoly, "Стоять крепко! За нами Киев, Подол и трактир с пивом!!!"},
			 {ESpellMsg::kCastChrist, "Стоять крепко! За нами Киев, Подол и трактир с пивом!!!"},
			 {ESpellMsg::kAffExpired, "Действие клича доблести закончилось."}}},
		 {ESpell::kWarcryOfCourage, {
			 {ESpellMsg::kCastPoly, "Орлы! Будем биться как львы!"},
			 {ESpellMsg::kCastChrist, "Орлы! Будем биться как львы!"},
			 {ESpellMsg::kAffExpired, "Действие клича отваги закончилось."}}},
		 {ESpell::kRuneLabel, {
			 {ESpellMsg::kCastPoly, "...пьсати черты и резы."},
			 {ESpellMsg::kCastChrist, "...и Сам отошел от них на вержение камня."},
			 {ESpellMsg::kAffExpired, "Магические письмена на земле угасли."}}},
		 {ESpell::kAconitumPoison, {
			 {ESpellMsg::kCastPoly, "трутизна"},
			 {ESpellMsg::kCastChrist, "... и пошлю на них зубы зверей и яд ползающих по земле."},
			 {ESpellMsg::kAffExpired, "В вашей крови не осталось ни капельки яда."},
			 {ESpellMsg::kAffImposedForVict, "Вы почувствовали себя отравленным."},
			 {ESpellMsg::kAffImposedForRoom, "$n позеленел$g от действия яда."}}},
		 {ESpell::kScopolaPoison, {
			 {ESpellMsg::kCastPoly, "трутизна"},
			 {ESpellMsg::kCastChrist, "... и пошлю на них зубы зверей и яд ползающих по земле."},
			 {ESpellMsg::kAffExpired, "В вашей крови не осталось ни капельки яда."},
			 {ESpellMsg::kAffImposedForVict, "Вы почувствовали себя отравленным."},
			 {ESpellMsg::kAffImposedForRoom, "$n позеленел$g от действия яда."}}},
		 {ESpell::kBelenaPoison, {
			 {ESpellMsg::kCastPoly, "трутизна"},
			 {ESpellMsg::kCastChrist, "... и пошлю на них зубы зверей и яд ползающих по земле."},
			 {ESpellMsg::kAffExpired, "В вашей крови не осталось ни капельки яда."},
			 {ESpellMsg::kAffImposedForVict, "Вы почувствовали себя отравленным."},
			 {ESpellMsg::kAffImposedForRoom, "$n позеленел$g от действия яда."}}},
		 {ESpell::kDaturaPoison, {
			 {ESpellMsg::kCastPoly, "трутизна"},
			 {ESpellMsg::kCastChrist, "... и пошлю на них зубы зверей и яд ползающих по земле."},
			 {ESpellMsg::kAffExpired, "В вашей крови не осталось ни капельки яда."},
			 {ESpellMsg::kAffImposedForVict, "Вы почувствовали себя отравленным."},
			 {ESpellMsg::kAffImposedForRoom, "$n позеленел$g от действия яда."}}},
		 {ESpell::kTimerRestore, {
			 {ESpellMsg::kAffExpired, "SPELL_TIMER_REPAIR"}}},
		 {ESpell::kCombatLuck, {
			 {ESpellMsg::kAffExpired, "!SPELL_CombatLuck!"},
			 {ESpellMsg::kAffImposedForRoom, "$n вдохновенно выпятил$g грудь."},
			 {ESpellMsg::kAffImposedForVict, "Вы почувствовали вдохновение."}}},
		 {ESpell::kBandage, {
			 {ESpellMsg::kAffExpired, "Вы аккуратно перевязали свои раны."}}},
		 {ESpell::kNoBandage, {
			 {ESpellMsg::kAffExpired, "Вы снова можете перевязывать свои раны."}}},
		 {ESpell::kCapable, {
			 {ESpellMsg::kAffExpired, "!SPELL_CAPABLE!"}}},
		 {ESpell::kStrangle, {
			 {ESpellMsg::kAffExpired, "Удушье отпустило вас, и вы вздохнули полной грудью."}}},
		 {ESpell::kRecallSpells, {
			 {ESpellMsg::kAffExpired, "Вам стало не на чем концентрироваться."}}},
		 {ESpell::kHypnoticPattern, {
			 {ESpellMsg::kCastPoly, "ажбо супостаты блазнити да клюковати"},
			 {ESpellMsg::kCastChrist, "...и утроба его приготовляет обман."},
			 {ESpellMsg::kComponentUse, "Вы разожгли палочку заморских благовоний.\r\n"},
			 {ESpellMsg::kComponentMissing, "Вы начали суматошно искать свои благовония, но тщетно.\r\n"},
			 {ESpellMsg::kComponentExhausted, "$o дотлели и рассыпались пеплом.\r\n"},
			 {ESpellMsg::kAffExpired, "Плывший в воздухе огненный узор потускнел и растаял струйками дыма."}}},
		 {ESpell::kSolobonus, {
			 {ESpellMsg::kAffExpired, "Одна из наград прекратила действовать."}}},
		 {ESpell::kVampirism, {
			 {ESpellMsg::kAffExpired, "Кровавая жажда оставила вас."},
			 {ESpellMsg::kAffImposedForRoom, "Зрачки $n3 приобрели красный оттенок."},
			 {ESpellMsg::kAffImposedForVict, "Ваши зрачки приобрели красный оттенок."}}},
		 {ESpell::kRestoration, {
			 {ESpellMsg::kCastPoly, "Да прими вид прежний, якой был."},
			 {ESpellMsg::kCastChrist, ".. Воззри на предмет сей Отче и верни ему силу прежнюю."},
			 {ESpellMsg::kAffExpired, "!SPELLS_RESTORATION!"}}},
		 {ESpell::kDeathAura, {
			 {ESpellMsg::kCastPoly, "Надели силою своею Навь, дабы собрать урожай тебе."},
			 {ESpellMsg::kCastChrist, "...налякай ворогов наших и покарай их немощью."},
			 {ESpellMsg::kAffExpired, "Силы нави покинули вас."}}},
		 {ESpell::kRecovery, {
			 {ESpellMsg::kCastPoly, "Обрасти плотью сызнова."},
			 {ESpellMsg::kCastChrist, "... прости Господи грехи, верни плоть созданию."},
			 {ESpellMsg::kAffExpired, "!SPELL_RECOVERY!"}}},
		 {ESpell::kMassRecovery, {
			 {ESpellMsg::kCastPoly, "Обрастите плотью сызнова."},
			 {ESpellMsg::kCastChrist, "... прости Господи грехи, верни плоть созданиям."},
			 {ESpellMsg::kAffExpired, "!SPELL_MASS_RECOVERY!"}}},
		 {ESpell::kAuraOfEvil, {
			 {ESpellMsg::kCastPoly, "Возьми личину зла для жатвы славной."},
			 {ESpellMsg::kCastChrist, "Надели силой злою во благо."},
			 {ESpellMsg::kAffExpired, "Аура зла больше не помогает вам."}}},
		 {ESpell::kMentalShadow, {
			 {ESpellMsg::kCastPoly, "Силою мысли защиту будую себе."},
			 {ESpellMsg::kCastChrist, "Даруй Отче защиту, силой разума воздвигнутую."},
			 {ESpellMsg::kAffExpired, "!SPELL_MENTAL_SHADOW!"}}},
		 {ESpell::kBlackTentacles, {
			 {ESpellMsg::kCastPoly, "Ато егоже руци попасти."},
			 {ESpellMsg::kCastChrist, "И он не знает, что мертвецы там и что в глубине..."},
			 {ESpellMsg::kAffExpired, "Жуткие черные руки побледнели и расплылись зловонной дымкой."}}},
		 {ESpell::kWhirlwind, {
			 {ESpellMsg::kCastPoly, "Вждати бурю обло створити."},
			 {ESpellMsg::kCastChrist, "И поднялась великая буря..."},
			 {ESpellMsg::kAffExpired, "!SPELL_WHIRLWIND!"}}},
		 {ESpell::kIndriksTeeth, {
			 {ESpellMsg::kCastPoly, "Идеже индрика зубы супостаты изъмати."},
			 {ESpellMsg::kCastChrist, "Есть род, у которого зубы - мечи и челюсти - ножи..."},
			 {ESpellMsg::kAffExpired, "Каменные зубы исчезли, возвратив способность двигаться."},
			 {ESpellMsg::kAffImposedForRoom, "$n0 теперь прикован$a к $N2."},
			 {ESpellMsg::kAffImposedForVict, "Вы не сможете покинуть $N3."}}},
		 {ESpell::kAcidArrow, {
			 {ESpellMsg::kCastPoly, "Варно сожжет струя!"},
			 {ESpellMsg::kCastChrist, "...и на коже его сделаются как бы язвы проказы"},
			 {ESpellMsg::kAffExpired, "!SPELL_MELFS_ACID_ARROW!"}}},
		 {ESpell::kThunderStone, {
			 {ESpellMsg::kCastPoly, "Небесе тутнет!"},
			 {ESpellMsg::kCastChrist, "...и взял оттуда камень, и бросил из пращи."},
			 {ESpellMsg::kAffExpired, "!SPELL_THUNDERSTONE!"}}},
		 {ESpell::kClod, {
			 {ESpellMsg::kCastPoly, "Онома утес низринется!"},
			 {ESpellMsg::kCastChrist, "...доколе камень не оторвался от горы без содействия рук."},
			 {ESpellMsg::kAffExpired, "!SPELL_CLOD!"}}},
		 {ESpell::kExpedient, {
			 {ESpellMsg::kCastPoly, "!Применил боевой прием!"},
			 {ESpellMsg::kCastChrist, "!Применил боевой прием!"},
			 {ESpellMsg::kAffExpired, "Эффект боевого приема завершился."}}},
		 {ESpell::kSightOfDarkness, {
			 {ESpellMsg::kCastPoly, "Что свет, что тьма - глазу одинаково."},
			 {ESpellMsg::kCastChrist, "Станьте зрячи в тьме кромешной!"},
			 {ESpellMsg::kAffExpired, "Вы больше не можете видеть ночью."},
			 {ESpellMsg::kAffImposedForRoom, "$n0 погрузил$g комнату во мрак."},
			 {ESpellMsg::kAffImposedForVict, "Вы погрузили комнату в непроглядную тьму."}}},
		 {ESpell::kGroupSincerity, {
			 {ESpellMsg::kCastPoly, "...да не скроются намерения."},
			 {ESpellMsg::kCastChrist, "И узрим братья намерения окружающих."},
			 {ESpellMsg::kAffExpired, "Вы не в состоянии больше видеть невидимых."},
			 {ESpellMsg::kAffImposedForVict, "Ваши глаза приобрели зеленый оттенок."},
			 {ESpellMsg::kAffImposedForRoom, "Глаза $n1 приобрели зеленый оттенок."}}},
		 {ESpell::kMagicalGaze, {
			 {ESpellMsg::kCastPoly, "Узрим же все, что с магией навкруги нас."},
			 {ESpellMsg::kCastChrist, "Покажи, Спаситель, магические силы братии."},
			 {ESpellMsg::kAffExpired, "Вы не в состоянии более определять магию."},
			 {ESpellMsg::kAffImposedForVict, "Ваши глаза приобрели желтый оттенок."},
			 {ESpellMsg::kAffImposedForRoom, "Глаза $n1 приобрели желтый оттенок."}}},
		 {ESpell::kAllSeeingEye, {
			 {ESpellMsg::kCastPoly, "Все тайное станет явным."},
			 {ESpellMsg::kCastChrist, "Не спрячется, не скроется, ни заяц, ни блоха."},
			 {ESpellMsg::kAffExpired, "Вы более не можете определять наклонности."},
			 {ESpellMsg::kAffImposedForVict, "Ваши глаза приобрели золотистый оттенок."},
			 {ESpellMsg::kAffImposedForRoom, "Глаза $n1 приобрели золотистый оттенок."}}},
		 {ESpell::kEyeOfGods, {
			 {ESpellMsg::kCastPoly, "Осязаемое откройся взору!"},
			 {ESpellMsg::kCastChrist, "Да не скроется от взора вашего, ни одна живая душа."},
			 {ESpellMsg::kAffExpired, "Вы больше не можете чувствовать жизнь."},
			 {ESpellMsg::kAffImposedForVict, "Вы способны разглядеть даже микроба."},
			 {ESpellMsg::kAffImposedForRoom, "$n0 начал$g замечать любые движения."}}},
		 {ESpell::kBreathingAtDepth, {
			 {ESpellMsg::kCastPoly, "Аки стайка рыбок, плывите вместе."},
			 {ESpellMsg::kCastChrist, "Что в воде, что на земле, воздух свежим будет."},
			 {ESpellMsg::kAffExpired, "Вы более не способны дышать водой."},
			 {ESpellMsg::kAffImposedForVict, "У вас выросли жабры."},
			 {ESpellMsg::kAffImposedForRoom, "У $n1 выросли жабры."}}},
		 {ESpell::kGeneralRecovery, {
			 {ESpellMsg::kCastPoly, "...дабы пройти вместе не одну сотню верст"},
			 {ESpellMsg::kCastChrist, "Сохрани Отче от усталости детей своих!"},
			 {ESpellMsg::kAffExpired, "!SPELL GENERAL RECOVERY!"},
			 {ESpellMsg::kAffImposedForVict, "Вас наполнила живительная сила."},
			 {ESpellMsg::kAffImposedForRoom, "$n расцвел$g на ваших глазах."}}},
		 {ESpell::kCommonMeal, {
			 {ESpellMsg::kCastPoly, "Благодарите богов за хлеб и соль!"},
			 {ESpellMsg::kCastChrist, "...дабы не осталось голодающих на свете белом"},
			 {ESpellMsg::kAreaForChar, "Вы услышали гомон невидимых лакеев, готовящих трапезу.\r\n"},
			 {ESpellMsg::kAffExpired, "!SPELL COMMON MEAL!"},
			 {ESpellMsg::kAffImposedForVict, ""},
			 {ESpellMsg::kAffImposedForRoom, ""}}},
		 {ESpell::kStoneWall, {
			 {ESpellMsg::kCastPoly, "Станем други крепки як николы!"},
			 {ESpellMsg::kCastChrist, "Укрепим тела наши перед битвой!"},
			 {ESpellMsg::kAffExpired, "!SPELL STONE WALL!"},
			 {ESpellMsg::kAffImposedForVict, "Вы стали менее чувствительны к ударам."},
			 {ESpellMsg::kAffImposedForRoom, "Кожа $n1 покрылась каменными пластинами."}}},
		 {ESpell::kSnakeEyes, {
			 {ESpellMsg::kCastPoly, "Что яд, а что мед. Не обманемся!"},
			 {ESpellMsg::kCastChrist, "...и самый сильный яд станет вам виден."},
			 {ESpellMsg::kAffExpired, "Вы не в состоянии более определять яды."},
			 {ESpellMsg::kAffImposedForVict, "Ваши глаза приобрели карий оттенок."},
			 {ESpellMsg::kAffImposedForRoom, "Глаза $n1 приобрели карий оттенок."}}},
		 {ESpell::kEarthAura, {
			 {ESpellMsg::kCastPoly, "Велес, даруй защиту."},
			 {ESpellMsg::kCastChrist, "... земля благословенна твоя."},
			 {ESpellMsg::kAreaForChar, "Земля одарила вас своей защитой.\r\n"},
			 {ESpellMsg::kAffExpired, "Матушка земля забыла про Вас."},
			 {ESpellMsg::kAffImposedForVict, "Глубокий поклон тебе, матушка земля."},
			 {ESpellMsg::kAffImposedForRoom, "$n глубоко поклонил$u земле."}}},
		 {ESpell::kGroupProtectFromEvil, {
			 {ESpellMsg::kCastPoly, "други, супостат нощи"},
			 {ESpellMsg::kCastChrist, "други, свет который в нас, да убоится тьма."},
			 {ESpellMsg::kAreaForChar, "Сила света подавила в вас страх к тьме.\r\n"},
			 {ESpellMsg::kAffExpired, "Вы вновь ощущаете страх перед тьмой."},
			 {ESpellMsg::kAffImposedForVict, "Вы подавили в себе страх к тьме."},
			 {ESpellMsg::kAffImposedForRoom, "$n подавил$g в себе страх к тьме."}}},
		 {ESpell::kArrowsFire, {
			 {ESpellMsg::kCastPoly, "!магический выстрел!"},
			 {ESpellMsg::kCastChrist, "!use battle expedient!"},
			 {ESpellMsg::kAffExpired, "!NONE"}}},
		 {ESpell::kArrowsWater, {
			 {ESpellMsg::kCastPoly, "!магический выстрел!"},
			 {ESpellMsg::kCastChrist, "!use battle expedient!"},
			 {ESpellMsg::kAffExpired, "!NONE"}}},
		 {ESpell::kArrowsEarth, {
			 {ESpellMsg::kCastPoly, "!магический выстрел!"},
			 {ESpellMsg::kCastChrist, "!use battle expedient!"},
			 {ESpellMsg::kAffExpired, "!NONE"}}},
		 {ESpell::kArrowsAir, {
			 {ESpellMsg::kCastPoly, "!магический выстрел!"},
			 {ESpellMsg::kCastChrist, "!use battle expedient!"},
			 {ESpellMsg::kAffExpired, "!NONE"}}},
		 {ESpell::kArrowsDeath, {
			 {ESpellMsg::kCastPoly, "!магический выстрел!"},
			 {ESpellMsg::kCastChrist, "!use battle expedient!"},
			 {ESpellMsg::kAffExpired, "!NONE"}}},
		 {ESpell::kPaladineInspiration, {
			 {ESpellMsg::kAreaForChar, "Ваш точный удар воодушевил и придал новых сил!"},
			 {ESpellMsg::kAreaForRoom, "Точный удар $n1 воодушевил и придал новых сил!"},
			 {ESpellMsg::kAffExpired, "*Боевое воодушевление угасло, а с ним и вся жажда подвигов!"}}},
		 {ESpell::kDexterity, {
			 {ESpellMsg::kCastPoly, "будет ловким"},
			 {ESpellMsg::kCastChrist, "... и человек разумный укрепляет ловкость свою."},
			 {ESpellMsg::kAffExpired, "Вы стали менее шустрым."},
			 {ESpellMsg::kAffImposedForVict, "Вы почувствовали себя более шустрым."},
			 {ESpellMsg::kAffImposedForRoom, "$n0 будет двигаться более шустро."}}},
		 {ESpell::kGroupBlink, {
			 {ESpellMsg::kCastPoly, "защити нас от железа разящего"},
			 {ESpellMsg::kCastChrist, "... ни стрела, ни меч не пронзят печень вашу."},
			 {ESpellMsg::kAreaForChar, "Очертания вас и соратников замерцали в такт биения сердца, став прозрачней.\r\n"},
			 {ESpellMsg::kAffExpired, "Вы перестали мигать."},
			 {ESpellMsg::kAffImposedForRoom, "$n начал$g мигать."},
			 {ESpellMsg::kAffImposedForVict, "Вы начали мигать."}}},
		 {ESpell::kGroupCloudly, {
			 {ESpellMsg::kCastPoly, "огрожу беззакония их туманом"},
			 {ESpellMsg::kCastChrist, "...да защитит и покроет рассветная пелена тела ваши."},
			 {ESpellMsg::kAreaForChar, "Пелена тумана окутала вас и окружающих, скрыв очертания.\r\n"},
			 {ESpellMsg::kAffExpired, "Ваши очертания приобрели отчетливость."},
			 {ESpellMsg::kAffImposedForVict, "Ваше тело стало прозрачным, как туман."},
			 {ESpellMsg::kAffImposedForRoom, "Очертания $n1 расплылись и стали менее отчетливыми."}}},
		 {ESpell::kGroupAwareness, {
			 {ESpellMsg::kCastPoly, "буде вежды ваши открыты"},
			 {ESpellMsg::kCastChrist, "... и забота о ближнем отгоняет сон от очей их."},
			 {ESpellMsg::kAreaForChar, "Произнесенные слова обострили ваши чувства и внимательность ваших соратников.\r\n"},
			 {ESpellMsg::kAffExpired, "Вы стали менее внимательны."},
			 {ESpellMsg::kAffImposedForVict, "Вы стали более внимательны к окружающему."},
			 {ESpellMsg::kAffImposedForRoom, "$n начал$g внимательно осматриваться по сторонам."}}},
		 {ESpell::kWarcryOfExperience, {
			 {ESpellMsg::kCastPoly, "найдем новизну в рутине сражений!"},
			 {ESpellMsg::kCastChrist, "найдем новизну в рутине сражений!"},
			 {ESpellMsg::kAreaForChar, "Вы приготовились к обретению нового опыта."},
			 {ESpellMsg::kAffExpired, "Действие клича 'обучение' закончилось."}}},
		 {ESpell::kWarcryOfLuck, {
			 {ESpellMsg::kCastPoly, "и пусть удача будет нашей спутницей!"},
			 {ESpellMsg::kCastChrist, "и пусть удача будет нашей спутницей!"},
			 {ESpellMsg::kAreaForChar, "Вы ощутили, что вам улыбнулась удача."},
			 {ESpellMsg::kAffExpired, "Действие клича 'везение' закончилось."}}},
		 {ESpell::kWarcryOfPhysdamage, {
			 {ESpellMsg::kCastPoly, "бей в глаз, не порти шкуру"},
			 {ESpellMsg::kCastChrist, "бей в глаз, не порти шкуру."},
			 {ESpellMsg::kAreaForChar, "Боевой клич придал вам сил!"},
			 {ESpellMsg::kAffExpired, "Действие клича 'точность' закончилось."}}},
		 {ESpell::kMassFailure, {
			 {ESpellMsg::kCastPoly, "...отче Велес, очи отвержеши!"},
			 {ESpellMsg::kCastChrist, "...надежда тщетна: не упадешь ли от одного взгляда его?"},
			 {ESpellMsg::kAreaForChar, "Вняв вашему призыву, Змей Велес коснулся недобрым взглядом ваших супостатов.\r\n"},
			 {ESpellMsg::kAreaForVict, "$n провыл$g несколько странно звучащих слов и от тяжелого взгляда из-за края мира у вас подкосились ноги."},
			 {ESpellMsg::kAffExpired, "Удача снова повернулась к вам лицом... и залепила пощечину."},
			 {ESpellMsg::kAffImposedForRoom, "Тяжелое бурое облако сгустилось над $n4."},
			 {ESpellMsg::kAffImposedForVict, "Тяжелые тучи сгустились над вами, и вы почувствовали, что удача покинула вас."}}},
		 {ESpell::kSnare, {
			 {ESpellMsg::kCastPoly, "Заклинати поврещение в сети заскопиены!"},
			 {ESpellMsg::kCastChrist, "...будет трапеза их сетью им, и мирное пиршество их - западнею."},
			 {ESpellMsg::kAreaForChar, "Вы соткали магические тенета, опутавшие ваших врагов.\r\n"},
			 {ESpellMsg::kAreaForVict, "$n что-то прошептал$g, странно скрючив пальцы, и взлетевшие откуда ни возьмись ловчие сети опутали вас"},
			 {ESpellMsg::kAffExpired, "Покрывавшие вас сети колдовской западни растаяли."},
			 {ESpellMsg::kAffImposedForRoom, "$n0 теперь прикован$a к $N2."},
			 {ESpellMsg::kAffImposedForVict, "Вы не сможете покинуть $N3."}}},
		 {ESpell::kQUest, {
			 {ESpellMsg::kAffExpired, "Наложенные на вас чары рассеялись."}}},
		 {ESpell::kExpedientFail, {
			 {ESpellMsg::kCastPoly, "!Провалил боевой прием!"},
			 {ESpellMsg::kCastChrist, "!Провалил боевой прием!"},
			 {ESpellMsg::kAffExpired, "Вы восстановили равновесие."}}},
		 {ESpell::kPortalTimer, {
			 {ESpellMsg::kAffExpired, "Пентаграмма медленно растаяла."}}
		 }
		};

	static const std::string empty_str;
	try {
		return spells_msg.at(spell_id).at(msg_id);
	} catch (std::out_of_range &) {
		return empty_str;
	}
}

std::string GetAffExpiredText(ESpell spell_id) {
	auto msg = GetSpellMsg(spell_id, ESpellMsg::kAffExpired);
	if (msg.empty()) {
		std::stringstream log_text;
		log_text << "!нет сообщения при спадении аффекта под номером: " << to_underlying(spell_id) << "!";
		return log_text.str();
	} else {
		return msg;
	}
}

const std::string &GetCastPhrase(ESpell spell_id, int religion) {
	auto msg_type = (religion == kReligionPoly ? ESpellMsg::kCastPoly : ESpellMsg::kCastChrist);
	return GetSpellMsg(spell_id, msg_type);
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

	for (const auto &i: ESpell_name_by_value) {
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

std::ostream &operator<<(std::ostream &os, const ESpell &s) {
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

	for (const auto &i: EElement_name_by_value) {
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

	for (const auto &i: ESpellType_name_by_value) {
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

	for (const auto &i: EMagic_name_by_value) {
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

	for (const auto &i: ETarget_name_by_value) {
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

typedef std::map<ESpellMsg, std::string> ESpellMsg_name_by_value_t;
typedef std::map<const std::string, ESpellMsg> ESpellMsg_value_by_name_t;
ESpellMsg_name_by_value_t ESpellMsg_name_by_value;
ESpellMsg_value_by_name_t ESpellMsg_value_by_name;
void init_ESpellMsg_ITEM_NAMES() {
	ESpellMsg_value_by_name.clear();
	ESpellMsg_name_by_value.clear();

	ESpellMsg_name_by_value[ESpellMsg::kCastPoly] = "kCastPoly";
	ESpellMsg_name_by_value[ESpellMsg::kCastChrist] = "kCastChrist";
	ESpellMsg_name_by_value[ESpellMsg::kAreaForChar] = "kAreaForChar";
	ESpellMsg_name_by_value[ESpellMsg::kAreaForRoom] = "kAreaForRoom";
	ESpellMsg_name_by_value[ESpellMsg::kAreaForVict] = "kAreaForVict";
	ESpellMsg_name_by_value[ESpellMsg::kComponentUse] = "kComponentUse";
	ESpellMsg_name_by_value[ESpellMsg::kComponentMissing] = "kComponentMissing";
	ESpellMsg_name_by_value[ESpellMsg::kComponentExhausted] = "kComponentExhausted";

	for (const auto &i: ESpellMsg_name_by_value) {
		ESpellMsg_value_by_name[i.second] = i.first;
	}
}

template<>
const std::string &NAME_BY_ITEM<ESpellMsg>(const ESpellMsg item) {
	if (ESpellMsg_name_by_value.empty()) {
		init_ESpellMsg_ITEM_NAMES();
	}
	return ESpellMsg_name_by_value.at(item);
}

template<>
ESpellMsg ITEM_BY_NAME(const std::string &name) {
	if (ESpellMsg_name_by_value.empty()) {
		init_ESpellMsg_ITEM_NAMES();
	}
	return ESpellMsg_value_by_name.at(name);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
