#include "msdp_senders.h"

#include "engine/network/descriptor_data.h"
#include "engine/network/telnet.h"
#include "msdp_parser.h"
#include "msdp_constants.h"

namespace msdp {
AbstractSender::buffer_t AbstractSender::build(const AbstractReporter::shared_ptr &reporter) {
	Variable::shared_ptr response;
	reporter->get(response);
	if (!response) {
		// Репортер намеренно промолчал, и это не ошибка: RoomReporter не шлет карту слепому, в
		// темноте, в комнате с kMoMapper и скриптеру (см. blockReport), а также когда комната не
		// менялась. Раньше тут стоял SYSERR, и один играющий скриптер давал 700 с лишним строк за
		// два часа -- в них тонули настоящие ошибки. Если репортер забыл положить ответ по ошибке,
		// это видно в debug-логе рядом с остальным разбором MSDP.
		debug_log("Report skipped: reporter left the response unset.");
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

ReportSender::ReportSender(DescriptorData *descriptor) : m_descriptor(descriptor) {
}

void ReportSender::send(const AbstractReporter::shared_ptr &reporter) {
	if (!m_descriptor->character) {
		// Клиент спросил переменную до того, как игрок выбрал персонажа: MSDP он согласовывает
		// сразу после коннекта, а по SEND мы отвечаем без оглядки на состояние соединения. Наш
		// собственный код сюда с пустым персонажем не приходит -- msdp_report_changed_vars
		// выходит раньше. Так что это не ошибка сервера, а ранний вопрос клиента.
		log("WARNING: MSDP report requested for descriptor %d without character.", m_descriptor->desc_num);
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
	iosystem::write_to_descriptor_with_options(m_descriptor, buffer.ptr.get(), buffer.size, written);
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
