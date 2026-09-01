/**
\file trade_log.h - a part of the Bylins engine.
\brief Единая телеметрия торговых площадок (базар и аукцион).
\detail Один механизм на оба рынка: событие уезжает по OTLP в Loki,
        машиночитаемые поля -- в атрибутах (structured metadata), поэтому в
        Grafana они фильтруются и агрегируются без regex. На диск события не
        пишутся: это телеметрия для аналитики, а не syslog для чтения глазами.

        Разделение труда между логом и метриками принципиальное:
        * имя предмета и внум идут ТОЛЬКО в лог -- это тысячи значений,
          в лейбле Prometheus они бы навсегда размножили серии в TSDB;
        * в метрики уходит лишь тип предмета (десятки значений) и рынок.
        Тренды по конкретным товарам считаются в Loki: sum by (item) (...).
*/

#ifndef BYLINS_SRC_GAMEPLAY_ECONOMICS_TRADE_LOG_H_
#define BYLINS_SRC_GAMEPLAY_ECONOMICS_TRADE_LOG_H_

#include <ctime>

class ObjData;

namespace trade_log {

enum class EMarket {
	kBazaar,
	kAuction
};

// Лот выставлен на продажу.
void Listed(EMarket market, int lot, const ObjData *obj, long price, long seller_id);

// Владелец изменил цену лота.
void Repriced(EMarket market, int lot, const ObjData *obj, long old_price, long new_price, long seller_id);

// Новая ставка (аукцион).
void Bid(EMarket market, int lot, const ObjData *obj, long price, long seller_id, long buyer_id);

// Лот ушёл с площадки без продажи.
// reason: "owner", "god", "no_demand", "trader".
void Withdrawn(EMarket market, int lot, const ObjData *obj, long price, long seller_id, const char *reason);

// Сделка состоялась. listed_at -- момент выставления (0, если неизвестен):
// по нему считается время до продажи, то есть ликвидность товара.
void Sold(EMarket market, int lot, const ObjData *obj, long price, long seller_id, long buyer_id, time_t listed_at);

// Снимок витрины: сколько лотов стоит и на какую сумму.
void ActiveLots(EMarket market, int count, long total_value);

}  // namespace trade_log

#endif  // BYLINS_SRC_GAMEPLAY_ECONOMICS_TRADE_LOG_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
