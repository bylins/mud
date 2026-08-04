/**
\file utf8.cpp - a part of the Bylins engine.
\brief Implementation of the character-semantic UTF-8 helpers declared in utf8.h (issue #3681).
*/

#include "utf8.h"

namespace utf8 {

namespace {

// Result of decoding one position: the code point, the byte length consumed, and whether the
// sequence was well-formed. On a malformed byte, `valid` is false, `len` is 1 and `cp` is the
// raw byte -- so scanners always advance and lenient callers can pass the byte through untouched.
struct Decoded {
	char32_t cp;
	std::size_t len;
	bool valid;
};

// Decode per the Unicode 3-7 grammar: the first continuation byte has a lead-specific range
// (which is what rejects overlong forms and surrogates), the rest are plain 0x80..0xBF.
Decoded decode_core(std::string_view s, std::size_t pos) {
	const std::size_t n = s.size();
	const unsigned char c0 = static_cast<unsigned char>(s[pos]);
	if (c0 < 0x80) {
		return {c0, 1, true};
	}

	int len = 0;
	char32_t cp = 0;
	unsigned char b1_lo = 0x80;
	unsigned char b1_hi = 0xBF;
	if (c0 >= 0xC2 && c0 <= 0xDF) {
		len = 2;
		cp = c0 & 0x1F;
	} else if (c0 == 0xE0) {
		len = 3;
		cp = c0 & 0x0F;
		b1_lo = 0xA0;
	} else if (c0 >= 0xE1 && c0 <= 0xEC) {
		len = 3;
		cp = c0 & 0x0F;
	} else if (c0 == 0xED) {
		len = 3;
		cp = c0 & 0x0F;
		b1_hi = 0x9F;
	} else if (c0 >= 0xEE && c0 <= 0xEF) {
		len = 3;
		cp = c0 & 0x0F;
	} else if (c0 == 0xF0) {
		len = 4;
		cp = c0 & 0x07;
		b1_lo = 0x90;
	} else if (c0 >= 0xF1 && c0 <= 0xF3) {
		len = 4;
		cp = c0 & 0x07;
	} else if (c0 == 0xF4) {
		len = 4;
		cp = c0 & 0x07;
		b1_hi = 0x8F;
	} else {
		// 0xC0, 0xC1, 0xF5..0xFF or a stray continuation byte: cannot start a sequence.
		return {c0, 1, false};
	}

	if (pos + static_cast<std::size_t>(len) > n) {
		return {c0, 1, false};
	}

	const unsigned char b1 = static_cast<unsigned char>(s[pos + 1]);
	if (b1 < b1_lo || b1 > b1_hi) {
		return {c0, 1, false};
	}
	cp = (cp << 6) | (b1 & 0x3F);

	for (int k = 2; k < len; ++k) {
		const unsigned char b = static_cast<unsigned char>(s[pos + static_cast<std::size_t>(k)]);
		if (b < 0x80 || b > 0xBF) {
			return {c0, 1, false};
		}
		cp = (cp << 6) | (b & 0x3F);
	}

	return {cp, static_cast<std::size_t>(len), true};
}

}  // namespace

int sequence_length(unsigned char c) {
	if (c < 0x80) {
		return 1;
	}
	if (c >= 0xC0 && c <= 0xDF) {
		return 2;
	}
	if (c >= 0xE0 && c <= 0xEF) {
		return 3;
	}
	if (c >= 0xF0 && c <= 0xF7) {
		return 4;
	}
	// Continuation byte (0x80..0xBF) or an out-of-range lead (0xF8..0xFF): not a valid start.
	return 1;
}

std::size_t decode(std::string_view s, std::size_t pos, char32_t &cp) {
	if (pos >= s.size()) {
		cp = 0;
		return 0;
	}
	const Decoded d = decode_core(s, pos);
	cp = d.cp;
	return d.len;
}

std::size_t encode(char32_t cp, std::string &out) {
	if (cp <= 0x7F) {
		out.push_back(static_cast<char>(cp));
		return 1;
	}
	if (cp <= 0x7FF) {
		out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
		out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
		return 2;
	}
	if (cp >= 0xD800 && cp <= 0xDFFF) {
		return 0;  // surrogate half: not a Unicode scalar value
	}
	if (cp <= 0xFFFF) {
		out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
		out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
		out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
		return 3;
	}
	if (cp <= 0x10FFFF) {
		out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
		out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
		out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
		out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
		return 4;
	}
	return 0;
}

std::size_t encode(char32_t cp, char *out) {
	if (cp <= 0x7F) {
		out[0] = static_cast<char>(cp);
		return 1;
	}
	if (cp <= 0x7FF) {
		out[0] = static_cast<char>(0xC0 | (cp >> 6));
		out[1] = static_cast<char>(0x80 | (cp & 0x3F));
		return 2;
	}
	if (cp >= 0xD800 && cp <= 0xDFFF) {
		return 0;
	}
	if (cp <= 0xFFFF) {
		out[0] = static_cast<char>(0xE0 | (cp >> 12));
		out[1] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
		out[2] = static_cast<char>(0x80 | (cp & 0x3F));
		return 3;
	}
	if (cp <= 0x10FFFF) {
		out[0] = static_cast<char>(0xF0 | (cp >> 18));
		out[1] = static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
		out[2] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
		out[3] = static_cast<char>(0x80 | (cp & 0x3F));
		return 4;
	}
	return 0;
}

bool is_valid(std::string_view s) {
	std::size_t pos = 0;
	const std::size_t n = s.size();
	while (pos < n) {
		const Decoded d = decode_core(s, pos);
		if (!d.valid) {
			return false;
		}
		pos += d.len;
	}
	return true;
}

std::size_t length(std::string_view s) {
	std::size_t count = 0;
	std::size_t pos = 0;
	const std::size_t n = s.size();
	while (pos < n) {
		pos += decode_core(s, pos).len;
		++count;
	}
	return count;
}

std::size_t byte_offset(std::string_view s, std::size_t index) {
	std::size_t pos = 0;
	const std::size_t n = s.size();
	while (index > 0 && pos < n) {
		pos += decode_core(s, pos).len;
		--index;
	}
	return pos;
}

std::string_view char_at(std::string_view s, std::size_t index) {
	const std::size_t start = byte_offset(s, index);
	if (start >= s.size()) {
		return {};
	}
	const std::size_t len = decode_core(s, start).len;
	return s.substr(start, len);
}

std::string substr(std::string_view s, std::size_t pos, std::size_t count) {
	const std::size_t start = byte_offset(s, pos);
	if (count == std::string_view::npos) {
		return std::string(s.substr(start));
	}
	const std::size_t stop = byte_offset(s, pos + count);
	return std::string(s.substr(start, stop - start));
}

char32_t to_lower(char32_t cp) {
	if (cp >= 'A' && cp <= 'Z') {
		return cp + 0x20;
	}
	if (cp >= 0x0410 && cp <= 0x042F) {  // U+0410..U+042F (upper) -> U+0430..U+044F (lower)
		return cp + 0x20;
	}
	if (cp == 0x0401) {  // U+0401 (Yo) -> U+0451 (yo)
		return 0x0451;
	}
	return cp;
}

char32_t to_upper(char32_t cp) {
	if (cp >= 'a' && cp <= 'z') {
		return cp - 0x20;
	}
	if (cp >= 0x0430 && cp <= 0x044F) {  // U+0430..U+044F (lower) -> U+0410..U+042F (upper)
		return cp - 0x20;
	}
	if (cp == 0x0451) {  // U+0451 (yo) -> U+0401 (Yo)
		return 0x0401;
	}
	return cp;
}

namespace {

// Shared body for the whole-string case folders: decode, fold each code point, re-encode.
// Malformed bytes (valid == false) are copied through verbatim so nothing is silently dropped.
std::string fold_string(std::string_view s, char32_t (*fold)(char32_t)) {
	std::string out;
	out.reserve(s.size());
	std::size_t pos = 0;
	const std::size_t n = s.size();
	while (pos < n) {
		const Decoded d = decode_core(s, pos);
		if (d.valid) {
			encode(fold(d.cp), out);
		} else {
			out.push_back(s[pos]);
		}
		pos += d.len;
	}
	return out;
}

}  // namespace

std::string to_lower(std::string_view s) {
	return fold_string(s, to_lower);
}

std::string to_upper(std::string_view s) {
	return fold_string(s, to_upper);
}

}  // namespace utf8

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
