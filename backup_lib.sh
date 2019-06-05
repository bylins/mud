#!/bin/bash

DIRECTORY=$(dirname "${BASH_SOURCE[0]}")

tar --exclude='${DIRECTORY}/lib/mirror' -cjf ${DIRECTORY}/backup/lib.$(date +%d%m%y).tgz ${DIRECTORY}/lib 

# vim: set ts=4 sw=4 tw=0 noet :
