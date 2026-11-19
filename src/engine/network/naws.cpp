/**
\file naws.cpp - a part of the Bylins engine.
*/

#include "naws.h"

#include "telnet.h"

#include <array>

namespace naws {

namespace {
constexpr unsigned char kIac = static_cast<unsigned char>(IAC);
constexpr unsigned char kSe = static_cast<unsigned char>(SE);
}  // namespace

Parsed Parse(std::string_view bytes) {
	Parsed result;
	std::array<unsigned char, 4> size{};
	std::size_t filled = 0;
	// Данные идут после IAC SB NAWS
	for (std::size_t i = 3; i + 1 < bytes.size(); ++i) {
		const auto byte = static_cast<unsigned char>(bytes[i]);
		if (byte == kIac) {
			const auto next = static_cast<unsigned char>(bytes[i + 1]);
			if (next == kIac) {
				// Удвоенный 255 -- это один байт данных. Без этого окно шириной 255 знаков
				// съедало бы конец последовательности.
				if (filled < size.size()) {
					size[filled++] = kIac;
				}
				++i;
				continue;
			}
			if (next == kSe) {
				result.consumed = i + 2;
				if (filled == size.size()) {
					result.complete = true;
					result.width = (size[0] << 8) | size[1];
					result.height = (size[2] << 8) | size[3];
				}
				return result;
			}
			// Мусор вместо IAC SE: вырезаем до него и живём дальше
			result.consumed = i + 1;
			return result;
		}
		if (filled < size.size()) {
			size[filled++] = byte;
		}
	}
	return result;   // не дочитано: ждём следующего чтения
}

}  // namespace naws
