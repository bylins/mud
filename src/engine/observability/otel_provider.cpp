#include "otel_provider.h"
#include "otel_helpers.h"
#include "utils/timestamp.h"
#include "engine/core/config.h"
#include "utils/logging/log_manager.h"
#include "otel_log_sender.h"
#include "otel_trace_sender.h"
#include "utils/tracing/trace_manager.h"

#include <iostream>
#include <chrono>
#include <ctime>
#include <cstdio>

#ifdef WITH_OTEL
#include "absl/log/initialize.h"
#include "opentelemetry/sdk/trace/tracer_provider.h"
#include "opentelemetry/sdk/trace/tracer_provider_factory.h"
#include "opentelemetry/sdk/trace/batch_span_processor_factory.h"
#include "opentelemetry/sdk/metrics/meter_provider.h"
#include "opentelemetry/sdk/metrics/meter_provider_factory.h"
#include "opentelemetry/sdk/metrics/meter_context_factory.h"
#include "opentelemetry/sdk/logs/logger_provider.h"
#include "opentelemetry/sdk/logs/logger_provider_factory.h"
#include "opentelemetry/sdk/logs/batch_log_record_processor_factory.h"
#include "opentelemetry/exporters/otlp/otlp_http_exporter_factory.h"
#include "opentelemetry/exporters/otlp/otlp_http_exporter_options.h"
#include "opentelemetry/exporters/otlp/otlp_http_metric_exporter_factory.h"
#include "opentelemetry/exporters/otlp/otlp_http_metric_exporter_options.h"
#include "opentelemetry/exporters/otlp/otlp_http_log_record_exporter_factory.h"
#include "opentelemetry/exporters/otlp/otlp_http_log_record_exporter_options.h"
#include "opentelemetry/sdk/metrics/export/periodic_exporting_metric_reader_factory.h"
#include "opentelemetry/sdk/metrics/export/periodic_exporting_metric_reader_options.h"
#include "opentelemetry/sdk/resource/resource.h"
#include "opentelemetry/trace/provider.h"
#include "opentelemetry/metrics/provider.h"
#include "opentelemetry/logs/provider.h"

namespace otel = opentelemetry;
#endif

