#!/usr/bin/env bash
#
# migrate_playerdata_layout.sh — issue.playerdata-migration production migration.
#
# Reorganises a server's runtime data directory to the layout introduced on the
# issue.playerdata-migration branch: the per-character save trees, the account
# and clan data, and the whole plrstuff/ + stat/ catch-alls are relocated into
# three purpose-named roots.
#
#     plrs/ plrobjs/ plralias/ plrvars/  --> userdata/chardata/{characters,items,aliases,variables}
#     plrstuff/depot/                    --> userdata/chardata/depots/  (parcel.db -> userdata/)
#     plrs/accounts/                     --> userdata/accounts/
#     plrstuff/house/ + shop/item_desc   --> userdata/clans/
#     plrstuff/<player-owned files>      --> userdata/
#     plrstuff/<server state/stats>      --> state/  and  state/statistics/
#     stat/spellstat.txt                 --> state/statistics/
#
# It MOVES live files into the new directories, DELETES files the new engine no
# longer reads, and leaves anything it does not recognise in place (with a
# warning) so nothing is lost silently.
#
# PREREQUISITE: this patch sits on top of issue.misc-migrate (misc/ + etc/ +
# text/ -> state/ userdata/ worlddata/ help/). If that migration has not been
# applied on this server yet, run its script (migrate_lib_layout.sh) first. This
# script additionally renames the state/ list files to the .lst convention if it
# finds them under their old names.
#
# Run it in the server's data root (the directory that currently contains
# plrs/, plrobjs/, plrstuff/, world/, …) BEFORE starting the new binary.
#
# Usage:
#     tools/migrate_playerdata_layout.sh [DATA_DIR] [--dry-run] [--no-backup]
#         DATA_DIR      server data root (default: current directory)
#         --dry-run,-n  print what would happen, change nothing
#         --no-backup   skip the safety tarball (NOT recommended)
#
# Idempotent: re-running after a successful migration is a no-op.

set -euo pipefail

DATA_DIR="."
DRY_RUN=0
BACKUP=1
for arg in "$@"; do
	case "$arg" in
		-n|--dry-run) DRY_RUN=1 ;;
		--no-backup)  BACKUP=0 ;;
		-h|--help)    grep -E '^# ' "$0" | sed 's/^# \?//'; exit 0 ;;
		-*)           echo "unknown option: $arg" >&2; exit 2 ;;
		*)            DATA_DIR="$arg" ;;
	esac
done

cd "$DATA_DIR"; DATA_DIR="$(pwd)"

if [[ ! -d plrs && ! -d plrstuff && ! -d plrobjs && ! -d world ]]; then
	echo "ERROR: '$DATA_DIR' does not look like a server data root" >&2
	echo "       (expected one of: plrs/ plrstuff/ plrobjs/ world/)" >&2
	exit 1
fi

log()  { printf '%s\n' "$*"; }
run()  { if [[ $DRY_RUN -eq 1 ]]; then log "  [dry-run] $*"; else eval "$@"; fi; }
WARN=0
warn() { WARN=$((WARN+1)); log "  WARNING: $*"; }

log "issue.playerdata-migration data migration"
log "  data root : $DATA_DIR"
log "  mode      : $([[ $DRY_RUN -eq 1 ]] && echo 'DRY RUN (no changes)' || echo 'APPLY')"
log ""

