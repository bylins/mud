# World Loading Tests

This directory contains scripts for testing world loading functionality across different data sources (Legacy, SQLite, YAML).

## Quick Start

```bash
# Run quick comparison test (builds binaries and sets up worlds automatically)
./tools/run_load_tests.sh --quick
```

**Note:** `run_load_tests.sh` handles all setup automatically:
- Builds required binaries if missing
- Prepares test worlds (copies lib/ + lib.template/)
- Converts worlds to SQLite/YAML formats as needed

**Obsolete scripts:**
- `setup_test_dirs.sh` - No longer needed, run_load_tests.sh handles setup

## Test Script Usage

### Basic Usage

```bash
# Run all tests (Legacy, SQLite, YAML × small/full × checksums/no-checksums)
./tools/run_load_tests.sh

# Run quick comparison (Legacy vs YAML, small world, with checksums)
./tools/run_load_tests.sh --quick
```

### Filtered Testing

```bash
# Test specific loader
./tools/run_load_tests.sh --loader=yaml
./tools/run_load_tests.sh --loader=legacy
./tools/run_load_tests.sh --loader=sqlite

# Test specific world size
./tools/run_load_tests.sh --world=small
./tools/run_load_tests.sh --world=full

# Test with/without checksums
./tools/run_load_tests.sh --checksums
./tools/run_load_tests.sh --no-checksums

# Combine multiple filters
./tools/run_load_tests.sh --loader=yaml --world=small --checksums
./tools/run_load_tests.sh --loader=sqlite --world=full --no-checksums
```

## Common Development Workflows

### 1. YAML Loader Development

When working on the YAML loader implementation:

```bash
# Quick test after code changes
./tools/run_load_tests.sh --quick

# Full YAML testing
./tools/run_load_tests.sh --loader=yaml

# Compare YAML vs Legacy on small world
./tools/run_load_tests.sh --loader=yaml --world=small
./tools/run_load_tests.sh --loader=legacy --world=small
```

### 2. Checksum Verification

To verify checksum parity between loaders:

```bash
# Run with checksums enabled for all loaders
./tools/run_load_tests.sh --checksums --world=small

# The script will automatically compare checksums and report:
# - MATCH: Checksums are identical
# - DIFFER: Checksums differ (shows first 10 differences)
```

### 3. Performance Testing

To measure loading performance without checksum overhead:

```bash
# Test loading speed without checksums
./tools/run_load_tests.sh --no-checksums

# Compare specific loaders
./tools/run_load_tests.sh --loader=legacy --no-checksums
./tools/run_load_tests.sh --loader=yaml --no-checksums
```

## Understanding Output

The test script outputs:

1. **Boot time**: Time from "Boot db -- BEGIN" to "Boot db -- DONE"
2. **Entity counts**: Number of zones, rooms, mobs, objects, triggers loaded
3. **Checksums**: CRC32 checksums for each entity type (if enabled)
4. **Checksum time**: Time spent calculating checksums (if enabled)
5. **Comparison results**: MATCH/DIFFER status when comparing loaders

Example output:
```
--- Small_YAML_checksums ---
  Boot time:     1.234s
  Zones:    7C788E1F (55 zones)
  Rooms:    8C0277A7 (5109 rooms)
  Mobs:     CEBB697B (5123 mobs)
  Objects:  928B1B5A (5192 objects)
  Triggers: 91924F29 (5064 triggers)
  Checksum time: 0.456s

=== CHECKSUM COMPARISON ===
Small_Legacy_checksums vs Small_YAML_checksums: MATCH
```

## Test Directories

Test directories are created automatically by `run_load_tests.sh`:

```
test/
├── small_legacy/     # Small world, legacy text files
├── small_sqlite/     # Small world, SQLite database
├── small_yaml/       # Small world, YAML files
├── full_legacy/      # Full world, legacy text files
├── full_sqlite/      # Full world, SQLite database
└── full_yaml/        # Full world, YAML files
```

Each directory contains:
- Symlinks to lib/cfg, lib/etc, lib/misc
- World data in the appropriate format
- Generated syslog and checksum files after tests

## Binaries

The script expects three binaries:

1. **build_test/circle**: Legacy loader (default, no special flags)
2. **build_sqlite/circle**: SQLite loader (built with -DHAVE_SQLITE=ON)
3. **build_yaml/circle**: YAML loader (built with -DHAVE_YAML=ON)

If a binary is missing, tests for that loader are skipped with a warning.

## Troubleshooting

### Test directories missing
They will be created automatically by `run_load_tests.sh`.

### "Binary not found"
Build the missing binary:
```bash
cd build_sqlite && cmake -DHAVE_SQLITE=ON .. && make circle -j$(nproc)
```

### Tests timeout
Increase the timeout in `run_load_tests.sh` (line 59, default 300 seconds).

### Checksum mismatches
Use detailed checksum files in `/tmp/*_checksums.txt` to investigate:
```bash
diff /tmp/Small_Legacy_checksums_checksums.txt /tmp/Small_YAML_checksums_checksums.txt
```

## Advanced Usage

### Running specific test manually

```bash
cd test/small_yaml
../../build_yaml/circle 4000  # With checksums
../../build_yaml/circle -C 4000  # Without checksums
grep "Zones:" syslog  # Check results
```

### Comparing individual checksum files

```bash
# After running tests with checksums
diff test/small_legacy/checksums_buffers/zones/1.txt \
     test/small_yaml/checksums_buffers/zones/1.txt
```

## Adding New Loaders

To add support for a new loader:

1. Add binary path variable (e.g., `NEW_BIN="$MUD_DIR/build_new/circle"`)
2. Add prerequisite check
3. Add test invocations in both small and full sections
4. Add checksum comparisons
5. Update this documentation

## See Also

- `convert_to_yaml.py`: Converts legacy files to SQLite/YAML
- `../src/engine/db/world_checksum.cpp`: Checksum calculation implementation
