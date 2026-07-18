/* ************************************************************************
*   File: im.cpp                                        Part of Bylins    *
*  Usage: Ingradient handling function                                    *
*                                                                         *
*                                                                         *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                       *
************************************************************************ */

// Реализация ингредиентной магии

#include "im.h"
#include "utils/parser_wrapper.h"
#include "utils/utils_parse.h"
#include <unordered_map>
#include <vector>
#include <cstdlib>
#include "gameplay/mechanics/damage.h"
#include "gameplay/fight/fight_constants.h"
#include "utils/random.h"
#include "administration/privilege.h"
#include "gameplay/magic/magic.h"
#include "gameplay/abilities/talents_actions.h"

#include <string>

#include "engine/db/world_characters.h"
#include "utils/utils_string.h"
#include "engine/db/world_objects.h"
#include "gameplay/mechanics/mob_races.h"
#include "engine/db/obj_prototypes.h"
#include "engine/core/obj_handler.h"
#include "engine/entities/char_data.h"
#include "gameplay/mechanics/inventory.h"
#include "engine/core/target_resolver.h"
#include "engine/ui/color.h"
#include "engine/ui/modify.h"
#include "engine/entities/zone.h"
#include "engine/db/global_objects.h"
#include "gameplay/core/base_stats.h"
#include "gameplay/core/remort.h"
#include "gameplay/classes/pc_classes_info.h"   // issue.class-recipes: MUD::Class().FindIngredientRecipe()

#define        VAR_CHAR    '@'
#define imlog(lvl, str)    mudlog(str, lvl, kLvlBuilder, IMLOG, true)

const short kMaxRecipeLevel = 200;

extern CharData *mob_proto;
extern IndexData *mob_index;

void do_rset(CharData *ch, char *argument, int cmd, int subcmd);
void do_recipes(CharData *ch, char *argument, int cmd, int subcmd);
void do_cook(CharData *ch, char *argument, int cmd, int subcmd);
void do_imlist(CharData *ch, char *argument, int cmd, int subcmd);

im_type *imtypes = nullptr;    // Список зарегестрированных ТИПОВ/МЕТАТИПОВ
int top_imtypes = -1;        // Последний номер типа ИМ
im_recipe *imrecipes = nullptr;    // Список зарегестрированных рецептов
int top_imrecipes = -1;        // Последний номер рецепта ИМ

// Поиск типа по имени name. mode=0-только элементарные,1-все подряд
int im_get_type_by_name(char *name, int mode) {
	int i;
	for (i = 0; i <= top_imtypes; ++i) {
		if (mode == 0 && imtypes[i].proto_vnum == -1)
			continue;
		if (!strn_cmp(name, imtypes[i].name, strlen(imtypes[i].name)))
			return i;
	}
	return -1;
}
//Поиск index по номеру ТИПА

int im_get_idx_by_type(int type) {
	int i;
	for (i = 0; i <= top_imtypes; ++i) {
		if (imtypes[i].id == type)
			return i;
	}
	return -1;
}

// Поиск rid по id
int im_get_recipe(int id) {
	int rid;
	for (rid = top_imrecipes; rid >= 0; --rid)
		if (imrecipes[rid].id == id)
			break;
	return rid;
}

// Поиск rid по имени
int im_get_recipe_by_name(char *name) {
	int rid;
	int ok;
	char *temp, *temp2;
	char first[256], first2[256];

	if (*name == 0)
		return -1;

	for (rid = top_imrecipes; rid >= 0; --rid) {
		if (utils::IsAbbr(name, imrecipes[rid].name))
			break;

		ok = true;
		temp = any_one_arg(imrecipes[rid].name, first);
		temp2 = any_one_arg(name, first2);
		while (*first && *first2 && ok) {
			if (!utils::IsAbbr(first2, first))
				ok = false;
			temp = any_one_arg(temp, first);
			temp2 = any_one_arg(temp2, first2);
		}
		if (ok && !*first2)
			break;
	}
	return rid;
}

// Поиск rid по строковому идентификатору рецепта (англ. CamelCase)
int im_get_recipe_by_str_id(const char *str_id) {
	if (!str_id || !*str_id) {
		return -1;
	}
	for (int rid = top_imrecipes; rid >= 0; --rid) {
		if (imrecipes[rid].str_id && !str_cmp(imrecipes[rid].str_id, str_id)) {
			return rid;
		}
	}
	return -1;
}

im_rskill *im_get_char_rskill(CharData *ch, int rid) {
	im_rskill *rs;
	for (rs = GET_RSKILL(ch); rs; rs = rs->link)
		if (rs->rid == rid)
			break;
	return rs;
}

int im_get_char_rskill_count(CharData *ch) {
	int cnt;
	im_rskill *rs;
	for (cnt = 0, rs = GET_RSKILL(ch); rs; rs = rs->link, ++cnt);
	return cnt;
}

// Расширяет хранилище битов, до указанного размера (в случае необходимости)
void TypeListAllocate(im_tlist *tl, int size) {
	if (tl->size < size) {
		long *ptr;
		CREATE(ptr, size);
		if (tl->size) {
			memcpy(ptr, tl->types, size * 4);
			free(tl->types);
		}
		tl->size = size;
		tl->types = ptr;
	}
}

// устанавливает в битовой маске списка типов tl бит номер index
void TypeListSetSingle(im_tlist *tl, int index) {
	TypeListAllocate(tl, (index >> 8) + 1);
	tl->types[index >> 8] |= (1 << (index & 31));
}

// устанавливает в битовой маске списка типов tl все биты списка src
void TypeListSet(im_tlist *tl, im_tlist *src) {
	int i;
	TypeListAllocate(tl, src->size);
	for (i = 0; i < src->size; ++i)
		tl->types[i] |= src->types[i];
}

// проверяет в битовой маске списка типов tl бит index
int TypeListCheck(im_tlist *tl, int index) {
	TypeListAllocate(tl, (index >> 8) + 1);
	return tl->types[index >> 8] & (1 << (index & 31));
}

// Рассчет силы ингредиента, при погрузке на землю
const int def_probs[] =
	{
		20,            // 20%    - 1
		35,            // 15%    - 2
		45,            // 10%    - 3
		53,            //  8%    - 4
		60,            //  7%    - 5
		66,            //  6%    - 6
		71,            //  5%    - 7
		76,            //  5%    - 8
		80,            //  4%    - 9
		84,            //  4%    - 10
		88,            //  4%    - 11
		91,            //  3%    - 12
		94,            //  3%    - 13
		96,            //  2%    - 14
		98,            //  2%    - 15
		99,            //  1%    - 16
		100            //  1%    - 17
		// старше 17 вообще не погрузятся
	};
int im_calc_power(void) {
	int j, i = number(1, 100);
	for (j = 0; i > def_probs[j++];);
	return j;
}


/*
   Загрузка магического ингредиента происходит в два этапа:
     1. Стандартными способами создается объект из прототипа ТИПА ингредиента
     2. Созданному объекту нзаначается сила
*/

// Поиск алиаса
char *get_im_alias(im_memb *s, const char *name) {
	char **al;
	for (al = s->aliases; al[0]; al += 2)
		if (!str_cmp(al[0], name))
			break;
	return al[1];
}

// Функция заменяет alias в названиях ингредиентов
const char *replace_alias(const char *ptr, im_memb *sample, int rnum, const char *std) {
	char *dst, *al;
	char aname[16];

	if (!sample && rnum == -1)
		return ptr;

	// Строю соответствуюжую строку в buf
	if (sample) {
		// поиск в образце
		if (std && (al = get_im_alias(sample, std)) != nullptr) {
			ptr = al;
		} else {
			// Посимвольный разбор строки
			dst = buf;
			do {
				if (*ptr == VAR_CHAR) {
					int k;
					++ptr;
					for (k = 0; (*ptr) && a_isalnum(*ptr); aname[k++] = *ptr++);
					aname[k] = 0;
					al = get_im_alias(sample, aname);
					strcpy(dst, al ? al : aname);
					while (*dst != 0)
						++dst;
				}
			} while ((*dst++ = *ptr++) != 0);
			ptr = buf;
		}
	} else {
		// поиск в мобе (только p0-p5)
		// Посимвольный разбор строки
		for (dst = buf; (*dst++ = *ptr++) != 0;) {
			int k;
			if (*ptr != VAR_CHAR)
				continue;
			sscanf(ptr + 1, "p%d", &k);
			if (k < 0 || k > 5)
				continue;
			ptr += 3;
			strcpy(dst, GET_PAD(mob_proto + rnum, k));
			while (*dst != 0)
				++dst;
		}
		ptr = buf;
	}

	return ptr;
}

