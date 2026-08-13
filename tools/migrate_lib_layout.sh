#!/usr/bin/env bash
#
# migrate_lib_layout.sh — issue.misc-migrate production data migration.
#
# Reorganises a production server's runtime data directory to the new layout
# introduced on the issue.misc-migrate branch:
#
#     misc/  (catch-all)          -->  state/      (server-wide runtime state)
#     etc/   (bans, mail, boards) -->  userdata/   (per-account data + boards)
#     misc/stuff.lst              -->  worlddata/  (world content tables)
#     text/help/                  -->  help/       (help database)
#
# It MOVES live data files into the new directories, DELETES files that the new
# engine no longer reads, and leaves anything it does not recognise in place
# (with a warning) so nothing is lost silently.
#
# Run it in the server's data root (the directory that currently contains the
# `misc/`, `etc/`, `world/`, `plrs/` … subdirectories) BEFORE starting the new
# binary. The new engine auto-creates state/ userdata/ userdata/boards/
# worlddata/ on boot, but it will NOT find the existing bans, names, boards,
# mail, schedule or globaluid counter unless they have been moved here first.
#
# Usage:
#     tools/migrate_lib_layout.sh [DATA_DIR] [--dry-run] [--no-backup]
#
#     DATA_DIR      server data root (default: current directory)
#     --dry-run,-n  print what would happen, change nothing
#     --no-backup   skip the safety tarball (NOT recommended)
#
# The script is idempotent: re-running it after a successful migration is a
# no-op (sources already gone, destinations already present).

set -euo pipefail

# ---- args -------------------------------------------------------------------
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

cd "$DATA_DIR"
DATA_DIR="$(pwd)"

# ---- sanity: does this look like a MUD data root? ---------------------------
if [[ ! -d misc && ! -d etc && ! -d text && ! -d world ]]; then
	echo "ERROR: '$DATA_DIR' does not look like a server data root" >&2
	echo "       (expected one of: misc/ etc/ text/ world/)" >&2
	exit 1
fi

log()  { printf '%s\n' "$*"; }
run()  { if [[ $DRY_RUN -eq 1 ]]; then log "  [dry-run] $*"; else eval "$@"; fi; }

WARN=0
warn() { WARN=$((WARN+1)); log "  WARNING: $*"; }

log "issue.misc-migrate data migration"
log "  data root : $DATA_DIR"
log "  mode      : $([[ $DRY_RUN -eq 1 ]] && echo 'DRY RUN (no changes)' || echo 'APPLY')"
log ""

