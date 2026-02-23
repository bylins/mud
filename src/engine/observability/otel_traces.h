#ifndef BYLINS_OTEL_TRACES_H
#define BYLINS_OTEL_TRACES_H

#include <string>
#include <map>

#ifdef WITH_OTEL
#include "opentelemetry/trace/span.h"
namespace trace_api = opentelemetry::trace;
#endif

namespace observability {

class Span {
public:
    Span() = default;
    
#ifdef WITH_OTEL
    explicit Span(opentelemetry::nostd::shared_ptr<trace_api::Span> span);
    void End();
    void AddEvent(const std::string& name);
    void SetAttribute(const std::string& key, const std::string& value);
    void SetAttribute(const std::string& key, int64_t value);
    void SetAttribute(const std::string& key, double value);
#else
    void End() {}
    void AddEvent(const std::string&) {}
    void SetAttribute(const std::string&, const std::string&) {}
    void SetAttribute(const std::string&, int64_t) {}
    void SetAttribute(const std::string&, double) {}
#endif

private:
#ifdef WITH_OTEL
    opentelemetry::nostd::shared_ptr<trace_api::Span> m_span;
#endif
};

class OtelTraces {
public:
    static Span StartSpan(const std::string& name);
    static Span StartSpan(const std::string& name, 
                         const std::map<std::string, std::string>& attributes);
};

// RAII span guard для автоматического завершения
class SpanGuard {
public:
    explicit SpanGuard(Span span) : m_span(std::move(span)) {}
    ~SpanGuard() { m_span.End(); }
    
    Span& GetSpan() { return m_span; }
    
private:
    Span m_span;
};

} // namespace observability

#endif // BYLINS_OTEL_TRACES_H