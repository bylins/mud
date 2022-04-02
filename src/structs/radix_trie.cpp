#include "radix_trie.h"

void compare(const std::string &string1, size_t &pos1, const std::string &string2, size_t &pos2) {
	while (pos1 < string1.size()
		&& pos2 < string2.size()
		&& string1[pos1] == string2[pos2]) {
		++pos1;
		++pos2;
	}
}

bool RadixTrie::AddString(const std::string &string) {
	if (!root_) {
		root_ = std::make_shared<Node>(string);
	} else {
		size_t from = 0;
		root_->AddSuffix(string, from);
	}

	return true;
}

bool RadixTrie::HasString(const std::string &string) const {
	bool result = false;

	if (root_) {
		result = root_->HasSuffix(string, 0);
	}

	return result;
}

void RadixTrie::Node::AddSuffix(const std::string &string, size_t from) {
	if (from >= string.size()) {
		return;
	}

	size_t pos = 0;
	compare(string, from, piece_, pos);

	if (pos == piece_.size()) {
		if (from == string.size()) {
			is_terminal_ = true;    // node is the suffix we are appending
		} else // from < string.size()
		{
			// need to find existing or create new child
			const unsigned char branch = string[from];

			if (children_[branch]) {
				children_[branch]->AddSuffix(string, from);
			} else {
				const auto tail = string.substr(from);
				children_[branch] = std::make_shared<Node>(tail);
			}
		}
	} else // pos < m_piece.size()
	{
		// need to split piece
		const auto old_tail = piece_.substr(pos);
		piece_.resize(pos);
		const auto new_child = std::make_shared<Node>(old_tail);
		const auto size = children_.size();
		new_child->children_ = std::move(children_);
		children_.resize(size);
		const unsigned char old_branch = old_tail[0];
		children_[old_branch] = new_child;
		is_terminal_ = from == string.size();

		if (!is_terminal_) {
			const unsigned char new_branch = string[from];
			const auto new_tail = string.substr(from);
			children_[new_branch] = std::make_shared<Node>(new_tail);
		}
	}
}

bool RadixTrie::Node::HasSuffix(const std::string &string, size_t from) const {
	size_t pos = 0;
	compare(piece_, pos, string, from);

	if (piece_.size() > pos) {
		return false;
	} else if (from == string.size()) {
		// found suffix
		return is_terminal_;
	} else // from < string.size()
	{
		const unsigned char branch = string[from];
		if (!children_[branch]) {
			return false;
		}

		return children_[branch]->HasSuffix(string, from);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
