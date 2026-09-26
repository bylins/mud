/**
\file naws.h - a part of the Bylins engine.
\brief Разбор телнет-подпереговоров о размере окна (NAWS, RFC 1073).
\detail Клиент присылает размер окна при подключении и при каждом его изменении -- по нему
ширина переноса текста совпадает с настоящим окном, и задавать её руками не нужно. Разбор
байтов вынесен отдельно от сетевого цикла, чтобы его можно было проверить тестами: в
последовательности есть две ловушки -- байт 255 внутри удваивается, и последовательность
может прийти не целиком.
*/

#ifndef ENGINE_NETWORK_NAWS_H_
#define ENGINE_NETWORK_NAWS_H_

#include <cstddef>
#include <string_view>

namespace naws {

/// Итог разбора куска, начинающегося с IAC SB NAWS.
struct Parsed {
	/// Сколько байт занимает последовательность вместе с IAC SE; 0 -- пришла не целиком.
	std::size_t consumed{0};
	/// Разобран ли размер. false при обрыве или мусоре вместо IAC SE.
	bool complete{false};
	int width{0};
	int height{0};
};

/**
 * Разбирает IAC SB NAWS <ширина: 2 байта> <высота: 2 байта> IAC SE.
 *
 * @param bytes кусок ввода, начинающийся с IAC SB NAWS.
 */
Parsed Parse(std::string_view bytes);

}  // namespace naws

#endif  // ENGINE_NETWORK_NAWS_H_
