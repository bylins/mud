# Loot Tables Configuration Guide

This guide explains how to configure the loot tables system in Bylins MUD.

## Overview

The loot tables system allows you to define what items and currencies drop from mobs when they die. Configuration is done through YAML files in the `lib/cfg/loot_tables/` directory.

## Directory Structure

```
lib/cfg/loot_tables/
├── example.yaml      # Example configuration
├── zone40.yaml       # Zone 40 loot tables
└── common_drops.yaml # Shared loot pools
```

All `.yaml` files in this directory are automatically loaded at server startup and can be reloaded with the `loottable reload` command.

## Basic Concepts

### Table Types

1. **Source-centric tables** (`type: source_centric`) - Tables attached to specific mobs, defining what they drop.
2. **Item-centric tables** (`type: item_centric`) - Tables defining where specific items drop.

### Entry Types

- **item** - Drops a specific item by vnum
- **currency** - Drops currency (gold, glory, ice, nogata)
- **nested_table** - References another loot table for reusable loot pools

### Filters

Filters determine which mobs a loot table applies to:

- **vnum** - Match exact mob vnum
- **level_range** - Match mobs within level range
- **zone** - Match all mobs in a zone
- **race** - Match mobs by race

### Probability

Each entry has a `weight` that determines its relative drop chance. The `probability_width` of the table controls how likely any drop is:

```
chance_per_entry = weight / probability_width
```

If `probability_width = 1000` and entry `weight = 100`, the entry has 10% chance.

## YAML Configuration Structure

```yaml
# Table identifier (must be unique)
id: "zone40_bandit_drops"

# Optional description
description: "Loot for bandits in zone 40"

# Table type: source_centric or item_centric
type: source_centric

# Probability divisor (higher = rarer drops)
probability_width: 1000

# Whether generation can produce empty result
allow_empty: true

# Maximum items per generation
max_items: 3

# Filters for which mobs this table applies to
source_filters:
  - type: vnum
    vnum: 4000
  - type: zone
    zone: 40
  - type: level_range
    min_level: 10
    max_level: 20

# Loot entries
entries:
  - type: item
    vnum: 1234
    weight: 100
    count:
      min: 1
      max: 1

  - type: currency
    currency: gold
    weight: 500
    count:
      min: 10
      max: 50

  - type: nested_table
    table_id: "common_drops"
    weight: 200
```

## Example: Zone 40 Configuration

Zone 40 contains training mobs like bandits (razboniki), peasants (krestyane), bears (medved), geese (gusi), and chickens (kury).

### zone40.yaml

```yaml
# Bandit Leader (vnum 4000)
---
id: "zone40_bandit_leader"
description: "Drops for the bandit leader"
type: source_centric
probability_width: 100
allow_empty: true
max_items: 2
source_filters:
  - type: vnum
    vnum: 4000
entries:
  - type: currency
    currency: gold
    weight: 80
    count:
      min: 50
      max: 100
  - type: item
    vnum: 4001  # Example: weapon
    weight: 15
  - type: item
    vnum: 4002  # Example: armor piece
    weight: 10

# All bandits in zone (vnums 4000-4010)
---
id: "zone40_bandits_common"
description: "Common drops for all bandits"
type: source_centric
probability_width: 1000
allow_empty: true
max_items: 1
source_filters:
  - type: zone
    zone: 40
  - type: level_range
    min_level: 10
    max_level: 20
entries:
  - type: currency
    currency: gold
    weight: 300
    count:
      min: 5
      max: 25
  - type: nested_table
    table_id: "common_weapons"
    weight: 50

# Animals (geese, chickens, rabbits)
---
id: "zone40_animals"
description: "Drops from animals"
type: source_centric
probability_width: 100
allow_empty: true
source_filters:
  - type: level_range
    min_level: 1
    max_level: 5
  - type: zone
    zone: 40
entries:
  - type: item
    vnum: 4010  # Meat
    weight: 60
    count:
      min: 1
      max: 2
  - type: item
    vnum: 4011  # Feathers
    weight: 30
```

### Shared Loot Pools

Create `common_drops.yaml` for reusable loot:

```yaml
---
id: "common_weapons"
description: "Basic weapons that can drop anywhere"
type: source_centric
probability_width: 100
allow_empty: true
entries:
  - type: item
    vnum: 100  # Rusty sword
    weight: 40
  - type: item
    vnum: 101  # Old dagger
    weight: 35
  - type: item
    vnum: 102  # Wooden club
    weight: 25

---
id: "common_armor"
description: "Basic armor pieces"
type: source_centric
probability_width: 100
allow_empty: true
entries:
  - type: item
    vnum: 200  # Leather cap
    weight: 30
  - type: item
    vnum: 201  # Cloth shirt
    weight: 40
  - type: item
    vnum: 202  # Worn boots
    weight: 30
```

## Player Filters

You can restrict drops based on player characteristics:

```yaml
entries:
  - type: item
    vnum: 5000  # Epic sword
    weight: 10
    player_filters:
      min_level: 30
      max_level: 0  # No max
      allowed_classes:
        - 1  # Warrior
        - 5  # Paladin
```

## Admin Commands

The `loottable` command is available to implementors:

```
loottable list              - List all loaded loot tables
loottable info <table_id>   - Show table details
loottable reload            - Reload all loot tables from files
loottable test <table_id>   - Test generation from a table
loottable stats             - Show usage statistics
loottable errors            - Show loading errors
```

## MIW (Max-In-World) Support

Items can respect MIW limits. If the item count in world exceeds the limit, it won't drop:

```yaml
entries:
  - type: item
    vnum: 9999  # Rare item with MIW limit
    weight: 5
    respect_miw: true  # Won't drop if MIW exceeded
```

## Best Practices

1. **Organize by zone** - Create separate files for each zone
2. **Use nested tables** - Create shared pools for common drops
3. **Balance probability_width** - Higher values = rarer drops
4. **Test your tables** - Use `loottable test` before going live
5. **Start with allow_empty: true** - Prevents guaranteed drops on every kill
6. **Use level filters** - Ensure appropriate drops for mob difficulty

## Troubleshooting

### Tables not loading

1. Check YAML syntax with a validator
2. Run `loottable errors` to see parsing errors
3. Ensure files have `.yaml` extension
4. Check file permissions

### No drops happening

1. Verify `probability_width` isn't too high
2. Check `allow_empty` setting
3. Verify source filters match your mobs
4. Use `loottable test` to debug

### Cycle detection errors

Nested tables cannot reference each other in a cycle. Check your `nested_table` references.

## Building with YAML Support

To enable the loot tables system, build with:

```bash
cmake -DHAVE_YAML=ON ..
make
```

Requires: `libyaml-cpp-dev` package.
