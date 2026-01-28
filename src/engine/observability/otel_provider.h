#ifndef BYLINS_OTEL_PROVIDER_H
#define BYLINS_OTEL_PROVIDER_H

#include <string>
#include <memory>

#ifdef WITH_OTEL
#include "opentelemetry/sdk/trace/tracer_provider.h"
#include "opentelemetry/sdk/metrics/meter_provider.h"
#include "opentelemetry/sdk/logs/logger_provider.h"
#include "opentelemetry/nostd/shared_ptr.h"

namespace otel = opentelemetry;
namespace trace_api = otel::trace;
namespace metrics_api = otel::metrics;
namespace logs_api = otel::logs;
namespace nostd = otel::nostd;
#endif

namespace observability {

// Forward declaration
class ILogSender;

class OtelProvider {
public:
    static OtelProvider& Instance();
    
    void Initialize(const std::string& metrics_endpoint,
                   const std::string& traces_endpoint,
                   const std::string& logs_endpoint,
                   const std::string& service_name,
                   const std::string& service_version);
    void Shutdown();
    
    bool IsEnabled() const { return m_enabled; }

#ifdef WITH_OTEL
    nostd::shared_ptr<trace_api::Tracer> GetTracer();
    nostd::shared_ptr<metrics_api::Meter> GetMeter();
    nostd::shared_ptr<logs_api::Logger> GetLogger();
#endif

private:
    OtelProvider();
    ~OtelProvider() = default;
    OtelProvider(const OtelProvider&) = delete;
    OtelProvider& operator=(const OtelProvider&) = delete;

    bool m_enabled = false;

#ifdef WITH_OTEL
    std::shared_ptr<otel::sdk::trace::TracerProvider> m_tracer_provider;
    std::shared_ptr<otel::sdk::metrics::MeterProvider> m_meter_provider;
    std::shared_ptr<otel::sdk::logs::LoggerProvider> m_logger_provider;
#endif
};

} // namespace observability

#endif // BYLINS_OTEL_PROVIDER_H
