#ifndef BYLINS_OTEL_PROVIDER_H
#define BYLINS_OTEL_PROVIDER_H

#include <string>
#include <memory>

namespace observability {

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

private:
    struct Providers;

    OtelProvider();
    ~OtelProvider();
    OtelProvider(const OtelProvider&) = delete;
    OtelProvider& operator=(const OtelProvider&) = delete;

    bool m_enabled = false;
    std::unique_ptr<Providers> m_providers;
};

} // namespace observability

#endif // BYLINS_OTEL_PROVIDER_H
