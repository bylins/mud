/**
\file animate_dead.cpp - a part of the Bylins engine.
\brief kAnimateDead mechanic, driven by cfg/mechanics/animate_dead.xml (issue.animate-dead / #3548).
*/

#include "animate_dead.h"

#include "engine/db/db.h"               // mob_proto
#include "engine/db/global_objects.h"   // MUD::AnimateDead()
#include "engine/entities/char_data.h"
#include "gameplay/skills/skills.h"
#include "gameplay/affects/affect_data.h"   // Affect / affect_to_char
#include "gameplay/mechanics/saving.h"  // ESaving, GetSave/SetSave
#include "gameplay/core/remort.h"       // remort::SkillCapStart / SkillCapIncrement
#include "utils/utils.h"                // GET_* macros, err_log, number, GetRealLevel
#include "utils/utils_parse.h"

#include <algorithm>
#include <cmath>
#include <string>

using parser_wrapper::DataNode;

namespace animate_dead {

namespace {

// Parse a single <stat beta= mode= cap=/> child of the current <scaling> node.
StatScale ParseStat(DataNode &scaling, const char *name) {
	StatScale s;
	if (!scaling.GoToChild(name)) {
		return s;
	}
	const char *b = scaling.GetValue("beta");
	s.beta = (b && *b) ? parse::ReadAsDouble(b) : 0.0;
	const char *m = scaling.GetValue("mode");
	s.mode = (m && std::string(m) == "mult") ? EScaleMode::kMult : EScaleMode::kAdd;
	const char *c = scaling.GetValue("cap");
	if (c && *c) {
		s.cap = parse::ReadAsDouble(c);
		s.has_cap = (s.cap != 0.0);   // cap="0" (or an absent cap) means "no cap" -- the stat is unbounded
	}
	scaling.GoToParent();
	return s;
}

int RoundC(double beta, double c) {
	return static_cast<int>(std::lround(beta * c));
}

int UsedBudget(CharData *ch) {
	int used = 0;
	for (auto *k : ch->followers) {
		if (k && AFF_FLAGGED(k, EAffect::kCharmed) && k->get_master() == ch) {
			used += MUD::AnimateDead().WeightOf(GET_MOB_VNUM(k));
		}
	}
	return used;
}

}  // namespace

void AnimateDeadInfo::Load(DataNode data) {
	creatures_.clear();
	for (auto &node : data.Children()) {
		const std::string name = node.GetName();
		if (name == "control") {
			const char *sk = node.GetValue("skill");
			if (sk && *sk) {
				control_skill_ = parse::ReadAsConstant<ESkill>(sk);
			}
			const char *bc = node.GetValue("budget_cap");
			if (bc && *bc) {
				budget_cap_ = parse::ReadAsInt(bc);
			}
			continue;
		}
		if (name != "creature") {
			continue;
		}
		try {
			CreatureInfo ci;
			ci.vnum = parse::ReadAsInt(node.GetValue("vnum"));
			ci.id = parse::ReadAsStr(node.GetValue("id"));
			ci.proto_vnum = parse::ReadAsInt(node.GetValue("proto_vnum"));
			ci.weight = parse::ReadAsInt(node.GetValue("weight"));
			// Атрибут висит на самом <creature>, а не в дочернем <cost> -- читать его надо
			// здесь, пока node указывает на вид.
			const char *dc = node.GetValue("damroll_cap");
			ci.damroll_cap = (dc && *dc) ? parse::ReadAsInt(dc) : 0;
			if (node.GoToChild("cost")) {
				ci.corpse_max_level = parse::ReadAsInt(node.GetValue("corpse_max_level"));
				const char *mr = node.GetValue("min_rating");
				ci.min_rating = (mr && *mr) ? parse::ReadAsInt(mr) : 0;
				const char *pk = node.GetValue("pick");
				ci.pick = (pk && *pk) ? parse::ReadAsInt(pk) : 0;
				node.GoToParent();
			}
			if (node.GoToChild("scaling")) {
				ci.scaling.hp          = ParseStat(node, "hp");
				ci.scaling.damage_dice = ParseStat(node, "damage_dice");
				ci.scaling.damage_size = ParseStat(node, "damage_size");
				ci.scaling.damage_bonus = ParseStat(node, "damage_bonus");
				ci.scaling.hitroll     = ParseStat(node, "hitroll");
				ci.scaling.ac          = ParseStat(node, "ac");
				ci.scaling.skills      = ParseStat(node, "skills");
				ci.scaling.saving      = ParseStat(node, "saving");
				ci.scaling.armor       = ParseStat(node, "armor");
				ci.scaling.morale      = ParseStat(node, "morale");
				ci.scaling.initiative  = ParseStat(node, "initiative");
				node.GoToParent();
			}
			creatures_.push_back(std::move(ci));
		} catch (std::exception &e) {
			err_log("animate_dead creature parse error: %s", e.what());
		}
	}
}

const CreatureInfo *AnimateDeadInfo::ByProtoVnum(int proto_vnum) const {
	for (const auto &c : creatures_) {
		if (c.proto_vnum == proto_vnum) {
			return &c;
		}
	}
	return nullptr;
}

int AnimateDeadInfo::WeightOf(int proto_vnum) const {
	const auto *c = ByProtoVnum(proto_vnum);
	return c ? c->weight : 0;   // non-tier followers do not consume the budget
}

int AnimateDeadInfo::DamrollCapOf(int proto_vnum) const {
	const auto *c = ByProtoVnum(proto_vnum);
	return c ? c->damroll_cap : 0;
}

int AnimateDeadInfo::LadderIndex(int proto_vnum) const {
	for (int i = 0; i < static_cast<int>(creatures_.size()); ++i) {
		if (creatures_[i].proto_vnum == proto_vnum) {
			return i;
		}
	}
	return static_cast<int>(creatures_.size()) - 1;   // unknown -> treat as the top tier
}

void AnimateDeadLoader::Load(DataNode data) {
	MUD::AnimateDead().Load(data);
}

void AnimateDeadLoader::Reload(DataNode data) {
	MUD::AnimateDead().Load(data);
}

std::string AnimateDeadLoader::EditableWhat() const {
	return "creature";
}

std::vector<cfg_manager::EditableElement> AnimateDeadLoader::ListElements() const {
	std::vector<cfg_manager::EditableElement> out;
	for (const auto &c : MUD::AnimateDead().Creatures()) {
		out.push_back({std::to_string(c.vnum), c.id});
	}
	return out;
}

cfg_manager::ValidationResult AnimateDeadLoader::Validate(DataNode &doc) const {
	AnimateDeadInfo trial;
	trial.Load(doc);
	if (trial.empty()) {
		return {false, "animate_dead: no <creature> tiers parsed (see syslog)."};
	}
	return {true, ""};
}

DataNode AnimateDeadLoader::FindElementNode(DataNode root, const std::string &id) const {
	// A <creature> is keyed by its integer vnum; iterate children with a name check (a node copied
	// out of a keyed range would otherwise carry that range's filter -- see RegionsLoader).
	for (auto &child : root.Children()) {
		if (std::string(child.GetName()) == "creature" && id == child.GetValue("vnum")) {
			return child;
		}
	}
	return DataNode{};
}

std::string AnimateDeadLoader::CanonicalElementId(const std::string &id) const {
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

DataNode AnimateDeadLoader::CreateElementNode(DataNode root, const std::string &id) const {
	auto node = root.AddChild("creature");
	node.SetValue("vnum", id);
	node.SetValue("id", "kUndefined");
	node.SetValue("proto_vnum", "0");
	node.SetValue("weight", "0");
	auto cost = node.AddChild("cost");
	cost.SetValue("corpse_max_level", "0");
	cost.SetValue("min_rating", "0");
	return node;
}

int PickTier(CharData *ch, int corpse_mob_level) {
	const auto &cfg = MUD::AnimateDead();
	const auto &cr = cfg.Creatures();
	if (cr.empty()) {
		return 0;
	}
	// 1) corpse-level band: the weakest tier whose corpse_max_level covers the corpse.
	int band_max = cr.back().corpse_max_level;
	for (const auto &c : cr) {
		if (corpse_mob_level <= c.corpse_max_level) {
			band_max = c.corpse_max_level;
			break;
		}
	}
	// 2) random pick among the tiers sharing that band (pick odds; single tier = itself).
	std::vector<const CreatureInfo *> band;
	for (const auto &c : cr) {
		if (c.corpse_max_level == band_max) {
			band.push_back(&c);
		}
	}
	const CreatureInfo *chosen = band.front();
	if (band.size() > 1) {
		int total = 0;
		for (auto *c : band) {
			total += std::max(1, c->pick);
		}
		int roll = number(1, total);
		for (auto *c : band) {
			roll -= std::max(1, c->pick);
			if (roll <= 0) {
				chosen = c;
				break;
			}
		}
	}
	// 3) caster-rating gate: demote down the ladder until min_rating is met. The rating is
	// level + a skill-derived remort-equivalent + 4 -- NOT the actual remort count (issue.animate-dead:
	// the control skill, not remort, gates tiers). The remort-equivalent inverts CalcSkillCap: a remort
	// R has skill cap SkillCapStart + R*SkillCapIncrement, so R = (skill - SkillCapStart) / increment.
	const int inc = std::max(1, remort::SkillCapIncrement());
	const int pseudo_remort = std::max(0, (GetSkill(ch, cfg.ControlSkill()) - remort::SkillCapStart()) / inc);
	const int rating = GetRealLevel(ch) + pseudo_remort + 4;
	int idx = cfg.LadderIndex(chosen->proto_vnum);
	while (idx > 0 && rating < cr[idx].min_rating) {
		--idx;
	}
	return cr[idx].proto_vnum;
}

int FitUndeadTier(CharData *ch, int desired) {
	const auto &cfg = MUD::AnimateDead();
	const auto &cr = cfg.Creatures();
	if (cr.empty()) {
		return 0;
	}
	const int budget = std::min(GetSkill(ch, cfg.ControlSkill()), cfg.BudgetCap());
	const int remaining = budget - UsedBudget(ch);
	for (int i = cfg.LadderIndex(desired); i >= 0; --i) {
		if (cr[i].weight <= remaining) {
			return cr[i].proto_vnum;
		}
	}
	return 0;   // even the weakest tier does not fit -> caller reports "too many minions"
}

// Броня, спасы, удача и инициатива живут в add_abils, а affect_total у NPC очищает этот
// блок до прототипного и заново навешивает аффекты. Значит прямая запись не живёт: её
// стирает первый же наложенный бафф. Поэтому подъём выдаёт эту группу аффектами -- их
// affect_total возвращает сам, сколько бы раз ни пересчитывал.
void ApplyVolatileUndeadStats(CharData *mob, double competence, int duration) {
	const auto *info = MUD::AnimateDead().ByProtoVnum(GET_MOB_VNUM(mob));
	if (!info) {
		return;
	}
	const auto rnum = mob->get_rnum();
	const CharData *base = (rnum >= 0) ? &mob_proto[rnum] : mob;
	const double c = std::max(0.0, competence);
	const CreatureScaling &s = info->scaling;

	auto up = [c](const StatScale &sc, int cur) {
		if (sc.beta == 0.0) {
			return cur;
		}
		int v = cur + RoundC(sc.beta, c);
		if (sc.has_cap) {
			v = std::min(v, static_cast<int>(sc.cap));
		}
		return v;
	};
	auto down = [c](const StatScale &sc, int cur) {
		if (sc.beta == 0.0) {
			return cur;
		}
		int v = cur - RoundC(sc.beta, c);
		if (sc.has_cap) {
			v = std::max(v, static_cast<int>(sc.cap));
		}
		return v;
	};

	// Аффект без флага (affect_type kUndefined) -- он ничего не сообщает игроку и не виден
	// в списке, только несёт прибавку. Нулевые прибавки не навешиваем вовсе.
	auto grant = [mob, duration](EApply location, int modifier) {
		if (modifier == 0) {
			return;
		}
		Affect<EApply> af;
		af.duration = duration;
		af.modifier = modifier;
		af.location = location;
		af.affect_type = EAffect::kUndefined;
		af.battleflag = {kAfCharmBond};   // issue #3971: прибавки нежити -- часть её пакета, снятию не подлежат
		affect_to_char(mob, af);
	};

	const int armour_base = GET_ARMOUR(base);
	grant(EApply::kArmour, up(s.armor, armour_base) - armour_base);
	const int morale_base = GET_MORALE(base);
	grant(EApply::kMorale, up(s.morale, morale_base) - morale_base);
	const int initiative_base = GET_INITIATIVE(base);
	grant(EApply::kInitiative, up(s.initiative, initiative_base) - initiative_base);
	if (s.saving.beta != 0.0) {
		static const std::pair<ESaving, EApply> kSaves[] = {
			{ESaving::kWill, EApply::kSavingWill},
			{ESaving::kCritical, EApply::kSavingCritical},
			{ESaving::kStability, EApply::kSavingStability},
			{ESaving::kReflex, EApply::kSavingReflex},
		};
		for (const auto &[saving, location] : kSaves) {
			const int save_base = GetSave(const_cast<CharData *>(base), saving);
			grant(location, down(s.saving, save_base) - save_base);
		}
	}
}

void SetupUndeadStats(CharData * /*ch*/, CharData *mob, double competence) {
	const auto *info = MUD::AnimateDead().ByProtoVnum(GET_MOB_VNUM(mob));
	if (!info) {
		return;
	}
	const double c = std::max(0.0, competence);
	const CreatureScaling &s = info->scaling;

	// "up" stat (higher is better): += round(beta*C), capped to a max.
	auto up = [c](const StatScale &sc, int cur) {
		if (sc.beta == 0.0) {
			return cur;
		}
		int v = cur + RoundC(sc.beta, c);
		if (sc.has_cap) {
			v = std::min(v, static_cast<int>(sc.cap));
		}
		return v;
	};
	// "down" stat (lower is better, e.g. AC / saves): -= round(beta*C), floored to cap.
	auto down = [c](const StatScale &sc, int cur) {
		if (sc.beta == 0.0) {
			return cur;
		}
		int v = cur - RoundC(sc.beta, c);
		if (sc.has_cap) {
			v = std::max(v, static_cast<int>(sc.cap));
		}
		return v;
	};

	// HP (multiplicative on the prototype; keeps tier proportions).
	if (s.hp.beta != 0.0) {
		const int proto = mob->get_max_hit();
		int hp;
		if (s.hp.mode == EScaleMode::kMult) {
			double factor = 1.0 + s.hp.beta * c;
			if (s.hp.has_cap) {
				factor = std::min(factor, s.hp.cap);   // cap the multiplier, e.g. proto*2.0
			}
			hp = static_cast<int>(proto * factor);
		} else {
			hp = proto + RoundC(s.hp.beta, c);
			if (s.hp.has_cap) {
				hp = std::min(hp, static_cast<int>(s.hp.cap));
			}
		}
		mob->set_max_hit(hp);
		mob->set_hit(hp);
	}
	// Damage dice count (additive).
	if (s.damage_dice.beta != 0.0) {
		const int dice = std::clamp(up(s.damage_dice, mob->mob_specials.damnodice), 1, 100);
		mob->mob_specials.damnodice = dice;
	}
	// Damage die SIZE / magnitude (additive).
	if (s.damage_size.beta != 0.0) {
		const int size = std::clamp(up(s.damage_size, mob->mob_specials.damsizedice), 1, 100);
		mob->mob_specials.damsizedice = size;
	}
	// Flat damage bonus (+B added to every hit): the mob's damroll.
	if (s.damage_bonus.beta != 0.0) {
		GET_DR(mob) = up(s.damage_bonus, GET_DR(mob));
	}
	GET_HR(mob)         = up(s.hitroll, GET_HR(mob));
	GET_AC(mob)         = down(s.ac, GET_AC(mob));   // lower AC = better
	if (s.skills.beta != 0.0) {
		// scale every skill the prototype already has, uniformly.
		std::vector<ESkill> ids;
		for (const auto &kv : mob->GetCharSkills()) {
			ids.push_back(kv.first);
		}
		for (const ESkill id : ids) {
			SetSkill(mob, id, up(s.skills, GetSkill(mob, id)));
		}
	}
}

}  // namespace animate_dead

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
