#include <gtest/gtest.h>

#include "engine/core/iosystem.h"
#include "engine/network/descriptor_data.h"
#include "engine/network/msdp/msdp_constants.h"
#include "engine/network/telnet.h"
#include "engine/structs/structs.h"

#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <string>

// Досылка хвоста против полного прохода -- на настоящем сокете. MSDP-отчёт не пишет в сокет
// сам, а кладёт байты в хвост дескриптора, и тут проверяется, что к ним ничего не
// приклеивается: полный проход всегда дописывает промпт, и именно поэтому в #4238 игрок
// получал промпт на пустом месте каждый раз, когда ему уходил отчёт.

namespace {

// Пара сокетов: один отдаём движку, из другого читаем как клиент.
class SocketPair {
 public:
	SocketPair() {
		EXPECT_EQ(0, socketpair(AF_UNIX, SOCK_STREAM, 0, m_fd));
		// Движок держит сокеты неблокирующими (nonblock() в comm.cpp), и для проверки это
		// важно: только на таком сокете «буфер полон» возвращается как ноль байтов, а не
		// подвешивает запись.
		EXPECT_EQ(0, fcntl(m_fd[0], F_SETFL, O_NONBLOCK));
	}
	~SocketPair() {
		close(m_fd[0]);
		close(m_fd[1]);
	}

	int server() const { return m_fd[0]; }

	// Всё, что уже лежит на клиентской стороне. Не блокирует: пусто -- значит пусто.
	std::string Drain() const {
		std::string out;
		char buffer[4096];
		for (;;) {
			const auto got = recv(m_fd[1], buffer, sizeof(buffer), MSG_DONTWAIT);
			if (got <= 0) {
				return out;
			}
			out.append(buffer, static_cast<size_t>(got));
		}
	}

 private:
	int m_fd[2] = {-1, -1};
};

// Дескриптор не в игре: промпт у такого пустой, а персонажа нет -- так из проверки уходит
// всё, кроме самого вывода.
void SetUpDescriptor(DescriptorData &d, const SocketPair &sockets) {
	d.descriptor = sockets.server();
	d.state = EConState::kMenu;
	d.output = d.small_outbuf;
	d.bufspace = kSmallBufsize - 1;
	d.bufptr = 0;
	*d.output = '\0';
	d.has_prompt = 1;
}

// Правдоподобный MSDP-отчёт: IAC SB MSDP VAR "GROUP" VAL IAC SE.
std::string MsdpReport() {
	std::string out;
	out += static_cast<char>(IAC);
	out += static_cast<char>(SB);
	out += static_cast<char>(msdp::constants::TELOPT_MSDP);
	out += static_cast<char>(1);    // MSDP_VAR
	out += "GROUP";
	out += static_cast<char>(2);    // MSDP_VAL
	out += static_cast<char>(IAC);
	out += static_cast<char>(SE);
	return out;
}

}  // namespace

// Отчёт уходит клиенту ровно теми байтами, что положили в хвост, и ничем больше.
TEST(OutputTail, FlushSendsTheTailAndNothingElse) {
	SocketPair sockets;
	DescriptorData d;
	SetUpDescriptor(d, sockets);

	const auto report = MsdpReport();
	ASSERT_TRUE(iosystem::write_to_descriptor_with_options(&d, report.data(), report.size()));
	// Запись в сокет -- дело досылки, до неё клиент не видит ничего.
	EXPECT_TRUE(sockets.Drain().empty());
	EXPECT_EQ(report, d.pending_output);

	ASSERT_EQ(0, iosystem::FlushOutputTail(&d));

	EXPECT_EQ(report, sockets.Drain());
	EXPECT_TRUE(d.pending_output.empty());
	// Промпт не выводился заново, значит и признак его не менялся.
	EXPECT_EQ(1, d.has_prompt);
}

// А полный проход к тому же хвосту дописывает свой перевод строки с промптом -- та самая
// разница, из-за которой досылку и отделили. У дескриптора вне игры промпт пустой, так что в
// глаза бросается только перевод строки; у играющего там же оказывается всё приглашение.
TEST(OutputTail, FullPassAppendsItsOwnLineToTheTail) {
	SocketPair sockets;
	DescriptorData d;
	SetUpDescriptor(d, sockets);

	const auto report = MsdpReport();
	ASSERT_TRUE(iosystem::write_to_descriptor_with_options(&d, report.data(), report.size()));

	ASSERT_LT(0, iosystem::process_output(&d));

	const auto received = sockets.Drain();
	EXPECT_EQ(report + "\r\n", received);
	EXPECT_LT(report.size(), received.size());
}

// Хвост, который сокет взял не целиком, остаётся с начала и уходит следующей досылкой --
// порядок байтов при этом не рвётся.
TEST(OutputTail, WhatTheSocketDidNotTakeStaysInOrder) {
	SocketPair sockets;
	DescriptorData d;
	SetUpDescriptor(d, sockets);

	// Заведомо больше, чем ядро возьмёт за раз, но меньше предела хвоста (512 КБ), на
	// котором связь уже считается оборванной.
	const std::string lump(256 * 1024, 'x');
	ASSERT_TRUE(iosystem::write_to_descriptor_with_options(&d, lump.data(), lump.size()));

	std::string received;
	for (int round = 0; round < 10000; ++round) {
		ASSERT_EQ(0, iosystem::FlushOutputTail(&d));
		received += sockets.Drain();
		if (d.pending_output.empty()) {
			break;
		}
	}

	EXPECT_TRUE(d.pending_output.empty());
	ASSERT_EQ(lump.size(), received.size());
	EXPECT_TRUE(lump == received);
}
