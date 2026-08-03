/**
\file native_text.cpp - a part of the Bylins engine.
\brief Native-encoding character helpers declared in native_text.h (issue #3681).

Two implementations selected by the INTERNAL_ENCODING_UTF8 build macro. The KOI8-R branch is kept
byte-for-byte identical to the open-coded logic these helpers replace so that routing call sites
through them changes nothing until the encoding flip.
*/

#include "native_text.h"

#ifdef INTERNAL_ENCODING_UTF8
#include "utf8.h"
#else
#include "utils.h"  // UPPER() / a_ucc() -- the KOI8-R case table
#endif

#include <string>

namespace native_text {

#ifdef INTERNAL_ENCODING_UTF8

bool native_is_utf8() {
	return true;
}

std::size_t char_count(const char *begin, const char *end) {
	return utf8::length(std::string_view(begin, static_cast<std::size_t>(end - begin)));
}

std::size_t char_count(std::string_view s) {
	return utf8::length(s);
}

void capitalize_first(char *s) {
	if (s == nullptr || *s == '\0') {
		return;
	}
	const std::string_view sv(s);
	char32_t cp = 0;
	const std::size_t len = utf8::decode(sv, 0, cp);
	if (len == 0) {
		return;
	}
	const char32_t upper = utf8::to_upper(cp);
	if (upper == cp) {
		return;
	}
	std::string encoded;
	if (utf8::encode(upper, encoded) == len) {
		for (std::size_t i = 0; i < len; ++i) {
			s[i] = encoded[i];
		}
	}
}

std::size_t truncate_offset(std::string_view s, std::size_t max_bytes) {
	if (max_bytes >= s.size()) {
		return s.size();
	}
	std::size_t pos = 0;
	while (true) {
		char32_t cp = 0;
		const std::size_t len = utf8::decode(s, pos, cp);
		if (len == 0 || pos + len > max_bytes) {
			break;
		}
		pos += len;
	}
	return pos;
}

std::size_t char_bytes(const char *s) {
	const unsigned char lead = static_cast<unsigned char>(*s);
	if (lead < 0x80) {
		return 1;
	}
	const std::size_t want = static_cast<std::size_t>(utf8::sequence_length(lead));
	std::size_t n = 1;
	while (n < want && (static_cast<unsigned char>(s[n]) & 0xC0) == 0x80) {
		++n;
	}
	return n;
}

#else  // KOI8-R: 1 byte == 1 character

bool native_is_utf8() {
	return false;
}

std::size_t char_count(const char *begin, const char *end) {
	return static_cast<std::size_t>(end - begin);
}

std::size_t char_count(std::string_view s) {
	return s.size();
}

void capitalize_first(char *s) {
	if (s != nullptr && *s != '\0') {
		*s = static_cast<char>(UPPER(static_cast<unsigned char>(*s)));
	}
}

std::size_t truncate_offset(std::string_view s, std::size_t max_bytes) {
	return max_bytes < s.size() ? max_bytes : s.size();
}

std::size_t char_bytes(const char *) {
	return 1;
}

#endif

}  // namespace native_text

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
