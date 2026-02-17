//
// Created by Sventovit on 15.01.2023.
//

#include <memory>
#include <deque>
#include <vector>
#include <map>
#include <string>
#include <string_view>

#ifndef BYLINS_SRC_CMD_GOD_INSPECT_H_
#define BYLINS_SRC_CMD_GOD_INSPECT_H_

class CharData;
class InspectRequest;
using InspectRequestPtr = std::shared_ptr<InspectRequest>;

class InspectRequestDeque : private std::deque<InspectRequestPtr> {
 public:
  InspectRequestDeque() = default;

  void NewRequest(const CharData *ch, char *argument);
  void Inspecting();
  bool IsBusy(const CharData *ch);

 private:
  enum EKind { kMail, kIp, kChar, kAll };
  const std::map<std::string_view, EKind>
	  request_kinds_ = {{"mail", kMail}, {"ip", kIp}, {"char", kChar}, {"all", kAll}};

  bool IsQueueAvailable(const CharData *ch);
  bool IsArgsValid(const CharData *ch, const std::vector<std::string> &args);
  InspectRequestPtr CreateRequest(const CharData *ch, const std::vector<std::string> &args);
};

void DoInspect(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);

#endif //BYLINS_SRC_CMD_GOD_INSPECT_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
