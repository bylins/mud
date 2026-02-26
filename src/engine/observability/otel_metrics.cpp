#include "otel_metrics.h"
#include "otel_provider.h"
#include <unordered_map>
#include <memory>

#ifdef WITH_OTEL
#include "otel_helpers.h"
#include "opentelemetry/metrics/provider.h"
#include "opentelemetry/context/context.h"
#endif

namespace observability {

#ifdef WITH_OTEL
static std::unordered_map<std::string, std::unique_ptr<opentelemetry::metrics::Histogram<double>>> histogram_cache;

// Convert all string attribute values from KOI8-R to UTF-8 at the wrapper boundary.
// This ensures callers never need to manually convert - the OtelMetrics API handles it.
static std::map<std::string, std::string> ToUtf8Attrs(const std::map<std::string, std::string>& attrs) {
    std::map<std::string, std::string> result;
    for (const auto& [k, v] : attrs) {
        result[k] = koi8r_to_utf8(v);
    }
    return result;
}
#endif

void OtelMetrics::RecordCounter(const std::string& name, int64_t value) {
#ifdef WITH_OTEL
    if (OtelProvider::Instance().IsEnabled()) {
        auto meter = OtelProvider::Instance().GetMeter();
        if (meter) {
            if (value >= 0) {
                auto counter = meter->CreateUInt64Counter(name);
                counter->Add(static_cast<uint64_t>(value));
            }
        }
    }
#endif
    (void)name;
    (void)value;
}

void OtelMetrics::RecordCounter(const std::string& name, int64_t value,
                               const std::map<std::string, std::string>& attributes) {
#ifdef WITH_OTEL
    if (OtelProvider::Instance().IsEnabled()) {
        auto meter = OtelProvider::Instance().GetMeter();
        if (meter) {
            if (value >= 0) {
                auto counter = meter->CreateUInt64Counter(name);
                counter->Add(static_cast<uint64_t>(value), ToUtf8Attrs(attributes));
            }
        }
    }
#endif
    (void)name;
    (void)value;
    (void)attributes;
}

void OtelMetrics::RecordHistogram(const std::string& name, double value) {
#ifdef WITH_OTEL
    if (OtelProvider::Instance().IsEnabled()) {
        auto meter = OtelProvider::Instance().GetMeter();
        if (meter) {
            auto histogram = meter->CreateDoubleHistogram(name);
            auto context = opentelemetry::context::Context{};
            histogram->Record(value, context);
        }
    }
#endif
    (void)name;
    (void)value;
}

void OtelMetrics::RecordHistogram(const std::string& name, double value,
                                 const std::map<std::string, std::string>& attributes) {
#ifdef WITH_OTEL
    if (OtelProvider::Instance().IsEnabled()) {
        auto meter = OtelProvider::Instance().GetMeter();
        if (meter) {
            // Cache histogram instruments to avoid recreation
            auto it = histogram_cache.find(name);
            opentelemetry::metrics::Histogram<double>* histogram = nullptr;
            if (it == histogram_cache.end()) {
                auto h = meter->CreateDoubleHistogram(name);
                histogram = h.get();
                histogram_cache[name] = std::move(h);
            } else {
                histogram = it->second.get();
            }
            auto context = opentelemetry::context::Context{};
            histogram->Record(value, ToUtf8Attrs(attributes), context);
        }
    }
#endif
    (void)name;
    (void)value;
    (void)attributes;
}

void OtelMetrics::RecordGauge(const std::string& name, double value) {
#ifdef WITH_OTEL
    if (OtelProvider::Instance().IsEnabled()) {
        auto meter = OtelProvider::Instance().GetMeter();
        if (meter) {
            // Gauges in OTEL require callbacks, so we'll use histogram instead
            auto histogram = meter->CreateDoubleHistogram(name + ".gauge");
            auto context = opentelemetry::context::Context{};
            histogram->Record(value, context);
        }
    }
#endif
    (void)name;
    (void)value;
}

void OtelMetrics::RecordGauge(const std::string& name, double value,
                             const std::map<std::string, std::string>& attributes) {
#ifdef WITH_OTEL
    if (OtelProvider::Instance().IsEnabled()) {
        auto meter = OtelProvider::Instance().GetMeter();
        if (meter) {
            auto histogram = meter->CreateDoubleHistogram(name + ".gauge");
            auto context = opentelemetry::context::Context{};
            histogram->Record(value, ToUtf8Attrs(attributes), context);
        }
    }
#endif
    (void)name;
    (void)value;
    (void)attributes;
}

} // namespace observability
