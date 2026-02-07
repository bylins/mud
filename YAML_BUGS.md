# YAML World Data Format - Known Issues

## Critical Bugs

### 1. special_bitvector - Not Human Readable
**Status:** Critical
**Component:** Mob data format
**Description:** The `special_bitvector` field stores flags as cryptic strings like `"a0b0c0d0e0f0"` or `"l0A0B0b1c1m3B3C3"` instead of human-readable flag names.

**Current format:**
```yaml
enhanced:
  special_bitvector: l0A0B0b1c1m3B3C3
```

**Expected format:**
```yaml
enhanced:
  special_flags:
    - some_readable_flag
    - another_flag
```

**Impact:** Makes manual editing nearly impossible, requires knowledge of internal encoding.

### 2. YAML Structure Corruption on Save
**Status:** Critical - **FIXED**
**Component:** Koi8rYamlEmitter
**Description:** The custom YAML emitter writes string values without proper quoting or escaping of special YAML characters (`:`, `#`, `|`, `>`, etc.). This causes invalid YAML syntax when strings contain these characters.

**Example of broken output:**
```yaml
special_bitvector: Warning, if bugs appear: disable logging
                                           ^-- second colon breaks syntax
```

**Root cause:** Line 108 in `Koi8rYamlEmitter::Value()` writes unquoted strings:
```cpp
out_ << " " << value << std::endl;  // No quoting!
```

**Fix applied:** Added detection of special YAML characters and automatic quoting with single quotes when needed.

**Files affected:**
- `src/engine/db/yaml_world_data_source.cpp` (Koi8rYamlEmitter class)

**How it happened:**
- Legacy mob data or converter error placed multi-line text into `special_bitvector` field
- Field should contain short flag strings like `"0"` or `"a0b0"`
- Emitter didn't quote the value, creating invalid YAML with unescaped colons

## Medium Priority Issues

### 3. Missing Validation on Data Load
**Component:** YAML loader
**Description:** No validation that loaded data matches expected types/formats.

### 4. Error Messages Don't Show File Context
**Component:** Error reporting
**Description:** When YAML fails to load, error shows line/column but not the actual content causing the issue.

## How special_bitvector Got Corrupted

Investigating corrupted mob 102: special_bitvector contained a multi-paragraph text block instead of a flag string. Possible causes:
1. Manual edit error in legacy .mob file
2. Converter bug that misplaced a field value
3. Memory corruption during initial conversion

**Action:** Deleted corrupted `world/mobs/102.yaml`, needs reconversion from clean legacy source.
