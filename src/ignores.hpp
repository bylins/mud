#ifndef __IGNORES_HPP__
#define __IGNORES_HPP__

#include <memory>

struct ignore_data
{
	using shared_ptr = std::shared_ptr<ignore_data>;

	ignore_data() : id(0), mode(0) {}

	long id;
	unsigned long mode;
};

#endif // __IGNORES_HPP__

/* vim: set ts=4 sw=4 tw=0 noet syntax=cpp :*/
