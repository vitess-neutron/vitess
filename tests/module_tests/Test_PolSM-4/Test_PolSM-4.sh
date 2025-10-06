#!/bin/sh
[ -z "$V" ] && V=/tmp/vitess/MODULES
[ -z "$P" ] && P=/tmp/vitess/tests/module_tests/Test_PolSM-4
[ -z "$L" ] && L=/tmp/vitess7ac5dd1vpipelog
[ -z "${SUFFIX}" ] && SUFFIX="_`uname -s`_`uname -m`"
GSL_RNG_SEED=1
export GSL_RNG_SEED
GSL_RNG_TYPE=ran3
export GSL_RNG_TYPE

$V/read_in${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N1 --L${L}01 -f1 -F1 -A$P/polariser_SM-4_in.dat -a1.0 -b0.0 -d0.0 -R1 -J0 -I1.0 -C-1 -t0 | \
$V/polariser_sm${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N2 --L${L}02 -P$P/polariser_SM.par -U$P/mirr3+.dat -D$P/mirr1a.dat -a50 -b0 -c0 -V0 -R50 -E0 -G0 -h0 -v0 | \
$V/writeout${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N3 --L${L}03 -A$P/polariser_SM-4_out.dat -a1 -h0 -f1 -F1 -S0 -I1.0 -c111111111 -C-1 | \
$V/monitorpol_1d${SUFFIX} -k1 --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N4 --L${L}04 -O$P/p_lambda.dat -n100 -C-1 -m0 -M10 -p1 -e0 -a1 -b1 -c1 --Fno_file
rm -f $P/result.txt
cat ${L}?? >> $P/result.txt
rm ${L}*