int im_type_rnum(int vnum) {
	int rind;
	for (rind = top_imtypes; rind >= 0; --rind)
		if (imtypes[rind].id == vnum)
			break;
	return rind;
}

const char *def_alias[] = {"n0", "n1", "n2", "n3", "n4", "n5"};

// Указание силы ингредиента
int im_assign_power(ObjData *obj)
/*
   obj - загруженный ингредиент

   В случае успеха возвращает 0, иначе ошибка

   GET_OBJ_VAL(obj,IM_INDEX_SLOT) =        - уровень ингредиента или VNUM моба
   GET_OBJ_VAL(obj,IM_TYPE_SLOT)  = index  - номер ТИПА ингредиента (из im.lst)
   GET_OBJ_VAL(obj,IM_POWER_SLOT) = lev    - уровень ингредиента
*/
{
	int rind, onum;
	int rnum = -1;
	im_memb *sample, *p;
	int j;

	// Перевод номера index в rnum
	rind = im_type_rnum(GET_OBJ_VAL(obj, IM_TYPE_SLOT));
	if (rind == -1)
		return 1;    // неверный номер ТИПА ингредиента

// Поиск образца или моба
// Если используется живь, получить уровень из моба
	onum = GetObjRnum(imtypes[rind].proto_vnum);
	if (onum < 0)
		return 4;
	if (GET_OBJ_VAL(obj_proto[onum], 3) == static_cast<int>(EIngredientClass::kJiv)) {
		if (GET_OBJ_VAL(obj, IM_INDEX_SLOT) == -1)
			return 3;
		rnum = GetMobRnum(GET_OBJ_VAL(obj, IM_INDEX_SLOT));
		if (rnum < 0) {
			return 4;    // неверный VNUM базового моба
		}
		obj->set_val(IM_POWER_SLOT, (GetRealLevel(mob_proto + rnum) + 3) * 3 / 4);
	}
	// Попробовать найти описатель ВИДА
	for (p = imtypes[rind].head, sample = nullptr; p && p->power <= GET_OBJ_VAL(obj, IM_POWER_SLOT);
		 sample = p, p = p->next);

	if (sample) {
		obj->set_sex(sample->sex);
	}

	// Замена описаний
	// Падежи, описание, alias
	for (j = grammar::ECase::kFirstCase; j <= grammar::ECase::kLastCase; ++j) {
		auto name_case = static_cast<grammar::ECase>(j);
		const char *ptr = obj_proto[obj->get_rnum()]->get_PName(name_case).c_str();
		obj->set_PName(name_case, replace_alias(ptr, sample, rnum, def_alias[j]));
	}
	const char *ptr = obj_proto[obj->get_rnum()]->get_description().c_str();
	obj->set_description(replace_alias(ptr, sample, rnum, "s"));

	ptr = obj_proto[obj->get_rnum()]->get_short_description().c_str();
	obj->set_short_description(replace_alias(ptr, sample, rnum, def_alias[0]));

	ptr = obj_proto[obj->get_rnum()]->get_aliases().c_str();
	obj->set_aliases(replace_alias(ptr, sample, rnum, "m"));
	obj->set_is_rename(true); // чтоб в олц не портилось имя

	// Обработка других полей объекта
	// -- пока не сделано --

	return 0;
}

// Загрузка магического ингредиента
ObjData *load_ingredient(int index, int power, int rnum)
/*
   index - номер магического ингредиента для загрузки (в imtypes[])
   power - сила ингредиента
   rnum  - VNUM моба, в который грузится ингредиент
*/
{
	int err;

	while (1) {
		if (imtypes[index].proto_vnum < 0) {
			sprintf(buf, "IM METATYPE ingredient loading %d", imtypes[index].id);
			break;
		}

		const auto ing = world_objects.create_from_prototype_by_vnum(imtypes[index].proto_vnum);
		if (!ing) {
			sprintf(buf, "IM ingredient prototype %d not found", imtypes[index].proto_vnum);
			break;
		}

		ing->set_val(IM_INDEX_SLOT, rnum);
		ing->set_val(IM_POWER_SLOT, power);
		ing->set_val(IM_TYPE_SLOT, imtypes[index].id);

		err = im_assign_power(ing.get());
		if (err != 0) {
			ExtractObjFromWorld(ing.get());
			sprintf(buf, "IM power assignment error %d", err);
			break;
		}

		return ing.get();
	}

	imlog(CMP, buf);
	return nullptr;
}

void im_translate_rskill_to_id(void) {
	im_rskill *rs;
	for (const auto &ch : character_list) {
		if (ch->IsNpc()) {
			continue;
		}

		for (rs = GET_RSKILL(ch); rs; rs = rs->link) {
			rs->rid = imrecipes[rs->rid].id;
		}
	}
}

void im_translate_rskill_to_rid(void) {
	im_rskill *rs, **prs;
	int rid;
	for (const auto &ch : character_list) {
		if (ch->IsNpc())
			continue;
		prs = &GET_RSKILL(ch);
		while ((rs = *prs) != nullptr) {
			rid = im_get_recipe(rs->rid);
			if (rid >= 0) {
				rs->rid = rid;
				prs = &rs->link;
				continue;
			} else {
				*prs = rs->link;
				free(rs);
			}
		}
	}
}

void im_cleanup_type(im_type *t) {
	free(t->name);
	if (t->tlst.size != 0) {
		free(t->tlst.types);
		t->tlst.size = 0;
	}
}

void im_cleanup_recipe(im_recipe *r) {
	im_addon *a;
	free(r->name);
	free(r->str_id);
	free(r->require);
	free(r->msg_char[0]);
	free(r->msg_char[1]);
	free(r->msg_char[2]);
	free(r->msg_room[0]);
	free(r->msg_room[1]);
	free(r->msg_room[2]);
	while ((a = r->addon) != nullptr) {
		r->addon = a->link;
		free(a);
	}
}

