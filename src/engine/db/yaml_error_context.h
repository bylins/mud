/**
\file yaml_error_context.h - a part of the Bylins engine.
\brief Подсказка к ошибке разбора yaml-файла мира.
\detail Сообщение yaml-cpp вида "error at line 360, column 1: end of map not found" указывает
не на поломку, а на место, где парсер упёрся: блок оборвался выше, когда строка вышла из
отступа (обычное дело при правке руками в редакторе). Эти функции по тексту файла собирают
то, чего не хватает билдеру: саму строку, запись, внутри которой она лежит, и ближайшую выше
строку без отступа -- она почти всегда и есть причина.
*/

#ifndef ENGINE_DB_YAML_ERROR_CONTEXT_H_
#define ENGINE_DB_YAML_ERROR_CONTEXT_H_

#include <string>
#include <string_view>

namespace world_format {

// error_line -- номер строки от 1, как его печатает yaml-cpp. Возвращает готовый
// хвост для строки лога или пусто, если сказать нечего (файл пуст, номер за краем).
std::string DescribeYamlError(std::string_view content, int error_line);

// То же, но текст читается из файла; при неудаче чтения возвращает пусто.
std::string DescribeYamlErrorInFile(const std::string &path, int error_line);

}  // namespace world_format

#endif  // ENGINE_DB_YAML_ERROR_CONTEXT_H_
