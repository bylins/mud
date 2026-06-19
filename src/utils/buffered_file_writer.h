// Part of Bylins http://www.mud.ru

#ifndef UTILS_BUFFERED_FILE_WRITER_H_
#define UTILS_BUFFERED_FILE_WRITER_H_

#include <cstdarg>
#include <cstdio>
#include <string>

// printf-совместимый писатель в буфер std::string в памяти.
//
// Формат-строки и аргументы -- ровно как у fprintf, поэтому вывод побайтово
// совпадает с прежней записью fprintf. Отличия от FBFILE/fbprintf: форматирует
// через vsnprintf (с измерением длины), поэтому переполнения буфера нет при
// любой длине поля. Буфер потом отдаётся на запись файла и на подсчёт CRC --
// без перечитывания только что записанного файла.
class BufferedFileWriter {
 public:
	BufferedFileWriter() { m_buf.reserve(8192); }

// MSVC ne ponimaet GCC-style __attribute__((format(...))). MSVC-shim
// '#define __attribute__(x)' zhivet v engine/core/sysdep.h, no esli etot
// zagolovok podtyanyut do sysdep.h (chto v unity-sborkah byvayet --
// poryadok TU peremeshivayetsya) MSVC padayet s C2059. Stavim guard
// pryamo zdes, chtoby BufferedFileWriter byl include-order-agnostichnyy.
#if defined(__GNUC__) || defined(__clang__)
	__attribute__((format(printf, 2, 3)))
#endif
	void printf(const char *format, ...) {
		va_list args;
		va_start(args, format);
		va_list measure;
		va_copy(measure, args);
		const int need = vsnprintf(nullptr, 0, format, measure);
		va_end(measure);
		if (need > 0) {
			const std::size_t old = m_buf.size();
			m_buf.resize(old + static_cast<std::size_t>(need) + 1);
			vsnprintf(&m_buf[old], static_cast<std::size_t>(need) + 1, format, args);
			m_buf.resize(old + static_cast<std::size_t>(need));  // отбросить добавленный '\0'
		}
		va_end(args);
	}

	const std::string &str() const { return m_buf; }

 private:
	std::string m_buf;
};

#endif  // UTILS_BUFFERED_FILE_WRITER_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
