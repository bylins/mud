#include "otel_traces.h"
#include "otel_provider.h"

#ifdef WITH_OTEL
#include "opentelemetry/trace/provider.h"
#endif

namespace observability {

#ifdef WITH_OTEL
Span::Span(opentelemetry::nostd::shared_ptr<trace_api::Span> span) : m_span(span) {}

void Span::End() {
    if (m_span) {
        m_span->End();
    }
}

void Span::AddEvent(const std::string& name) {
    if (m_span) {
        std::string utf8_name = Koi8ToUtf8(name.c_str());
        m_span->AddEvent(utf8_name);
    }
}

void Span::SetAttribute(const std::string& key, const std::string& value) {
    if (m_span) {
        std::string utf8_value = Koi8ToUtf8(value.c_str());
        m_span->SetAttribute(key, utf8_value);
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
#endif

Span OtelTraces::StartSpan(const std::string& name) {
#ifdef WITH_OTEL
    if (OtelProvider::Instance().IsEnabled()) {
        auto tracer = OtelProvider::Instance().GetTracer();
        if (tracer) {
            return Span(tracer->StartSpan(name));
        }
    }
#endif
    return Span();
}

Span OtelTraces::StartSpan(const std::string& name, 
                          const std::map<std::string, std::string>& attributes) {
#ifdef WITH_OTEL
    if (OtelProvider::Instance().IsEnabled()) {
        auto tracer = OtelProvider::Instance().GetTracer();
        if (tracer) {
            auto span = tracer->StartSpan(name);
            for (const auto& attr : attributes) {
                span->SetAttribute(attr.first, attr.second);
            }
            return Span(span);
        }
    }
#endif
    return Span();
}

} // namespace observability