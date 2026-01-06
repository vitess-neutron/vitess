#!/bin/sh
[ -z "$V" ] && V=/tmp/vitess/MODULES
[ -z "$P" ] && P=/tmp/vitess/tests/module_tests/Test_SM-Ensemble-1
[ -z "$L" ] && L=/tmp/vitess8305e4avpipelog
[ -z "${SUFFIX}" ] && SUFFIX="_`uname -s`_`uname -m`"
GSL_RNG_SEED=1
export GSL_RNG_SEED
GSL_RNG_TYPE=ran3
export GSL_RNG_TYPE

$V/read_in${SUFFIX} --Z1 --U1.0e-26 --G0 --T0 --B10000 --P$P --N1 --L${L}01 -f1 -F1 -A$P/sm-ensemble-1_in.dat -a1.0 -b0.0 -d0.0 -R1 -J0 -I1.0 -C-1 -t0 | \
$V/sm_ensemble${SUFFIX} --Z1 --U1.0e-26 --G0 --T0 --B10000 --P$P --N2 --L${L}02 -P$P/V-Polarizer.dat -F1 -S1 -R0 -Q2 -r200 -s0 -t0 -h0 -v0 -T0 -o1 -c0 -M1000 -C$P/collision.dat -m500 | \
$V/writeout${SUFFIX} --Z1 --U1.0e-26 --G0 --T0 --B10000 --P$P --N3 --L${L}03 -A$P/sm-ensemble-1_out.dat -a1 -h0 -f1 -F1 -S0 -I1.0 -c111111111 -C-1 --Fno_file
rm -f $P/result.txt
cat ${L}?? >> $P/result.txt
rm ${L}*
