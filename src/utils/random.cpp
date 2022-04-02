// $RCSfile$     $Date$     $$
// Copyright (c) 2008 Krodo
// Part of Bylins http://www.mud.ru

#include "random.h"
#include "utils/utils.h"

#include <random>

namespace Random {
class NormalRand {
 public:
	NormalRand() :
		_rng(_rd()) {}

	int number(int from, int to);
	//Нормальное распределение
	double NormalDistributionNumber(double mean, double sigma);
	bool BernoulliTrial(double prob);

 private:
	std::random_device _rd;
	std::mt19937 _rng;
};

NormalRand rnd;

int NormalRand::number(int from, int to) {
	std::uniform_int_distribution<> dist(from, to);
	return dist(_rng);
}

//функция возвращает случайное число с нормальным распределением
//с параметрами mean - матожидание и sigma - дисперсия
double NormalRand::NormalDistributionNumber(double mean, double sigma) {
	std::normal_distribution<double> NormalDistribution(mean, sigma);
	return NormalDistribution(_rng);
}

bool NormalRand::BernoulliTrial(double p) {
	std::binomial_distribution<> dist(p);
	return dist(_rng);
}
} // namespace Random

// править везде вызовы, чтобы занести эти две функции в неймспейс меня чет обломало

// * Генерация рандомного числа в диапазоне от from до to.
int number(int from, int to) {
	if (from > to) {
		int tmp = from;
		from = to;
		to = tmp;
	}

	return Random::rnd.number(from, to);
}

// * Аналог кидания кубиков.
int RollDices(int number, int size) {
	if (size <= 0 || number <= 0)
		return 0;

	int sum = 0;

	while (number--)
		sum += Random::rnd.number(1, size);

	return sum;
}

bool bernoulli_trial(double p) {
	return Random::rnd.BernoulliTrial(p);
}

/**
* Генерация целого числа в заданном диапазоне с нормальным распределением
  Вообще говоря, это все кривовато, но будет по идее работать примерно как надо.
  Сигму надо подбирать экспериментально. При слишком малых значениях выпадение
  значений, далеких от mean, будет очень маловероятно. При слишком большой сигме
  и meam, близком к краю отрезка, в область высоких значений наротив попадет и сам край.
  Защиты от дурака в функции нет, так что пользоваться осторожно.
*/
int GaussIntNumber(double mean, double sigma, int min_val, int max_val) {
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


// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