// Инициализация подсистемы ингредиентной магии
void initIngredientsMagic(void) {
	char text[1024];
	int i, j;

	// Очистка всего, что есть, на случай reset (преобразование rid у игроков)
	im_translate_rskill_to_id();
	for (i = 0; i <= top_imtypes; ++i)
		im_cleanup_type(imtypes + i);
	for (i = 0; i <= top_imrecipes; ++i)
		im_cleanup_recipe(imrecipes + i);
	if (imtypes)
		free(imtypes);
	imtypes = nullptr;
	top_imtypes = -1;
	if (imrecipes)
		free(imrecipes);
	imrecipes = nullptr;
	top_imrecipes = -1;

	// issue.ingradient-magic: данные теперь в cfg/craft/ingredient_magic/*.xml (был misc/im.lst)
	parser_wrapper::DataNode types_doc, sorts_doc, recipes_doc;
	try {
		types_doc = parser_wrapper::DataNode(LIB_CFG "craft/ingredient_magic/ingredient_types.xml");
		sorts_doc = parser_wrapper::DataNode(LIB_CFG "craft/ingredient_magic/ingredient_sorts.xml");
		recipes_doc = parser_wrapper::DataNode(LIB_CFG "craft/ingredient_magic/recipes.xml");
	} catch (const std::exception &) {
		imlog(BRF, "Can not open cfg/craft/ingredient_magic/*.xml");
		return;
	}

	auto AttrInt = [](parser_wrapper::DataNode &n, const char *key, int def) -> int {
		const char *v = n.GetValue(key);
		if (!v || !*v) {
			return def;
		}
		try {
			return parse::ReadAsInt(v);
		} catch (const std::exception &) {
			return def;
		}
	};

	// id (английский) -> индекс в imtypes
	std::unordered_map<std::string, int> id2idx;

	// --- ТИПЫ и МЕТАТИПЫ (ingredient_types.xml) ---
	std::vector<parser_wrapper::DataNode> type_nodes, meta_nodes;
	{
		auto sec = types_doc;
		if (sec.GoToChild("types")) {
			for (auto &t : sec.Children("type"))
				type_nodes.push_back(t);
		}
	}
	{
		auto sec = types_doc;
		if (sec.GoToChild("metatypes")) {
			for (auto &m : sec.Children("metatype"))
				meta_nodes.push_back(m);
		}
	}
	CREATE(imtypes, static_cast<int>(type_nodes.size() + meta_nodes.size()));
	top_imtypes = -1;

	for (auto &t : type_nodes) {
		++top_imtypes;
		imtypes[top_imtypes].id = AttrInt(t, "num", 0);
		imtypes[top_imtypes].name = str_dup(t.GetValue("name"));
		imtypes[top_imtypes].proto_vnum = AttrInt(t, "proto_vnum", -1);
		imtypes[top_imtypes].head = nullptr;
		imtypes[top_imtypes].tlst.size = 0;
		imtypes[top_imtypes].tlst.types = nullptr;
		TypeListSetSingle(&imtypes[top_imtypes].tlst, top_imtypes);
		id2idx[t.GetValue("id")] = top_imtypes;
	}
	for (auto &m : meta_nodes) {
		++top_imtypes;
		imtypes[top_imtypes].id = AttrInt(m, "num", 0);
		imtypes[top_imtypes].name = str_dup(m.GetValue("name"));
		imtypes[top_imtypes].proto_vnum = -1;
		imtypes[top_imtypes].head = nullptr;
		imtypes[top_imtypes].tlst.size = 0;
		imtypes[top_imtypes].tlst.types = nullptr;
		for (auto &mem : m.Children("member")) {
			const char *mid = mem.GetValue("id");
			auto it = id2idx.find(mid ? mid : "");
			if (it == id2idx.end()) {
				snprintf(text, sizeof(text), "[IM] metatype '%s': unknown member '%s'",
						 m.GetValue("name"), mid ? mid : "");
				imlog(NRM, text);
				continue;
			}
			TypeListSet(&imtypes[top_imtypes].tlst, &imtypes[it->second].tlst);
		}
		id2idx[m.GetValue("id")] = top_imtypes;
	}

	// --- ВИДЫ / описатели (ingredient_sorts.xml). Ключи алиасов в рантайме: n0..n5, a, s ---
	static const std::unordered_map<std::string, std::string> kCaseKey = {
		{"kNom", "n0"}, {"kGen", "n1"}, {"kDat", "n2"},
		{"kAcc", "n3"}, {"kIns", "n4"}, {"kPre", "n5"}};
	static const std::unordered_map<std::string, EGender> kGenderById = {
		{"kNeutral", EGender::kNeutral}, {"kMale", EGender::kMale},
		{"kFemale", EGender::kFemale}, {"kPoly", EGender::kPoly}};
	{
		auto sec = sorts_doc;
		for (auto &s : sec.Children("sort")) {
			const char *stype = s.GetValue("type");
			auto it = id2idx.find(stype ? stype : "");
			if (it == id2idx.end()) {
				snprintf(text, sizeof(text), "[IM] sort: unknown type '%s'", stype ? stype : "");
				imlog(NRM, text);
				continue;
			}
			std::vector<std::pair<std::string, std::string>> pairs;
			for (auto &c : s.Children()) {
				const std::string tag = c.GetName();
				const char *nm = c.GetValue("name");
				if (tag == "case") {
					auto ck = kCaseKey.find(c.GetValue("id") ? c.GetValue("id") : "");
					if (ck != kCaseKey.end())
						pairs.emplace_back(ck->second, nm ? nm : "");
				} else if (tag == "alias") {
					pairs.emplace_back("a", nm ? nm : "");
				} else if (tag == "room_desc") {
					pairs.emplace_back("s", nm ? nm : "");
				}
			}
			im_memb *mb;
			CREATE(mb, 1);
			mb->power = AttrInt(s, "power", 1);
			const char *g = s.GetValue("gender");
			auto gi = kGenderById.find(g ? g : "");
			mb->sex = (gi != kGenderById.end()) ? gi->second : EGender::kNeutral;
			CREATE(mb->aliases, 2 * (static_cast<int>(pairs.size()) + 1));
			char **p = mb->aliases;
			for (auto &kv : pairs) {
				*p++ = str_dup(kv.first.c_str());
				*p++ = str_dup(kv.second.c_str());
			}
			p[0] = p[1] = nullptr;
			// вставка в список описателей типа по возрастанию силы (как в старом парсере)
			int idx = it->second;
			im_memb *ins_after = nullptr, *ins_before = imtypes[idx].head;
			while (ins_before && ins_before->power < mb->power) {
				ins_after = ins_before;
				ins_before = ins_before->next;
			}
			if (ins_after == nullptr)
				imtypes[idx].head = mb;
			else
				ins_after->next = mb;
			mb->next = ins_before;
		}
	}

	// --- РЕЦЕПТЫ (recipes.xml) ---
	std::vector<parser_wrapper::DataNode> recipe_nodes;
	for (auto &r : recipes_doc.Children("recipe"))
		recipe_nodes.push_back(r);
	CREATE(imrecipes, static_cast<int>(recipe_nodes.size()));
	top_imrecipes = -1;
	for (auto &r : recipe_nodes) {
		++top_imrecipes;
		im_recipe &rec = imrecipes[top_imrecipes];
		rec.id = AttrInt(r, "vnum", 0);
		rec.name = str_dup(r.GetValue("name"));
		rec.str_id = str_dup(r.GetValue("id") ? r.GetValue("id") : "");
		rec.k_improve = AttrInt(r, "k_improve", 1000);
		rec.result = AttrInt(r, "result_vnum", 0);
		{
			auto en = r;
			if (en.GoToChild("energy")) {
				const char *a0 = en.GetValue("k0"), *a1 = en.GetValue("k1");
				const char *a2 = en.GetValue("k2"), *ap = en.GetValue("kp");
				rec.k[0] = a0 ? atof(a0) : 0;
				rec.k[1] = a1 ? atof(a1) : 0;
				rec.k[2] = a2 ? atof(a2) : 0;
				rec.kp = ap ? atof(ap) : 0;
			}
		}
		{
			const char *d = r.GetValue("damage");
			if (d && *d)
				sscanf(d, "%dd%d", &rec.x, &rec.y);
		}
		{
			const char *de = r.GetValue("damage_enabled");
			rec.damage_enabled = de && (!strcmp(de, "true") || !strcmp(de, "1"));
		}
		{
			std::vector<int> req;
			for (auto &q : r.Children("require")) {
				const char *qt = q.GetValue("type");
				auto it = id2idx.find(qt ? qt : "");
				if (it == id2idx.end()) {
					snprintf(text, sizeof(text), "[IM] recipe %d: unknown require type '%s'", rec.id, qt ? qt : "");
					imlog(NRM, text);
					continue;
				}
				const int power = AttrInt(q, "power", 0);
				const int limit = AttrInt(q, "limit", 0xFFFF);
				req.push_back(it->second);
				req.push_back((limit << 16) | power);
			}
			CREATE(rec.require, static_cast<int>(req.size()) + 1);
			for (size_t n = 0; n < req.size(); ++n)
				rec.require[n] = req[n];
			rec.require[req.size()] = -1;
		}
		for (auto &a : r.Children("addon")) {
			const char *at = a.GetValue("type");
			auto it = id2idx.find(at ? at : "");
			if (it == id2idx.end()) {
				snprintf(text, sizeof(text), "[IM] recipe %d: unknown addon type '%s'", rec.id, at ? at : "");
				imlog(NRM, text);
				continue;
			}
			int n = AttrInt(a, "count", 1);
			const int a0 = AttrInt(a, "k0", 0), a1 = AttrInt(a, "k1", 0), a2 = AttrInt(a, "k2", 0);
			while (n-- > 0) {
				im_addon *adi;
				CREATE(adi, 1);
				adi->id = it->second;
				adi->k0 = a0;
				adi->k1 = a1;
				adi->k2 = a2;
				adi->obj = nullptr;
				adi->link = rec.addon;
				rec.addon = adi;
			}
		}
		{
			auto msgs = r;
			if (msgs.GoToChild("messages")) {
				const char *tags[3] = {"success", "fail", "damage"};
				for (int slot = 0; slot < 3; ++slot) {
					auto node = msgs;
					if (node.GoToChild(tags[slot])) {
						const char *mc = node.GetValue("char"), *mr = node.GetValue("room");
						if (mc)
							rec.msg_char[slot] = str_dup(mc);
						if (mr)
							rec.msg_room[slot] = str_dup(mr);
					}
				}
			}
		}
	}

	// Перестройка принадлежности элементарных типов
	for (i = 0; i <= top_imtypes; ++i) {
		if (imtypes[i].proto_vnum == -1)
			continue;
		for (j = 0; j <= top_imtypes; ++j)
			if (i != j && imtypes[j].proto_vnum == -1 && TypeListCheck(&imtypes[j].tlst, i))
				TypeListSetSingle(&imtypes[i].tlst, j);
	}
	// Удаление списков для составных типов
	for (i = 0; i <= top_imtypes; ++i) {
		if (imtypes[i].proto_vnum != -1)
			continue;
		imtypes[i].tlst.size = 0;
		free(imtypes[i].tlst.types);
	}

	// issue.class-recipes: принадлежность рецептов классам (владение/уровень/реморт)
	// перенесена из misc/class.recipes.lst в cfg/classes/pc_*.xml (секция <ingredient_magic>).
	// Здесь рецепты больше ничего не знают о классах - это свойство класса.

	im_translate_rskill_to_rid();

#if 0
	log(NRM, "IM types dump");
	for (i = 0; i <= top_imtypes; ++i)
	{
		int j;
		log("RNUM=%d,ID=%d,NAME: %s", i, imtypes[i].id, imtypes[i].name);
		for (j = 0; j < imtypes[i].tlst.size; ++j)
			log("%08lX", imtypes[i].tlst.types[j]);
	}
	log("IM recipes dump");
	for (i = 0; i <= top_imrecipes; ++i)
	{
		log("RNUM=%d,ID=%d,NAME: %s", i, imrecipes[i].id, imrecipes[i].name);
	}
#endif

}

