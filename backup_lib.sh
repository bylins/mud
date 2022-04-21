#!/bin/bash

DIRECTORY=$(dirname "${BASH_SOURCE[0]}")

tar  -cpjf ${DIRECTORY}/backup/lib.$(date +%d%m%y).tgz  ${DIRECTORY}/lib 
gdrive upload ${DIRECTORY}/backup/lib.$(date +%d%m%y).tgz

# vim: set ts=4 sw=4 tw=0 noet :
