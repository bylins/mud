// $RCSfile$     $Date$     $$
// Copyright (c) 2008 Krodo
// Part of Bylins http://www.mud.ru

#include "conf.h"
#include <boost/version.hpp>
#if BOOST_VERSION >= 104700
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#else
#include <boost/random.hpp>
#  pragma message("HINT: You are using old version of boost")
#endif
#include "random.hpp"
#include "sysdep.h"

namespace Random
{

class NormalRand
{
public:
	NormalRand()
	{
#if BOOST_VERSION >= 104700
		_rng.seed(static_cast<unsigned>(time(0)));
#else
		rng.seed(static_cast<unsigned>(time(0)));
#endif
	};
	int number(int from, int to);

private:
#if BOOST_VERSION >= 104700
	boost::random::mt19937 _rng;
#else
	boost::mt19937 rng;
#endif
};

NormalRand rnd;

int NormalRand::number(int from, int to)
{
#if BOOST_VERSION >= 104700
	boost::random::uniform_int_distribution<> dist(from, to);
	return dist(_rng);
#else
	boost::uniform_int<> dist(from, to);
	boost::variate_generator<boost::mt19937&, boost::uniform_int<> >  dice(rng, dist);
	return dice();
#endif
}

} // namespace Random

// править везде вызовы, чтобы занести эти две функции в неймспейс меня чет обломало

/**
* Генерация рандомного числа в диапазоне от from до to.
*/
int number(int from, int to)
{
	if (from > to)
	{
		int tmp = from;
		from = to;
		to = tmp;
	}

	return Random::rnd.number(from, to);
}

/**
* Аналог кидания кубиков.
*/
int dice(int number, int size)
{
	int sum = 0;

	if (size <= 0 || number <= 0)
		return (0);

	while (number-- > 0)
		sum += Random::rnd.number(1, size);

	return sum;
}
