#include "otel_provider.h"
#include "engine/core/config.h"
#include "utils/logging/log_manager.h"
#include "otel_log_sender.h"

#ifdef WITH_OTEL
#include "opentelemetry/sdk/trace/tracer_provider_factory.h"
#include "opentelemetry/sdk/trace/batch_span_processor_factory.h"
#include "opentelemetry/sdk/metrics/meter_provider_factory.h"
#include "opentelemetry/sdk/metrics/meter_context_factory.h"
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

#include <iostream>
#include <chrono>
#endif

namespace observability {

OtelProvider& OtelProvider::Instance() {
    static OtelProvider instance;
    return instance;
}

OtelProvider::OtelProvider() {
    // Constructor - log senders are managed by LogManager
    // See Initialize() for OTEL log sender registration
}

void OtelProvider::Initialize(const std::string& endpoint,
                             const std::string& service_name,
                             const std::string& service_version) {
#ifdef WITH_OTEL
    if (m_enabled) {
        return; // Already initialized
    }

    try {
        // Create resource attributes
        auto resource = otel::sdk::resource::Resource::Create({
            {"service.name", service_name},
            {"service.version", service_version}
        });

        // Initialize TracerProvider with OTLP HTTP exporter
        // Based on examples/otlp/http_log_main.cc
        {
            otel::exporter::otlp::OtlpHttpExporterOptions trace_options;
            trace_options.url = endpoint + "/v1/traces";

            auto exporter = otel::exporter::otlp::OtlpHttpExporterFactory::Create(trace_options);
            auto processor = otel::sdk::trace::BatchSpanProcessorFactory::Create(std::move(exporter), {});
            m_tracer_provider = otel::sdk::trace::TracerProviderFactory::Create(std::move(processor), resource);

            // Set as global provider
            std::shared_ptr<otel::trace::TracerProvider> api_provider = m_tracer_provider;
            otel::trace::Provider::SetTracerProvider(api_provider);
        }

        // Initialize MeterProvider with OTLP HTTP exporter
        // Based on examples/otlp/http_metric_main.cc
        {
            otel::exporter::otlp::OtlpHttpMetricExporterOptions metric_options;
            metric_options.url = endpoint + "/v1/metrics";

            auto exporter = otel::exporter::otlp::OtlpHttpMetricExporterFactory::Create(metric_options);

            otel::sdk::metrics::PeriodicExportingMetricReaderOptions reader_options;
            reader_options.export_interval_millis = std::chrono::milliseconds(5000);
            reader_options.export_timeout_millis = std::chrono::milliseconds(30000);

            auto reader = otel::sdk::metrics::PeriodicExportingMetricReaderFactory::Create(
                std::move(exporter), reader_options
            );

            // Create context and add reader
            auto meter_context = otel::sdk::metrics::MeterContextFactory::Create();
            meter_context->AddMetricReader(std::move(reader));

            // Create provider from context
            auto u_provider = otel::sdk::metrics::MeterProviderFactory::Create(std::move(meter_context));
            m_meter_provider = std::move(u_provider);

            // Set as global provider
            std::shared_ptr<otel::metrics::MeterProvider> api_provider = m_meter_provider;
            otel::metrics::Provider::SetMeterProvider(api_provider);
        }

        // Initialize LoggerProvider with OTLP HTTP exporter
        // Based on examples/otlp/http_log_main.cc
        {
            otel::exporter::otlp::OtlpHttpLogRecordExporterOptions log_options;
            log_options.url = endpoint + "/v1/logs";

            auto exporter = otel::exporter::otlp::OtlpHttpLogRecordExporterFactory::Create(log_options);

            otel::sdk::logs::BatchLogRecordProcessorOptions processor_options;
            processor_options.max_queue_size = 2048;
            processor_options.schedule_delay_millis = std::chrono::milliseconds(5000);
            processor_options.max_export_batch_size = 512;

            auto processor = otel::sdk::logs::BatchLogRecordProcessorFactory::Create(
                std::move(exporter), processor_options
            );

            m_logger_provider = otel::sdk::logs::LoggerProviderFactory::Create(std::move(processor), resource);

            // Set as global provider
            std::shared_ptr<otel::logs::LoggerProvider> api_provider = m_logger_provider;
            otel::logs::Provider::SetLoggerProvider(api_provider);
        }

        // Register OTEL log sender with LogManager based on config mode
        const auto mode = ::runtime_config.telemetry_log_mode();

        if (mode == RuntimeConfiguration::ETelemetryLogMode::kDuplicate) {
            // File sender already registered by LogManager constructor, add OTEL
            logging::LogManager::Instance().AddSender(std::make_unique<OtelLogSender>());
            std::cout << "Log mode: duplicate (file + OTEL)" << std::endl;
        } else if (mode == RuntimeConfiguration::ETelemetryLogMode::kOtelOnly) {
            // Replace file sender with OTEL only
            logging::LogManager::Instance().ClearSenders();
            logging::LogManager::Instance().AddSender(std::make_unique<OtelLogSender>());
            std::cout << "Log mode: otel-only" << std::endl;
        } else {
            // kFileOnly - no OTEL senders
            // File sender already registered by LogManager constructor, do nothing
            std::cout << "Log mode: file-only (OTEL initialized but not used for logs)" << std::endl;
        }

        m_enabled = true;
        std::cout << "OpenTelemetry initialized successfully with endpoint: " << endpoint << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize OpenTelemetry: " << e.what() << std::endl;
        m_enabled = false;
    }
#else
    (void)endpoint;
    (void)service_name;
    (void)service_version;
#endif
}

void OtelProvider::Shutdown() {
#ifdef WITH_OTEL
    if (!m_enabled) {
        return;
    }

    try {
        // Shutdown providers to flush remaining telemetry
        if (m_tracer_provider) {
            m_tracer_provider->ForceFlush();
            m_tracer_provider->Shutdown();
        }
        if (m_meter_provider) {
            m_meter_provider->ForceFlush();
            m_meter_provider->Shutdown();
        }
        if (m_logger_provider) {
            m_logger_provider->ForceFlush();
            m_logger_provider->Shutdown();
        }

        m_enabled = false;
        std::cout << "OpenTelemetry shutdown successfully" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error during OpenTelemetry shutdown: " << e.what() << std::endl;
    }
#endif
}

#ifdef WITH_OTEL
nostd::shared_ptr<trace_api::Tracer> OtelProvider::GetTracer() {
    if (!m_enabled || !m_tracer_provider) {
        return nostd::shared_ptr<trace_api::Tracer>(nullptr);
    }
    return m_tracer_provider->GetTracer("bylins-tracer", "1.0.0");
}

nostd::shared_ptr<metrics_api::Meter> OtelProvider::GetMeter() {
    if (!m_enabled || !m_meter_provider) {
        return nostd::shared_ptr<metrics_api::Meter>(nullptr);
    }
    return m_meter_provider->GetMeter("bylins-meter", "1.0.0");
}

nostd::shared_ptr<logs_api::Logger> OtelProvider::GetLogger() {
    if (!m_enabled || !m_logger_provider) {
        return nostd::shared_ptr<logs_api::Logger>(nullptr);
    }
    return m_logger_provider->GetLogger("bylins-logger", "", "", "");
}
#endif

} // namespace observability

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
