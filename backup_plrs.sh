#!/bin/bash

DIRECTORY=$(dirname "${BASH_SOURCE[0]}")
PLRS_BACKUP_PREFIX=plrs

tar -cjf ${DIRECTORY}/backup/${PLRS_BACKUP_PREFIX}.$(date +%d%m%y-%H%M%S).tgz ${DIRECTORY}/lib/{plrs,plrobjs,plrstuff,plrvars,etc,plralias}
find ${DIRECTORY}/backup/ -name "${PLRS_BACKUP_PREFIX}.*" -atime 2 -exec rm '{}' \;

# vim: set ts=4 sw=4 tw=0 noet :
