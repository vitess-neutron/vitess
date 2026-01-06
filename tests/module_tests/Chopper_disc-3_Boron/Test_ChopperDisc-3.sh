#!/bin/sh
[ -z "$V" ] && V=/Users/violini/coding/testing-folder/test_names/MODULES
[ -z "$P" ] && P=/Users/violini/coding/testing-folder/test_names/tests/module_tests/Chopper_disc-3
[ -z "$L" ] && L=/tmp/violini693ae987vpipelog
[ -z "${SUFFIX}" ] && SUFFIX="_`uname -s`_`uname -m`"
GSL_RNG_SEED=1
export GSL_RNG_SEED
GSL_RNG_TYPE=ran3
export GSL_RNG_TYPE

$V/read_in${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N1 --L${L}01 -f1 -F1 -A$P/chopper_disc-3_in.dat -a1.0 -b0.0 -d0.0 -R1 -J0 -I1.0 -C-1 -t0 | \
$V/chopper_disc${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N2 --L${L}02 -s6000 -o-72.8 -g2 -r0 -z1 -p1 -c0 -C$P/chop_105.dat -l0 | \
$V/writeout${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N3 --L${L}03 -A$P/chopper_disc-3_out.dat -a1 -h0 -f1 -F1 -S0 -I1.0 -c111111111 -C-1 --Fno_file
rm -f $P/result.txt
cat ${L}?? >> $P/result.txt
rm ${L}*
