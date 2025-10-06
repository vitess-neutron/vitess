#!/bin/sh
[ -z "$V" ] && V=/tmp/vitess/MODULES
[ -z "$P" ] && P=/tmp/vitess/tests/module_tests/Test_EvalSans-1
[ -z "$L" ] && L=/tmp/vitess830584bvpipelog
[ -z "${SUFFIX}" ] && SUFFIX="_`uname -s`_`uname -m`"

$V/read_in${SUFFIX} --Z1 --U1.0e-26 --G0 --T0 --B10000 --P$P --N1 --L${L}01 -f1 -F1 -A$P/eval_sans-1_in.dat -a1.0 -b0.0 -d0.0 -R1 -J0 -I1.0 -C-1 -t0 | \
$V/eval_sans${SUFFIX} --Z1 --U1.0e-26 --G0 --T0 --B10000 --P$P --N2 --L${L}02 -i$P/eval-sans-1_out.dat -n100 -m0.001 -M0.2 -R5 -d0.2 -p0.1 -w1 -t1 -l2010 -L1000 -T0.5 -e-1.e10 -E1.e10 -C-1 --Fno_file
rm -f $P/result.txt
cat ${L}?? >> $P/result.txt
rm ${L}*