// Просматривает строку line и добавляет загружаемые ингрединенты
// Формат строки: <номер>:<вер-ть>
// Не используется, т.к. ингредиенты перенесены в конфиг зон и мобов в misc
void im_parse(int **ing_list, char *line) {
	int local_count = 0;
	int count = 0;
	int *local_list = nullptr;
	int *res;
	int n, l, p, *ptr;

	while (1) {
		skip_spaces(&line);
		if (*line == 0)
			break;
		if (a_isdigit(*line)) {
			n = strtol(line, &line, 10);
			n = im_type_rnum(n);
		} else {
			n = im_get_type_by_name(line, 1);
			if (n >= 0)
				line += strlen(imtypes[n].name);
		}
		if (n < 0)
			break;
		l = 0xFFFF;
		if (*line == ',') {
			++line;
			l = strtol(line, &line, 10);
		}
		if (*line++ != ':') {
			break;
		}

		p = strtol(line, &line, 10);
		if (!p) {
			break;
		}

		if (!local_count) {
			CREATE(local_list, 2);
		} else {
			RECREATE(local_list, local_count + 2);
		}

		local_list[local_count++] = n;
		local_list[local_count++] = (l << 16) | p;
	}
	if (*ing_list) {
		for (ptr = *ing_list; (*ptr++ != -1););
		count = ptr - *ing_list - 1;
	}
	CREATE(res, local_count + count + 1);
	if (count) {
		memcpy(res, *ing_list, count * sizeof(int));
	}
	memcpy(res + count, local_list, local_count * sizeof(int));
	res[count + local_count] = -1;
	if (*ing_list) {
		free(*ing_list);
	}
	free(local_list);
	*ing_list = res;
}

void im_reset_room(RoomData *room, int level, int type) {
	ObjData *o;
	int i, indx;
	im_memb *after, *before;
	int pow, lev = level;
	// 40 * level / MAX_ZONE_LEVEL;

	for (auto it = room->contents.begin(); it != room->contents.end(); ) {
		auto o = *it; ++it;
		if (o->get_type() == EObjType::kMagicComponent) {
			ExtractObjFromWorld(o, false);
		}
	}

	// пропускаем виртуальные комнаты
	if (zone_table[room->zone_rn].vnum * 100 + 99 == room->vnum) {
		return;
	}

	// (issue.ztypes-migrate) Reach zone-type data through the new info_container
	// registry. Unknown vnum yields the kUndefined entry whose ingredient list is
	// empty, so the short-circuit below covers both "no ingredients configured"
	// and "zone.type out of range" cases the legacy array used to hit.
	const auto &zone_type_info = MUD::ZoneTypes()[type];
	const auto &ingredients = zone_type_info.GetIngredients();
	if (ingredients.empty()
		|| room->get_flag(ERoomFlag::kDeathTrap)) {
		return;
	}
	for (i = 0; i < static_cast<int>(ingredients.size()); i++) {
		//	3% - 1-17
		//	2% - 18-34
		//	1% - 35-50
		//		if (number(1, 100) <= 3 - 3 * (level - 1) / MAX_ZONE_LEVEL)
		if (number(1, 1000) <= (4 - level / 10) * 10) {
			indx = im_type_rnum(ingredients[i]);
			if (indx == -1) {
				log("SYSERR: WRONG INGREDIENT TYPE Id %d IN zone_types.xml", ingredients[i]);
				continue;
			}
			after = nullptr;
			before = imtypes[indx].head;
			while (before && before->power < lev) {
				after = before;
				before = before->next;
			}
			if (after == nullptr && before == nullptr) {
				log("SYSERR: NO INGREDIENTS OF TYPE %d AVAILABLE NOW", indx);
				continue;
			} else if (after == nullptr)
				pow = before->power;
			else if (before == nullptr)
				pow = after->power;
			else
				pow = lev - after->power < before->power - lev ? after->power : before->power;
			o = load_ingredient(indx, pow, -1);
			if (o) {
				PlaceObjToRoom(o, GetRoomRnum(room->vnum));
			}
		}
	}
}

ObjData *try_make_ingr(int *ing_list, int vnum, int max_prob) {
	for (int indx = 0; ing_list[indx] != -1; indx += 2) {
		int power;
		if (number(1, max_prob) >= (ing_list[indx + 1] & 0xFFFF)) {
			continue;
		}
		power = (ing_list[indx + 1] >> 16) & 0xFFFF;
		if (power == 0xFFFF) {
			power = im_calc_power();
		}
		return load_ingredient(ing_list[indx], power, vnum);
	}
	return nullptr;
}

ObjData *try_make_ingr(CharData *mob, int prob_default) {
	const int vnum = GET_MOB_VNUM(mob);
	if (MUD::MobRaces().IsKnown(GET_RACE(mob))) {
		const auto &ingrlist = MUD::MobRaces()[GET_RACE(mob)].GetIngredients();
		size_t num_inrgs = ingrlist.size();
		int *ingr_to_load_list = nullptr;
		CREATE(ingr_to_load_list, num_inrgs * 2 + 1);
		size_t j = 0;
		const int level_mob = GetRealLevel(mob) > 0 ? GetRealLevel(mob) : 1;
		for (; j < num_inrgs; j++) {
			ingr_to_load_list[2 * j] = im_get_idx_by_type(ingrlist[j].imtype);
			ingr_to_load_list[2 * j + 1] = ingrlist[j].prob.at(level_mob - 1);
			ingr_to_load_list[2 * j + 1] |= (level_mob << 16);
		}
		ingr_to_load_list[2 * j] = -1;
		return try_make_ingr(ingr_to_load_list, vnum, prob_default);
	}

	return nullptr;
}

