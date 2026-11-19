//
// Created by Sventovit on 07.09.2024.
//

#ifndef BYLINS_SRC_CMD_DO_MODE_H_
#define BYLINS_SRC_CMD_DO_MODE_H_

class DescriptorData;

class CharData;
void DoMode(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);

// Допустимая ширина экрана в знаках -- то, что принимает "режим ширина". Вынесено сюда,
// чтобы вопрос при создании персонажа и сама команда не разъехались в границах.
inline constexpr int kMinScreenWidth = 30;
inline constexpr int kMaxScreenWidth = 300;
inline constexpr int kDefaultScreenWidth = 80;

// Высота экрана в строках -- те же границы, что принимает "режим высота".
inline constexpr int kMinScreenHeight = 10;
inline constexpr int kMaxScreenHeight = 100;

// Применяет размер окна, присланный клиентом по NAWS, к персонажу дескриптора.
// Ничего не делает, если размер не присылали или он вне допустимых границ.
void ApplyNawsScreenSize(DescriptorData *d);

#endif //BYLINS_SRC_CMD_DO_MODE_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
