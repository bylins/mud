#include "metrics.h"
#include "provider.h"
#include <unordered_map>
#include <memory>

#ifdef WITH_OTEL
#include "helpers.h"
#include "opentelemetry/metrics/provider.h"
#include "opentelemetry/context/context.h"

namespace metrics_api = opentelemetry::metrics;
#endif

namespace observability {

#ifdef WITH_OTEL
static std::unordered_map<std::string, std::unique_ptr<opentelemetry::metrics::Histogram<double>>> histogram_cache;

#endif

void OtelMetrics::RecordCounter(const std::string& name, int64_t value) {
#ifdef WITH_OTEL
    if (OtelProvider::Instance().IsEnabled()) {
        auto meter = metrics_api::Provider::GetMeterProvider()->GetMeter("bylins-meter", "1.0.0");
        if (meter) {
            if (value >= 0) {
                auto counter = meter->CreateUInt64Counter(name);
                counter->Add(static_cast<uint64_t>(value));
            }
        }
    }
#else
    (void)name;
    (void)value;
#endif
}

void OtelMetrics::RecordCounter(const std::string& name, int64_t value,
                               const std::map<std::string, std::string>& attributes) {
#ifdef WITH_OTEL
    if (OtelProvider::Instance().IsEnabled()) {
        auto meter = metrics_api::Provider::GetMeterProvider()->GetMeter("bylins-meter", "1.0.0");
        if (meter) {
            if (value >= 0) {
                auto counter = meter->CreateUInt64Counter(name);
                counter->Add(static_cast<uint64_t>(value), attributes);
            }
        }
    }
#else
    (void)name;
    (void)value;
    (void)attributes;
#endif
}

void OtelMetrics::RecordHistogram(const std::string& name, double value) {
#ifdef WITH_OTEL
    if (OtelProvider::Instance().IsEnabled()) {
        auto meter = metrics_api::Provider::GetMeterProvider()->GetMeter("bylins-meter", "1.0.0");
        if (meter) {
            auto histogram = meter->CreateDoubleHistogram(name);
            auto context = opentelemetry::context::Context{};
            histogram->Record(value, context);
        }
    }
#else
    (void)name;
    (void)value;
#endif
}

void OtelMetrics::RecordHistogram(const std::string& name, double value,
                                 const std::map<std::string, std::string>& attributes) {
#ifdef WITH_OTEL
    if (OtelProvider::Instance().IsEnabled()) {
        auto meter = metrics_api::Provider::GetMeterProvider()->GetMeter("bylins-meter", "1.0.0");
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
            histogram->Record(value, attributes, context);
        }
    }
#else
    (void)name;
    (void)value;
    (void)attributes;
#endif
}

void OtelMetrics::RecordGauge(const std::string& name, double value) {
#ifdef WITH_OTEL
    if (OtelProvider::Instance().IsEnabled()) {
        auto meter = metrics_api::Provider::GetMeterProvider()->GetMeter("bylins-meter", "1.0.0");
        if (meter) {
            // Gauges in OTEL require callbacks, so we'll use histogram instead
            auto histogram = meter->CreateDoubleHistogram(name + ".gauge");
            auto context = opentelemetry::context::Context{};
            histogram->Record(value, context);
        }
    }
#else
    (void)name;
    (void)value;
#endif
}

void OtelMetrics::RecordGauge(const std::string& name, double value,
                             const std::map<std::string, std::string>& attributes) {
#ifdef WITH_OTEL
    if (OtelProvider::Instance().IsEnabled()) {
        auto meter = metrics_api::Provider::GetMeterProvider()->GetMeter("bylins-meter", "1.0.0");
        if (meter) {
            auto histogram = meter->CreateDoubleHistogram(name + ".gauge");
            auto context = opentelemetry::context::Context{};
            histogram->Record(value, attributes, context);
        }
    }
#else
    (void)name;
    (void)value;
    (void)attributes;
#endif
}

} // namespace observability