void list_recipes(CharData *ch, bool all_recipes) {
	int i = 0, sortpos;
	im_rskill *rs;

	if (all_recipes) {
		if (!ch->IsFlagged(EPrf::kBlindMode)) {
			sprintf(buf, " Список доступных рецептов.\r\n"
						 " Зеленым цветом выделены уже изученные рецепты.\r\n"
						 " Красным цветом выделены рецепты, недоступные вам в настоящий момент.\r\n"
						 "\r\n     Рецепт                Уровень (реморт)\r\n"
						 "------------------------------------------------\r\n");
		} else {
			sprintf(buf, " Список доступных рецептов.\r\n"
						 " Пометкой [И] выделены уже изученные рецепты.\r\n"
						 " Пометкой [Д] выделены доступные для изучения рецепты.\r\n"
						 " Пометкой [Н] выделены рецепты, недоступные вам в настоящий момент.\r\n"
						 "\r\n     Рецепт                Уровень (реморт)\r\n"
						 "------------------------------------------------\r\n");
		}
		strcpy(buf1, buf);
		// issue.class-recipes: доступность рецепта классу спрашиваем у самого класса.
		const auto &char_class = MUD::Class(ch->GetClass());
		for (sortpos = 0, i = 0; sortpos <= top_imrecipes; sortpos++) {
			const auto *req = char_class.FindIngredientRecipe(imrecipes[sortpos].str_id);
			if (!req) {
				continue;
			}

			if (strlen(buf1) >= kMaxStringLength - 60) {
				strcat(buf1, "***ПЕРЕПОЛНЕНИЕ***\r\n");
				break;
			}
			rs = im_get_char_rskill(ch, sortpos);
			const bool unavailable = req->level > GetRealLevel(ch) || req->remort > remort::GetRealRemort(ch);
			if (!ch->IsFlagged(EPrf::kBlindMode)) {
				sprintf(buf, "     %s%-30s%s %2d (%2d)%s\r\n",
						unavailable ? kColorRed : rs ? kColorGrn : kColorNrm,
						imrecipes[sortpos].name, kColorCyn,
						req->level, req->remort, kColorNrm);
			} else {
				sprintf(buf, " %s %-30s %2d (%2d)\r\n",
						unavailable ? "[Н]" : rs ? "[И]" : "[Д]", imrecipes[sortpos].name,
						req->level, req->remort);
			}
			strcat(buf1, buf);
			++i;
		}
		if (!i)
			sprintf(buf1 + strlen(buf1), "Нет рецептов.\r\n");
		page_string(ch->desc, buf1, 1);
		return;
	}

	sprintf(buf, "Вы владеете следующими рецептами :\r\n");

	strcpy(buf2, buf);

	for (rs = GET_RSKILL(ch), i = 0; rs; rs = rs->link) {
		if (strlen(buf2) >= kMaxStringLength - 60) {
			strcat(buf2, "***ПЕРЕПОЛНЕНИЕ***\r\n");
			break;
		}
		if (rs->perc <= 0)
			continue;
		sprintf(buf, "%-30s %s%s\r\n", imrecipes[rs->rid].name, how_good(rs->perc, kMaxRecipeLevel), kColorBoldBlk);
		strcat(buf2, buf);
		++i;
	}

	if (!i)
		sprintf(buf2 + strlen(buf2), "Нет рецептов.\r\n");

	page_string(ch->desc, buf2, 1);
}

void do_recipes(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (ch->IsNpc())
		return;
	skip_spaces(&argument);
	if (utils::IsAbbr(argument, "все") || utils::IsAbbr(argument, "all"))
		list_recipes(ch, true);
	else
		list_recipes(ch, false);
}

void do_rset(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharData *vict;
	char name[kMaxInputLength], buf2[128];
	char buf[kMaxInputLength], help[kMaxStringLength];
	int rcpt = -1, value, i, qend;
	im_rskill *rs;

	argument = one_argument(argument, name);

	// * No arguments. print an informative text.
	if (!*name) {
		SendMsgToChar("Формат: rset <игрок> '<рецепт>' <значение>\r\n", ch);
		strcpy(help, "Зарегистрированные рецепты:\r\n");
		for (qend = 0, i = 0; i <= top_imrecipes; i++) {
			sprintf(help + strlen(help), "%30s", imrecipes[i].name);
			if (qend++ % 2 == 1) {
				strcat(help, "\r\n");
				SendMsgToChar(help, ch);
				*help = '\0';
			}
		}
		if (*help)
			SendMsgToChar(help, ch);
		SendMsgToChar("\r\n", ch);
		return;
	}

	vict = target_resolver::FindCharInWorld(ch, name);

	if (!vict) {
		SendMsgToChar(CommonMsg(ECommonMsg::kNoPerson) + "\r\n", ch);
		return;
	}
	skip_spaces(&argument);

	// If there is no entities in argument
	if (!*argument) {
		SendMsgToChar("Пропущено название рецепта.\r\n", ch);
		return;
	}
	if (*argument != '\'') {
		SendMsgToChar("Рецепт надо заключить в символы : ''\r\n", ch);
		return;
	}
	// Locate the last quote and lowercase the magic words (if any)

	for (qend = 1; argument[qend] && argument[qend] != '\''; qend++)
		argument[qend] = LOWER(argument[qend]);

	if (argument[qend] != '\'') {
		SendMsgToChar("Рецепт должен быть заключен в символы : ''\r\n", ch);
		return;
	}
	strcpy(help, (argument + 1));
	help[qend - 1] = '\0';

	rcpt = im_get_recipe_by_name(help);

	if (rcpt < 0) {
		SendMsgToChar("Неизвестный рецепт.\r\n", ch);
		return;
	}
	argument += qend + 1;    // skip to next parameter
	argument = one_argument(argument, buf);

	if (!*buf) {
		SendMsgToChar("Пропущен уровень рецепта.\r\n", ch);
		return;
	}
	value = atoi(buf);
	if (value < 0) {
		SendMsgToChar("Минимальное значение рецепта 0.\r\n", ch);
		return;
	}
	if (value > kMaxRecipeLevel) {
		SendMsgToChar("Превышено максимальное значение рецепта.\r\n", ch);
		value = kMaxRecipeLevel;
	}
	if (vict->IsNpc()) {
		SendMsgToChar("Вы не можете добавить рецепт для мобов.\r\n", ch);
		return;
	}
	// Задача - найти рецепт rcpt и установить его в value
	rs = im_get_char_rskill(vict, rcpt);
	if (!rs) {
		CREATE(rs, 1);
		rs->rid = rcpt;
		rs->link = GET_RSKILL(vict);
		GET_RSKILL(vict) = rs;
	}
	rs->perc = value;

	sprintf(buf2, "%s changed %s's %s to %d.", GET_NAME(ch), GET_NAME(vict), imrecipes[rcpt].name, value);
	mudlog(buf2, BRF, -1, SYSLOG, true);
	imm_log("%s changed %s's %s to %d.", GET_NAME(ch), GET_NAME(vict), imrecipes[rcpt].name, value);
	SendMsgToChar(buf2, ch);
}

void im_improve_recipe(CharData *ch, im_rskill *rs, int success) {
	int prob, div, diff;

	if (ch->IsNpc() || (rs->perc >= kMaxRecipeLevel))
		return;

	if (ch->in_room != kNowhere && (CalcSkillWisdomCap(ch) - rs->perc > 0)) {
		int n = ch->get_skills_count();
		n = (n + 1) >> 1;
		n += im_get_char_rskill_count(ch);
		prob = success ? 20000 : 15000;
		div = IntApp(GetRealInt(ch)).improve;
		div += imrecipes[rs->rid].k_improve / 100;
		prob /= std::max(1, div);
		diff = n - wis_bonus(GetRealWis(ch), WIS_MAX_SKILLS);
		if (diff < 0)
			prob += (5 * diff);
		else
			prob += (10 * diff);
		prob += number(1, rs->perc * 5);
		if (number(1, MAX(1, prob)) <= GetRealInt(ch)) {
			if (success)
				sprintf(buf,
						"%sВы постигли тонкости приготовления рецепта \"%s\".%s\r\n",
						kColorBoldCyn, imrecipes[rs->rid].name, kColorNrm);
			else
				sprintf(buf,
						"%sНеудача позволила вам осознать тонкости приготовления рецепта \"%s\".%s\r\n",
						kColorBoldCyn, imrecipes[rs->rid].name, kColorNrm);
			SendMsgToChar(buf, ch);
			rs->perc += number(1, 2);
			if (!privilege::IsImmortal(ch))
				rs->perc = MIN(CalcSkillRemortCap(ch), rs->perc);
		}
	}
}

