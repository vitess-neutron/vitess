#!/bin/sh
[ -z "$V" ] && V=/tmp/vitess/MODULES
[ -z "$P" ] && P=/tmp/vitess/tests/module_tests/Test_Mon1Pol-1
[ -z "$L" ] && L=/tmp/vitess8305ac4vpipelog
[ -z "${SUFFIX}" ] && SUFFIX="_`uname -s`_`uname -m`"
GSL_RNG_SEED=1
export GSL_RNG_SEED
GSL_RNG_TYPE=ran3
export GSL_RNG_TYPE

$V/read_in${SUFFIX} --Z1 --U1.0e-26 --G0 --T0 --B10000 --P$P --N1 --L${L}01 -f1 -F1 -A$P/mon1pol-1_in.dat -a1.0 -b0.0 -d0.0 -R1 -J0 -I1.0 -C-1 -t0 | \
$V/mon1_pol${SUFFIX} -k1 --Z1 --U1.0e-26 --G0 --T0 --B10000 --P$P --N2 --L${L}02 -O$P/mon1pol-1_out.dat -n100 -C-1 -m0 -M10 -p1 -e0 -a0 -b0 -c1 --Fno_file
rm -f $P/result.txt
cat ${L}?? >> $P/result.txt
rm ${L}*
