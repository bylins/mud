// $RCSfile$     $Date$     $$
// Copyright (c) 2008 Krodo
// Part of Bylins http://www.mud.ru

#ifndef RANDOM_HPP_INCLUDED
#define RANDOM_HPP_INCLUDED

int number(int from, int to);
int RollDices(int number, int size);
int GaussIntNumber(double mean, double sigma, int min_val, int max_val);
bool BernoulliTrial(double p);

// Sets the seed of the global RNG used by number()/RollDices()/etc.
// Intended for the headless balance simulator to make runs reproducible.
void SetRandomSeed(unsigned value);

#endif // RANDOM_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
