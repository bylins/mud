// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
// extracted from SpellCharm (spells.cpp) where it lived
// as a ~390-line inline if-block guarding `EFeat::kAnimalMaster + race==104`. The
// function applies the AnimalMaster feat's shape-shift / stat boost / per-type
// ability assignment to a freshly-charmed kAnimal NPC. Logic is byte-for-byte
// identical to the previous inline block; only the hardcoded race id (104) was
// replaced by ENpcRace::kAnimal in the caller.

#include "animal_master.h"
#include "gameplay/mechanics/minions.h"

#include "engine/entities/char_data.h"
#include "engine/entities/entities_constants.h"
#include "engine/core/handler.h"
#include "engine/ui/color.h"
#include "gameplay/abilities/feats.h"
#include "gameplay/magic/spells_info.h"   // MUD::Spell(...).GetSuccessRoll().GetBaseSkill()
#include "engine/db/global_objects.h"
#include "gameplay/magic/spells.h"
#include "utils/grammar/cases.h"
#include "utils/random.h"

#include <algorithm>
#include <cmath>
#include <vector>

void ApplyAnimalMaster(CharData *ch, CharData *victim, Affect<EApply> &af,
                       int &k_skills, ESkill &skill_id) {
	int type_mob;
	std::vector<int> rndcharmice = {1, 2, 3, 4, 5, 6, 7, 8};
	for (auto *k : ch->followers) {
		if (IsCharmice(k) && k->get_type_charmice() > 0) {
			auto it = std::find(rndcharmice.begin(), rndcharmice.end(),  k->get_type_charmice());
			if (it != rndcharmice.end()) {
				rndcharmice.erase(it);
			}
		}
	}
	int rnd = number(0, rndcharmice.size() - 1);
	if (std::find(rndcharmice.begin(), rndcharmice.end(), victim->get_type_charmice()) !=  rndcharmice.end()) {
		type_mob = victim->get_type_charmice();
	}
	else {
		type_mob = rndcharmice.at(rnd);
		victim->set_type_charmice(type_mob);
	}
	act("$N0 обрел$G часть вашей магической силы, и стал$G намного опаснее...",
		false, ch, nullptr, victim, kToChar);
	act("$N0 обрел$G часть магической силы $n1.",
		false, ch, nullptr, victim, kToRoom | kToArenaListen);
	// начинаем модификации victim
	// создаем переменные модификаторов
	int r_cha = GetRealCha(ch);
	int perc = ch->GetSkill(MUD::Spell(ESpell::kCharm).GetSuccessRoll().GetBaseSkill());
	ch->send_to_TC(false, true, false, "Значение хари:  %d.\r\n", r_cha);
	ch->send_to_TC(false, true, false, "Значение скила магии: %d.\r\n", perc);
	
	// вычисляем % владения умений у victim
	k_skills = floorf(0.8*r_cha + 0.5*perc);
	ch->send_to_TC(false, true, false, "Владение скилом: %d.\r\n", k_skills);
	// === Формируем новые статы ===
	// Устанавливаем на виктим флаг маг-сумон (маг-зверь)
	victim->SetFlag(EMobFlag::kCompanion);
	// Модифицируем имя в зависимости от хари
	static char descr[kMaxStringLength];
	int gender;
	// ниже идет просто порнуха
	// по идее могут быть случаи "огромная огромная макака" или "громадная большая собака"
	// как бороться думаю
	// Sventovit:
	// Для начала - вынести повторяющийся много раз кусок кода в функцию и вызывать её.
	// Также вынести отсюда стену строковых констант.
	// Тело функции в идеале должно занимать не более трех строк, никак не 30 экранов.
	// У функции в идеале не должно быть параметров. В допустимом пределе - три параметра.
	// Если их больше - программист что-то не так делает.
	// А бороться - просто сравнивать добавляемое слово, если оно уже есть - то не добавлять.
	// Варнинги по поводу неявного приведения типов тоже стоит почистить.
	const char *state[4][9][6] = {
					{  						
					{"крепкие",  "крепких", "крепким", "крепкие", "крепкими", "крепких"},
					{"сильные",  "сильных", "сильным", "сильные", "сильными", "сильных"},
					{"упитанные",  "упитанных", "упитанным", "упитанных", "упитанными", "упитанных"},
					{"крупные",  "крупные", "крупным", "крупные", "крупными", "крупных"},
					{"большые",  "большые", "большым", "большых", "большыми", "большых"},
					{"громадные", "громадные", "громадным", "громадные", "громадными", "громадных"},
					{"огромные",  "огромных", "огромным", "огромные", "огромными", "огромных"},
					{"исполинские",  "исполинские", "исполинским", "исполинские", "исполинскими", "исполинских"},
					{"гигантские" ,"гигантские", "гигантские", "гигантские", "гигантские", "гигантские"},
					},
	 				{ // род ОН
					{"крепкий",  "крепкого", "крепкому", "крепкого", "крепким", "крепком"},
					{"сильный",  "сильного", "сильному", "сильного", "сильным", "сильном"},
					{"упитанный",  "упитанного", "упитанному", "упитанного", "упитанным", "упитанном"},
					{"крупный",  "крупного", "крупному", "крупного", "крупным", "крупном"},
					{"большой",  "большого", "большому", "большого", "большым", "большом"},
					{"громадный",  "громадного", "громадному", "громадного", "громадным", "громадном"},
					{"огромный",  "огромного", "огромному", "огромного", "огромным", "огромном"},
					{"исполинский",  "исполинского", "исполинскому", "исполинского", "исполинским", "исполинском"},
					{"гигантский",  "гигантского", "гигантскому", "гигантского", "гигантским", "гигантском"},
					},
	 				{ // род ОНА
					{"крепкая", "крепкой", "крепкой", "крепкую", "крепкой", "крепкой"},
					{"сильная", "сильной", "сильной", "сильную", "сильной", "сильной"},
					{"упитанная", "упитанной", "упитанной", "упитанную", "упитанной", "упитанной"},
					{"крупная",  "крупной", "крупной", "крупную", "крупной", "крупной"},
					{"большая", "большой", "большой", "большую", "большой", "большой"},
					{"громадная",  "громадной", "громадной", "громадную", "громадной", "громадной"},
					{"огромная",  "огромной", "огромной", "огромную", "огромной", "огромной"},
					{"исполинская", "исполинской", "исполинской", "исполинскую", "исполинской", "исполинской"},
					{"гигантская",  "гигантской", "гигантской", "гигантскую", "гигантской", "гигантской"},
					},
	 				{  // род ОНО
					{"крепкое", "крепкое", "крепкому", "крепкое", "крепким", "крепком"},
					{"сильное",  "сильное", "сильному", "сильное", "сильным", "сильном"},
					{"упитанное","упитанное", "упитанному", "упитанное", "упитанным", "упитанном"},
					{"крупное", "крупное", "крупному", "крупное", "крупным", "крупном"},
					{"большое",  "большое", "большому", "большое", "большым", "большом"},
					{"громадное", "громадное", "громадному", "громадное", "громадным", "громадном"},
					{"огромное",  "огромное", "огромному", "огромное", "огромным", "огромном"},
					{"исполинское",  "исполинское", "исполинскому", "исполинское", "исполинским", "исполинском"},
					{"гигантское" , "гигантское", "гигантскому", "гигантское", "гигантским", "гигантском"},
					}
					};
	//проверяем GENDER 
	switch ((victim)->get_sex()) {
			case EGender::kNeutral:
			gender = 0;
			break;
			case EGender::kMale:
			gender = 1;
			break;
			case EGender::kFemale:
			gender = 2;
			break;
			default:
			gender = 3;
			break;
	}
 		// 1 при 10-19, 2 при 20-29 , 3 при 30-39....
	int adj = r_cha/10;
	sprintf(descr, "%s %s %s", state[gender][adj - 1][0], GET_PAD(victim, 0), GET_NAME(victim));
	victim->SetCharAliases(descr);
	sprintf(descr, "%s %s", state[gender][adj - 1][0], GET_PAD(victim, 0));
	victim->set_npc_name(descr);
	sprintf(descr, "%s %s", state[gender][adj - 1][0], GET_PAD(victim, 0));
	victim->player_data.PNames[grammar::ECase::kNom] = std::string(descr);
	sprintf(descr, "%s %s", state[gender][adj - 1][1], GET_PAD(victim, 1));
	victim->player_data.PNames[grammar::ECase::kGen] = std::string(descr);
	sprintf(descr, "%s %s", state[gender][adj - 1][2], GET_PAD(victim, 2));
	victim->player_data.PNames[grammar::ECase::kDat] = std::string(descr);
	sprintf(descr, "%s %s", state[gender][adj - 1][3], GET_PAD(victim, 3));
	victim->player_data.PNames[grammar::ECase::kAcc] = std::string(descr);
	sprintf(descr, "%s %s", state[gender][adj - 1][4], GET_PAD(victim, 4));
	victim->player_data.PNames[grammar::ECase::kIns] = std::string(descr);
	sprintf(descr, "%s %s", state[gender][adj - 1][5], GET_PAD(victim, 5));
	victim->player_data.PNames[grammar::ECase::kPre] = std::string(descr);
	victim->set_max_hit(victim->get_max_hit() + floorf( GetRealLevel(ch)*15 + r_cha*4 + perc*2));
	victim->set_hit(victim->get_max_hit());
	// статы
	victim->set_int(std::min(90, static_cast<int>(floorf(r_cha*0.2 + perc*0.15))));
	victim->set_dex(std::min(90, static_cast<int>(floorf(r_cha*0.3 + perc*0.15))));
	victim->set_str(std::min(90, static_cast<int>(floorf(r_cha*0.3 + perc*0.15))));
	victim->set_con(std::min(90, static_cast<int>(floorf(r_cha*0.3 + perc*0.15))));
	victim->set_wis(std::min(90, static_cast<int>(floorf(r_cha*0.2 + perc*0.15))));
	victim->set_cha(std::min(90, static_cast<int>(floorf(r_cha*0.2 + perc*0.15))));
	// боевые показатели
	GET_INITIATIVE(victim) = floorf(k_skills/4.0);	// инициатива
	GET_MORALE(victim) = floorf(k_skills/5.0); 		// удача
	GET_HR(victim) = floorf(r_cha/3.5 + perc/10.0);  // попадание
	GET_AC(victim) = -floorf(r_cha/5.0 + perc/15.0); // АС
	GET_DR(victim) = floorf(r_cha/6.0 + perc/20.0);  // дамрол
	GET_ARMOUR(victim) = floorf(r_cha/4.0 + perc/10.0); // броня
	// спелы не работают пока 
	// SET_SPELL_MEM(victim, SPELL_CURE_BLIND, 1); // -?
	// SET_SPELL_MEM(victim, SPELL_REMOVE_DEAFNESS, 1); // -?
	// SET_SPELL_MEM(victim, SPELL_REMOVE_HOLD, 1); // -?
	// SET_SPELL_MEM(victim, SPELL_REMOVE_POISON, 1); // -?
	// SET_SPELL_MEM(victim, SPELL_HEAL, 1);

	//NPC_FLAGS(victim).set(NPC_WIELDING); // тут пока закомитим
	GET_LIKES(victim) = 10 + r_cha; // устанавливаем возможность авто применения умений
	
	// создаем кубики и доп атаки (пока без + а просто сет)
	victim->mob_specials.damnodice = floorf((r_cha*1.3 + perc*0.2) / 5.0);
	victim->mob_specials.damsizedice = floorf((r_cha*1.2 + perc*0.1) / 11.0);
	victim->mob_specials.extra_attack = floorf((r_cha*1.2 + perc) / 120.0);
	

	// простые аффекты
	if (r_cha > 25)  {
		af.affect_type = EAffect::kInfravision;
		affect_to_char(victim, af);
	} 
	 if (r_cha >= 30) {
		af.affect_type = EAffect::kDetectInvisible;
		affect_to_char(victim, af);
	} 
	if (r_cha >= 35) {
		af.affect_type = EAffect::kFly;
		affect_to_char(victim, af);
	} 
	if (r_cha >= 39) {	
		af.affect_type = EAffect::kStoneHands;
		affect_to_char(victim, af);
	}
	
	// расщет крутых маг аффектов
	if (r_cha > 56) {
		af.affect_type = EAffect::kShadowCloak;
		affect_to_char(victim, af);
	} 
	
	if ((r_cha > 65) && (r_cha < 74)) {
		af.affect_type = EAffect::kFireShield;
	} else if ((r_cha >= 74) && (r_cha < 82)){
		af.affect_type = EAffect::kAirShield;
	} else if (r_cha >= 82) {
		af.affect_type = EAffect::kIceShield;
		affect_to_char(victim, af);
		af.affect_type = EAffect::kBrokenChains;
	}
	affect_to_char(victim, af);
	
	// почистим изначальные скиллы, перки
	RemoveAllSkills(victim);
	victim->real_abils.Feats.reset();
	// выбираем тип бойца - рандомно из 8 вариантов
	af.affect_type = EAffect::kHelper;
	affect_to_char(victim, af);
	switch (type_mob)
	{ // готовим наборы скиллов / способностей
	case 1:
		act("Лапы $N1 увеличились в размерах и обрели огромную, дикую мощь.\nТуловище $N1 стало огромным.",
			false, ch, nullptr, victim, kToChar); // тут потом заменим на валидные фразы
		act("Лапы $N1 увеличились в размерах и обрели огромную, дикую мощь.\nТуловище $N1 стало огромным.",
			false, ch, nullptr, victim, kToRoom | kToArenaListen);
		victim->set_skill(ESkill::kHammer, k_skills);
		victim->set_skill(ESkill::kRescue, k_skills*0.8);
		victim->set_skill(ESkill::kPunch, k_skills*0.9);
		victim->set_skill(ESkill::kNoParryHit, k_skills*0.4);
		victim->set_skill(ESkill::kIntercept, k_skills*0.75);
		victim->SetFeat(EFeat::kPunchMaster);
			if (floorf(r_cha*0.9 + perc/5.0) > number(1, 150)) {
			victim->SetFeat(EFeat::kPunchFocus);
			victim->SetFeat(EFeat::kBerserker);
			act("&B$N0 теперь сможет просто удавить всех своих врагов.&n\n",
				false, ch, nullptr, victim, kToChar);
		}
		victim->set_str(floorf(GetRealStr(victim)*1.3));
		skill_id = ESkill::kPunch;
		break;
	case 2:
		act("Лапы $N1 удлинились, и на них выросли гигантские острые когти.\nТуловище $N1 стало более мускулистым.",
			false, ch, nullptr, victim, kToChar);
		act("Лапы $N1 удлинились и на них выросли гигантские острые когти.\nТуловище $N1 стало более мускулистым.",
			false, ch, nullptr, victim, kToRoom | kToArenaListen);
		victim->set_skill(ESkill::kOverwhelm, k_skills);
		victim->set_skill(ESkill::kRescue, k_skills*0.8);
		victim->set_skill(ESkill::kTwohands, k_skills*0.95);
		victim->set_skill(ESkill::kNoParryHit, k_skills*0.4);
		victim->SetFeat(EFeat::kTwohandsMaster);
		victim->SetFeat(EFeat::kTwohandsFocus);
		if (floorf(r_cha + perc/5.0) > number(1, 150)) {
			act("&G$N0 стал$G намного более опасным хищником.&n\n",
				false, ch, nullptr, victim, kToChar);
			victim->set_skill(ESkill::kFirstAid, k_skills*0.4);
			victim->set_skill(ESkill::kParry, k_skills*0.7);
		}
		victim->set_str(floorf(GetRealStr(victim)*1.2));
		skill_id = ESkill::kTwohands;
		break;
	case 3:
		act("Когти на лапах $N1 удлинились и приобрели зеленоватый оттенок.\nДвижения $N1 стали более размытыми.",
			false, ch, nullptr, victim, kToChar);
		act("Когти на лапах $N1 удлинились и приобрели зеленоватый оттенок.\nДвижения $N1 стали более размытыми.",
			false, ch, nullptr, victim, kToRoom | kToArenaListen);
		victim->set_skill(ESkill::kBackstab, k_skills);
		victim->set_skill(ESkill::kRescue, k_skills*0.6);
		victim->set_skill(ESkill::kPicks, k_skills*0.75);
		victim->set_skill(ESkill::kNoParryHit, k_skills*0.75);
		victim->SetFeat(EFeat::kPicksMaster);
		victim->SetFeat(EFeat::kThieveStrike);
		if (floorf(r_cha*0.8 + perc/5.0) > number(1, 150)) {
			victim->SetFeat(EFeat::kShadowStrike);
			act("&c$N0 затаил$U в вашей тени...&n\n", false, ch, nullptr, victim, kToChar);
		}
		victim->set_dex(floorf(GetRealDex(victim)*1.3));
		skill_id = ESkill::kPicks;
		break;
	case 4:
		act("Рефлексы $N1 обострились, и туловище раздалось в ширь.\nНа огромных лапах засияли мелкие острые коготки.",
			false, ch, nullptr, victim, kToChar);
		act("Рефлексы $N1 обострились, и туловище раздалось в ширь.\nНа огромных лапах засияли мелкие острые коготки.",
			false, ch, nullptr, victim, kToRoom | kToArenaListen);
		victim->set_skill(ESkill::kAwake, k_skills);
		victim->set_skill(ESkill::kRescue, k_skills*0.85);
		victim->set_skill(ESkill::kShieldBlock, k_skills*0.75);
		victim->set_skill(ESkill::kAxes, k_skills*0.85);
		victim->set_skill(ESkill::kNoParryHit, k_skills*0.65);
		if (floorf(r_cha*0.9 + perc/5.0) > number(1, 140)) {
			victim->set_skill(ESkill::kProtect, k_skills*0.75);
			act("&WЧуткий взгляд $N1 остановился на вас, и вы ощутили себя под защитой.&n\n",
				false, ch, nullptr, victim, kToChar);
			victim->set_protecting(ch);
		}
		victim->SetFeat(EFeat::kAxesMaster);
		victim->SetFeat(EFeat::kThieveStrike);
		victim->SetFeat(EFeat::kDefender);
		victim->SetFeat(EFeat::kLiveShield);
		victim->set_con(floorf(GetRealCon(victim)*1.3));
		victim->set_str(floorf(GetRealStr(victim)*1.2));
		skill_id = ESkill::kAxes;
		break;
	case 5:
		act("Движения $N1 сильно ускорились.\nИз туловища выросло несколько новых лап, которые покрылись длинными когтями.",
			false, ch, nullptr, victim, kToChar);
		act("Движения $N1 сильно ускорились.\nИз туловища выросло несколько новых лап, которые покрылись длинными когтями.",
			false, ch, nullptr, victim, kToRoom | kToArenaListen);
		victim->set_skill(ESkill::kChopoff, k_skills);
		victim->set_skill(ESkill::kDodge, k_skills*0.7);
		victim->set_skill(ESkill::kAddshot, k_skills*0.7);
		victim->set_skill(ESkill::kBows, k_skills*0.85);
		victim->set_skill(ESkill::kRescue, k_skills*0.65);
		victim->set_skill(ESkill::kNoParryHit, k_skills*0.5);
		victim->SetFeat(EFeat::kThieveStrike);
		victim->SetFeat(EFeat::kBowsMaster);
		if (floorf(r_cha*0.8 + perc/5.0) > number(1, 150)) {
			af.affect_type = EAffect::kCloudOfArrows;
			act("&YВокруг когтей $N1 засияли яркие магические всполохи.&n\n",
				false, ch, nullptr, victim, kToChar);
			affect_to_char(victim, af);
		}
		victim->set_dex(floorf(GetRealDex(victim)*1.2));
		victim->set_str(floorf(GetRealStr(victim)*1.15));
		victim->mob_specials.extra_attack = floorf((r_cha*1.2 + perc) / 180.0); // срежем доп атаки
		skill_id = ESkill::kBows;
		break;
	case 6:
		act("Туловище $N1 увеличилось, лапы сильно удлинились.\nНа них выросли острые когти-шипы.",
			false, ch, nullptr, victim, kToChar);
		act("Туловище $N1 увеличилось, лапы сильно удлинились.\nНа них выросли острые когти-шипы.",
			false, ch, nullptr, victim, kToRoom | kToArenaListen);
		victim->set_skill(ESkill::kClubs, k_skills);
		victim->set_skill(ESkill::kThrow, k_skills*0.85);
		victim->set_skill(ESkill::kDodge, k_skills*0.7);
		victim->set_skill(ESkill::kRescue, k_skills*0.6);
		victim->set_skill(ESkill::kNoParryHit, k_skills*0.6);
		victim->SetFeat(EFeat::kClubsMaster);
		victim->SetFeat(EFeat::kDoubleThrower);
		victim->SetFeat(EFeat::kTripleThrower);
		victim->SetFeat(EFeat::kPowerThrow);
		victim->SetFeat(EFeat::kDeadlyThrow);
		if (floorf(r_cha*0.8 + perc/5.0) > number(1, 140)) {
			victim->SetFeat(EFeat::kShadowThrower);
			victim->SetFeat(EFeat::kShadowClub);
			victim->set_skill(ESkill::kDarkMagic, k_skills*0.7);
			act("&cКогти $N1 преобрели &Kчерный цвет&c, будто смерть коснулась их.&n\n",
				false, ch, nullptr, victim, kToChar);
			victim->mob_specials.extra_attack = floorf((r_cha*1.2 + perc) / 100.0);
		}
		victim->set_str(floorf(GetRealStr(victim)*1.25));
		skill_id = ESkill::kClubs;
	break;
	case 7:
		act("Туловище $N1 увеличилось, мышцы налились дикой силой.\nА когти на лапах удлинились и заострились.",
			false, ch, nullptr, victim, kToChar);
		act("Туловище $N1 увеличилось, мышцы налились дикой силой.\nА когти на лапах удлинились и заострились.",
			false, ch, nullptr, victim, kToRoom | kToArenaListen);
		victim->set_skill(ESkill::kLongBlades, k_skills);
		victim->set_skill(ESkill::kKick, k_skills*0.95);
		victim->set_skill(ESkill::kNoParryHit, k_skills*0.7);
		victim->set_skill(ESkill::kRescue, k_skills*0.4);
		victim->SetFeat(EFeat::kLongsMaster);
		if (floorf(r_cha*0.8 + perc/5.0) > number(1, 150)) {
			victim->set_skill(ESkill::kIronwind, k_skills*0.8);
			victim->SetFeat(EFeat::kBerserker);
			act("&mДвижения $N1 сильно ускорились, и в глазах появились &Rогоньки&m безумия.&n\n",
				false, ch, nullptr, victim, kToChar);
		}
		victim->set_dex(floorf(GetRealDex(victim)*1.1));
		victim->set_str(floorf(GetRealStr(victim)*1.35));
		skill_id = ESkill::kLongBlades;
	break;		
	default:
		act("Рефлексы $N1 обострились, а передние лапы сильно удлинились.\nНа них выросли острые когти.",
			false, ch, nullptr, victim, kToChar);
		act("Рефлексы $N1 обострились, а передние лапы сильно удлинились.\nНа них выросли острые когти.",
			false, ch, nullptr, victim, kToRoom | kToArenaListen);
		victim->set_skill(ESkill::kParry, k_skills);
		victim->set_skill(ESkill::kRescue, k_skills*0.75);
		victim->set_skill(ESkill::kThrow, k_skills*0.95);
		victim->set_skill(ESkill::kSpades, k_skills*0.9);
		victim->set_skill(ESkill::kNoParryHit, k_skills*0.6);
		victim->SetFeat(EFeat::kLiveShield);
		victim->SetFeat(EFeat::kSpadesMaster);
		if (floorf(r_cha*0.9 + perc/4.0) > number(1, 140)) {
			victim->SetFeat(EFeat::kShadowThrower);
			victim->SetFeat(EFeat::kShadowSpear);
			victim->set_skill(ESkill::kDarkMagic, k_skills*0.8);
			act("&KКогти $N1 преобрели темный оттенок, будто сама тьма коснулась их.&n\n",
				false, ch, nullptr, victim, kToChar);
		}
		victim->SetFeat(EFeat::kDoubleThrower);
		victim->SetFeat(EFeat::kTripleThrower);
		victim->SetFeat(EFeat::kPowerThrow);
		victim->SetFeat(EFeat::kDeadlyThrow);
		victim->set_str(floorf(GetRealStr(victim)*1.2));
		victim->set_con(floorf(GetRealCon(victim)*1.2));
		skill_id = ESkill::kSpades;
		break;
	}
}
