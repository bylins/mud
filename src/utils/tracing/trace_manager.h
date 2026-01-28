#ifndef BYLINS_TRACE_MANAGER_H
#define BYLINS_TRACE_MANAGER_H

#include "trace_sender.h"
#include <memory>

namespace tracing {

class TraceManager {
public:
	static TraceManager& Instance();

	void SetSender(std::unique_ptr<ITraceSender> sender);
	ITraceSender& GetSender();

	// Convenience methods (delegate to sender)
	std::unique_ptr<ISpan> StartSpan(const std::string& name);
	std::unique_ptr<ISpan> StartChildSpan(const std::string& name, const ISpan& parent);

private:
	TraceManager();
	~TraceManager() = default;
	TraceManager(const TraceManager&) = delete;
	TraceManager& operator=(const TraceManager&) = delete;

	std::unique_ptr<ITraceSender> m_sender;
};

} // namespace tracing

#endif // BYLINS_TRACE_MANAGER_H

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
