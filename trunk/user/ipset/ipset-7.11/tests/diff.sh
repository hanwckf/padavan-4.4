#!/bin/bash

diff -u -I 'Revision: .*' -I 'Size in memory.*' \
    <(sed -e 's/timeout [0-9]*/timeout x/' -e 's/initval 0x[0-9a-fA-F]\{8\}/initval 0x00000000/' $1) \
    <(sed -e 's/timeout [0-9]*/timeout x/' -e 's/initval 0x[0-9a-fA-F]\{8\}/initval 0x00000000/' $2)


