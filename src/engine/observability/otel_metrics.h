#ifndef BYLINS_OTEL_METRICS_H
#define BYLINS_OTEL_METRICS_H

#include <string>
#include <map>

namespace observability {

class OtelMetrics {
public:
    // п║я┤я▒я┌я┤п╦п╨ (Counter) - п╪п╬п╫п╬я┌п╬п╫п╫п╬ я─п╟я│я┌я┐я┴п╣п╣ п╥п╫п╟я┤п╣п╫п╦п╣
    static void RecordCounter(const std::string& name, int64_t value);
    static void RecordCounter(const std::string& name, int64_t value,
                            const std::map<std::string, std::string>& attributes);
    
    // п⌠п╦я│я┌п╬пЁя─п╟п╪п╪п╟ (Histogram) - я─п╟я│п©я─п╣п╢п╣п╩п╣п╫п╦п╣ п╥п╫п╟я┤п╣п╫п╦п╧
    static void RecordHistogram(const std::string& name, double value);
    static void RecordHistogram(const std::string& name, double value,
                               const std::map<std::string, std::string>& attributes);
    
    // п≤п╥п╪п╣я─п╦я┌п╣п╩я▄ (Gauge) - я┌п╣п╨я┐я┴п╣п╣ п╥п╫п╟я┤п╣п╫п╦п╣
    static void RecordGauge(const std::string& name, double value);
    static void RecordGauge(const std::string& name, double value,
                           const std::map<std::string, std::string>& attributes);
};

} // namespace observability

#endif // BYLINS_OTEL_METRICS_H