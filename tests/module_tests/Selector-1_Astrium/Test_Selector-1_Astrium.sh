#!/bin/sh
[ -z "$V" ] && V=/tmp/vitess/MODULES
[ -z "$P" ] && P=/tmp/vitess/tests/module_tests/Test_Selector-1
[ -z "$L" ] && L=/tmp/vitess8305df3vpipelog
[ -z "${SUFFIX}" ] && SUFFIX="_`uname -s`_`uname -m`"

$V/read_in${SUFFIX} --Z1 --U1.0e-26 --G0 --T0 --B10000 --P$P --N1 --L${L}01 -f1 -F1 -A$P/velselect-1_in.dat -a1.0 -b0.0 -d0.0 -R1 -J0 -I1.0 -C-1 -t0 | \
$V/velselect${SUFFIX} --Z1 --U1.0e-26 --G0 --T0 --B10000 --P$P --N2 --L${L}02 -W5 -H5 -l25.0 -r14.5 -i8.5 -s400 -w72 -c48.3 -d0.04 -Y0.0 -Z-11.5 | \
$V/writeout${SUFFIX} --Z1 --U1.0e-26 --G0 --T0 --B10000 --P$P --N3 --L${L}03 -A$P/velselect-1_out.dat -a1 -h0 -f1 -F1 -S0 -I1.0 -c111111111 -C-1 --Fno_file
rm -f $P/result.txt
cat ${L}?? >> $P/result.txt
rm ${L}*
