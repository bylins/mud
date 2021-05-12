#include "msdp.senders.hpp"

#include "structs.h"
#include "telnet.h"
#include "comm.h"
#include "msdp.parser.hpp"
#include "msdp.constants.hpp"

namespace msdp {
AbstractSender::buffer_t AbstractSender::build(const AbstractReporter::shared_ptr &reporter) {
  Variable::shared_ptr response;
  reporter->get(response);
  if (!response) {
    log("SYSERR: MSDP response was not set.");
    return buffer_t();
  }

  debug_log("Report:");
  response->dump();

  const auto buffer_size = WRAPPER_LENGTH + response->required_size();
  buffer_t buffer(buffer_size);
  buffer.ptr.get()[0] = char(IAC);
  buffer.ptr.get()[1] = char(SB);
  buffer.ptr.get()[2] = constants::TELOPT_MSDP;
  response->serialize(HEAD_LENGTH + buffer.ptr.get(), buffer_size - WRAPPER_LENGTH);
  buffer.ptr.get()[buffer_size - 2] = char(IAC);
  buffer.ptr.get()[buffer_size - 1] = char(SE);

  return buffer;
}

ReportSender::ReportSender(DESCRIPTOR_DATA *descriptor) : m_descriptor(descriptor) {
}

void ReportSender::send(const AbstractReporter::shared_ptr &reporter) {
  if (!m_descriptor->character) {
    log("SYSERR: Requested MSDP report for descriptor without character.");
    return;
  }

  if (!reporter) {
    log("SYSERR: MSDP reporter was not set at send() function.");
    return;
  }

  const auto buffer = build(reporter);
  if (!buffer.ptr) {
    return;
  }

  hexdump(buffer.ptr.get(), buffer.size, "Response buffer:");

  int written = 0;
  write_to_descriptor_with_options(m_descriptor, buffer.ptr.get(), buffer.size, written);
}

StreamSender::StreamSender(std::ostream &stream) : m_stream(stream) {
}

void StreamSender::send(const AbstractReporter::shared_ptr &reporter) {
  if (!reporter) {
    return;
  }

  const auto buffer = build(reporter);
  if (!buffer.ptr) {
    return;
  }

  m_stream.write(buffer.ptr.get(), buffer.size);
}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
