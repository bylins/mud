/**
 * \file Contains implementation of craft model for Bylins MUD.
 * \date 2015/12/28
 * \author Anton Gorev <kvirund@gmail.com>
 */

#include "craft.hpp"

#include "object.prototypes.hpp"
#include "logger.hpp"
#include "craft.logger.hpp"
#include "craft.commands.hpp"
#include "craft.static.hpp"
#include "time_utils.hpp"
#include "xml_loading_helper.hpp"
#include "parse.hpp"
#include "skills.h"
#include "comm.h"
#include "db.h"
#include "utils.h"
#include "pugixml.hpp"

#include <boost/filesystem.hpp>
#include <boost/algorithm/string/detail/util.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

#include <iostream>
#include <string>
#include <sstream>

namespace craft {
using xml::loading::CHelper;

const char *suffix(const size_t number) {
  return 1 == number % 10
         ? "st"
         : (2 == number % 10
            ? "nd"
            : (3 == number % 10
               ? "rd"
               : "th"));
}

bool start() {
  utils::CExecutionTimer timer;

  const bool load_result = model.load();
  const auto loading_duration = timer.delta();

  if (!load_result) {
    logger("ERROR: Failed to load craft model.\n");
    return false;
  }

  logger("INFO: Craft system took %.6f seconds for loading.\n", loading_duration.count());

  return model.merge();
}

const char *BODY_PREFIX = "| ";
const char *END_PREFIX = "> ";

const std::string CCraftModel::FILE_NAME = LIB_MISC_CRAFT "index.xml";

bool Cases::load_from_node(const pugi::xml_node *node) {
  for (int c = 0; c < CASES_COUNT; ++c) {
    const std::string node_name = std::string("case") + std::to_string(1 + c);
    const pugi::xml_node case_node = node->child(node_name.c_str());
    if (!case_node) {
      logger("ERROR: Could not find case '%s'.\n", node_name.c_str());
      return false;
    }
    m_cases[c] = case_node.child_value();
  }

  const pugi::xml_node aliases_node = node->child("aliases");
  if (aliases_node) {
    for (const pugi::xml_node alias_node : aliases_node.children("alias")) {
      const char *value = alias_node.child_value();
      m_joined_aliases.append(std::string(m_aliases.empty() ? "" : " ") + value);
      m_aliases.push_back(value);
    }
  }

  return true;
}

void Cases::load_from_object(const CObjectPrototype::shared_ptr &object) {
  const std::string &aliases = object->get_aliases();
  boost::algorithm::split(m_aliases, aliases, boost::algorithm::is_any_of(" "), boost::token_compress_on);
  for (size_t n = 0; n < CASES_COUNT; ++n) {
    m_cases[n] = object->get_PName(n);
  }
}

bool Cases::save_to_node(pugi::xml_node *node) const {
  try {
    size_t number = 0;
    for (const auto &c : m_cases) {
      ++number;
      const auto case_str = std::string("case") + std::to_string(number);
      CHelper::save_string(*node, case_str.c_str(), c.c_str(),
                           [&]() { throw std::runtime_error("failed to save case value"); });
    }

    auto aliases = node->append_child("aliases");
    if (!aliases) {
      logger("WARNING: Failed to create aliases node");
      return false;
    }

    for (const auto &a : m_aliases) {
      CHelper::save_string(aliases, "alias", a.c_str(),
                           [&]() { throw std::runtime_error("failed to save alias value"); });
    }
  }
  catch (...) {
    return false;
  }

  return true;
}

OBJ_DATA::pnames_t Cases::build_pnames() const {
  OBJ_DATA::pnames_t result;
  for (size_t n = 0; n < CASES_COUNT; ++n) {
    result[n] = str_dup(m_cases[n].c_str());
  }
  return result;
}

bool CObject::load_from_node(const pugi::xml_node *node) {
  logger("Loading object with VNUM %d of type '%s'...\n", get_vnum(), kind().c_str());
  Logger::CPrefix prefix(logger, BODY_PREFIX);

  const auto description = node->child("description");
  if (description) {
    // these fields are optional for objects
    set_short_description(description.child_value("short"));
    set_description(description.child_value("long"));
    set_ex_description(description.child_value("keyword"), description.child_value("extended"));
  }

  const auto item = node->child("item");
  if (!item) {
    logger("ERROR: The object with VNUM %d does not contain required \"item\" tag.\n", get_vnum());
    return false;
  }

  if (!m_cases.load_from_node(&item)) {
    logger("ERROR: could not load item cases for the object with VNUM %d.\n", get_vnum());
    return false;
  }

  const auto cost = node->child("cost");
  int cost_value = -1;
  if (cost) {
    CHelper::load_integer(cost.child_value(), cost_value, [&]() { /* just do nothing: keep default value */});
  } else {
    logger("WARNING: Could not find \"cost\" tag for the object with VNUM %d.\n", get_vnum());
  }

  if (0 > cost_value) {
    logger("WARNING: Wrong \"cost\" value of the object with VNUM %d. Setting to the default value %d.\n",
           get_vnum(), OBJ_DATA::DEFAULT_COST);
    cost_value = OBJ_DATA::DEFAULT_COST;
  }
  set_cost(cost_value);

  const auto rent = node->child("rent");
  int rent_on_value = -1;
  int rent_off_value = -1;
  if (rent) {
    const auto rent_on = rent.child("on");
    if (!rent_on) {
      logger("WARNING: Could not find \"on\" tag for object with VNUM %d.\n", get_vnum());
    } else {
      CHelper::load_integer(rent_on.child_value(), rent_on_value,
                            [&]() {
                              logger("WARNING: Wrong value \"%s\" of the \"rent\"/\"on\" tag for object with VNUM %d.\n",
                                     rent_on.child_value(), get_vnum());
                            });
    }

    const auto rent_off = rent.child("off");
    if (!rent_off) {
      logger("WARNING: Could not find \"off\" tag for object with VNUM %d.\n", get_vnum());
    } else {
      CHelper::load_integer(rent_off.child_value(), rent_off_value,
                            [&]() {
                              logger(
                                  "WARNING: Wrong value \"%s\" of the \"rent\"/\"off\" tag for object with VNUM %d.\n",
                                  rent_off.child_value(),
                                  get_vnum());
                            });
    }
  } else {
    logger("WARNING: Could not find \"rent\" tag for the object with VNUM %d.\n", get_vnum());
  }

  if (0 > rent_on_value) {
    logger("WARNING: Wrong \"rent/on\" value of the object with VNUM %d. Setting to the default value %d.\n",
           get_vnum(), OBJ_DATA::DEFAULT_RENT_ON);
    rent_on_value = OBJ_DATA::DEFAULT_RENT_ON;
  }
  set_rent_on(rent_on_value);

  if (0 > rent_off_value) {
    logger("WARNING: Wrong \"rent/off\" value of the object with VNUM %d. Setting to the default value %d.\n",
           get_vnum(), OBJ_DATA::DEFAULT_RENT_OFF);
    rent_off_value = OBJ_DATA::DEFAULT_RENT_OFF;
  }
  set_rent_off(rent_off_value);

  const auto global_maximum = node->child("global_maximum");
  if (global_maximum) {
    int global_maximum_value = OBJ_DATA::DEFAULT_MAX_IN_WORLD;
    CHelper::load_integer(global_maximum.child_value(), global_maximum_value,
                          [&]() {
                            logger(
                                "WARNING: \"global_maximum\" value of the object with VNUM %d is not valid integer. Setting to the default value %d.\n",
                                get_vnum(),
                                global_maximum_value);
                          });

    if (0 >= global_maximum_value
        && OBJ_DATA::DEFAULT_MAX_IN_WORLD != global_maximum_value) {
      logger("WARNING: Wrong \"global_maximum\" value %d of the object with VNUM %d. Setting to the default value %d.\n",
             global_maximum_value, get_vnum(), OBJ_DATA::DEFAULT_MAX_IN_WORLD);
      global_maximum_value = OBJ_DATA::DEFAULT_MAX_IN_WORLD;
    }

    set_max_in_world(global_maximum_value);
  }

  const auto minimum_remorts = node->child("minimal_remorts");
  if (minimum_remorts) {
    int minimum_remorts_value = OBJ_DATA::DEFAULT_MINIMUM_REMORTS;
    CHelper::load_integer(minimum_remorts.child_value(), minimum_remorts_value,
                          [&]() {
                            logger(
                                "WARNING: \"minimal_remorts\" value of the object with VNUM %d is not valid integer. Setting to the default value %d.\n",
                                get_vnum(),
                                minimum_remorts_value);
                          });

    if (0 > minimum_remorts_value) {
      logger(
          "WARNING: Wrong \"minimal_remorts\" value %d of the object with VNUM %d. Setting to the default value %d.\n",
          minimum_remorts_value,
          get_vnum(),
          OBJ_DATA::DEFAULT_MINIMUM_REMORTS);
      minimum_remorts_value = OBJ_DATA::DEFAULT_MINIMUM_REMORTS;
    }
    set_minimum_remorts(minimum_remorts_value);
  }

  CHelper::ELoadFlagResult load_result = CHelper::load_flag<EObjectType>(*node, "type",
                                                                         [&](const auto type) { this->set_type(type); },
                                                                         [&](const auto name) {
                                                                           logger(
                                                                               "WARNING: Failed to set object type '%s' for object with VNUM %d. Object will be skipped.\n",
                                                                               name,
                                                                               this->get_vnum());
                                                                         },
                                                                         [&]() {
                                                                           logger(
                                                                               "WARNING: \"type\" tag not found for object with VNUM %d not found. Setting to default value: %s.\n",
                                                                               this->get_vnum(),
                                                                               NAME_BY_ITEM(get_type()).c_str());
                                                                         });
  if (CHelper::ELFR_FAIL == load_result) {
    return false;
  }

  const auto durability = node->child("durability");
  if (durability) {
    const auto maximum = durability.child("maximum");
    if (maximum) {
      CHelper::load_integer(maximum.child_value(),
                            [&](const auto value) { this->set_maximum_durability(std::max(value, 0)); },
                            [&]() {
                              logger(
                                  "WARNING: Wrong integer value of tag \"maximum_durability\" for object with VNUM %d. Leaving default value %d\n",
                                  this->get_vnum(),
                                  this->get_maximum_durability());
                            });
    }

    const auto current = durability.child("current");
    if (current) {
      CHelper::load_integer(current.child_value(),
                            [&](const auto value) {
                              this->set_current_durability(std::min(std::max(value, 0),
                                                                    this->get_maximum_durability()));
                            },
                            [&]() {
                              logger(
                                  "WARNING: Wrong integer value of tag \"current_durability\" for object with VNUM %d. Setting to value of \"maximum_durability\" %d\n",
                                  this->get_vnum(),
                                  this->get_maximum_durability());
                              this->set_current_durability(this->get_maximum_durability());
                            });
    }
  }

  load_result = CHelper::load_flag<ESex>(*node, "sex",
                                         [&](const auto sex) { this->set_sex(sex); },
                                         [&](const auto name) {
                                           logger(
                                               "WARNING: Failed to set sex '%s' for object with VNUM %d. object will be skipped.\n",
                                               name,
                                               this->get_vnum());
                                         },
                                         [&]() {
                                           logger(
                                               "WARNING: \"sex\" tag for object with VNUM %d not found. Setting to default value: %s.\n",
                                               this->get_vnum(),
                                               NAME_BY_ITEM(this->get_sex()).c_str());
                                         });
  if (CHelper::ELFR_FAIL == load_result) {
    return false;
  }

  const auto level = node->child("level");
  if (level) {
    CHelper::load_integer(level.child_value(),
                          [&](const auto value) { this->set_level(std::max(value, 0)); },
                          [&]() {
                            logger(
                                "WARNING: Wrong integer value of the \"level\" tag for object with VNUM %d. Leaving default value %d.\n",
                                this->get_vnum(),
                                this->get_level());
                          });
  }

  const auto weight = node->child("weight");
  if (weight) {
    CHelper::load_integer(weight.child_value(),
                          [&](const auto value) { this->set_weight(std::max(value, 1)); },
                          [&]() {
                            logger(
                                "WARNING: Wrong integer value of the \"weight\" tag for object with VNUM %d. Leaving default value %d.\n",
                                this->get_vnum(),
                                this->get_weight());
                          });
  }

  const auto timer = node->child("timer");
  {
    const std::string timer_value = timer.child_value();
    if ("unlimited" == timer_value) {
      set_timer(OBJ_DATA::UNLIMITED_TIMER);
    } else {
      CHelper::load_integer(weight.child_value(),
                            [&](const auto value) { this->set_timer(std::max(value, 0)); },
                            [&]() {
                              logger(
                                  "WARNING: Wrong integer value of the \"timer\" tag for object with VNUM %d. Leaving default value %d.\n",
                                  get_vnum(),
                                  this->get_timer());
                            });
    }
  }

  const auto item_parameters = node->child("item_parameters");
  if (item_parameters) {
    const bool load_result = load_item_parameters(&item_parameters);
    if (!load_result) {
      return false;
    }
  }

  load_result = CHelper::load_flag<EObjectMaterial>(*node, "material",
                                                    [&](const auto material) { this->set_material(material); },
                                                    [&](const auto name) {
                                                      logger(
                                                          "WARNING: Failed to set material '%s' for object with VNUM %d. Object will be skipped.\n",
                                                          name,
                                                          this->get_vnum());
                                                    },
                                                    [&]() {
                                                      logger(
                                                          "WARNING: \"material\" tag for object with VNUM %d not found. Setting to default value: %s.\n",
                                                          this->get_vnum(),
                                                          NAME_BY_ITEM(this->get_material()).c_str());
                                                    });
  if (CHelper::ELFR_FAIL == load_result) {
    return false;
  }

  load_result = CHelper::load_flag<ESpell>(*node, "spell",
                                           [&](const auto spell) { this->set_spell(spell); },
                                           [&](const auto value) {
                                             logger(
                                                 "WARNING: Failed to set spell '%s' for object with VNUM %d. Spell will not be set.\n",
                                                 value,
                                                 this->get_vnum());
                                           },
                                           [&]() {});

  // loading of object extraflags
  CHelper::load_flags<EExtraFlag>(*node, "extraflags", "extraflag",
                                  [&](const auto flag) { this->set_extra_flag(flag); },
                                  [&](const auto value) {
                                    logger("Setting extra flag '%s' for object with VNUM %d.\n",
                                           NAME_BY_ITEM(value).c_str(),
                                           this->get_vnum());
                                  },
                                  [&](const auto flag) {
                                    logger(
                                        "WARNING: Skipping extra flag '%s' of object with VNUM %d, because this value is not valid.\n",
                                        flag,
                                        this->get_vnum());
                                  });

  // loading of object weapon affect flags
  CHelper::load_flags<EWeaponAffectFlag>(*node, "weapon_affects", "weapon_affect",
                                         [&](const auto flag) { this->set_affect_flag(flag); },
                                         [&](const auto value) {
                                           logger("Setting weapon affect flag '%s' for object with VNUM %d.\n",
                                                  NAME_BY_ITEM(value).c_str(),
                                                  this->get_vnum());
                                         },
                                         [&](const auto flag) {
                                           logger(
                                               "WARNING: Skipping weapon affect flag '%s' of object with VNUM %d, because this value is not valid.\n",
                                               flag,
                                               this->get_vnum());
                                         });

  // loading of object antiflags
  CHelper::load_flags<EAntiFlag>(*node, "antiflags", "antiflag",
                                 [&](const auto flag) { this->set_anti_flag(flag); },
                                 [&](const auto value) {
                                   logger("Setting antiflag '%s' for object with VNUM %d.\n",
                                          NAME_BY_ITEM(value).c_str(),
                                          this->get_vnum());
                                 },
                                 [&](const auto flag) {
                                   logger(
                                       "WARNING: Skipping antiflag '%s' of object with VNUM %d, because this value is not valid.\n",
                                       flag,
                                       this->get_vnum());
                                 });

  // loading of object noflags
  CHelper::load_flags<ENoFlag>(*node, "noflags", "noflag",
                               [&](const auto flag) { this->set_no_flag(flag); },
                               [&](const auto value) {
                                 logger("Setting noflag '%s' for object with VNUM %d.\n",
                                        NAME_BY_ITEM(value).c_str(),
                                        this->get_vnum());
                               },
                               [&](const auto flag) {
                                 logger(
                                     "WARNING: Skipping noflag '%s' of object with VNUM %d, because this value is not valid.\n",
                                     flag,
                                     this->get_vnum());
                               });

  // loading of object wearflags
  CHelper::load_flags<EWearFlag>(*node, "wearflags", "wearflag",
                                 [&](const auto flag) { this->set_wear_flag(flag); },
                                 [&](const auto value) {
                                   logger("Setting wearflag '%s' for object with VNUM %d.\n",
                                          NAME_BY_ITEM(value).c_str(),
                                          this->get_vnum());
                                 },
                                 [&](const auto flag) {
                                   logger(
                                       "WARNING: Skipping wearflag '%s' of object with VNUM %d, because this value is not valid.\n",
                                       flag,
                                       this->get_vnum());
                                 });

  // loading of object skills
  load_skills(node);

  // load object vals
  for (size_t i = 0; i < VALS_COUNT; ++i) {
    std::stringstream val_name;
    val_name << "val" << i;

    const auto val_node = node->child(val_name.str().c_str());
    if (val_node) {
      CHelper::load_integer(val_node.child_value(),
                            [&](const auto value) { this->set_val(i, value); },
                            [&]() {
                              logger(
                                  "WARNING: \"%s\" tag of object with VNUM %d has wrong integer value. Leaving default value %d.\n",
                                  val_name.str().c_str(),
                                  this->get_vnum(),
                                  this->get_val(i));
                            });
    }
  }

  const auto triggers_node = node->child("triggers");
  for (const auto trigger_node : triggers_node.children("trigger")) {
    const char *vnum_str = trigger_node.child_value();
    CHelper::load_integer(vnum_str,
                          [&](const auto value) { this->add_proto_script(value); },
                          [&]() {
                            logger("WARNING: Invalid trigger's VNUM value \"%s\" for object with VNUM %d. Skipping.\n",
                                   vnum_str, this->get_vnum());
                          });
  }

  load_extended_values(node);
  load_applies(node);

  if (!check_object_consistency()) {
    logger("WARNING: Object with VNUM %d has not passed consistency check.\n", get_vnum());
    return false;
  }

  prefix.change_prefix(END_PREFIX);
  logger("End of loading object with VNUM %d.\n", get_vnum());

  return true;
}

void CObject::load_from_object(const CObjectPrototype::shared_ptr &object) {
  *this = CObject(*object);
}

bool CObject::save_to_node(pugi::xml_node *node) const {
  try {
    auto description = node->append_child("description");
    if (!description) {
      logger("WARNIGN: Failed to create node \"description\".\n");
      return false;
    }

    CHelper::save_string(description, "short", get_short_description().c_str(),
                         [&]() { throw std::runtime_error("WARNING: Failed to save short description"); });
    CHelper::save_string(description, "long", get_description().c_str(),
                         [&]() { throw std::runtime_error("WARNING: Failed to save long description"); });
    CHelper::save_string(description, "action", get_action_description().c_str(),
                         [&]() { throw std::runtime_error("WARNING: Failed to save action description"); });
    if (get_ex_description()) {
      CHelper::save_string(description, "keyword", get_ex_description()->keyword,
                           [&]() { throw std::runtime_error("WARNING: Failed to save keyword"); });
      CHelper::save_string(description, "extended", get_ex_description()->description,
                           [&]() { throw std::runtime_error("WARNING: Failed to save extended description"); });
    }

    auto item = node->append_child("item");
    if (!item) {
      logger("WARNIGN: Failed to create node\"item\".\n");
      return false;
    }

    if (!m_cases.save_to_node(&item)) {
      // Error message was output inside m_cases.save_to_node
      return false;
    }

    CHelper::save_string(*node, "cost", std::to_string(get_cost()).c_str(),
                         [&]() { throw std::runtime_error("WARNING: Failed to save object cost"); });

    auto rent = node->append_child("rent");
    if (!rent) {
      logger("WARNIGN: Failed to create node \"rent\".\n");
      return false;
    }

    CHelper::save_string(rent, "on", std::to_string(get_rent_on()).c_str(),
                         [&]() { throw std::runtime_error("WARNING: Failed to save rent/on value"); });
    CHelper::save_string(rent, "off", std::to_string(get_rent_off()).c_str(),
                         [&]() { throw std::runtime_error("WARNING: Failed to save rent/off value"); });

    CHelper::save_string(*node, "global_maximum", std::to_string(get_max_in_world()).c_str(),
                         [&]() { throw std::runtime_error("WARNING: Failed to save global maximum"); });
    CHelper::save_string(*node, "minimum_remorts", std::to_string(get_minimum_remorts()).c_str(),
                         [&]() { throw std::runtime_error("WARNING: Failed to save minimal remorts"); });

    CHelper::save_string(*node, "type", NAME_BY_ITEM(get_type()).c_str(),
                         [&]() { throw std::runtime_error("WARNING: Failed to save object type"); });

    auto durability = node->append_child("durability");
    if (!durability) {
      logger("WARNIGN: Failed to create node \"durability\".\n");
      return false;
    }

    CHelper::save_string(durability, "maximum", std::to_string(get_maximum_durability()).c_str(),
                         [&]() { throw std::runtime_error("WARNING: Failed to save maximum durability"); });
    CHelper::save_string(durability, "current", std::to_string(get_current_durability()).c_str(),
                         [&]() { throw std::runtime_error("WARNING: Failed to save current durability"); });

    CHelper::save_string(*node, "sex", NAME_BY_ITEM(get_sex()).c_str(),
                         [&]() { throw std::runtime_error("WARNING: Failed to save gender"); });
    CHelper::save_string(*node, "level", std::to_string(get_level()).c_str(),
                         [&]() { throw std::runtime_error("WARNING: Failed to save object level"); });
    CHelper::save_string(*node, "weight", std::to_string(get_weight()).c_str(),
                         [&]() { throw std::runtime_error("WARNING: Failed to save object weight"); });

    if (OBJ_DATA::UNLIMITED_TIMER != get_timer()) {
      CHelper::save_string(*node, "timer", std::to_string(get_timer()).c_str(),
                           [&]() { throw std::runtime_error("WARNING: Failed to save object timer"); });
    } else {
      CHelper::save_string(*node, "timer", "unlimited",
                           [&]() { throw std::runtime_error("WARNING: Failed to save object timer"); });
    }

    {
      // unpack item_parameters
      std::list<std::string> item_parameters;
      switch (get_type()) {
        case OBJ_DATA::ITEM_INGREDIENT: {
          int flag = 1;
          while (flag <= get_skill()) {
            if (IS_SET(get_skill(), flag)) {
              item_parameters.push_back(NAME_BY_ITEM(static_cast<EIngredientFlag>(flag)));
            }
            flag <<= 1;
          }
        }
          break;

        case OBJ_DATA::ITEM_WEAPON: item_parameters.push_back(NAME_BY_ITEM(static_cast<ESkill>(get_skill())));
          break;

        default: break;
      }
      // and save them
      CHelper::save_list(*node, "item_parameters", "parameter", item_parameters,
                         [&](const auto &value) -> auto { return value.c_str(); },
                         [&]() { throw std::runtime_error("WARNING: Failed to create node \"item_parameters\".\n"); },
                         [&](const auto value) {
                           throw std::runtime_error("WARNING: Could not save item parameter value " + value);
                         });
    }

    CHelper::save_string(*node, "material", NAME_BY_ITEM(get_material()).c_str(),
                         [&]() { throw std::runtime_error("WARNING: Failed to save object material"); });
    CHelper::save_string(*node, "spell", NAME_BY_ITEM(get_spell()).c_str(),
                         [&]() { throw std::runtime_error("WARNING: Failed to save object spell"); });

    CHelper::save_list<EExtraFlag>(*node, "extraflags", "extraflag", get_extra_flags(),
                                   [&]() { throw std::runtime_error("WARNING: Failed to create node \"extraflags\".\n"); },
                                   [&](const auto value) {
                                     throw std::runtime_error(
                                         "WARNING: Could not save extraflag " + NAME_BY_ITEM(value));
                                   });

    CHelper::save_list<EWeaponAffectFlag>(*node, "weapon_affects", "weapon_affect", get_affect_flags(),
                                          [&]() {
                                            throw std::runtime_error(
                                                "WARNING: Failed to create node \"weapon_affects\".\n");
                                          },
                                          [&](const auto value) {
                                            throw std::runtime_error(
                                                "WARNING: Could not save weapon affect " + NAME_BY_ITEM(value));
                                          });

    CHelper::save_list<EAntiFlag>(*node, "antiflags", "antiflag", get_anti_flags(),
                                  [&]() { throw std::runtime_error("WARNING: Failed to create node \"antiflags\".\n"); },
                                  [&](const auto value) {
                                    throw std::runtime_error("WARNING: Could not save antiflag " + NAME_BY_ITEM(value));
                                  });

    CHelper::save_list<ENoFlag>(*node, "noflags", "noflag", get_no_flags(),
                                [&]() { throw std::runtime_error("WARNING: Failed to create node \"noflags\".\n"); },
                                [&](const auto value) {
                                  logger("%s",
                                         ("WARNING: Could not save noflag "
                                             + std::to_string(static_cast<unsigned int>(value))
                                             + ". Will be skipped.\n").c_str());
                                });

    CHelper::save_list<EWearFlag>(*node, "wearflags", "wearflag", get_wear_flags(),
                                  [&]() { throw std::runtime_error("WARNING: Failed to create node \"wearflags\".\n"); },
                                  [&](const auto value) {
                                    throw std::runtime_error(
                                        "WARNING: Could not save wear flag " + NAME_BY_ITEM(value));
                                  });

    CHelper::save_pairs_list(*node, "skills", "skill", "id", "value", get_skills(),
                             [&](const auto &value) -> auto { return NAME_BY_ITEM(value.first); },
                             [&](const auto &value) -> auto { return std::to_string(value.second); },
                             [&]() { throw std::runtime_error("WARNING: Could not save skills"); });

    CHelper::save_pairs_list(*node, "applies", "apply", "location", "modifier", get_all_affected(),
                             [&](const auto &value) -> auto {
                               return APPLY_NONE != value.location ? NAME_BY_ITEM(value.location) : "";
                             },
                             [&](const auto &value) -> auto { return std::to_string(value.modifier); },
                             [&]() { throw std::runtime_error("WARNING: Could not save applies"); });

    for (size_t i = 0; i < VALS_COUNT; ++i) {
      const auto node_name = "val" + std::to_string(i);
      CHelper::save_string(*node, node_name.c_str(), std::to_string(get_val(i)).c_str(),
                           [&]() { throw std::runtime_error("WARNING: Failed to save " + node_name + " value"); });
    }

    CHelper::save_pairs_list(*node, "extended_values", "entry", "key", "value", get_all_values(),
                             [&](const auto &value) -> auto {
                               return TextId::to_str(TextId::OBJ_VALS,
                                                     static_cast<int>(value.first));
                             },
                             [&](const auto &value) -> auto { return std::to_string(value.second); },
                             [&]() { throw std::runtime_error("WARNING: Could not save extended values"); });
  }
  catch (const std::runtime_error &error) {
    logger("%s\n", error.what());
    return false;
  }
  catch (...) {
    return false;
  }

  return true;
}

OBJ_DATA *CObject::build_object() const {
  return new OBJ_DATA(*this);
}

bool CObject::load_item_parameters(const pugi::xml_node *node) {
  switch (get_type()) {
    case OBJ_DATA::ITEM_INGREDIENT:
      for (const auto flags : node->children("parameter")) {
        const char *flag = flags.child_value();
        try {
          const auto value = ITEM_BY_NAME<EIngredientFlag>(flag);
          set_skill(get_skill() | to_underlying(value));
          logger("Setting ingredient flag '%s' for object with VNUM %d.\n",
                 NAME_BY_ITEM(value).c_str(), get_vnum());
        }
        catch (const std::out_of_range &) {
          logger("WARNING: Skipping ingredient flag '%s' of object with VNUM %d, because this value is not valid.\n",
                 flag, get_vnum());
        }
      }
      break;

    case OBJ_DATA::ITEM_WEAPON: {
      const char *skill_value = node->child_value("parameter");
      try {
        set_skill(to_underlying(ITEM_BY_NAME<ESkill>(skill_value)));
      }
      catch (const std::out_of_range &) {
        logger("WARNING: Failed to set skill value '%s' for object with VNUM %d. Object will be skipped.\n",
               skill_value, get_vnum());
        return false;
      }
      break;
    }

    default:
      // For other item types "skills" tag should be ignored.
      break;
  }

  return true;
}

void CObject::load_skills(const pugi::xml_node *node) {
  CHelper::load_pairs_list<ESkill>(node, "skills", "skill", "id", "value",
                                   [&](const size_t number) {
                                     logger(
                                         "WARNING: %zd-%s \"skill\" tag of \"skills\" group does not have the \"id\" tag. Object with VNUM %d.\n",
                                         number,
                                         suffix(number),
                                         this->get_vnum());
                                   },
                                   [&](const auto value) -> auto { return ITEM_BY_NAME<ESkill>(value); },
                                   [&](const auto key) {
                                     logger(
                                         "WARNING: Could not convert value \"%s\" to skill ID. Object with VNUM %d.\n Skipping entry.\n",
                                         key,
                                         this->get_vnum());
                                   },
                                   [&](const auto key) {
                                     logger(
                                         "WARNING: skill with key \"%s\" does not have \"value\" tag. Object with VNUM %d. Skipping entry.\n",
                                         key,
                                         this->get_vnum());
                                   },
                                   [&](const auto key, const auto value) {
                                     CHelper::load_integer(value,
                                                           [&](const auto int_value) {
                                                             this->set_skill(key, int_value);
                                                             logger(
                                                                 "Adding skill pair (%s, %d) to object with VNUM %d.\n",
                                                                 NAME_BY_ITEM(key).c_str(),
                                                                 int_value,
                                                                 this->get_vnum());
                                                           },
                                                           [&]() {
                                                             logger(
                                                                 "WARNIGN: Could not convert skill value of \"value\" tag to integer. Entry key value \"%s\". Object with VNUM %d",
                                                                 NAME_BY_ITEM(key).c_str(),
                                                                 this->get_vnum());
                                                           });
                                   });
}

void CObject::load_extended_values(const pugi::xml_node *node) {
  CHelper::load_pairs_list<ObjVal::EValueKey>(node, "extended_values", "entry", "key", "value",
                                              [&](const size_t number) {
                                                logger(
                                                    "WARNING: %zd-%s \"entry\" tag of \"extended_values\" group does not have the \"key\" tag. Object with VNUM %d.\n",
                                                    number,
                                                    suffix(number),
                                                    this->get_vnum());
                                              },
                                              [&](const auto value) -> auto {
                                                return static_cast<ObjVal::EValueKey>(TextId::to_num(TextId::OBJ_VALS,
                                                                                                     value));
                                              },
                                              [&](const auto key) {
                                                logger(
                                                    "WARNING: Could not convert extended value \"%s\" to key value. Object with VNUM %d.\n Skipping entry.\n",
                                                    key,
                                                    this->get_vnum());
                                              },
                                              [&](const auto key) {
                                                logger(
                                                    "WARNING: entry with key \"%s\" does not have \"value\" tag. Object with VNUM %d. Skipping entry.\n",
                                                    key,
                                                    this->get_vnum());
                                              },
                                              [&](const auto key, const auto value) {
                                                CHelper::load_integer(value,
                                                                      [&](const auto int_value) {
                                                                        this->set_value(key, int_value);
                                                                        logger(
                                                                            "Adding extended values pair (%s, %d) to object with VNUM %d.\n",
                                                                            TextId::to_str(TextId::OBJ_VALS,
                                                                                           to_underlying(key)).c_str(),
                                                                            int_value,
                                                                            this->get_vnum());
                                                                      },
                                                                      [&]() {
                                                                        logger(
                                                                            "WARNIGN: Could not convert extended value of \"value\" tag to integer. Entry key value \"%s\". Object with VNUM %d",
                                                                            TextId::to_str(TextId::OBJ_VALS,
                                                                                           to_underlying(key)).c_str(),
                                                                            this->get_vnum());
                                                                      });
                                              });
}

void CObject::load_applies(const pugi::xml_node *node) {
  using applies_t = std::list<obj_affected_type>;
  applies_t applies;
  CHelper::load_pairs_list<EApplyLocation>(node, "applies", "apply", "location", "modifier",
                                           [&](const size_t number) {
                                             logger(
                                                 "WARNING: %zd-%s \"apply\" tag of \"applies\" group does not have the \"location\" tag. Object with VNUM %d.\n",
                                                 number,
                                                 suffix(number),
                                                 get_vnum());
                                           },
                                           [&](const auto value) -> auto { return ITEM_BY_NAME<EApplyLocation>(value); },
                                           [&](const auto key) {
                                             logger(
                                                 "WARNING: Could not convert value \"%s\" to apply location. Object with VNUM %d.\n Skipping entry.\n",
                                                 key,
                                                 this->get_vnum());
                                           },
                                           [&](const auto key) {
                                             logger(
                                                 "WARNING: apply with key \"%s\" does not have \"modifier\" tag. Object with VNUM %d. Skipping entry.\n",
                                                 key,
                                                 this->get_vnum());
                                           },
                                           [&](const auto key, const auto value) {
                                             CHelper::load_integer(value,
                                                                   [&](const auto int_value) {
                                                                     applies.push_back(applies_t::value_type(key,
                                                                                                             int_value));
                                                                     logger(
                                                                         "Adding apply pair (%s, %d) to object with VNUM %d.\n",
                                                                         NAME_BY_ITEM(key).c_str(),
                                                                         int_value,
                                                                         this->get_vnum());
                                                                   },
                                                                   [&]() {
                                                                     logger(
                                                                         "WARNIGN: Could not convert apply value of \"modifier\" tag to integer. Entry key value \"%s\". Object with VNUM %d",
                                                                         NAME_BY_ITEM(key).c_str(),
                                                                         this->get_vnum());
                                                                   });
                                           });

  std::stringstream ignored_applies;
  bool first = true;
  size_t i = 0;
  for (const auto &apply : applies) {
    if (i < MAX_OBJ_AFFECT) {
      set_affected(i, apply);
    } else {
      const auto &apply = applies.back();
      ignored_applies << (first ? "" : ", ") << NAME_BY_ITEM(apply.location);
      applies.pop_back();
      first = false;
    }
  }

  if (!ignored_applies.str().empty()) {
    logger("WARNING: Object with VNUM %d has applies over the limit of %d. The following applies is ignored: { %s }.\n",
           get_vnum(), MAX_OBJ_AFFECT, ignored_applies.str().c_str());
  }
}

bool CObject::check_object_consistency() const {
  // perform some checks here.

  return true;
}

bool CMaterialClass::load(const pugi::xml_node *node) {
  logger("Loading material class with ID '%s'...\n", m_id.c_str());
  Logger::CPrefix prefix(logger, BODY_PREFIX);

  const auto desc_node = node->child("description");
  if (!desc_node) {
    logger("ERROR: material class with ID '%s' does not contain required \"description\" tag.\n",
           m_id.c_str());
    return false;
  }

  const auto short_desc = desc_node.child("short");
  if (!short_desc) {
    logger("ERROR: material class with ID '%s' does not contain required \"description/short\" tag.\n",
           m_id.c_str());
    return false;
  }
  m_short_desc = short_desc.child_value();

  const auto long_desc = desc_node.child("long");
  if (!long_desc) {
    logger("ERROR: material class with ID '%s' does not contain required \"description/long\" tag.\n",
           m_id.c_str());
    return false;
  }
  m_long_desc = long_desc.child_value();

  const auto item = node->child("item");
  if (!item) {
    logger("ERROR: material class with ID '%s' does not contain required \"item\" tag.\n", m_id.c_str());
    return false;
  }
  if (!m_item_cases.load_from_node(&item)) {
    logger("ERROR: could not load item cases for material class '%s'.\n", m_id.c_str());
    return false;
  }

  const auto adjectives = node->child("adjectives");
  if (adjectives) {
    const bool load_adjectives_result = load_adjectives(&adjectives);
    if (!load_adjectives_result) {
      logger("ERROR: Failed to load adjectives for material class %s.\n", m_id.c_str());
    }
  }

  prefix.change_prefix(END_PREFIX);
  logger("End of loading material class with ID '%s'.\n", m_id.c_str());

  return true;
}

bool CMaterialClass::load_adjectives(const pugi::xml_node *node) {
  const auto male = node->child("male");
  if (male) {
    m_male_adjectives.reset(new Cases());
    if (!m_male_adjectives->load_from_node(&male)) {
      logger("ERROR: could not load male adjective cases for material class '%s'.\n", m_id.c_str());
      return false;
    }
  }

  const auto female = node->child("female");
  if (female) {
    m_female_adjectives.reset(new Cases());
    if (!m_female_adjectives->load_from_node(&female)) {
      logger("ERROR: Could not load female adjective cases for material class '%s'.\n", m_id.c_str());
      return false;
    }
  }

  const auto neuter = node->child("neuter");
  if (neuter) {
    m_neuter_adjectives.reset(new Cases);
    if (!m_neuter_adjectives->load_from_node(&neuter)) {
      logger("ERROR: Could not load neuter adjective cases for material class '%s'.\n", m_id.c_str());
      return false;
    }
  }

  return true;
}

bool CMaterial::load(const pugi::xml_node *node) {
  logger("Loading material with ID %s...\n", m_id.c_str());
  Logger::CPrefix prefix(logger, BODY_PREFIX);

  // load material name
  const auto node_name = node->child("name");
  if (!node_name) {
    logger("ERROR: could not find required node 'name' for material with ID '%s'.\n", m_id.c_str());
    return false;
  }
  const std::string name = node_name.child_value();
  set_name(name);

  // load material classes
  for (const auto node_class: node->children("class")) {
    if (node_class.attribute("id").empty()) {
      logger("WARNING: class tag of material with ID '%s' does not contain ID of class. Class will be skipped.\n",
             m_id.c_str());
      continue;
    }
    const std::string class_id = node_class.attribute("id").value();
    CMaterialClass mc(class_id);
    if (!mc.load(&node_class)) {
      logger("WARNING: class with ID '%s' has not been loaded. Class will be skipped.\n", class_id.c_str());
    }
    m_classes.push_back(mc);
  }

  prefix.change_prefix(END_PREFIX);
  logger("End of loading material with ID '%s'.\n", m_id.c_str());

  return true;
}

bool CRecipe::load(const pugi::xml_node * /*node*/) {
  logger("Loading recipe with ID %s...\n", m_id.c_str());
  Logger::CPrefix prefix(logger, BODY_PREFIX);

  prefix.change_prefix(END_PREFIX);
  logger("End of loading recipe with ID %s\n", m_id.c_str());

  return true;
}

bool CSkillBase::load(const pugi::xml_node * /*node*/) {
  logger("Loading skill with ID %s...\n", m_id.c_str());
  Logger::CPrefix prefix(logger, BODY_PREFIX);

  prefix.change_prefix(END_PREFIX);
  logger("End of loading skill with ID %s\n", m_id.c_str());

  return true;
}

bool CCraft::load(const pugi::xml_node * /*node*/) {
  logger("Loading craft with ID %s...\n", m_id.c_str());
  Logger::CPrefix prefix(logger, BODY_PREFIX);

  prefix.change_prefix(END_PREFIX);
  logger("End of loading craft with ID %s\n", m_id.c_str());

  return true;
}

bool CCraftModel::load() {
  logger("Loading craft model from file '%s'...\n",
         FILE_NAME.c_str());
  Logger::CPrefix prefix(logger, BODY_PREFIX);

  pugi::xml_document doc;
  const auto result = doc.load_file(FILE_NAME.c_str());

  if (!result) {
    logger("Craft load error: '%s' at offset %zu\n",
           result.description(),
           result.offset);
    return false;
  }

  const auto model = doc.child("craftmodel");
  if (!model) {
    logger("Craft model is not defined in XML file %s\n", FILE_NAME.c_str());
    return false;
  }

  // Load model properties.
  const auto base_count_node = model.child("base_crafts");
  if (base_count_node) {
    CHelper::load_integer(base_count_node.child_value(), m_base_count,
                          [&]() {
                            logger("WARNING: \"base_crafts\" tag has wrong integer value. Leaving default value %d.\n",
                                   m_base_count);
                          });
  }

  const auto remort_for_count_bonus_node = model.child("crafts_bonus");
  if (remort_for_count_bonus_node) {
    CHelper::load_integer(remort_for_count_bonus_node.child_value(), m_remort_for_count_bonus,
                          [&]() {
                            logger("WARNING: \"crafts_bonus\" tag has wrong integer value. Leaving default value %d.\n",
                                   m_remort_for_count_bonus);
                          });
  }

  const auto base_top_node = model.child("skills_cap");
  if (base_top_node) {
    CHelper::load_integer(base_top_node.child_value(), m_base_top,
                          [&]() {
                            logger("WARNING: \"skills_cap\" tag has wrong integer value. Leaving default value %d.\n",
                                   m_base_top);
                          });
  }

  const auto remorts_bonus_node = model.child("skills_bonus");
  if (remorts_bonus_node) {
    CHelper::load_integer(remorts_bonus_node.child_value(), m_remorts_bonus,
                          [&]() {
                            logger("WARNING: \"skills_bonus\" tag has wrong integer value. Leaving default value %d.\n",
                                   m_remorts_bonus);
                          });
  }

  // Load VNUM ranges allocated for craft system
  if (!load_vnum_ranges(&model)) {
    return false;
  }

  // TODO: load remaining properties.

  load_from_node(&model);
  for (const auto include : model.children("include")) {
    const auto filename_attribute = include.attribute("filename");
    if (!filename_attribute.empty()) {
      using boost::filesystem::path;
      const std::string filename = (path(FILE_NAME).parent_path() / filename_attribute.value()).generic_string();
      pugi::xml_document idoc;
      const auto iresult = idoc.load_file(filename.c_str());
      if (!iresult) {
        logger("WARNING: could not load include file '%s' with model data: '%s' "
               "at offset %zu. Model data from this file will be skipped.\n",
               filename.c_str(),
               iresult.description(),
               iresult.offset);
        continue;
      }

      const auto model_data = idoc.child("model_data");
      if (!model_data) {
        logger("WARNING: External file '%s' does not contain \"model_data\" tag. This file will be skipped.\n",
               filename.c_str());
        continue;
      }

      logger("Begin loading included file %s.\n", filename.c_str());
      Logger::CPrefix include_prefix(logger, BODY_PREFIX);
      load_from_node(&model_data);
      include_prefix.change_prefix(END_PREFIX);
      logger("End of loading included file.\n");
    }
  }

  prefix.change_prefix(END_PREFIX);
  logger("End of loading craft model.\n");
  // TODO: print statistics of the model (i. e. count of materials, recipes, crafts, missed entries and so on).

  return true;
}

bool CCraftModel::merge() {
  for (const auto &p : m_prototypes) {
    obj_proto.add(p, p->get_vnum());
  }

  return true;
}

void CCraftModel::create_item() const {
}

bool CCraftModel::load_prototype(const pugi::xml_node *prototype, const size_t number) {
  if (prototype->attribute("vnum").empty()) {
    logger("%zd-%s prototype tag does not have VNUM attribute. Will be skipped.\n",
           number, suffix(number));
    return false;
  }

  obj_vnum vnum = prototype->attribute("vnum").as_int(0);
  if (0 == vnum) {
    logger("Failed to get VNUM of the %zd-%s prototype. This prototype entry will be skipped.\n",
           number, suffix(number));
  }

  // Check VNum before loading: we shouldn't tell about loading errors if we won't add this prototype in any case.
  const auto check_vnum_result = check_vnum(vnum);
  if (EAVNR_OK != check_vnum_result) {
    report_vnum_error(vnum, check_vnum_result);
    return false;
  }

  CObject *p = new CObject(vnum);
  CObjectPrototype::shared_ptr prototype_object(p);
  if (prototype->attribute("filename").empty()) {
    if (!p->load_from_node(prototype)) {
      logger("WARNING: Skipping %zd-%s prototype with VNUM %d.\n",
             number, suffix(number), vnum);
      return false;
    }
  } else {
    using boost::filesystem::path;
    const std::string
        filename = (path(FILE_NAME).parent_path() / prototype->attribute("filename").value()).generic_string();
    pugi::xml_document pdoc;
    const auto presult = pdoc.load_file(filename.c_str());
    if (!presult) {
      logger("WARNING: could not load external file '%s' with %zd-%s prototype (ID: %d): '%s' "
             "at offset %zu. Prototype will be skipped.\n",
             filename.c_str(),
             number,
             suffix(number),
             vnum,
             presult.description(),
             presult.offset);
      return false;
    }
    const auto proot = pdoc.child("prototype");
    if (!proot) {
      logger("WARNING: could not find root \"prototype\" tag for prototype with VNUM "
             "%d in the external file '%s'. Prototype will be skipped.\n",
             vnum,
             filename.c_str());
      return false;
    }
    logger("Using external file '%s' for %zd-%s prototype with VNUM %d.\n",
           filename.c_str(),
           number,
           suffix(number),
           vnum);
    if (!p->load_from_node(&proot)) {
      logger("WARNING: Skipping %zd-%s prototype with VNUM %d.\n",
             number, suffix(number), vnum);
      return false;
    }
  }

  const auto add_vnum_result = add_vnum(p->get_vnum());
  if (EAVNR_OK == add_vnum_result) {
    m_prototypes.push_back(prototype_object);
  } else {
    report_vnum_error(p->get_vnum(), add_vnum_result);
    return false;
  }

  return true;
}

bool CCraftModel::load_material(const pugi::xml_node *material, const size_t number) {
  if (material->attribute("id").empty()) {
    logger("%zd-%s material tag does not have ID attribute. Will be skipped.\n",
           number, suffix(number));
    return false;
  }

  id_t id = material->attribute("id").as_string();
  const auto m = std::make_shared<CMaterial>(id);
  if (material->attribute("filename").empty()) {
    if (!m->load(material)) {
      logger("WARNING: Skipping material with ID '%s'.\n", id.c_str());
      return false;
    }
  } else {
    using boost::filesystem::path;
    const std::string
        filename = (path(FILE_NAME).parent_path() / material->attribute("filename").value()).generic_string();
    pugi::xml_document mdoc;
    const auto mresult = mdoc.load_file(filename.c_str());

    if (!mresult) {
      logger("WARNING: could not load external file '%s' with material '%s': '%s' "
             "at offset %zu. Material will be skipped.\n",
             filename.c_str(),
             id.c_str(),
             mresult.description(),
             mresult.offset);
      return false;
    }

    const auto mroot = mdoc.child("material");
    if (!mroot) {
      logger("WARNING: could not find root \"material\" tag for material with ID "
             "'%s' in the external file '%s'. Material will be skipped.\n",
             id.c_str(),
             filename.c_str());
      return false;
    }

    logger("Using external file '%s' for material with ID '%s'.\n",
           filename.c_str(),
           id.c_str());

    if (!m->load(&mroot)) {
      logger("WARNING: Skipping material with ID '%s'.\n",
             id.c_str());
      return false;
    }
  }
  m_materials.push_back(m);

  return true;
}

bool CCraftModel::load_recipe(const pugi::xml_node *recipe, const size_t number) {
  if (recipe->attribute("id").empty()) {
    logger("%zd-%s recipe tag does not have ID attribute. Will be skipped.\n",
           number, suffix(number));
    return false;
  }

  id_t id = recipe->attribute("id").as_string();
  const auto r = std::make_shared<CRecipe>(id);
  if (recipe->attribute("filename").empty()) {
    if (!r->load(recipe)) {
      logger("WARNING: Skipping recipe with ID '%s'.\n", id.c_str());
      return false;
    }
  } else {
    using boost::filesystem::path;
    const std::string
        filename = (path(FILE_NAME).parent_path() / recipe->attribute("filename").value()).generic_string();
    pugi::xml_document rdoc;
    const auto rresult = rdoc.load_file(filename.c_str());

    if (!rresult) {
      logger("WARNING: could not load external file '%s' with recipe '%s': '%s' "
             "at offset %zu. Recipe will be skipped.\n",
             filename.c_str(),
             id.c_str(),
             rresult.description(),
             rresult.offset);
      return false;
    }

    const auto rroot = rdoc.child("recipe");
    if (!rroot) {
      logger("WARNING: could not find root \"recipe\" tag for recipe with ID "
             "'%s' in the external file '%s'. Recipe will be skipped.\n",
             id.c_str(),
             filename.c_str());
      return false;
    }

    logger("Using external file '%s' for recipe with ID '%s'.\n",
           filename.c_str(),
           id.c_str());

    if (!r->load(&rroot)) {
      logger("WARNING: Skipping recipe with ID '%s'.\n",
             id.c_str());
      return false;
    }
  }

  m_recipes.push_back(r);

  return true;
}

bool CCraftModel::load_from_node(const pugi::xml_node *model) {
  // Load materials.
  const auto materials = model->child("materials");
  if (materials) {
    size_t number = 0;
    for (const auto material : materials.children("material")) {
      load_material(&material, ++number);
    }
  }

  // Load recipes.
  const auto recipes = model->child("recipes");
  if (recipes) {
    size_t number = 0;
    for (const auto recipe : recipes.children("recipe")) {
      load_recipe(&recipe, ++number);
    }
  }

  // Load skills.
  // TODO: load it.

  // Load crafts.
  // TODO: load it.

  // Load prototypes
  const auto prototypes = model->child("prototypes");
  if (prototypes) {
    size_t number = 0;
    for (const auto prototype : prototypes.children("prototype")) {
      load_prototype(&prototype, ++number);
    }
  }

  return true;
}

bool CCraftModel::load_vnum_ranges(const pugi::xml_node *node) {
  const auto vnums = node->child("vnums");
  if (!vnums) {
    logger("ERROR: Required attribute \"vnums\" for craft system is missing in the configuration file.\n");
    return false;
  }

  size_t number = 0;
  bool vnums_ok = false;
  for (const auto range : vnums.children("range")) {
    ++number;
    try {
      const auto min_node = range.child("min");
      if (!min_node) {
        logger("WARNING: Required attribute \"min\" for VNum range is missing. Range will be ignored.\n");
        continue;
      }
      obj_vnum min = 0;
      CHelper::load_integer(min_node.child_value(), min,
                            [&]() {
                              logger(
                                  "WARNING: \"min\" tag of %zd-%s range has wrong integer value. Range will be ignored.\n",
                                  number,
                                  suffix(number));
                              throw std::runtime_error("tag \"min\" has wrong integer value");
                            });

      const auto max_node = range.child("max");
      if (!max_node) {
        logger("WARNING: Required attribute \"max\" for VNum range is missing. Range will be ignored.\n");
        continue;
      }
      obj_vnum max = 0;
      CHelper::load_integer(max_node.child_value(), max,
                            [&]() {
                              logger(
                                  "WARNING: \"max\" tag of %zd-%s range has wrong integer value. Range will be ignored.\n",
                                  number,
                                  suffix(number));
                              throw std::runtime_error("tag \"max\" has wrong integer value");
                            });

      if (min > max) {
        logger("WARNING: Minimal value of %zd-%s range is above than maximal value. Values will be swapped.\n",
               number, suffix(number));
        std::swap(min, max);
      }

      CVNumRange r(min, max);
      m_allowed_vnums.insert(r);
      vnums_ok = true;
    }
    catch (...) {
      continue;
    }
  }

  if (!vnums_ok) {
    logger("ERROR: No any VNum ranges has been allowed for craft system.\n");
    return false;
  }

  return true;
}

craft::CCraftModel::EAddVNumResult CCraftModel::check_vnum(const obj_vnum vnum) const {
  const bool exists = m_existing_vnums.find(vnum) != m_existing_vnums.end();
  if (exists) {
    return EAVNR_ALREADY_EXISTS;
  }

  auto right =
      m_allowed_vnums.upper_bound(CVNumRange(vnum, 0));    // Here does not matter a value of the second argument
  decltype(m_allowed_vnums)::reverse_iterator p(right);    // going opposite direction
  while (p != m_allowed_vnums.rend()) {
    if (p->max() >= vnum) {
      return EAVNR_OK;
    }
    ++p;
  }

  return EAVNR_OUT_OF_RANGE;
}

CCraftModel::EAddVNumResult CCraftModel::add_vnum(const obj_vnum vnum) {
  const auto result = check_vnum(vnum);
  if (EAVNR_OK == result) {
    m_existing_vnums.insert(vnum);
  }
  return result;
}

void CCraftModel::report_vnum_error(const obj_vnum vnum, const EAddVNumResult add_vnum_result) {
  std::string reason;
  switch (add_vnum_result) {
    case EAVNR_ALREADY_EXISTS: reason = "VNUM already exists in the model";
      break;

    case EAVNR_OUT_OF_RANGE: reason = "VNUM is out of range";
      break;

    default: {
      std::stringstream ss;
      ss << "<unexpected error " << add_vnum_result << ">";
      reason = ss.str();
    }
      break;
  }
  logger("WARNING: Entity with VNUM %d has not been added into craft model because %s.\n",
         vnum, reason.c_str());
}

bool CCraftModel::export_object(const obj_vnum vnum, const char *filename) {
  const auto rnum = obj_proto.rnum(vnum);
  if (-1 == rnum) {
    logger("WARNING: Failed to export prototype with VNUM %d", vnum);
    return false;
  }

  const auto object = obj_proto[rnum];
  pugi::xml_document document;
  pugi::xml_node root = document.append_child("object");
  CObject prototype(vnum);
  prototype.load_from_object(object);

  if (!prototype.save_to_node(&root)) {
    logger("WARNING: Could not save prototype to XML.\n");
    return false;
  }

  pugi::xml_node decl = document.prepend_child(pugi::node_declaration);
  decl.append_attribute("version") = "1.0";
  decl.append_attribute("encoding") = "koi8-r";

  return document.save_file(filename);
}

const std::string CObject::KIND = "simple object";
}

/* vim: set ts=4 sw=4 tw=0 noet syntax=cpp :*/