ObjData **im_obtain_ingredients(CharData *ch, char *argument, int *count) {
	char name[kMaxInputLength], buf[kMaxInputLength];
	ObjData **array = nullptr;
	ObjData *o;
	int i, n = 0;

	while (1) {
		argument = one_argument(argument, name);
		if (!*name) {
			if (!n) {
				SendMsgToChar("Укажите магические ингредиенты для рецепта.\r\n", ch);
			}
			count[0] = n;
			return array;
		}
		o = get_obj_in_list_vis(ch, name, ch->carrying);
		if (!o) {
			snprintf(buf, kMaxInputLength, "У вас нет %s.\r\n", name);
			break;
		}
		if (o->get_type() != EObjType::kMagicComponent) {
			sprintf(buf, "Вы должны использовать только магические ингредиенты.\r\n");
			break;
		}
		if (im_type_rnum(GET_OBJ_VAL(o, IM_TYPE_SLOT)) < 0) {
			sprintf(buf, "Магическая сила %s утеряна, похоже, безвозвратно.\r\n", o->get_PName(grammar::ECase::kGen).c_str());
			break;
		}
		for (i = 0; i < n; ++i) {
			if (array[i] != o)
				continue;
			sprintf(buf, "Один и тот же ингредиент нельзя использовать дважды.\r\n");
			break;
		}
		if (i != n) {
			break;
		}
		if (!array) {
			CREATE(array, 1);
		} else {
			RECREATE(array, n + 1);
		}
		array[n++] = o;
	}
	if (array)
		free(array);
	imlog(NRM, buf);
	SendMsgToChar(buf, ch);
	return nullptr;
}

#define        IS_RECIPE_DELIM(c)        (((c)=='\'')||((c)=='*')||((c)=='!'))

