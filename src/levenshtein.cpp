#include "levenshtein.hpp"

#include <vector>
#include <memory>

/*
* Implementation of this function was taken from Git sources and adapted for C++.
*/

/*
* This function implements the Damerau-Levenshtein algorithm to
* calculate a distance between strings.
*
* Basically, it says how many letters need to be swapped, substituted,
* deleted from, or added to string1, at least, to get string2.
*
* The idea is to build a distance matrix for the substrings of both
* strings.  To avoid a large space complexity, only the last three rows
* are kept in memory (if swaps had the same or higher cost as one deletion
* plus one insertion, only two rows would be needed).
*
* At any stage, "i + 1" denotes the length of the current substring of
* string1 that the distance is calculated for.
*
* row2 holds the current row, row1 the previous row (i.e. for the substring
* of string1 of length "i"), and row0 the row before that.
*
* In other words, at the start of the big loop, row2[j + 1] contains the
* Damerau-Levenshtein distance between the substring of string1 of length
* "i" and the substring of string2 of length "j + 1".
*
* All the big loop does is determine the partial minimum-cost paths.
*
* It does so by calculating the costs of the path ending in characters
* i (in string1) and j (in string2), respectively, given that the last
* operation is a substitution, a swap, a deletion, or an insertion.
*
* This implementation allows the costs to be weighted:
*
* - w (as in "sWap")
* - s (as in "Substitution")
* - a (for insertion, AKA "Add")
* - d (as in "Deletion")
*
* Note that this algorithm calculates a distance _iff_ d == a.
*/
int levenshtein(const std::string& string1, const std::string& string2, int w, int s, int a, int d)
{
	const std::size_t len1 = string1.size();
	const std::size_t len2 = string2.size();
	
	using array_t = std::vector<int>;
	using array_ptr_t = std::shared_ptr<array_t>;

	array_ptr_t guard0 = std::make_shared<array_t>(len2 + 1);
	auto row0 = &(*guard0)[0];
	array_ptr_t guard1 = std::make_shared<array_t>(len2 + 1);
	auto row1 = &(*guard1)[0];
	array_ptr_t guard2 = std::make_shared<array_t>(len2 + 1);
	auto row2 = &(*guard2)[0];

	for (auto j = 0u; j <= len2; j++)
	{
		row1[j] = j * a;
	}

	for (auto i = 0u; i < len1; i++)
	{
		row2[0] = (i + 1) * d;
		for (auto j = 0u; j < len2; j++)
		{
			/* substitution */
			row2[j + 1] = row1[j] + s * (string1[i] != string2[j]);

			/* swap */
			if (i > 0 && j > 0 && string1[i - 1] == string2[j]
				&& string1[i] == string2[j - 1]
				&& row2[j + 1] > row0[j - 1] + w)
			{
				row2[j + 1] = row0[j - 1] + w;
			}

			/* deletion */
			if (row2[j + 1] > row1[j + 1] + d)
			{
				row2[j + 1] = row1[j + 1] + d;
			}

			/* insertion */
			if (row2[j + 1] > row2[j] + a)
			{
				row2[j + 1] = row2[j] + a;
			}
		}

		const auto dummy = row0;
		row0 = row1;
		row1 = row2;
		row2 = dummy;
	}

	const auto result = row1[len2];

	return result;
}

/* vim: set ts=4 sw=4 tw=0 noet syntax=cpp :*/
