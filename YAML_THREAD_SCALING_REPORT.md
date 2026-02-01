# YAML Thread Scaling Fix Report

## Problem
The YAML loader was ignoring the `YAML_THREADS` environment variable, causing it to always use `hardware_concurrency()` (8 threads on this machine) regardless of the environment variable setting.

## Root Cause
1. `RuntimeConfiguration::m_yaml_threads` was never initialized (missing in constructor)
2. `load_world_loader_configuration()` function was not implemented
3. Function was not being called during configuration loading

## Solution
Added proper initialization and configuration loading for YAML thread count:

```cpp
// In RuntimeConfiguration constructor
m_yaml_threads(0)

// New function to read YAML_THREADS
void RuntimeConfiguration::load_world_loader_configuration(const pugi::xml_node *root) {
    // Read YAML_THREADS environment variable (takes precedence)
    const char* env_threads = std::getenv("YAML_THREADS");
    if (env_threads) {
        size_t threads = static_cast<size_t>(std::strtoul(env_threads, nullptr, 10));
        if (threads > 0 && threads <= 64) {  // Sanity check
            m_yaml_threads = threads;
            return;
        }
    }
    
    // Fall back to XML configuration if available
    const auto world_loader = root->child("world_loader");
    if (!world_loader) {
        return;
    }
    
    const auto yaml_config = world_loader.child("yaml");
    if (yaml_config) {
        m_yaml_threads = static_cast<size_t>(std::strtoul(yaml_config.child_value("threads"), nullptr, 10));
    }
}

// Called from load_from_file()
load_world_loader_configuration(&root);
```

## Performance Results

### Small World (Release Build)
| Threads | Boot Time | Speedup |
|---------|-----------|---------|
| 1       | 1.386s    | 1.00x   |
| 2       | 1.260s    | 1.10x   |
| 4       | 1.229s    | 1.13x   |
| 8       | 1.227s    | 1.13x   |

**Analysis**: Small world shows minimal benefit from threading due to limited parallel work.

### Full World (Release Build)
| Threads | Boot Time | Speedup |
|---------|-----------|---------|
| 1       | 52.762s   | 1.00x   |
| 2       | 32.466s   | 1.62x   |
| 4       | 22.654s   | 2.33x   |
| 8       | 18.196s   | 2.90x   |

**Analysis**: Full world shows good thread scaling, approaching 3x speedup with 8 threads.

## Configuration Priority
1. **YAML_THREADS environment variable** (highest priority)
2. **XML configuration** `<world_loader><yaml><threads>N</threads></yaml></world_loader>`
3. **Hardware concurrency** `std::thread::hardware_concurrency()` (fallback if m_yaml_threads == 0)

## Important Note
The configuration file must exist at `lib/misc/configuration.xml` relative to the data directory. If the file doesn't exist or fails to load, `m_yaml_threads` remains 0 and the loader falls back to `hardware_concurrency()`.

## Testing
```bash
# Test with 1 thread
YAML_THREADS=1 ./circle -W -d full 4000

# Test with 4 threads
YAML_THREADS=4 ./circle -W -d full 4000

# Test with 8 threads (or omit to use hardware_concurrency)
YAML_THREADS=8 ./circle -W -d full 4000
```

## Verification
Check syslog for confirmation:
```
grep "YAML loading with" syslog
```

Expected output:
```
YAML loading with N threads
```

Where N matches your YAML_THREADS value.
