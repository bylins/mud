#!/bin/bash

DIRECTORY=$(dirname "${BASH_SOURCE[0]}")

tar -cjf ${DIRECTORY}/backup/lib-$(date +%d%m%y-%H%M%S).tar.bz2 ${DIRECTORY}/lib

# vim: set ts=4 sw=4 tw=0 noet :
