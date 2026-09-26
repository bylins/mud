/**
\file yaml_error_context.cpp - a part of the Bylins engine.
*/

#include "yaml_error_context.h"

#include <fstream>
#include <sstream>
#include <vector>

namespace world_format {

namespace {

std::vector<std::string_view> SplitLines(std::string_view content) {
	std::vector<std::string_view> lines;
	std::size_t start = 0;
	while (start <= content.size()) {
		const auto end = content.find('\n', start);
		if (end == std::string_view::npos) {
			lines.push_back(content.substr(start));
			break;
		}
		lines.push_back(content.substr(start, end - start));
		start = end + 1;
	}
	while (!lines.empty() && lines.back().empty()) {
		lines.pop_back();
	}
	return lines;
}

std::string_view Trimmed(std::string_view line) {
	while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t')) {
		line.remove_suffix(1);
	}
	return line;
}

bool IsBlank(std::string_view line) {
	return Trimmed(line).find_first_not_of(" \t") == std::string_view::npos;
}

bool StartsAtColumnZero(std::string_view line) {
	return !line.empty() && line.front() != ' ' && line.front() != '\t';
}

// "14:" -- ключ записи в плоском файле (номер внутри зоны).
bool IsEntryKey(std::string_view line) {
	if (!StartsAtColumnZero(line)) {
		return false;
	}
	std::size_t pos = 0;
	while (pos < line.size() && line[pos] >= '0' && line[pos] <= '9') {
		++pos;
	}
	return pos > 0 && pos < line.size() && line[pos] == ':';
}

// Любая строка без отступа, которую yaml примет за новый ключ верхнего уровня.
bool LooksLikeTopLevelKey(std::string_view line) {
	if (!StartsAtColumnZero(line)) {
		return false;
	}
	const auto colon = line.find(':');
	if (colon == std::string_view::npos) {
		return false;
	}
	return line.find(' ') > colon;
}

// "- MOB 0 58708 1 58701" -- законный элемент последовательности без отступа.
bool IsListItem(std::string_view line) {
	return line.size() > 1 && line[0] == '-' && line[1] == ' ';
}

std::string Quote(std::string_view line) {
	return "\"" + std::string(Trimmed(line)) + "\"";
}

}  // namespace

std::string DescribeYamlError(std::string_view content, int error_line) {
	const auto lines = SplitLines(content);
	if (lines.empty() || error_line < 1 || error_line > static_cast<int>(lines.size())) {
		return {};
	}
	const std::size_t error_idx = static_cast<std::size_t>(error_line) - 1;

	// Причина -- ближайшая сверху строка без отступа, которая не пустая, не комментарий,
	// не элемент списка и не ключ: такая обрывает блок, а парсер спотыкается уже дальше.
	// Ключи по дороге пропускаем, а не останавливаемся на них: между поломкой и местом,
	// где парсер упёрся, обычно лежит начало следующей записи.
	std::size_t suspect_idx = 0;
	bool has_suspect = false;
	for (std::size_t i = error_idx + 1; i-- > 0;) {
		const auto line = Trimmed(lines[i]);
		if (IsBlank(line) || !StartsAtColumnZero(line) || line.front() == '#') {
			continue;
		}
		if (IsEntryKey(line) || LooksLikeTopLevelKey(line) || IsListItem(line)) {
			continue;
		}
		suspect_idx = i;
		has_suspect = true;
		break;
	}

	// Запись, внутри которой это случилось, -- ближайший ключ вида "14:" сверху.
	std::size_t entry_idx = 0;
	bool has_entry = false;
	for (std::size_t i = (has_suspect ? suspect_idx : error_idx) + 1; i-- > 0;) {
		if (IsEntryKey(Trimmed(lines[i]))) {
			entry_idx = i;
			has_entry = true;
			break;
		}
	}

	std::ostringstream out;
	if (has_suspect && suspect_idx == error_idx) {
		// Парсер споткнулся ровно на виноватой строке -- незачем печатать её дважды.
		out << " [строка " << error_line << " без отступа: " << Quote(lines[error_idx]);
	} else {
		out << " [строка " << error_line << ": " << Quote(lines[error_idx]);
		if (has_suspect) {
			out << "; похоже на причину -- строка " << (suspect_idx + 1) << " без отступа: "
				<< Quote(lines[suspect_idx]);
		} else {
			out << "; разрыв блока обычно выше указанной строки";
		}
	}
	if (has_entry) {
		out << "; запись " << Quote(lines[entry_idx]) << " со строки " << (entry_idx + 1);
	}
	out << "]";
	return out.str();
}

std::string DescribeYamlErrorInFile(const std::string &path, int error_line) {
	std::ifstream in(path, std::ios::binary);
	if (!in) {
		return {};
	}
	std::ostringstream buffer;
	buffer << in.rdbuf();
	return DescribeYamlError(buffer.str(), error_line);
}

}  // namespace world_format
