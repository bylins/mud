// CodeMirror mode for DG Scripts (Bylins MUD trigger scripting language)
// Based on CircleMUD/DikuMUD DG Scripts syntax

(function(mod) {
    if (typeof exports == "object" && typeof module == "object") // CommonJS
        mod(require("../../lib/codemirror"));
    else if (typeof define == "function" && define.amd) // AMD
        define(["../../lib/codemirror"], mod);
    else // Plain browser env
        mod(CodeMirror);
})(function(CodeMirror) {
    "use strict";

    CodeMirror.defineMode("dgscript", function() {
        // Control flow keywords
        var keywords = /^(if|else|elseif|end|wait|halt|return|attach|detach|break|switch|case|default|done|while|until|foreach|set|unset|eval|extract|context|global|remote)\b/i;

        // Script commands (DG-specific actions)
        var commands = /^(act|send|teleport|force|damage|load|purge|echo|echoaround|zoneecho|asound|at|door|makeuid|transform|dg_cast|dg_affect|mgoto|oteleport|wteleport|mforce|oforce|recho|zecho|%purge%|%load%|%damage%|%teleport%|nop|mjunk|oload|mload|zreset|version|vdelete|setskill|skillset|calcuid|charuid|version)\b/i;

        // MUD server commands (passed through to server)
        var mudCommands = /^(say|г|говорить|tell|сказать|whisper|шепнуть|shout|крикнуть|emote|эмоция|smile|улыбнуться|laugh|смеяться|nod|кивнуть|shake|покачать|get|взять|drop|бросить|give|дать|put|положить|wear|надеть|remove|снять|wield|держать|kill|убить|flee|бежать|look|смотреть|examine|осмотреть|open|открыть|close|закрыть|unlock|отпереть|lock|запереть|north|север|south|юг|east|восток|west|запад|up|вверх|down|вниз)\b/i;

        // Functions and special variables
        var builtinFunctions = /^(%random\.\w+%|%findobj\.\w+%|%findmob\.\w+%|%actor\.\w+%|%self\.\w+%|%object\.\w+%|%victim\.\w+%|%room\.\w+%)\b/i;

        // Helper: Parse DG variable with nested structures
        // Returns the number of characters consumed, or 0 if not a variable
        // Supports: %var%, %var.prop%, %func(%arg1%, %arg2%)%, etc.
        function parseVariable(str, pos) {
            if (str[pos] !== '%') return 0;

            var i = pos + 1; // Skip opening %
            var len = str.length;
            var depth = 1; // We've opened one %

            // Variable must start with identifier character
            if (i >= len || !/[a-zA-Z_]/.test(str[i])) {
                return 0; // Not a variable
            }

            while (i < len && depth > 0) {
                var ch = str[i];

                if (ch === '%') {
                    // This is either opening nested or closing current
                    // Peek ahead to determine
                    if (i + 1 < len && /[a-zA-Z_]/.test(str[i + 1])) {
                        // Next char is identifier start → nested variable opening
                        depth++;
                    } else {
                        // This is a closing %
                        depth--;
                    }
                    i++;
                } else if (/[a-zA-Z0-9_.()\[\], -]/.test(ch)) {
                    // Valid variable content
                    i++;
                } else {
                    // Invalid character → abort
                    return 0;
                }
            }

            if (depth === 0) {
                return i - pos; // Success
            }

            return 0; // Unclosed variable
        }

        return {
            startState: function() {
                return {
                    inString: false,
                    stringDelim: null,
                    inComment: false,
                    lineStart: true  // Track if we're at line start
                };
            },

            token: function(stream, state) {
                var ch = stream.peek();

                // Reset lineStart flag at start of line
                if (stream.sol()) {
                    state.lineStart = true;
                }

                // Handle comments (lines starting with *)
                if (stream.sol() && ch === '*') {
                    stream.skipToEnd();
                    state.lineStart = false;
                    return "comment";
                }

                // Handle strings
                if (state.inString) {
                    if (stream.match(state.stringDelim)) {
                        state.inString = false;
                        state.stringDelim = null;
                        return "string";
                    }
                    stream.next();
                    return "string";
                }

                if (ch === '"' || ch === "'") {
                    state.inString = true;
                    state.stringDelim = ch;
                    stream.next();
                    return "string";
                }

                // Handle variables with smart parser (supports nesting)
                // Examples: %actor%, %array.item(%dirs%, %count2%)%, %findobj.%vnum%%
                if (ch === '%') {
                    var str = stream.string;
                    var pos = stream.pos;
                    var consumed = parseVariable(str, pos);

                    if (consumed > 0) {
                        // Successfully parsed a variable
                        for (var i = 0; i < consumed; i++) {
                            stream.next();
                        }
                        return "variable";
                    }

                    // Not a valid variable, just consume the %
                    stream.next();
                    return null;
                }

                // Handle numbers
                if (stream.match(/^-?[0-9]+(\.[0-9]+)?/)) {
                    return "number";
                }

                // Handle operators
                if (stream.match(/^(==|!=|<=|>=|&&|\|\||<|>|=)/)) {
                    return "operator";
                }

                // Skip whitespace
                if (stream.eatSpace()) {
                    return null;
                }

                // Handle keywords (control flow)
                if (stream.match(keywords)) {
                    state.lineStart = false;
                    return "keyword";
                }

                // Handle commands (DG actions)
                if (stream.match(commands)) {
                    state.lineStart = false;
                    return "builtin";
                }

                // Smart MUD command detection:
                // If we're at line start and see an identifier, treat it as a MUD server command
                // This handles Russian commands, abbreviations, and any other server commands
                if (state.lineStart && /[a-zA-Zа-яА-ЯёЁ]/.test(ch)) {
                    if (stream.match(/^[a-zA-Zа-яА-ЯёЁ]+/)) {
                        state.lineStart = false;
                        return "def";  // MUD server command style
                    }
                }

                // Default: consume character
                stream.next();
                return null;
            },

            lineComment: "*"
        };
    });

    CodeMirror.defineMIME("text/x-dgscript", "dgscript");
});
