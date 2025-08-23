#!/bin/sh
[ -z "$V" ] && V=/tmp/vitess/MODULES
[ -z "$P" ] && P=/tmp/vitess/FILES/EXAMPLES/SourceAI
[ -z "$L" ] && L=/tmp/vitess67d03267vpipelog
[ -z "${SUFFIX}" ] && SUFFIX="_`uname -s`_`uname -m`"

$V/source_ai${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N1 --L${L}01 -M$P/vae_benchmark.model -n10000 -b10 | \
$V/monitor1${SUFFIX} -k1 --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N2 --L${L}02 -O$P/lambda.dat -n100 -C-1 -m0 -M20 -f0 -c0 -p1 -e0 | \
$V/mon2_pos${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N3 --L${L}03 -O$P/pos.dat -w-6 -h-6 -W6 -H6 -y100 -z100 -p1 -e0 -F0 -l-1.0 -L-1.0 --Fno_file
rm -f $P/result.txt
cat ${L}?? >> $P/result.txt
rm ${L}*
