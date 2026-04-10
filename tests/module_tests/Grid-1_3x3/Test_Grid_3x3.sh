#!/bin/sh
[ -z "$V" ] && V=/tmp/vitess/MODULES
[ -z "$P" ] && P=/tmp/vitess/tests/module_tests/Test_Grid-1
[ -z "$L" ] && L=/tmp/vitess8305a57vpipelog
[ -z "${SUFFIX}" ] && SUFFIX="_`uname -s`_`uname -m`"

$V/read_in${SUFFIX} --Z1 --U1.0e-26 --G1 --T0 --B10000 --P$P --N1 --L${L}01 -f1 -F1 -A$P/grid-1_in.dat -a1.0 -b0.0 -d0.0 -R1 -J0 -I1.0 -C-1 -t0 | \
$V/grid${SUFFIX} --Z1 --U1.0e-26 --G1 --T0 --B10000 --P$P --N2 --L${L}02 -I$P/grid_6x6.dat -N0 -K1 -D200 -d0.0 -e0.0 -t0 -a12 -b12 -c6 -X0.0 -y0.0 -q0.0 -h0.0 -H0.0 -M200 -m200 -n25 | \
$V/writeout${SUFFIX} --Z1 --U1.0e-26 --G1 --T0 --B10000 --P$P --N3 --L${L}03 -A$P/grid-1_out.dat -a1 -h0 -f1 -F1 -S0 -I1.0 -c111111111 -C-1 -l-1.0 -L-1.0 --Fno_file
rm -f $P/result.txt
cat ${L}?? >> $P/result.txt
rm ${L}*
