#!/bin/sh
[ -z "$V" ] && V=/tmp/vitess/MODULES
[ -z "$P" ] && P=/tmp/vitess/tests/module_tests/Test_Guide-8
[ -z "$L" ] && L=/tmp/vitess799ea56vpipelog
[ -z "${SUFFIX}" ] && SUFFIX="_`uname -s`_`uname -m`"

$V/read_in${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N1 --L${L}01 -f1 -F1 -A$P/guide-8_in.dat -a1.0 -b0.0 -d0.0 -R1 -J0 -I1.0 -C-1 -t0 | \
$V/guide${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N2 --L${L}02 -Y3 -Z3 -S$P/guide_shape_out.dat -w3 -h3 -W5 -H5 -p50 -N10 -L3 -Q3 -G3 -M0 -m0 -a0 -g-1 -A0 -l0 -r0 -n0 -v0 -e0 -E0 -c0 -C0 -d0 -D0 -B0 -T10 -V17 -k1000 -x0 -X10000 -K100 -u0 -U10 | \
$V/writeout${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N3 --L${L}03 -A$P/guide-8_out.dat -a1 -h0 -f1 -F1 -S0 -I1.0 -c111111111 -C-1 --Fno_file
rm -f $P/result.txt
cat ${L}?? >> $P/result.txt
rm ${L}*