# ---- 0. backup ----
if [[ $BACKUP -eq 1 && $DRY_RUN -eq 0 ]]; then
	STAMP="$(date +%Y%m%d-%H%M%S 2>/dev/null || echo backup)"
	TAR="playerdata-migration-backup-${STAMP}.tar.gz"
	SRC=(); for d in plrs plrobjs plralias plrvars plrstuff stat; do [[ -e "$d" ]] && SRC+=("$d"); done
	if [[ ${#SRC[@]} -gt 0 ]]; then log "Backing up ${SRC[*]} -> $TAR"; tar czf "$TAR" "${SRC[@]}"; log ""; fi
fi

# ---- 1. create the new directories ----
log "Creating new directories"
for d in state state/statistics worlddata userdata userdata/accounts userdata/clans \
         userdata/chardata userdata/chardata/characters userdata/chardata/items \
         userdata/chardata/aliases userdata/chardata/variables userdata/chardata/depots; do
	[[ -d "$d" ]] || run "mkdir -p '$d'"
done
log ""

# move_one SRC DST — move one file, never clobbering an existing DST.
move_one() {
	local src="$1" dst="$2"
	if [[ -e "$src" ]]; then
		if [[ -e "$dst" ]]; then warn "both '$src' and '$dst' exist — left '$src' in place"
		else run "mkdir -p '$(dirname "$dst")'"; run "mv '$src' '$dst'"; log "  moved : $src -> $dst"; fi
	elif [[ -e "$dst" ]]; then log "  ok    : $dst already in place"; fi
}

# move_merge SRC DST — move every file under SRC into DST, merging shard subdirs,
# never clobbering. Leaves SRC as empty dirs (removed later).
move_merge() {
	local src="$1" dst="$2"
	[[ -d "$src" ]] || return 0
	local rel
	while IFS= read -r rel; do
		rel="${rel#./}"
		if [[ -e "$dst/$rel" ]]; then warn "both '$src/$rel' and '$dst/$rel' exist — left source in place"; continue; fi
		run "mkdir -p '$dst/$(dirname "$rel")'"
		run "mv '$src/$rel' '$dst/$rel'"
	done < <(cd "$src" && find . -type f)
	log "  merged: $src/ -> $dst/"
}

# ---- 2. per-character sharded trees -> userdata/chardata/* ----
log "Moving per-character save trees -> userdata/chardata/"
# accounts live under plrs/ but are NOT per-character -> pull them out first
if [[ -d plrs/accounts ]]; then move_merge "plrs/accounts" "userdata/accounts"; fi
move_merge "plrs"     "userdata/chardata/characters"   # .player shards + players.lst/.dup
move_merge "plrobjs"  "userdata/chardata/items"        # .textobjs/.timeobjs
move_merge "plralias" "userdata/chardata/aliases"      # .alias
move_merge "plrvars"  "userdata/chardata/variables"    # .mem
# depot: parcel.db is not depot data (it reused the dir) -> userdata/ ; the rest -> depots/
move_one   "plrstuff/depot/parcel.db" "userdata/parcel.db"
move_merge "plrstuff/depot" "userdata/chardata/depots" # depot.db, purge.db + .pers/.share/.purge shards
log ""

# ---- 3. clan data -> userdata/clans/ ----
log "Moving clan data -> userdata/clans/"
move_merge "plrstuff/house" "userdata/clans"
move_one   "plrstuff/shop/item_desc.xml" "userdata/clans/item_desc.xml"
log ""

# ---- 4. player-owned loose files -> userdata/ ----
log "Moving player-owned files -> userdata/"
move_one "plrstuff/exchange.db"        "userdata/exchange.db"
move_one "plrstuff/exchange.backup"    "userdata/exchange.backup"
move_one "plrstuff/glory.lst"          "userdata/glory.lst"
move_one "plrstuff/glory_const.xml"    "userdata/glory_const.xml"
move_one "plrstuff/named_stuff_list.xml" "userdata/named_items.xml"   # renamed
log ""

# ---- 5. server state + statistics -> state/ , state/statistics/ ----
log "Moving server state -> state/ and state/statistics/"
move_one "plrstuff/titles.lst"          "state/titles.lst"
move_one "plrstuff/registered-email.lst" "state/registered-email.lst"
move_one "plrstuff/crc.lst"             "state/player_save_checksums.lst"   # renamed
move_one "plrstuff/unique_mobs.xml"     "state/unique_mobs.xml"
move_one "plrstuff/mob_stat.bin"        "state/statistics/mob_stat.bin"
move_one "plrstuff/mob_stat_new.xml"    "state/statistics/mob_stat_new.xml"
move_one "plrstuff/zone_traffic.xml"    "state/statistics/zone_traffic.xml"
move_one "plrstuff/global_drop.tmp"     "state/statistics/global_drop.tmp"
move_one "stat/spellstat.txt"           "state/statistics/spellstat.txt"
log ""

# ---- 5b. world content -> worlddata/ ----
log "Moving world content -> worlddata/"
move_one "world"    "worlddata/world"     # YAML/legacy world tree (rooms/mobs/objects/zones/triggers)
move_one "world.db" "worlddata/world.db"  # SQLite world (only on SQLite servers)
log ""

# ---- 6. rename state/ list files to the .lst convention (if present) ----
log "Renaming state/ list files to .lst"
for f in stop_offtop apr_name dis_name new_name xnames badsites badproxy; do
	move_one "state/$f" "state/$f.lst"
done
log ""

# ---- 7. delete files the new engine no longer uses ----
log "Removing files the new engine no longer uses"
delete_one() { local p="$1" why="$2"; if [[ -e "$p" ]]; then run "rm -rf '$p'"; log "  removed: $p   ($why)"; fi; }
delete_one "plrstuff/sets_drop.xml" "obsolete: the drop table is regenerated as state/sets_drop_generated_table.lst"
delete_one "plrstuff/shop/shops.xml" "orphan: shops load from cfg/economics/shops.xml"
delete_one "stat/wholist.html"      "dead: nothing generates it"
log ""

# ---- 8. retire the now-empty legacy directories ----
log "Retiring legacy directories if empty"
for d in plrs plrobjs plralias plrvars plrstuff/depot plrstuff/house plrstuff/shop plrstuff stat; do
	[[ -d "$d" ]] || continue
	find "$d" -type d -empty -delete 2>/dev/null || true
	if [[ -d "$d" ]]; then
		if [[ -z "$(ls -A "$d" 2>/dev/null)" ]]; then run "rmdir '$d'"; log "  removed empty: $d/"
		else warn "'$d/' is not empty — left in place. Remaining:"; ls -A "$d" | sed 's/^/             /'; fi
	fi
done
log ""

log "Done."
[[ $DRY_RUN -eq 1 ]] && log "This was a DRY RUN — nothing changed. Re-run without --dry-run to apply."
[[ $WARN -gt 0 ]] && log "$WARN warning(s) above need a human look (leftover files were NOT deleted)."
log ""
log "Next: deploy the new cfg/ overlay and binary, then start the server. On boot"
log "      it recreates any missing userdata/state directories and reads player,"
log "      account, clan and state data from the new locations. The generated"
log "      caches (sets_drop_generated_table.lst, unique_mobs.xml) rebuild on their"
log "      own if absent."

# vim: ts=4 sw=4 tw=0 noet :
