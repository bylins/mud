/**
\file trade_log.cpp - a part of the Bylins engine.
\brief Единая телеметрия торговых площадок (базар и аукцион).
*/

#include "trade_log.h"

#include "engine/entities/entities_constants.h"
#include "engine/entities/obj_data.h"
#include "engine/observability/metrics.h"
#include "utils/logger.h"
#include "utils/utils_string.h"

#include <map>
#include <stdexcept>
#include <string>

namespace trade_log {

namespace {

using Attrs = std::map<std::string, std::string>;

// Событий торговли в syslog быть не должно -- они только для аналитики,
// поэтому тип перечислен в logging::kOtlpOnlyLogTypes и на диск не пишется.
constexpr const char *kTradeLogType = "trade";

const char *MarketName(EMarket market) {
	return market == EMarket::kAuction ? "auction" : "bazaar";
}

// Единственный атрибут предмета, который допустим в Prometheus: значений десятки.
std::string ObjTypeName(const ObjData *obj) {
	if (!obj) {
		return "unknown";
	}
	try {
		return NAME_BY_ITEM<EObjType>(obj->get_type());
	} catch (const std::out_of_range &) {
		return "unknown";
	}
}

// Название без цветовых кодов: с ними одна и та же вещь дала бы разные значения
// атрибута item, и группировка по товару в Grafana развалилась бы.
std::string ItemName(const ObjData *obj) {
	return obj ? utils::RemoveColors(obj->get_short_description()) : std::string("?");
}

int ItemVnum(const ObjData *obj) {
	return obj ? obj->get_vnum() : -1;
}

Attrs BaseAttrs(EMarket market, const char *event, int lot, const ObjData *obj, long price, long seller_id) {
	Attrs attrs{
		{"market", MarketName(market)},
		{"event", event},
		{"lot", std::to_string(lot)},
		{"price", std::to_string(price)},
		{"seller_id", std::to_string(seller_id)},
		{"obj_type", ObjTypeName(obj)},
		{"vnum", std::to_string(ItemVnum(obj))},
	};
	if (obj) {
		// Строка остаётся в KOI8-R: в UTF-8 её переведёт OTLP-сендер.
		attrs["item"] = ItemName(obj);
	}
	return attrs;
}

// Атрибуты метрик -- строго подмножество атрибутов лога с низкой кардинальностью.
Attrs MetricAttrs(EMarket market, const ObjData *obj) {
	return Attrs{
		{"market", MarketName(market)},
		{"obj_type", ObjTypeName(obj)},
	};
}

}  // namespace

void Listed(EMarket market, int lot, const ObjData *obj, long price, long seller_id) {
	log_event(kTradeLogType, BaseAttrs(market, "listed", lot, obj, price, seller_id),
			  "[trade] %s listed lot=%d vnum=%d price=%ld seller=%ld item=%s",
			  MarketName(market), lot, ItemVnum(obj), price, seller_id, ItemName(obj).c_str());

	observability::OtelMetrics::RecordCounter("trade.listed.total", 1, MetricAttrs(market, obj));
}

void Repriced(EMarket market, int lot, const ObjData *obj, long old_price, long new_price, long seller_id) {
	auto attrs = BaseAttrs(market, "repriced", lot, obj, new_price, seller_id);
	attrs["old_price"] = std::to_string(old_price);
	log_event(kTradeLogType, attrs,
			  "[trade] %s repriced lot=%d vnum=%d old_price=%ld price=%ld seller=%ld item=%s",
			  MarketName(market), lot, ItemVnum(obj), old_price, new_price, seller_id, ItemName(obj).c_str());

	observability::OtelMetrics::RecordCounter("trade.repriced.total", 1, MetricAttrs(market, obj));
}

void Bid(EMarket market, int lot, const ObjData *obj, long price, long seller_id, long buyer_id) {
	auto attrs = BaseAttrs(market, "bid", lot, obj, price, seller_id);
	attrs["buyer_id"] = std::to_string(buyer_id);
	log_event(kTradeLogType, attrs,
			  "[trade] %s bid lot=%d vnum=%d price=%ld seller=%ld buyer=%ld item=%s",
			  MarketName(market), lot, ItemVnum(obj), price, seller_id, buyer_id, ItemName(obj).c_str());

	observability::OtelMetrics::RecordCounter("trade.bid.total", 1, MetricAttrs(market, obj));
}

void Withdrawn(EMarket market, int lot, const ObjData *obj, long price, long seller_id, const char *reason) {
	const char *why = reason ? reason : "unknown";
	auto attrs = BaseAttrs(market, "withdrawn", lot, obj, price, seller_id);
	attrs["reason"] = why;
	log_event(kTradeLogType, attrs,
			  "[trade] %s withdrawn lot=%d vnum=%d price=%ld seller=%ld reason=%s item=%s",
			  MarketName(market), lot, ItemVnum(obj), price, seller_id, why, ItemName(obj).c_str());

	auto metric_attrs = MetricAttrs(market, obj);
	metric_attrs["reason"] = why;
	observability::OtelMetrics::RecordCounter("trade.withdrawn.total", 1, metric_attrs);
}

void Sold(EMarket market, int lot, const ObjData *obj, long price, long seller_id, long buyer_id, time_t listed_at) {
	const long listed_seconds = listed_at > 0 ? static_cast<long>(time(nullptr) - listed_at) : -1;

	auto attrs = BaseAttrs(market, "sold", lot, obj, price, seller_id);
	attrs["buyer_id"] = std::to_string(buyer_id);
	attrs["listed_seconds"] = std::to_string(listed_seconds);
	log_event(kTradeLogType, attrs,
			  "[trade] %s sold lot=%d vnum=%d price=%ld seller=%ld buyer=%ld listed_seconds=%ld item=%s",
			  MarketName(market), lot, ItemVnum(obj), price, seller_id, buyer_id, listed_seconds, ItemName(obj).c_str());

	const auto metric_attrs = MetricAttrs(market, obj);
	observability::OtelMetrics::RecordCounter("trade.sale.total", 1, metric_attrs);
	observability::OtelMetrics::RecordCounter("trade.turnover.total", price, metric_attrs);
	observability::OtelMetrics::RecordHistogram("trade.price", static_cast<double>(price), metric_attrs);
	if (listed_seconds >= 0) {
		observability::OtelMetrics::RecordHistogram("trade.time_to_sale.seconds",
												   static_cast<double>(listed_seconds),
												   {{"market", MarketName(market)}});
	}
}

void ActiveLots(EMarket market, int count, long total_value) {
	const Attrs attrs{{"market", MarketName(market)}};
	observability::OtelMetrics::RecordGauge("trade.lots.active", count, attrs);
	observability::OtelMetrics::RecordGauge("trade.lots.value", static_cast<double>(total_value), attrs);
}

}  // namespace trade_log

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
