#include "engine/ui/cmd_god/do_luainfo.h"

#include <sstream>
#include <string>

#include "engine/entities/char_data.h"
#include "engine/scripting/lua/lua_formatter.h"
#include "engine/scripting/lua/lua_internal.h"
#include "engine/ui/modify.h"

#if defined(WITH_LUAJIT_PROTOTYPE)
extern "C" {
#include <lua.h>
#include <luajit.h>
}
#endif

#if defined(WITH_LUAJIT_PROTOTYPE)
namespace {

void AppendLuaRuntimeDiagnostics(std::ostringstream &out) {
	sol::state lua;
	lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::jit);

	const auto result = lua.safe_script(R"(
		local require_jit_ok, require_jit_result = pcall(require, "jit")
		if require_jit_ok and type(require_jit_result) == "table" then
			jit = require_jit_result
		end

		return {
			jit_type = type(jit),
			jit_version = (type(jit) == "table" and jit.version) or "jit-unavailable",
			require_jit_ok = tostring(require_jit_ok),
			require_jit_error = require_jit_ok and "" or tostring(require_jit_result),
			loaded_jit_type = (package and package.loaded and type(package.loaded.jit)) or "nil",
			jit_status = (type(jit) == "table" and jit.status and tostring(select(1, jit.status()))) or "jit-status-unavailable",
			package_path = package and package.path or "package-unavailable",
			package_cpath = package and package.cpath or "package-unavailable",
		}
	)", sol::script_pass_on_error);

	if (!result.valid()) {
		const sol::error error = result;
		out << "  ошибка: " << error.what() << "\r\n";
		return;
	}

	const sol::object payload_object = result.get<sol::object>();
	if (payload_object.get_type() != sol::type::table) {
		out << "  неожиданный тип результата\r\n";
		return;
	}

	const sol::table payload = payload_object;
	out << "  jit.type: " << payload.get_or(std::string("jit_type"), std::string("unknown")) << "\r\n";
	out << "  jit.version: " << payload.get_or(std::string("jit_version"), std::string("unknown")) << "\r\n";
	out << "  require(\"jit\"): " << payload.get_or(std::string("require_jit_ok"), std::string("unknown")) << "\r\n";
	const auto require_jit_error = payload.get_or(std::string("require_jit_error"), std::string(""));
	if (!require_jit_error.empty()) {
		out << "  require(\"jit\") ошибка: " << require_jit_error << "\r\n";
	}
	out << "  package.loaded.jit: " << payload.get_or(std::string("loaded_jit_type"), std::string("unknown")) << "\r\n";
	out << "  jit.status: " << payload.get_or(std::string("jit_status"), std::string("unknown")) << "\r\n";
	out << "  package.path: " << payload.get_or(std::string("package_path"), std::string("unknown")) << "\r\n";
	out << "  package.cpath: " << payload.get_or(std::string("package_cpath"), std::string("unknown")) << "\r\n";
}

} // namespace
#endif

void DoLuaInfo(CharData *ch, char * /*argument*/, int /*cmd*/, int /*subcmd*/) {
	std::ostringstream out;
	out << "Информация Lua:\r\n";
	out << "  форматтер: "
		<< (lua_scripting::LuaFormatterAvailable() ? "включен (EmmyLuaCodeStyle)" : "выключен") << "\r\n";

#if defined(WITH_LUAJIT_PROTOTYPE)
	out << "  статус: включено\r\n";
	out << "  движок: LuaScriptEngine\r\n";
	out << "  макрос сборки: WITH_LUAJIT_PROTOTYPE\r\n";
#if defined(LUAJIT_VERSION)
	out << "  тип: LuaJIT\r\n";
	out << "  версия JIT: " << LUAJIT_VERSION << "\r\n";
#else
	out << "  тип: Lua\r\n";
#endif
#if defined(LUA_VERSION)
	out << "  версия Lua: " << LUA_VERSION << "\r\n";
#endif
#if defined(LUA_RELEASE)
	out << "  релиз Lua: " << LUA_RELEASE << "\r\n";
#endif
	out << "  язык триггеров: lua\r\n";
	out << "  wait: включен для поддерживаемых типов триггеров\r\n";
	out << "  глобалы времени выполнения: песочница\r\n";
	out << "  отключенные глобалы: ";
	bool first_disabled_global = true;
	for (const auto *name : lua_scripting::kLuaDisabledGlobals) {
		if (!first_disabled_global) {
			out << ", ";
		}
		first_disabled_global = false;
		out << name;
	}
	out << "\r\n";
	out << "  лимит инструкций: " << lua_scripting::kLuaInstructionBudget << "\r\n";
	out << "  шаг hook инструкций: " << lua_scripting::kLuaInstructionHookStep << "\r\n";
	out << "  максимум урона из триггера: " << lua_scripting::kLuaMaxTriggerDamage << "\r\n";
	out << "  пауза GC: " << lua_scripting::kLuaGcPause << "\r\n";
	out << "  множитель шага GC: " << lua_scripting::kLuaGcStepMul << "\r\n";
	out << "\r\n";
	out << "Диагностика загрузчика Lua (отдельная VM без hardening):\r\n";
	out << "  боевые Lua-триггеры используют LuaScriptEngine VM с песочницей; package/jit там недоступны\r\n";
	AppendLuaRuntimeDiagnostics(out);
#else
	out << "  статус: выключено\r\n";
	out << "  движок: заглушка\r\n";
	out << "  язык триггеров: записи lua игнорируются движком-заглушкой\r\n";
	out << "  макрос сборки: WITH_LUAJIT_PROTOTYPE не определен\r\n";
#endif

	page_string(ch->desc, out.str());
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
