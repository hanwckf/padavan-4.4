#!/bin/sh

grep -v Revision: $1 | sed 's/initval 0x[0-9a-fA-F]\{8\}/initval 0x00000000/' > .foo
rm $1
