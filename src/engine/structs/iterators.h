/*
 \authors Created by Sventovit
 \date 16.02.2022.
 \brief Модуль для всяких извращенных итераторов.
 \details Модуль для шаблонов итераторов, дипазонов и тому подобного,
 */

#ifndef BYLINS_SRC_STRUCTS_ITERATORS_H_
#define BYLINS_SRC_STRUCTS_ITERATORS_H_

namespace iterators {

template<class R>
class Range {
 public:
	explicit Range(R begin) :
		begin_{begin},
		end_{R()} {}

	[[nodiscard]] auto begin() const { return begin_; };
	[[nodiscard]] auto end() const { return end_; };

 private:
	R begin_;
	R end_;
};

template<typename T, typename I>
struct ConstIterator {
 private:
	T it_;
 public:
	using value_type        = I;
	using pointer           = const I*;
	using reference         = const I&;

	ConstIterator() = default;
	ConstIterator(const ConstIterator &p) = default;
	explicit ConstIterator(T it) :
		it_{it} {};

	ConstIterator &operator++() {
		++it_;
		return *this;
	};

	ConstIterator operator++(int) {
		auto retval = *this;
		++*this;
		return retval;
	};

	bool operator==(const ConstIterator &it) const { return it_ == it.it_; };

	bool operator!=(const ConstIterator &other) const { return !(*this == other); };

	reference operator*() const { return *(it_->second); }

	pointer operator->() { return it_->second.get(); }
};

} // namespace iterators

#endif //BYLINS_SRC_STRUCTS_ITERATORS_H_
