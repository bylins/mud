#ifndef BYLINS_FILE_EVENT_SINK_H
#define BYLINS_FILE_EVENT_SINK_H

#include "event_sink.h"

#include <cstdio>
#include <string>

namespace observability {

// Writes events as one JSON object per line (JSONL). Attributes are flattened to
// top-level keys, matching the OTLP log record attribute layout so the same files
// can later be ingested into Loki without conversion.
class FileEventSink : public EventSink {
public:
	explicit FileEventSink(const std::string& path);
	~FileEventSink() override;

	FileEventSink(const FileEventSink&) = delete;
	FileEventSink& operator=(const FileEventSink&) = delete;

	void Emit(const Event& event) override;
	void Flush() override;

private:
	std::FILE* m_file = nullptr;
	std::string m_path;
};

}  // namespace observability

#endif  // BYLINS_FILE_EVENT_SINK_H

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