namespace observability {

struct OtelProvider::Providers {
#ifdef WITH_OTEL
    std::shared_ptr<otel::sdk::trace::TracerProvider> tracer;
    std::shared_ptr<otel::sdk::metrics::MeterProvider> meter;
    std::shared_ptr<otel::sdk::logs::LoggerProvider> logger;
#endif
};

OtelProvider& OtelProvider::Instance() {
    static OtelProvider instance;
    return instance;
}

OtelProvider::OtelProvider() : m_providers(std::make_unique<Providers>()) {}

OtelProvider::~OtelProvider() = default;

void OtelProvider::Initialize(const std::string& metrics_endpoint,
                             const std::string& traces_endpoint,
                             const std::string& logs_endpoint,
                             const std::string& service_name,
                             const std::string& service_version) {
#ifdef WITH_OTEL
    if (m_enabled) {
        return; // Already initialized
    }

    try {
        absl::InitializeLog();

        auto resource = otel::sdk::resource::Resource::Create({
            {"service.name", service_name},
            {"service.version", service_version}
        });

        {
            otel::exporter::otlp::OtlpHttpExporterOptions trace_options;
            trace_options.url = traces_endpoint;

            auto exporter = otel::exporter::otlp::OtlpHttpExporterFactory::Create(trace_options);
            auto processor = otel::sdk::trace::BatchSpanProcessorFactory::Create(std::move(exporter), {});
            m_providers->tracer = otel::sdk::trace::TracerProviderFactory::Create(std::move(processor), resource);

            std::shared_ptr<otel::trace::TracerProvider> api_provider = m_providers->tracer;
            otel::trace::Provider::SetTracerProvider(api_provider);
        }

        {
            otel::exporter::otlp::OtlpHttpMetricExporterOptions metric_options;
            metric_options.url = metrics_endpoint;

            auto exporter = otel::exporter::otlp::OtlpHttpMetricExporterFactory::Create(metric_options);

            otel::sdk::metrics::PeriodicExportingMetricReaderOptions reader_options;
            reader_options.export_interval_millis = std::chrono::milliseconds(5000);
            reader_options.export_timeout_millis = std::chrono::milliseconds(3000);

            auto reader = otel::sdk::metrics::PeriodicExportingMetricReaderFactory::Create(
                std::move(exporter), reader_options
            );

            auto meter_context = otel::sdk::metrics::MeterContextFactory::Create();
            meter_context->AddMetricReader(std::move(reader));

            m_providers->meter = otel::sdk::metrics::MeterProviderFactory::Create(std::move(meter_context));

            std::shared_ptr<otel::metrics::MeterProvider> api_provider = m_providers->meter;
            otel::metrics::Provider::SetMeterProvider(api_provider);
        }

        {
            otel::exporter::otlp::OtlpHttpLogRecordExporterOptions log_options;
            log_options.url = logs_endpoint;

            auto exporter = otel::exporter::otlp::OtlpHttpLogRecordExporterFactory::Create(log_options);

            otel::sdk::logs::BatchLogRecordProcessorOptions processor_options;
            processor_options.max_queue_size = 2048;
            processor_options.schedule_delay_millis = std::chrono::milliseconds(5000);
            processor_options.max_export_batch_size = 512;

            auto processor = otel::sdk::logs::BatchLogRecordProcessorFactory::Create(
                std::move(exporter), processor_options
            );

            m_providers->logger = otel::sdk::logs::LoggerProviderFactory::Create(std::move(processor), resource);

            std::shared_ptr<otel::logs::LoggerProvider> api_provider = m_providers->logger;
            otel::logs::Provider::SetLoggerProvider(api_provider);
        }

        const auto mode = ::runtime_config.telemetry_log_mode();

        if (mode == RuntimeConfiguration::ETelemetryLogMode::kDuplicate) {
            logging::LogManager::Instance().AddSender(std::make_unique<OtelLogSender>());
            std::cout << "[" << utils::NowTs() << "] Log mode: duplicate (file + OTEL)" << std::endl;
        } else if (mode == RuntimeConfiguration::ETelemetryLogMode::kOtelOnly) {
            logging::LogManager::Instance().ClearSenders();
            logging::LogManager::Instance().AddSender(std::make_unique<OtelLogSender>());
            std::cout << "[" << utils::NowTs() << "] Log mode: otel-only" << std::endl;
        } else {
            std::cout << "[" << utils::NowTs() << "] Log mode: file-only (OTEL initialized but not used for logs)" << std::endl;
        }

        tracing::TraceManager::Instance().SetSender(std::make_unique<tracing::OtelTraceSender>());
        std::cout << "[" << utils::NowTs() << "] TraceManager initialized with OtelTraceSender" << std::endl;

        m_enabled = true;
        std::cout << "[" << utils::NowTs() << "] OpenTelemetry initialized successfully:" << std::endl;
        std::cout << "  Metrics: " << metrics_endpoint << std::endl;
        std::cout << "  Traces:  " << traces_endpoint << std::endl;
        std::cout << "  Logs:    " << logs_endpoint << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[" << utils::NowTs() << "] Failed to initialize OpenTelemetry: " << e.what() << std::endl;
        m_enabled = false;
        tracing::TraceManager::Instance().SetSender(std::make_unique<tracing::NoOpTraceSender>());
        std::cout << "[" << utils::NowTs() << "] TraceManager initialized with NoOpTraceSender (OTEL init failed)" << std::endl;
    }
#else
    (void)metrics_endpoint;
    (void)traces_endpoint;
    (void)logs_endpoint;
    (void)service_name;
    (void)service_version;
    tracing::TraceManager::Instance().SetSender(std::make_unique<tracing::NoOpTraceSender>());
    std::cout << "[" << utils::NowTs() << "] TraceManager initialized with NoOpTraceSender (no OTEL)" << std::endl;
#endif
}

void OtelProvider::Shutdown() {
#ifdef WITH_OTEL
    if (!m_enabled) {
        return;
    }

    try {
        if (m_providers->tracer) {
            m_providers->tracer->ForceFlush();
            m_providers->tracer->Shutdown();
        }
        if (m_providers->meter) {
            m_providers->meter->ForceFlush();
            m_providers->meter->Shutdown();
        }
        if (m_providers->logger) {
            m_providers->logger->ForceFlush();
            m_providers->logger->Shutdown();
        }

        m_enabled = false;
        std::cout << "[" << utils::NowTs() << "] OpenTelemetry shutdown successfully" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[" << utils::NowTs() << "] Error during OpenTelemetry shutdown: " << e.what() << std::endl;
    }
#endif
}

} // namespace observability

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
