#!/bin/sh
[ -z "$V" ] && V=/tmp/vitess/MODULES
[ -z "$P" ] && P=/tmp/vitess/tests/module_tests/Test_EvalElast2-1
[ -z "$L" ] && L=/tmp/vitess83057cavpipelog
[ -z "${SUFFIX}" ] && SUFFIX="_`uname -s`_`uname -m`"

$V/read_in${SUFFIX} --Z1 --U1.0e-26 --G0 --T0 --B10000 --P$P --N1 --L${L}01 -f1 -F1 -A$P/eval_elast2-1_in.dat -a1.0 -b0.0 -d0.0 -R1 -J0 -I1.0 -C-1 -t0 | \
$V/eval_elast2${SUFFIX} --Z1 --U1.0e-26 --G0 --T0 --B10000 --P$P --N2 --L${L}02 -k1 -F1 -s3 -f1 -o$P/eval_elast2-1_out.dat -n180 -x0 -X180 -m180 -y0 -Y4.5 -p1 -c0 -D0 -w0 -t1 -l2101 -L100 -T0.2 -e-1.e10 -E1.e10 -C-1 -a-1 -A-1 --Fno_file
rm -f $P/result.txt
cat ${L}?? >> $P/result.txt
rm ${L}*
