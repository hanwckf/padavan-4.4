#!/bin/sh

DIR="toolchain-4.4.x"
DL_NAME="mipsel-linux-uclibc.tar.xz"
DL_URL="https://github.com/hanwckf/padavan-toolchain/releases/download/linux-4.4-v1.0/$DL_NAME"

if [ -d $DIR ]; then
	echo "$DIR exists!"
	exit
fi

curl -O -L $DL_URL && \
mkdir -p $DIR && \
tar -xvf $DL_NAME -C $DIR
