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

	// True while the most recent stream write was a keep-chomping literal
	// block ("|+"). A "|+" block keeps *every* trailing line break up to the
	// next less-indented, non-empty line -- including a blank separator line
	// emitted right after it. That blank gets absorbed into the scalar, the
	// value gains one '\n', the next save writes one more blank, and the block
	// grows without bound on every round-trip (issue #3587). EmptyLine() reads
	// this flag to suppress the swallowable separator; a following comment or
	// key then terminates the block cleanly with the exact newline count.
	bool pending_keep_block_ = false;

public:
	explicit Koi8rYamlEmitter(std::ostream &out) : out_(out), indent_(0) {}

	std::string GetIndent() const { return std::string(indent_, ' '); }

	void BeginMap() {}
	void EndMap() {}

	void Key(const std::string &key) { pending_keep_block_ = false; out_ << GetIndent() << key << ":"; }

	void Value(const std::string &value, [[maybe_unused]] bool literal = false)
	{
		pending_keep_block_ = false;
		// Any value containing a newline has to go into a literal block --
		// plain and single-quoted scalars don't survive embedded \n on
		// reload (they get folded/truncated). Force literal regardless of
		// the caller's `literal` flag.
		const bool has_newline = value.find('\n') != std::string::npos;
		if (has_newline)
		{
			// Literal block with explicit indent indicator ("2") -- otherwise
			// the parser auto-detects indent from the first non-empty content
			// line, and leading whitespace there gets folded into indent and
			// breaks the block.
			//
			// Chomping mode is picked so the round-trip preserves the original
			// byte string exactly:
			//   * content is empty (value is just N>=1 newlines) -> keep "|+2"
			//     and emit N empty content lines; clip can't carry a newline
			//     when there's no real content.
			//   * content non-empty, 0 trailing \n   -> strip "|-2"
			//   * content non-empty, exactly 1       -> clip  "|2"
			//   * content non-empty, 2 or more       -> keep  "|+2"
			std::string cleaned = value;
			cleaned.erase(std::remove(cleaned.begin(), cleaned.end(), '\r'), cleaned.end());

			std::size_t trailing_nl = 0;
			while (trailing_nl < cleaned.size()
				   && cleaned[cleaned.size() - 1 - trailing_nl] == '\n')
			{
				++trailing_nl;
			}
			const bool content_only_newlines = (trailing_nl == cleaned.size());

			if (content_only_newlines)
			{
				pending_keep_block_ = true;
				out_ << " |+2" << std::endl;
			}
			else if (trailing_nl == 0)
			{
				out_ << " |-2" << std::endl;
			}
			else if (trailing_nl == 1)
			{
				out_ << " |2" << std::endl;
			}
			else
			{
				pending_keep_block_ = true;
				out_ << " |+2" << std::endl;
			}

			// Iterate WITHOUT stripping any trailing \n: std::getline already
			// consumes the terminator, so a value like "\n" becomes one empty
			// line in the block, "text\n\n" becomes "text" + "" -- and the
			// chomping indicator above keeps the count accurate on reload.
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
				value[0] == ',' ||
				// Trailing whitespace would be stripped by the YAML parser
				// from a plain scalar; quoting preserves it bit-for-bit.
				value.back() == ' ' || value.back() == '\t')
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
		pending_keep_block_ = false;
		out_ << " " << value;
		if (!comment.empty()) { out_ << "  # " << comment; }
		out_ << std::endl;
	}

	void Value(long value, const std::string &comment = "")
	{
		pending_keep_block_ = false;
		out_ << " " << value;
		if (!comment.empty()) { out_ << "  # " << comment; }
		out_ << std::endl;
	}

	void BeginSequence() { pending_keep_block_ = false; out_ << std::endl; }

	void SequenceItem(const std::string &value, const std::string &comment = "")
	{
		pending_keep_block_ = false;
		out_ << GetIndent() << "- " << value;
		if (!comment.empty()) { out_ << "  # " << comment; }
		out_ << std::endl;
	}

	void SequenceItem(int value, const std::string &comment = "")
	{
		pending_keep_block_ = false;
		out_ << GetIndent() << "- " << value;
		if (!comment.empty()) { out_ << "  # " << comment; }
		out_ << std::endl;
	}

	// Indent changes emit no output, so they must NOT reset the keep-block
	// flag -- the several DecreaseIndent() calls between a trailing "|+" block
	// and the next EmptyLine() would otherwise clear it and re-open the bug.
	void IncreaseIndent() { indent_ += 2; }
	void DecreaseIndent() { indent_ -= 2; }

	void Comment(const std::string &text) { pending_keep_block_ = false; out_ << GetIndent() << "# " << text << std::endl; }

	// Skip the blank line when it directly follows a "|+" keep block: that
	// block would swallow it (issue #3587). The next comment/key terminates
	// the block cleanly, so no separator is lost in any meaningful sense.
	void EmptyLine()
	{
		if (pending_keep_block_)
		{
			pending_keep_block_ = false;
			return;
		}
		out_ << std::endl;
	}

	void BeginBlock() { pending_keep_block_ = false; out_ << std::endl; indent_ += 2; }
	void EndBlock() { indent_ -= 2; }
};

#endif  // KOI8R_YAML_EMITTER_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
