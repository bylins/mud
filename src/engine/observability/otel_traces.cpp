#include "otel_traces.h"
#include "otel_provider.h"
#include "otel_helpers.h"
#include "opentelemetry/trace/provider.h"

namespace observability {

Span::Span(opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span> span) : m_span(span) {}

void Span::End() {
    if (m_span) {
        m_span->End();
    }
}

void Span::AddEvent(const std::string& name) {
    if (m_span) {
        m_span->AddEvent(koi8r_to_utf8(name));
    }
}

void Span::SetAttribute(const std::string& key, const std::string& value) {
    if (m_span) {
        m_span->SetAttribute(key, koi8r_to_utf8(value));
    }
}

void Span::SetAttribute(const std::string& key, int64_t value) {
    if (m_span) {
        m_span->SetAttribute(key, value);
    }
}

void Span::SetAttribute(const std::string& key, double value) {
    if (m_span) {
        m_span->SetAttribute(key, value);
    }
}

Span OtelTraces::StartSpan(const std::string& name) {
    if (OtelProvider::Instance().IsEnabled()) {
        auto tracer = opentelemetry::trace::Provider::GetTracerProvider()->GetTracer("bylins-tracer", "1.0.0");
        if (tracer) {
            return Span(tracer->StartSpan(koi8r_to_utf8(name)));
        }
    }
    return Span();
}

Span OtelTraces::StartSpan(const std::string& name,
                          const std::map<std::string, std::string>& attributes) {
    if (OtelProvider::Instance().IsEnabled()) {
        auto tracer = opentelemetry::trace::Provider::GetTracerProvider()->GetTracer("bylins-tracer", "1.0.0");
        if (tracer) {
            auto span = tracer->StartSpan(koi8r_to_utf8(name));
            for (const auto& attr : attributes) {
                span->SetAttribute(attr.first, koi8r_to_utf8(attr.second));
            }
            return Span(span);
        }
    }
    return Span();
}

} // namespace observability

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
