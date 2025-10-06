#!/bin/sh
[ -z "$V" ] && V=/tmp/vitess/MODULES
[ -z "$P" ] && P=/tmp/vitess/tests/module_tests/Test_CaptureFlux
[ -z "$L" ] && L=/tmp/vitess8304e94vpipelog
[ -z "${SUFFIX}" ] && SUFFIX="_`uname -s`_`uname -m`"

$V/read_in${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N1 --L${L}01 -f1 -F1 -A$P/capture-flux_in.dat -a1.0 -b0.0 -d0.0 -R1 -J0 -I1.0 -C-1 -t0 | \
$V/capture_flux${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N2 --L${L}02 -R1.798 -t2 -w-3 -W3 -h-3 -H3 | \
$V/writeout${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N3 --L${L}03 -A$P/capture-flux_out.dat -a1 -h0 -f1 -F1 -S0 -I1.0 -c111111111 -C-1 --Fno_file
rm -f $P/result.txt
cat ${L}?? >> $P/result.txt
rm ${L}*
