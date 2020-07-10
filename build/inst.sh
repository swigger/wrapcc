#!/bin/sh

for i in ~/bin/*; do
	[ -f $i ] || continue
	[ -x $i ] || continue
	[ -L $i ] && continue
	grep WRAPCC_CXXFLAGS2 $i >/dev/null 2>&1 || continue
	echo $i
	install wrapcc $i
done
