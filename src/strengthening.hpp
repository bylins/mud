#ifndef __STRENGTHENING_HPP__
#define __STRENGTHENING_HPP__

#include "logger.hpp"

#include <unordered_map>

class Strengthening
{
public:
    enum Type
    {
        TIMER,
        ARMOR,
        ABSORBTION,
        HEALTH,
        VITALITY,
        STAMINA,
        FIRE_PROTECTION,
        AIR_PROTECTION ,
        WATER_PROTECTION,
        EARTH_PROTECTION,
	REFLEX
    };

    using percentage_t = int;
    using param_id_t = std::pair<percentage_t, Type>;

    Strengthening();

    int operator()(percentage_t percentage, Type type) const;

private:
    struct ParamHash {
        std::size_t operator() (const param_id_t& p) const {
            auto h1 = std::hash<param_id_t::first_type>{}(p.first);
            auto h2 = std::hash<param_id_t::second_type>{}(p.second);

            return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
        }
    };

    // <%, type> -> strengthening value
    using storage_t = std::unordered_map<param_id_t, int, ParamHash>;

    void init();

    storage_t m_strengthening_table;
};

#endif // __STRENGTHENING_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :