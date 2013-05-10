// $RCSfile$     $Date$     $$
// Copyright (c) 2008 Krodo
// Part of Bylins http://www.mud.ru

#include "conf.h"
#include <boost/version.hpp>
#if BOOST_VERSION >= 104700
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/random/normal_distribution.hpp>
#include <boost/random/uniform_01.hpp>
#else
#include <boost/random.hpp>
#  pragma message("HINT: You are using old version of boost")
#endif
#include "random.hpp"
#include "sysdep.h"
#include "utils.h"

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
	//Нормальное распределение
	double NormalDistributionNumber(double mean, double sigma);

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

//функция возвращает случайное число с нормальным распределением
//с параметрами mean - матожидание и sigma - дисперсия
double NormalRand::NormalDistributionNumber(double mean, double sigma)
{
#if BOOST_VERSION >= 104700
	boost::uniform_01<boost::mt19937> _un_rng(_rng);
	boost::random::normal_distribution<double> NormalDistribution(mean, sigma);
	return NormalDistribution(_un_rng);
#else
	// Совершенно не уверен что это скомпилится, и тем более будет работать...
	boost::normal_distribution<double> NormDist(mean, sigma);
	boost::variate_generator< mt19937&, normal_distribution<double> > NormalDistribution(rng, NormDist);
	return NormalDistribution();
#endif
}

} // namespace Random

// править везде вызовы, чтобы занести эти две функции в неймспейс меня чет обломало

// * Генерация рандомного числа в диапазоне от from до to.
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

// * Аналог кидания кубиков.
int dice(int number, int size)
{
	int sum = 0;

	if (size <= 0 || number <= 0)
		return (0);

	while (number-- > 0)
		sum += Random::rnd.number(1, size);

	return sum;
}

/**
* Генерация целого числа в заданном диапазоне с нормальным распределением
  Вообще говоря, это все кривовато, но будет по идее работать примерно как надо.
  Сигму надо подбирать экспериментально. При слишком малых значениях выпадение
  значений, далеких от mean, будет очень маловероятно. При слишком большой сигме
  и meam, близком к краю отрезка, в область высоких значений наротив попадет и сам край.
  Защиты от дурака в функции нет, так что пользоваться осторожно.
*/
int GaussIntNumber(double mean, double sigma, int min_val, int max_val)
{
	double dresult = 0.0;
	int iresult = 0;

	// получаем случайное число с заданным матожиданием и распределением
	dresult = Random::rnd.NormalDistributionNumber(mean, sigma);
	//(dresult < mean) ? (iresult = ceil(dresult)) : iresult = floor(dresult);
	//округляем
	iresult = ((dresult < mean) ? ceil(dresult) : floor(dresult));
	//возвращаем с обрезанием по диапазону
	return MAX(MIN(iresult, max_val), min_val);
}

