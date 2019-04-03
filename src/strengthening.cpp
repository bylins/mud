#include "strengthening.hpp"

Strengthening::Strengthening()
{
    init();
}

int Strengthening::operator()(percentage_t percentage, Type type) const
{
    const auto cell_i = m_strengthening_table.find(std::make_pair(percentage, type));
    if (cell_i == m_strengthening_table.end())
    {
        log("ERROR: couldn't find cell <%d, %d> in the stengthening table. 0 will be returned.\n",
            percentage, static_cast<int>(type));

        return 0;
    }

    return cell_i->second;
}

void Strengthening::init()
{
//		%	таймер+	броня+	поглощение	здоровье	живучесть	стойкость	огня	воздуха	воды	земли
	static int table[][11] = {
		{100, 100, 1, 0, 0, 0, 0, 0, 0, 0, 0},
		{110, 102, 2, 0, 0, 0, 0, 0, 0, 0, 0},
		{120, 104, 2, 2, 2, 2, 2, 2, 2, 2, 2},
		{130, 106, 3, 2, 3, 3, 3, 3, 3, 3, 3},
		{140, 108, 3, 2, 3, 3, 3, 3, 3, 3, 3},
		{150, 110, 4, 4, 5, 5, 5, 5, 5, 5, 5},
		{160, 112, 4, 4, 5, 5, 5, 5, 5, 5, 5},
		{170, 114, 5, 4, 5, 5, 5, 5, 5, 5, 5},
		{180, 116, 5, 7, 7, 7, 7, 7, 7, 7, 7},
		{190, 118, 6, 7, 7, 7, 7, 7, 7, 7, 7},
		{200, 120, 6, 9, 10, 10, 10, 10, 10, 10, 10}
	};

//заполним таблицу
	for (int i = 0; i != 11; ++i)
	{
		for (int j = 1; j != 11; ++j)
		{
			m_strengthening_table.emplace(std::make_pair(table[i][0], static_cast<Type>(j - 1)), table[i][j]);
		}
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :

