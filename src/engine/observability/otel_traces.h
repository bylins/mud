#ifndef BYLINS_OTEL_TRACES_H
#define BYLINS_OTEL_TRACES_H

#include <string>
#include <cstdint>
#include <map>

#include "opentelemetry/trace/span.h"
#include "opentelemetry/nostd/shared_ptr.h"

namespace observability {

class Span {
public:
    Span() = default;
    explicit Span(opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span> span);
    void End();
    void AddEvent(const std::string& name);
    void SetAttribute(const std::string& key, const std::string& value);
    void SetAttribute(const std::string& key, int64_t value);
    void SetAttribute(const std::string& key, double value);

private:
    opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span> m_span;
};

class OtelTraces {
public:
    static Span StartSpan(const std::string& name);
    static Span StartSpan(const std::string& name,
                         const std::map<std::string, std::string>& attributes);
};

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
