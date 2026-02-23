#ifndef BYLINS_OTEL_METRICS_H
#define BYLINS_OTEL_METRICS_H

#include <string>
#include <map>

namespace observability {

class OtelMetrics {
public:
    // Счётчик (Counter) - монотонно растущее значение
    static void RecordCounter(const std::string& name, int64_t value);
    static void RecordCounter(const std::string& name, int64_t value,
                            const std::map<std::string, std::string>& attributes);
    
    // Гистограмма (Histogram) - распределение значений
    static void RecordHistogram(const std::string& name, double value);
    static void RecordHistogram(const std::string& name, double value,
                               const std::map<std::string, std::string>& attributes);
    
    // Измеритель (Gauge) - текущее значение
    static void RecordGauge(const std::string& name, double value);
    static void RecordGauge(const std::string& name, double value,
                           const std::map<std::string, std::string>& attributes);
};

} // namespace observability

#endif // BYLINS_OTEL_METRICS_H