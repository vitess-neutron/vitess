#!/bin/sh
[ -z "$V" ] && V=/tmp/vitess/MODULES
[ -z "$P" ] && P=/tmp/vitess/tests/module_tests/Test_SampleSofQ-4
[ -z "$L" ] && L=/tmp/vitess7bd8703vpipelog
[ -z "${SUFFIX}" ] && SUFFIX="_`uname -s`_`uname -m`"
GSL_RNG_SEED=1
export GSL_RNG_SEED
GSL_RNG_TYPE=ran3
export GSL_RNG_TYPE

$V/read_in${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N1 --L${L}01 -f1 -F1 -A$P/sample_sofq-4_in.dat -a1.0 -b0.0 -d0.0 -R1 -J0 -I1.0 -C-1 -t0 | \
$V/sample_s_q${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N2 --L${L}02 -D90 -d90 -P180 -p180 -A1 -I0 -S$P/glass.psq -f1e3 -o10.0 | \
$V/writeout${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N3 --L${L}03 -A$P/sample_sofq-4_out.dat -a1 -h0 -f1 -F1 -S0 -I1.0 -c111111111 -C-1 --Fno_file
rm -f $P/result.txt
cat ${L}?? >> $P/result.txt
rm ${L}*
