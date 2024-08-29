//
// Created by Sventovit on 15.01.2023.
//

#include <memory>
#include <deque>

#ifndef BYLINS_SRC_CMD_GOD_INSPECT_H_
#define BYLINS_SRC_CMD_GOD_INSPECT_H_

class CharData;
class InspectRequest;
using InspectRequestPtr = std::shared_ptr<InspectRequest>;

class InspectRequestDeque : private std::deque<InspectRequestPtr> {
 public:
  InspectRequestDeque() = default;

  void Create(const CharData *ch, char *argument);
  void Inspecting();
  bool IsBusy(const CharData *ch);

 private:
  bool IsQueueAvailable(const CharData *ch);
};

void DoInspect(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);

#endif //BYLINS_SRC_CMD_GOD_INSPECT_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
