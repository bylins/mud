// Part of Bylins http://www.mud.ru
// Koi8rYamlEmitter - Custom YAML emitter that preserves KOI8-R encoding.
//
// Scalars whose first character is a YAML indicator (&, *, !, ,, ...) are
// single-quoted, otherwise yaml-cpp interprets the prefix as anchor / alias /
// tag and the save -> reload round-trip breaks (issue #3273: MUD color codes
// like "&Yfoo &Wbar&n" in object/mob names saved unquoted produce
// "cannot assign multiple anchors to the same node" on next boot).

#ifndef KOI8R_YAML_EMITTER_H_
#define KOI8R_YAML_EMITTER_H_

#include <algorithm>
#include <ostream>
#include <regex>
#include <sstream>
#include <string>

class Koi8rYamlEmitter
{
	std::ostream &out_;
	int indent_;

public:
	explicit Koi8rYamlEmitter(std::ostream &out) : out_(out), indent_(0) {}

	std::string GetIndent() const { return std::string(indent_, ' '); }

	void BeginMap() {}
	void EndMap() {}

	void Key(const std::string &key) { out_ << GetIndent() << key << ":"; }

	void Value(const std::string &value, bool literal = false)
	{
		if (literal && value.find('\n') != std::string::npos)
		{
			// Literal block with strip chomping ("|-"): no trailing newline.
			// The Python converter writes "|-" too, so round-trip matches.
			// With plain "|" yaml-cpp/ruamel parse "a\nb\n" -- with "|-" they
			// parse "a\nb" -- and our scripts/descriptions don't carry an
			// expected trailing newline.
			out_ << " |-" << std::endl;

			std::string cleaned = value;
			cleaned.erase(std::remove(cleaned.begin(), cleaned.end(), '\r'), cleaned.end());

			// Strip a single trailing '\n' so the literal block's content
			// matches `value` exactly when re-loaded.
			if (!cleaned.empty() && cleaned.back() == '\n')
			{
				cleaned.pop_back();
			}

			std::istringstream iss(cleaned);
			std::string line;
			while (std::getline(iss, line))
			{
				out_ << GetIndent() << "  " << line << std::endl;
			}
			return;
		}

		// Simple value - quote if contains special YAML characters
		bool needs_quoting = value.empty();
		if (!value.empty())
		{
			if (value.find(':') != std::string::npos ||
				value.find('#') != std::string::npos ||
				value.find('[') != std::string::npos ||
				value.find(']') != std::string::npos ||
				value.find('{') != std::string::npos ||
				value.find('}') != std::string::npos ||
				value.find('|') != std::string::npos ||
				value.find('>') != std::string::npos ||
				value.find('\"') != std::string::npos ||
				value.find('\'') != std::string::npos ||
				value.find('%') != std::string::npos ||  // % is YAML directive indicator
				value[0] == ' ' || value[0] == '-' || value[0] == '?' ||
				value[0] == '@' || value[0] == '`' ||
				value[0] == '&' || value[0] == '*' || value[0] == '!' ||
				value[0] == ',')
			{
				needs_quoting = true;
			}
		}

		if (needs_quoting)
		{
			out_ << " '" << std::regex_replace(value, std::regex("'"), "''") << "'" << std::endl;
		}
		else
		{
			out_ << " " << value << std::endl;
		}
	}

	void Value(int value, const std::string &comment = "")
	{
		out_ << " " << value;
		if (!comment.empty()) { out_ << "  # " << comment; }
		out_ << std::endl;
	}

	void Value(long value, const std::string &comment = "")
	{
		out_ << " " << value;
		if (!comment.empty()) { out_ << "  # " << comment; }
		out_ << std::endl;
	}

	void BeginSequence() { out_ << std::endl; }

	void SequenceItem(const std::string &value, const std::string &comment = "")
	{
		out_ << GetIndent() << "- " << value;
		if (!comment.empty()) { out_ << "  # " << comment; }
		out_ << std::endl;
	}

	void SequenceItem(int value, const std::string &comment = "")
	{
		out_ << GetIndent() << "- " << value;
		if (!comment.empty()) { out_ << "  # " << comment; }
		out_ << std::endl;
	}

	void IncreaseIndent() { indent_ += 2; }
	void DecreaseIndent() { indent_ -= 2; }

	void Comment(const std::string &text) { out_ << GetIndent() << "# " << text << std::endl; }

	void EmptyLine() { out_ << std::endl; }

	void BeginBlock() { out_ << std::endl; indent_ += 2; }
	void EndBlock() { indent_ -= 2; }
};

#endif  // KOI8R_YAML_EMITTER_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