// Применение рецепта
// варить 'рецепт' <ингредиенты>
void do_cook(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	char name[kMaxStringLength];
	int rcpt = -1, qend, mres;
	im_rskill *rs;
	ObjData **objs;
	int tgt;
	int i, num, add_start;
	int *req;
	float W1, W2, osr, prob;
	float param[IM_NPARAM];
	int val[IM_NPARAM];
	im_addon *addon;

	// Определяем, что за рецепт пытаемся варить
	skip_spaces(&argument);
	if (!*argument) {
		SendMsgToChar("Пропущено название рецепта.\r\n", ch);
		return;
	}
	if (!IS_RECIPE_DELIM(*argument)) {
		SendMsgToChar("Рецепт надо заключить в символы : ' * или !\r\n", ch);
		return;
	}
	for (qend = 1; argument[qend] && !IS_RECIPE_DELIM(argument[qend]); qend++)
		argument[qend] = LOWER(argument[qend]);
	if (!IS_RECIPE_DELIM(argument[qend])) {
		SendMsgToChar("Рецепт должен быть заключен в символы : ' * или !\r\n", ch);
		return;
	}
	strcpy(name, (argument + 1));
	argument += qend + 1;
	name[qend - 1] = '\0';
	rcpt = im_get_recipe_by_name(name);
	if (rcpt < 0) {
		SendMsgToChar("Вам такой рецепт неизвестен.\r\n", ch);
		return;
	}
	rs = im_get_char_rskill(ch, rcpt);
	if (!rs) {
		SendMsgToChar("Вам такой рецепт неизвестен.\r\n", ch);
		return;
	}
	// rs - используемый рецепт
	// argument - список ингредиентов

	sprintf(name, "%s использует рецепт %s", GET_NAME(ch), imrecipes[rs->rid].name);
	imlog(BRF, name);

	// Преобразование строки аргументов в массив объектов
	// с проверкой повторных и т.д.
	objs = im_obtain_ingredients(ch, argument, &num);
	if (!objs)
		return;

	imlog(NRM, "Используемые ингредиенты:");
	name[0] = 0;
	for (i = 0; i < num; ++i) {
		sprintf(name + strlen(name), "%s:%d ",
				imtypes[im_type_rnum(GET_OBJ_VAL(objs[i], IM_TYPE_SLOT))].
					name, GET_OBJ_VAL(objs[i], IM_POWER_SLOT));
	}
	imlog(NRM, name);

	imlog(NRM, "Цикл обработки базовых компонентов");

	W1 = 0;
	W2 = 0;
	osr = 0;
	// Этап 1. Основные компоненты
	i = 0;
	req = imrecipes[rs->rid].require;
	while (*req != -1) {
		int itype, ktype, osk;
		if (i == num) {
			imlog(NRM, "Не хватает основных ингредиентов");
			SendMsgToChar("Похоже, вам не хватает ингредиентов.\r\n", ch);
			free(objs);
			return;
		}
		ktype = *req++;
		osk = *req++ & 0xFFFF;
		osr += osk;
		sprintf(name, "Запрошенный ингредиент: type=%d,osk=%d", ktype, osk);
		imlog(CMP, name);
		itype = im_type_rnum(GET_OBJ_VAL(objs[i], IM_TYPE_SLOT));
		// ktype - требуемый тип, itype - подставляемый тип
		if (!TypeListCheck(&imtypes[itype].tlst, ktype)) {
			imlog(NRM, "Ингредиенты перепутаны");
			SendMsgToChar("Похоже, вы перепутали ингредиенты.\r\n", ch);
			free(objs);
			return;
		}
		itype = GET_OBJ_VAL(objs[i], IM_POWER_SLOT);
		if (itype > osk)
			W1 += (itype - osk) * osk;
		else
			W2 += osk - itype;
		// минимальное качество ингров для варки рецепта (REQ рецепта/2)
		int min_osk = osk / 2;
		if (itype < min_osk) {
			SendMsgToChar(ch, "Качество %s ниже минимально допустимого.\r\n", objs[i]->get_PName(grammar::ECase::kGen).c_str());
			sprintf(name, "Качество ингров ниже допустимого: itype=%d, min_osk=%d", itype, min_osk);
			imlog(NRM, name);
			free(objs);
			return;
		}
		++i;
	}
	add_start = i;
	if (osr)
		W1 /= osr;
	sprintf(name, "Рассчитанные основные ингредиенты: W1=%f W2=%f", W1, W2);
	imlog(CMP, name);
	// Преобразование параметров прототипа
	tgt = GetObjRnum(imrecipes[rs->rid].result);
	if (tgt < 0) {
		imlog(NRM, "Прототип утерян");
		SendMsgToChar("Результат рецепта утерян.\r\n", ch);
		free(objs);
		return;
	}

	switch (obj_proto[tgt]->get_type()) {
		case EObjType::kScroll:
		case EObjType::kPotion: param[0] = GET_OBJ_VAL(obj_proto[tgt], 0);    // уровень
			param[1] = 1;    // количество
			param[2] = obj_proto[tgt]->get_timer();    // таймер
			break;

		case EObjType::kWand:
		case EObjType::kStaff: param[0] = GET_OBJ_VAL(obj_proto[tgt], 0);    // уровень
			param[1] = GET_OBJ_VAL(obj_proto[tgt], 1);    // количество
			param[2] = obj_proto[tgt]->get_timer();    // таймер
			break;

		default: imlog(NRM, "Прототип имеет неверный тип");
			SendMsgToChar("Результат варева непредсказуем.\r\n", ch);
			free(objs);
			return;
	}

	sprintf(name, "Базовые параметры и курс перевода в магические Дж: %f,%f %f,%f %f,%f",
			param[0], imrecipes[rs->rid].k[0],
			param[1], imrecipes[rs->rid].k[1], param[2], imrecipes[rs->rid].k[2]);
	imlog(CMP, name);

	W2 *= imrecipes[rs->rid].kp;
	prob = (float) rs->perc - 5 + 2 * W1 - W2;
	for (i = 0; i < IM_NPARAM; ++i) {
		param[i] *= imrecipes[rs->rid].k[i];
		W1 += param[i];
	}

	imlog(CMP, "Дамп расчета до поворота направляющей:");
	sprintf(name, "Вероятность: %f", prob);
	imlog(CMP, name);
	sprintf(name, "Закон сохранения ДжМ: x+y+z=%f", W1);
	imlog(CMP, name);
	sprintf(name, "Коэффициенты направления: x0=%f y0=%f z0=%f", param[0], param[1], param[2]);
	imlog(CMP, name);

	if (prob < 0) {
		SendMsgToChar("С ингредиентами такого качества вам лучше даже не пытаться...\r\n", ch);
		free(objs);
		return;
	}

	// Этап 2. Дополнительные компоненты
	for (addon = imrecipes[rs->rid].addon; addon; addon = addon->link)
		addon->obj = nullptr;
	for (i = add_start; i < num; ++i) {
		int itype = im_type_rnum(GET_OBJ_VAL(objs[i], IM_TYPE_SLOT));
		for (addon = imrecipes[rs->rid].addon; addon; addon = addon->link)
			if (addon->obj == nullptr && TypeListCheck(&imtypes[itype].tlst, addon->id))
				break;
		if (addon) {
			// "белый" список
			int s = addon->k0 + addon->k1 + addon->k2;
			addon->obj = objs[i];
			param[0] += (float) GET_OBJ_VAL(objs[i], IM_POWER_SLOT) * addon->k0 / s;
			param[1] += (float) GET_OBJ_VAL(objs[i], IM_POWER_SLOT) * addon->k1 / s;
			param[2] += (float) GET_OBJ_VAL(objs[i], IM_POWER_SLOT) * addon->k2 / s;
		} else {
			// "черный" список
			W1 -= GET_OBJ_VAL(objs[i], IM_POWER_SLOT);
		}
	}

	sprintf(name, "Закон сохранения ДжМ после поворота: x+y+z=%f", W1);
	imlog(CMP, name);
	sprintf(name, "Коэффициенты направления после поворота: x0=%f y0=%f z0=%f", param[0], param[1], param[2]);
	imlog(CMP, name);

	// Этап 3. Получение результата
	for (W2 = 0, i = 0; i < IM_NPARAM; ++i)
		W2 += param[i];
	for (i = 0; i < IM_NPARAM; ++i) {
		param[i] *= W1;
		param[i] /= W2;
	}
	for (i = 0; i < IM_NPARAM; ++i) {
		if (imrecipes[rs->rid].k[i])
			val[i] = (int) (0.5 + param[i] / imrecipes[rs->rid].k[i]);
		else
			val[i] = -1;    // не изменять
	}

	sprintf(name, "Параметры результата: %d %d %d", val[0], val[1], val[2]);
	imlog(CMP, name);

	// Удалаяю объекты
	for (i = 0; i < num; ++i)
		ExtractObjFromWorld(objs[i]);
	free(objs);

	imlog(CMP, "Ингредиенты удалены");

	// Кидаем кубики на создание
	mres = number(1, 100 - (CanUseFeat(ch, EFeat::kHerbalist) ? 5 : 0));
	if (mres < (int) prob)
		mres = IM_MSG_OK;
	else {
		mres = (100 - (int) prob) / 2;
		if (mres >= number(1, 100))
			mres = IM_MSG_FAIL;
		else
			mres = kImMsgDam;
	}

	sprintf(name, "Режим - %d", mres);
	imlog(CMP, name);

	im_improve_recipe(ch, rs, mres == IM_MSG_OK);

	// Рассылаю сообщения
	imlog(CMP, "Рассылка сообщений");
	act(imrecipes[rs->rid].msg_char[mres], true, ch, 0, 0, kToChar);
	act(imrecipes[rs->rid].msg_room[mres], true, ch, 0, 0, kToRoom);

	// issue.ingradient-magic: урон при критическом провале (DAM XdY), если рецепт его разрешает.
	// kTriggerDeath: своё сообщение уже выдано (msg_char/room[2]), система урона не дублирует.
	if (mres == kImMsgDam && imrecipes[rs->rid].damage_enabled && imrecipes[rs->rid].x > 0) {
		Damage potion_dmg(SimpleDmg(fight::EDamageSource::kTriggerDeath),
						  RollDices(imrecipes[rs->rid].x, imrecipes[rs->rid].y), fight::kUndefDmg);
		potion_dmg.flags.set(fight::kNoFleeDmg);
		potion_dmg.Process(ch, ch);
		return;
	}

	if (mres == IM_MSG_OK) {
		imlog(CMP, "Создание результата");
		const auto result = world_objects.create_from_prototype_by_rnum(tgt);
		if (result) {
			switch (result->get_type()) {
				case EObjType::kScroll:
					// issue.magic-items: a crafted scroll stores the crafter's competence (recipe skill + Int),
					// like a brewed potion; its spells come from the prototype's kSpellItem* keys.
					result->SetPotionValueKey(ObjVal::EValueKey::kMakerSkill, rs->perc);
					result->SetPotionValueKey(ObjVal::EValueKey::kMakerStat, GetRealBaseStat(ch, EBaseStat::kInt));
					if (val[2] > 0) {
						result->set_timer(val[2]);
					}
					break;
				case EObjType::kPotion:
					if (val[0] > 0) {
						result->set_val(0, val[0]);
					}
					if (val[2] > 0) {
						result->set_timer(val[2]);
					}
					// issue.potion-hotfix: brew the potion's strength into its OWN keys, never val3
					// (which is the 3rd spell). Two values, used separately at cast:
					//   kPotionPotency  = the brewer's COMPETENCE (recipe skill rs->perc + stat),
					//                     deterministic (no dice) -- the cast potency C.
					//   kPotionBrewRoll = ONE standard-normal "brew luck" draw (issue.random-noise-rework),
					//                     FROZEN so every quaff is identical; sigma-INDEPENDENT, so each of
					//                     the potion's spells applies its OWN sigma to it at cast. Encoded
					//                     (z + kBrewRollBias)*kBrewRollScale, always > 0.
					if (result->get_type() == EObjType::kPotion) {
						const auto potion_spell = static_cast<ESpell>(result->get_val(1));
						if (potion_spell > ESpell::kUndefined) {
							// issue.potion-hotfix: preserve the maker's INPUTS, not the (non-obvious)
							// computed potency: the brewing skill (rs->perc -- it stands in for the magic
							// skill, since non-mages brew too) and the key stat (Intelligence). The
							// competence is derived from these at cast, per spell.
							result->SetPotionValueKey(ObjVal::EValueKey::kMakerSkill, rs->perc);
							result->SetPotionValueKey(ObjVal::EValueKey::kMakerStat,
													  GetRealBaseStat(ch, EBaseStat::kInt));
							// One standard-normal draw (mean z=0, sd z=1), encoded with the bias so it
							// stores positive. sigma-independent: every spell scales it by its own sigma.
							const int brew_roll = GaussIntNumber(
									ObjVal::kBrewRollBias * ObjVal::kBrewRollScale, ObjVal::kBrewRollScale,
									1, 2 * ObjVal::kBrewRollBias * ObjVal::kBrewRollScale);
							result->SetPotionValueKey(ObjVal::EValueKey::kPotionBrewRoll, brew_roll);
						}
					}
					break;

				case EObjType::kWand:
				case EObjType::kStaff:
					// issue.magic-items: crafted wand/staff -- crafter competence + charges into kSpellItem keys.
					result->SetPotionValueKey(ObjVal::EValueKey::kMakerSkill, rs->perc);
					result->SetPotionValueKey(ObjVal::EValueKey::kMakerStat, GetRealBaseStat(ch, EBaseStat::kInt));
					if (val[1] > 0) {
						result->SetPotionValueKey(ObjVal::EValueKey::kMaxCharges, val[1]);
						result->SetPotionValueKey(ObjVal::EValueKey::kCurCharges, val[1]);
					}
					if (val[2] > 0) {
						result->set_timer(val[2]);
					}
					break;

				default: break;
			}
			PlaceObjToInventory(result.get(), ch);
		}
	}

	return;
}

void compose_recipe(CharData *ch, char *argument, int/* subcmd*/) {
	char name[kMaxStringLength];
	int qend, rcpt = -1;
	im_rskill *rs;

	// Определяем, что за рецепт пытаемся варить
	skip_spaces(&argument);
	if (!*argument) {
		SendMsgToChar("Пропущено название рецепта.\r\n", ch);
		return;
	}

	if (!IS_RECIPE_DELIM(*argument)) {
		SendMsgToChar("Рецепт надо заключить в символы : ' * или !\r\n", ch);
		return;
	}

	for (qend = 1; argument[qend] && !IS_RECIPE_DELIM(argument[qend]); qend++) {
		argument[qend] = LOWER(argument[qend]);
	}

	if (!IS_RECIPE_DELIM(argument[qend])) {
		SendMsgToChar("Рецепт должен быть заключен в символы : ' * или !\r\n", ch);
		return;
	}

	strcpy(name, (argument + 1));
	argument += qend + 1;
	name[qend - 1] = '\0';
	rcpt = im_get_recipe_by_name(name);
	if (rcpt < 0) {
		SendMsgToChar("Вам такой рецепт неизвестен.\r\n", ch);
		return;
	}

	rs = im_get_char_rskill(ch, rcpt);
	if (!rs) {
		SendMsgToChar("Вам такой рецепт неизвестен.\r\n", ch);
		return;
	}

	// rs - используемый рецепт

	SendMsgToChar("Вам потребуется :\r\n", ch);

	// Этап 1. Основные компоненты
	for (int i = 1, *req = imrecipes[rs->rid].require; *req != -1; req += 2, ++i) {
		sprintf(name, "%s%d%s) %s%s%s\r\n", kColorBoldGrn, i,
				kColorNrm, kColorBoldYel, imtypes[*req].name, kColorNrm);
		SendMsgToChar(name, ch);
	}
	sprintf(name, "для приготовления отвара '%s'\r\n", imrecipes[rs->rid].name);
	SendMsgToChar(name, ch);

	// Этап 2. Дополнительные компоненты *** НЕ ОБРАБАТЫВАЮТСЯ ***
}

