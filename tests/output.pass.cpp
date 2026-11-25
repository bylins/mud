#include <gtest/gtest.h>

#include "engine/core/iosystem.h"

// Фаза вывода решает, что сделать с дескриптором: полный проход (вывод плюс промпт) или
// только досылку неотправленного хвоста. Спутать их дорого: полный проход всегда дописывает
// промпт, а хвост за собой оставляет каждый MSDP-отчёт -- так в #4238 вышло, что игрок,
// стоящий рядом с дерущейся группой, получал промпт на пустом месте каждый боевой раунд
// (GROUP уходит всем согруппникам в комнате раз в раунд), не имея ни строки своего вывода.

namespace {

using iosystem::ChooseOutputPass;
using iosystem::EOutputPass;

constexpr bool kPromptShown = true;
constexpr bool kPromptPending = false;
constexpr bool kHasOutput = true;
constexpr bool kNoOutput = false;
constexpr bool kHasTail = true;
constexpr bool kNoTail = false;

}  // namespace

// Собственно та регрессия: промпт уже выведен, своего вывода нет, а хвост остался.
TEST(OutputPass, TailAloneDoesNotReprintThePrompt) {
	EXPECT_EQ(EOutputPass::kTailOnly, ChooseOutputPass(kPromptShown, kNoOutput, kHasTail));
}

TEST(OutputPass, NewOutputGetsTheFullPass) {
	EXPECT_EQ(EOutputPass::kFull, ChooseOutputPass(kPromptShown, kHasOutput, kNoTail));
}

// Промпт ещё не выведен (игрок только что ввёл команду) -- проход нужен даже без вывода.
TEST(OutputPass, PendingPromptGetsTheFullPass) {
	EXPECT_EQ(EOutputPass::kFull, ChooseOutputPass(kPromptPending, kNoOutput, kNoTail));
}

// Если есть и то и другое, хвост всё равно уйдёт: полный проход кончается его досылкой.
TEST(OutputPass, OutputAndTailTogetherGetTheFullPass) {
	EXPECT_EQ(EOutputPass::kFull, ChooseOutputPass(kPromptShown, kHasOutput, kHasTail));
	EXPECT_EQ(EOutputPass::kFull, ChooseOutputPass(kPromptPending, kNoOutput, kHasTail));
}

TEST(OutputPass, NothingWaitingIsNothingToDo) {
	EXPECT_EQ(EOutputPass::kNothing, ChooseOutputPass(kPromptShown, kNoOutput, kNoTail));
}