# ---- 0. backup --------------------------------------------------------------
if [[ $BACKUP -eq 1 && $DRY_RUN -eq 0 ]]; then
	# A plain timestamp; portable, no seconds-precision needed.
	STAMP="$(date +%Y%m%d-%H%M%S 2>/dev/null || echo backup)"
	TAR="lib-migration-backup-${STAMP}.tar.gz"
	SRC=()
	for d in misc etc text; do [[ -e "$d" ]] && SRC+=("$d"); done
	if [[ ${#SRC[@]} -gt 0 ]]; then
		log "Backing up ${SRC[*]} -> $TAR"
		tar czf "$TAR" "${SRC[@]}"
	fi
	log ""
fi

# ---- 1. create the new directories -----------------------------------------
log "Creating new directories (state/ userdata/ userdata/boards/ worlddata/ help/)"
for d in state userdata userdata/boards worlddata help; do
	if [[ -d "$d" ]]; then
		log "  exists: $d/"
	else
		run "mkdir -p '$d'"
	fi
done
log ""

# move_one SRC DST — move a single file, never clobbering an existing DST.
move_one() {
	local src="$1" dst="$2"
	if [[ -e "$src" ]]; then
		if [[ -e "$dst" ]]; then
			warn "both '$src' and '$dst' exist — leaving '$src' in place, resolve by hand"
		else
			run "mv '$src' '$dst'"
			log "  moved : $src -> $dst"
		fi
	elif [[ -e "$dst" ]]; then
		log "  ok    : $dst already in place"
	fi
	# src absent and dst absent: file simply never existed on this server — silent.
}

# ---- 2. server state: misc/ + etc/ -> state/ -------------------------------
log "Moving server-state files -> state/"
for f in schedule globaluid stop_offtop proxy xnames apr_name dis_name new_name unfreeze.lst; do
	move_one "misc/$f" "state/$f"
done
move_one "etc/badsites"  "state/badsites"
move_one "etc/badproxy"  "state/badproxy"
log ""

# ---- 3. world content: misc/stuff.lst -> worlddata/ ------------------------
log "Moving world-content tables -> worlddata/"
move_one "misc/stuff.lst" "worlddata/stuff.lst"
log ""

# ---- 4. account data: etc/ -> userdata/ ------------------------------------
log "Moving account data -> userdata/"
move_one "etc/plrmail.xml" "userdata/plrmail.xml"
if [[ -d etc/board ]]; then
	shopt -s nullglob dotglob
	moved_boards=0
	for b in etc/board/*; do
		[[ -e "$b" ]] || continue
		base="$(basename "$b")"
		[[ "$base" == "CVS" ]] && continue
		move_one "$b" "userdata/boards/$base"
		moved_boards=$((moved_boards+1))
	done
	shopt -u nullglob dotglob
	[[ $moved_boards -eq 0 ]] && log "  (no board files found under etc/board/)"
fi
log ""

# ---- 5. help database: text/help/ -> help/ ---------------------------------
log "Moving help database text/help/ -> help/"
if [[ -d text/help ]]; then
	shopt -s nullglob dotglob
	for h in text/help/*; do
		[[ -e "$h" ]] || continue
		base="$(basename "$h")"
		[[ "$base" == "CVS" ]] && continue          # legacy version-control cruft
		move_one "$h" "help/$base"
	done
	shopt -u nullglob dotglob
elif [[ -e help/index ]]; then
	log "  ok    : help/ already populated"
else
	warn "no text/help/ and no help/index — the server will FATAL on boot (help index is required)"
fi
log ""

# ---- 6. delete files the new engine no longer reads ------------------------
log "Removing files the new engine no longer uses"
delete_one() {
	local p="$1" why="$2"
	if [[ -e "$p" ]]; then
		run "rm -rf '$p'"
		log "  removed: $p   ($why)"
	fi
}
delete_one "misc/privilege.lst" "legacy privilege system removed; replaced by cfg/privilege.xml"
delete_one "etc/smtp.xml"       "orphan, never loaded"
delete_one "etc/plrmail"        "pre-XML mail store, superseded by userdata/plrmail.xml"
delete_one "text/godnews"       "dead; god news moved to boards"
delete_one "text/imotd"         "dead; superseded by system messages"
delete_one "text/info.hlp"      "stale duplicate; the live copy is help/info.hlp"
delete_one "text/spells.hlp"    "stale duplicate; the live copy is help/spells.hlp"
delete_one "misc/craft"         "unfinished metacraft config now ships in cfg/craft/metacraft via deploy"
# Legacy CVS metadata dirs — always cruft, safe to drop so the parents can retire.
for c in misc/CVS etc/CVS etc/board/CVS text/CVS text/help/CVS; do
	delete_one "$c" "legacy CVS metadata"
done
log ""

# ---- 7. retire now-empty legacy directories --------------------------------
log "Retiring legacy directories if empty"
for d in etc/board text/help misc etc text; do
	[[ -d "$d" ]] || continue
	if [[ -z "$(ls -A "$d" 2>/dev/null)" ]]; then
		run "rmdir '$d'"
		log "  removed empty: $d/"
	else
		warn "'$d/' is not empty — left in place. Remaining entries:"
		ls -A "$d" | sed 's/^/             /'
	fi
done
log ""

# ---- summary ----------------------------------------------------------------
log "Done."
if [[ $DRY_RUN -eq 1 ]]; then
	log "This was a DRY RUN — nothing was changed. Re-run without --dry-run to apply."
fi
if [[ $WARN -gt 0 ]]; then
	log "$WARN warning(s) above need a human look (leftover files were NOT deleted)."
fi
log ""
log "Next: deploy the new cfg/ overlay (adds cfg/privilege.xml, cfg/craft/metacraft/,"
log "      cfg/craft/… etc.) from lib.template, then start the new binary. On boot the"
log "      server recreates state/ userdata/ userdata/boards/ worlddata/ as needed and"
log "      reads its state, bans, names, boards, mail and help from the new locations."

# vim: ts=4 sw=4 tw=0 noet :
