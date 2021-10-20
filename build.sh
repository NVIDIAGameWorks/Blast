#!/bin/bash
set -e
SCRIPT_DIR=$(dirname ${BASH_SOURCE})
mkdir -p "$SCRIPT_DIR/_capnp"
source "$SCRIPT_DIR/repo.sh" build $@ || exit $?
