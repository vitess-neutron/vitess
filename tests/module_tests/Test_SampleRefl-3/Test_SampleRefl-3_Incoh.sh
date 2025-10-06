#!/bin/sh
[ -z "$V" ] && V=/tmp/vitess/MODULES
[ -z "$P" ] && P=/tmp/vitess/tests/module_tests/Test_SampleRefl-3
[ -z "$L" ] && L=/tmp/vitess8305cc6vpipelog
[ -z "${SUFFIX}" ] && SUFFIX="_`uname -s`_`uname -m`"
GSL_RNG_SEED=1
export GSL_RNG_SEED
GSL_RNG_TYPE=ran3
export GSL_RNG_TYPE

$V/read_in${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N1 --L${L}01 -f1 -F1 -A$P/sample_reflectom-3_in.dat -a1.0 -b0.0 -d0.0 -R1 -J0 -I1.0 -C-1 -t0 | \
$V/sample_reflectom${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N2 --L${L}02 -P$P/geom-3.ref -I$P/refl_d2o.dat -O1 -RZ -A0 -a1.0 -M0 -s0 -m0 -o0 -B1 -X0.05 -S50 -d50 -p11 -t7 | \
$V/writeout${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N3 --L${L}03 -A$P/sample_reflectom-3_out.dat -a1 -h0 -f1 -F1 -S0 -I1.0 -c111111111 -C-1 --Fno_file
rm -f $P/result.txt
cat ${L}?? >> $P/result.txt
rm ${L}*
