// $RCSfile$     $Date$     $$
// Copyright (c) 2008 Krodo
// Part of Bylins http://www.mud.ru

#include "conf.h"
#include <boost/random.hpp>
#include "random.hpp"
#include "sysdep.h"

namespace Random
{

class NormalRand
{
public:
	NormalRand()
	{
		rng.seed(static_cast<unsigned>(time(0)));
	};
	int number(int from, int to);

private:
	boost::mt19937 rng;
};

NormalRand rnd;

int NormalRand::number(int from, int to)
{
	boost::uniform_int<> dist(from, to);
	boost::variate_generator<boost::mt19937&, boost::uniform_int<> >  dice(rng, dist);
	return dice();
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
