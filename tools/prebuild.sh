#!/bin/bash
set -e

CAPNP_GEN=$(dirname ${BASH_SOURCE})/../source/sdk/extensions/serialization/generated
mkdir -p "$CAPNP_GEN"

CAPNP_DIR=$(dirname ${BASH_SOURCE})/../_build/host-deps/CapnProto
CAPNP_BIN=$CAPNP_DIR/tools/ubuntu64
CAPNP_SRC=$CAPNP_DIR/src

SRC_DIR=$(dirname ${BASH_SOURCE})/../source/sdk/extensions/serialization

$CAPNP_BIN/capnp compile -o $CAPNP_BIN/capnpc-c++:$CAPNP_GEN -I $CAPNP_SRC --src-prefix $SRC_DIR $SRC_DIR/NvBlastExtLlSerialization-capn
$CAPNP_BIN/capnp compile -o $CAPNP_BIN/capnpc-c++:$CAPNP_GEN -I $CAPNP_SRC --src-prefix $SRC_DIR $SRC_DIR/NvBlastExtTkSerialization-capn
