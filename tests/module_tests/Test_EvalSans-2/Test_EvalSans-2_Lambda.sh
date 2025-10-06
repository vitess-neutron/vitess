#!/bin/sh
[ -z "$V" ] && V=/tmp/vitess/MODULES
[ -z "$P" ] && P=/tmp/vitess/tests/module_tests/Test_EvalSans-2
[ -z "$L" ] && L=/tmp/vitess83058b9vpipelog
[ -z "${SUFFIX}" ] && SUFFIX="_`uname -s`_`uname -m`"

$V/read_in${SUFFIX} --Z1 --U1.0e-26 --G0 --T0 --B10000 --P$P --N1 --L${L}01 -f1 -F1 -A$P/eval_sans-2_in.dat -a1.0 -b0.0 -d0.0 -R1 -J0 -I1.0 -C-1 -t0 | \
$V/eval_sans${SUFFIX} --Z1 --U1.0e-26 --G0 --T0 --B10000 --P$P --N2 --L${L}02 -i$P/screen.emt -S$P/eval_sans-2_out.dat -I$P/isotropic.emt -n100 -m0.0002 -M0.1 -R5 -d0.01 -r5.0 -p0.0025 -w0 -t0 -e-1.e10 -E1.e10 -C-1 --Fno_file
rm -f $P/result.txt
cat ${L}?? >> $P/result.txt
rm ${L}*
