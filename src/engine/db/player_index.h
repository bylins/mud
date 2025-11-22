/**
\file player_index.h - a part of the Bylins engine.
\authors Created by Sventovit.
\date 22.11.2025.
\brief Brief description.
\detail Detail description.
*/

#ifndef BYLINS_SRC_ENGINE_DB_PLAYER_INDEX_H_
#define BYLINS_SRC_ENGINE_DB_PLAYER_INDEX_H_

#include <vector>
#include <unordered_map>

#include "administration/name_adviser.h"
#include "gameplay/classes/classes_constants.h"
#include "obj_save.h"

class PlayerIndexElement {
 public:
  explicit PlayerIndexElement(std::string_view name);

  char *mail{nullptr};
  char *last_ip{nullptr};
  int level{0};
  int remorts{0};
  ECharClass plr_class{ECharClass::kUndefined};
  int last_logon{0};
  int activity{0};        // When player be saved and checked
  SaveInfo *timer{nullptr};

  [[nodiscard]] const std::string &name() const { return name_; }
  [[nodiscard]] long uid() const { return uid_; }

  void set_name(std::string_view name);
  void set_uid(const long uid) { uid_ = uid; }
 private:
  long uid_{0L};
  std::string name_;
};

class PlayersIndex : public std::vector<PlayerIndexElement> {
 public:
  using parent_t = std::vector<PlayerIndexElement>;
  using parent_t::operator[];
  using parent_t::size;

  static const std::size_t NOT_FOUND;

  ~PlayersIndex();

  std::size_t Append(const PlayerIndexElement &element);
  [[nodiscard]] bool IsPlayerExists(long id) const { return m_id_to_index.find(id) != m_id_to_index.end(); }
  bool IsPlayerExists(const char *name) const { return NOT_FOUND != GetIndexByName(name); }
  std::size_t GetIndexByName(const char *name) const;
  void SetName(std::size_t index, const char *name);

  NameAdviser &GetNameAdviser() { return m_name_adviser; }

 private:
  class hasher {
   public:
	std::size_t operator()(const std::string &value) const;
  };

  class equal_to {
   public:
	bool operator()(const std::string &left, const std::string &right) const;
  };

  using id_to_index_t = std::unordered_map<long, std::size_t>;
  using name_to_index_t = std::unordered_map<std::string, std::size_t, hasher, equal_to>;

  void AddNameToIndex(const std::string &name, std::size_t index);

  id_to_index_t m_id_to_index;
  name_to_index_t m_name_to_index;
  NameAdviser m_name_adviser; // contains free names which are available for new players
};

extern PlayersIndex &player_table;

bool IsPlayerExists(long id);
std::string GetNameById(long id);
std::string GetPlayerNameByUnique(int unique);
long CmpPtableByName(char *name, int len);
long GetPlayerTablePosByName(const char *name);
long GetPtableByUnique(long unique);
long GetPlayerIdByName(char *name);
int GetPlayerUidByName(int id);
int GetLevelByUnique(long unique);
long GetLastlogonByUnique(long unique);
int IsCorrectUnique(int unique);
void RecreateSaveinfo(size_t number);
void BuildPlayerIndexNew();

inline SaveInfo *SAVEINFO(const size_t number) {
	return player_table[number].timer;
}

inline void ClearSaveinfo(const size_t number) {
	delete player_table[number].timer;
	player_table[number].timer = nullptr;
}

#endif //BYLINS_SRC_ENGINE_DB_PLAYER_INDEX_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
