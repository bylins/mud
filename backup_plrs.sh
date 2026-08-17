#!/bin/bash

DIRECTORY=$(dirname "${BASH_SOURCE[0]}")
PLRS_BACKUP_PREFIX=plrs

# Player/account/clan data all lives under userdata/ now; state/ holds the
# server-state bits the old backup caught via etc/ and plrstuff/ (bans, name
# moderation, titles, registered emails). Both were plrs/plrobjs/plrstuff/
# plrvars/etc/plralias before issue.playerdata-migration.
tar -cpjf ${DIRECTORY}/backup/${PLRS_BACKUP_PREFIX}.$(date +%d%m%y-%H%M%S).tgz ${DIRECTORY}/lib/{userdata,state}
find ${DIRECTORY}/backup/ -name "${PLRS_BACKUP_PREFIX}.*" -atime 2 -exec rm '{}' \;

# vim: set ts=4 sw=4 tw=0 noet :
