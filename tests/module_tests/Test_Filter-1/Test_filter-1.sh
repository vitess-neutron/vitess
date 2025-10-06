#!/bin/sh
[ -z "$V" ] && V=/tmp/vitess/MODULES
[ -z "$P" ] && P=/tmp/vitess/tests/module_tests/Test_filter-1
[ -z "$L" ] && L=/tmp/vitess7977639vpipelog
[ -z "${SUFFIX}" ] && SUFFIX="_`uname -s`_`uname -m`"

$V/read_in${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N1 --L${L}01 -f1 -F1 -A$P/filter-1_in.dat -a1.0 -b0.0 -d0.0 -R1 -J0 -I1.0 -C-1 -t0 | \
$V/filter${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N2 --L${L}02 -I1 -u-1 -U1 -J3 -v-0.3 -V0.3 -K5 -w3.5 -W5.5 -L0 -C1 | \
$V/filter${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N3 --L${L}03 -I2 -u-1 -U1 -J4 -v-0.3 -V0.3 -K6 -w2500 -W6000 -L0 -C1 | \
$V/writeout${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N4 --L${L}04 -A$P/filter-1_out.dat -a1 -h0 -f1 -F1 -S0 -I1.0 -c111111111 -C-1 --Fno_file
rm -f $P/result.txt
cat ${L}?? >> $P/result.txt
rm ${L}*
