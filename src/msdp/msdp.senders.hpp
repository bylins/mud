#ifndef __MSDP_SENDERS_HPP__
#define __MSDP_SENDERS_HPP__

#include "msdp.reporters.hpp"

namespace msdp {
class AbstractSender {
 public:
  virtual ~AbstractSender() = default;

  virtual void send(const AbstractReporter::shared_ptr &reporter) = 0;

 protected:
  struct buffer_t {
    buffer_t() : size(0) {}

    explicit buffer_t(const std::size_t size) : ptr(new char[size], std::default_delete<char[]>()), size(size) {}

    std::shared_ptr<char> ptr;
    std::size_t size;
  };

  static buffer_t build(const AbstractReporter::shared_ptr &reporter);
};

class ReportSender : public AbstractSender {
 public:
  explicit ReportSender(DESCRIPTOR_DATA *descriptor);

  void send(const AbstractReporter::shared_ptr &reporter) override;

 private:
  DESCRIPTOR_DATA *m_descriptor;
};

class StreamSender : public AbstractSender {
 public:
  explicit StreamSender(std::ostream &stream);

  void send(const AbstractReporter::shared_ptr &reporter) override;

 private:
  std::ostream &m_stream;
};
}

#endif // __MSDP_SENDERS_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
