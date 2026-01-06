#!/bin/sh
[ -z "$V" ] && V=/tmp/vitess/MODULES
[ -z "$P" ] && P=/tmp/vitess/tests/module_tests/Test_Mono-7
[ -z "$L" ] && L=/tmp/vitess8305c0bvpipelog
[ -z "${SUFFIX}" ] && SUFFIX="_`uname -s`_`uname -m`"
GSL_RNG_SEED=1
export GSL_RNG_SEED
GSL_RNG_TYPE=ran3
export GSL_RNG_TYPE

$V/read_in${SUFFIX} --Z21 --U1.0e-26 --G0 --T0 --B10000 --P$P --N1 --L${L}01 -A$P/monochromator-7_in.dat | \
$V/monochromator${SUFFIX} -O3 --Z21 --U1.0e-26 --G0 --T0 --B10000 --P$P --N2 --L${L}02 -P$P/crys.par -X1 -B0 -d2 -m0.3 -M0.3 -D0.00005 -R0.8 -b0 -K0 -f0 -p0 -Q0 -n0 -q0 -w0 -c0 -C0 -A1 -G$P/Mono-DblFoc.dat | \
$V/writeout${SUFFIX} --Z21 --U1.0e-26 --G0 --T0 --B10000 --P$P --N3 --L${L}03 -A$P/monochromator-7_out.dat -c111111111 --Fno_file
rm -f $P/result.txt
cat ${L}?? >> $P/result.txt
rm ${L}*
