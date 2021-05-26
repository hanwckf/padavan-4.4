#!/bin/sh

sed '/Members:/q' $1 > .foo
awk '/Members:/,EOF' $1 | grep -v 'Members:' | sort >> .foo
rm -f $1
