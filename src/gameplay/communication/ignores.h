#ifndef IGNORES_HPP_
#define IGNORES_HPP_

#include <memory>

struct ignore_data {
	using shared_ptr = std::shared_ptr<ignore_data>;

	ignore_data() : id(0), mode(0) {}

	long id;
	unsigned long mode;
};

bool ignores(CharData *who, CharData *whom, unsigned int flag);

#endif // __IGNORES_HPP__

/* vim: set ts=4 sw=4 tw=0 noet syntax=cpp :*/