// Поиск rid по имени
void forget_recipe(CharData *ch, char *argument, int/* subcmd*/) {
	char name[kMaxStringLength];
	int qend, rcpt = -1;
	im_rskill *rs;

	argument = one_argument(argument, arg);

	skip_spaces(&argument);
	if (!*argument) {
		SendMsgToChar("Пропущено название рецепта.\r\n", ch);
		return;
	}

	if (!IS_RECIPE_DELIM(*argument)) {
		SendMsgToChar("Рецепт надо заключить в символы : ' * или !\r\n", ch);
		return;
	}
	for (qend = 1; argument[qend] && !IS_RECIPE_DELIM(argument[qend]); qend++)
		argument[qend] = LOWER(argument[qend]);
	if (!IS_RECIPE_DELIM(argument[qend])) {
		SendMsgToChar("Рецепт должен быть заключен в символы : ' * или !\r\n", ch);
		return;
	}
	strcpy(name, (argument + 1));
	argument += qend + 1;
	name[qend - 1] = '\0';

	size_t i = strlen(name);
	for (rcpt = top_imrecipes; rcpt >= 0; --rcpt) {
		if (!strn_cmp(name, imrecipes[rcpt].name, i)) {
			break;
		}
	}

	if (rcpt < 0) {
		SendMsgToChar("Неизвестный рецепт, введите название рецепта полностью.\r\n", ch);
		return;
	}

	rs = im_get_char_rskill(ch, rcpt);
	if (!rs || !rs->perc) {
		SendMsgToChar("Изучите сначала этот рецепт.\r\n", ch);
		return;
	}
	rs->perc = 0;
	sprintf(buf, "Вы забыли рецепт отвара '%s'.\r\n", imrecipes[rcpt].name);
	SendMsgToChar(buf, ch);
}

int im_ing_dump(int *ping, char *s) {
	int pow;
	if (!ping || *ping == -1)
		return 0;
	pow = (ping[1] >> 16) & 0xFFFF;
	if (pow != 0xFFFF)
		sprintf(s, "%s,%d:%d", imtypes[ping[0]].name, pow, ping[1] & 0xFFFF);
	else
		sprintf(s, "%s:%d", imtypes[ping[0]].name, ping[1] & 0xFFFF);
	return 1;
}

void im_inglist_copy(int **pdst, int *src) {
	int i;
	*pdst = nullptr;
	if (!src)
		return;
	for (i = 0; src[i] != -1; i += 2);
	++i;
	CREATE(*pdst, i);
	memcpy(*pdst, src, i * sizeof(int));
	return;
}

// Данная функция в настоящий момент не используется,
// потому что конфиг ингредиентов вынесен из файлов мобов и комнат.
void im_inglist_save_to_disk(FILE *f, int *ping) {
	char str[128];
	for (; im_ing_dump(ping, str); ping += 2)
		fprintf(f, "I %s\r\n", str);
}

void im_extract_ing(int **pdst, int num) {
	int *p1, *p2;
	int i;
	if (!*pdst)
		return;
	p1 = p2 = *pdst;
	i = 0;
	while (*p1 != -1) {
		if (i != num) {
			p2[0] = p1[0];
			p2[1] = p1[1];
			p2 += 2;
		}
		++i;
		p1 += 2;
	}
	*p2 = *p1;
	if (**pdst == -1) {
		free(*pdst);
		*pdst = nullptr;
	}
}

void trg_recipeturn(CharData *ch, int rid, int recipediff) {
	im_rskill *rs;
	rs = im_get_char_rskill(ch, rid);
	if (rs) {
		if (recipediff)
			return;
		sprintf(buf, "Вас лишили рецепта '%s'.\r\n", imrecipes[rid].name);
		SendMsgToChar(buf, ch);
		rs->perc = 0;
	} else {
		if (!recipediff)
			return;
		if (MUD::Class(ch->GetClass()).FindIngredientRecipe(imrecipes[rid].str_id)) {
			CREATE(rs, 1);
			rs->rid = rid;
			rs->link = GET_RSKILL(ch);
			GET_RSKILL(ch) = rs;
			rs->perc = 5;
		}
		sprintf(buf, "Вы изучили рецепт '%s'.\r\n", imrecipes[rid].name);
		SendMsgToChar(buf, ch);
		sprintf(buf, "RECIPE: игроку %s добавлен рецепт %s", GET_NAME(ch), imrecipes[rid].name);
		log("%s", buf);
	}
}

void AddRecipe(CharData *ch, int rid, int recipediff) {
	im_rskill *rs;
	int skill;

	rs = im_get_char_rskill(ch, rid);
	if (!rs)
		return;

	skill = rs->perc;
	rs->perc = MAX(1, MIN(skill + recipediff, kMaxRecipeLevel));

	if (skill > rs->perc)
		sprintf(buf, "Ваше знание рецепта '%s' понизилось.\r\n", imrecipes[rid].name);
	else if (skill < rs->perc)
		sprintf(buf, "Вы повысили знание рецепта '%s'.\r\n", imrecipes[rid].name);
	else
		sprintf(buf, "Ваше знание рецепта '%s' осталось неизменным.\r\n", imrecipes[rid].name);
	SendMsgToChar(buf, ch);
}

void do_imlist(CharData *ch, char /**argument*/, int/* cmd*/, int/* subcmd*/) {
	SendMsgToChar("Команда отГлючена.\r\n", ch);
	return;
/*
	int zone, i, rnum;
	int *ping;
	char *str;

	one_argument(argument, buf);
	if (!*buf)
	{
		SendMsgToChar("Использование: исписок <номер зоны>\r\n", ch);
		return;
	}

	zone = atoi(buf);

	if ((zone < 0) || (zone > 999))
	{
		SendMsgToChar("Номер зоны должен быть между 0 и 999.\n\r", ch);
		return;
	}

	buf[0] = 0;

	for (i = 0; i < 100; ++i)
	{
		if ((rnum = GetRoomRnum((i + 100 * zone)) == kNowhere)
			continue;
		ping = world[rnum]->ing_list;
		for (str = buf1, str[0] = 0; im_ing_dump(ping, str); ping += 2)
		{
			strcat(str, " ");
			str += strlen(str);
		}
		if (buf1[0])
		{
			sprintf(buf + strlen(buf), "Комната %d [%s]\r\n%s\r\n",
					world[rnum]->number, world[rnum]->name, buf1);
		}
	}

	for (i = 0; i < 100; ++i)
	{
		if ((rnum = GetMobRnum(i + 100 * zone)) == -1)
		{
			continue;
		}

		ping = mob_proto[rnum].ing_list;
		for (str = buf1, str[0] = 0; im_ing_dump(ping, str); ping += 2)
		{
			strcat(str, " ");
			str += strlen(str);
		}

		if (buf1[0])
		{
			const auto mob = mob_proto + rnum;
			sprintf(buf + strlen(buf), "Моб %d [%s,%d]\r\n%s\r\n",
				GET_MOB_VNUM(mob),
				GET_NAME(mob),
				GetRealLevel(mob),
				buf1);
		}
	}

	if (!buf[0])
	{
		SendMsgToChar("В зоне ингредиенты не загружаются", ch);
	}
	else
	{
		page_string(ch->desc, buf, 1);
	}
	*/
}


// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
