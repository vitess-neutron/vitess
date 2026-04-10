#!/bin/sh
[ -z "$V" ] && V=/tmp/vitess/MODULES
[ -z "$P" ] && P=/tmp/vitess/tests/module_tests/Test_Mono-5
[ -z "$L" ] && L=/tmp/vitess8305bc5vpipelog
[ -z "${SUFFIX}" ] && SUFFIX="_`uname -s`_`uname -m`"
GSL_RNG_SEED=1
export GSL_RNG_SEED
GSL_RNG_TYPE=ran3
export GSL_RNG_TYPE

$V/read_in${SUFFIX} --Z21 --U1.0e-26 --G0 --T0 --B10000 --P$P --N1 --L${L}01 -f1 -F1 -A$P/monochromator-5_in.dat -a1.0 -b0.0 -d0.0 -R1 -J0 -I1.0 -C-1 -t0 | \
$V/monochromator${SUFFIX} -O2 --Z21 --U1.0e-26 --G0 --T0 --B10000 --P$P --N2 --L${L}02 -P$P/crys.par -X1 -B0 -d2 -m0.4 -M0.4 -D0.0001 -R0.9 -b1 -K1 -f10 -p-90 -Q0 -n0 -q0 -w0 -c0 -C0 -A1 -G$P/Mono-RotCyl.dat -g3 -H1 -V9 -I1 -s0 -r200 -a-1.718878 -o2.0 -J-1.0 -h0.0 -v0.0 -t0.0 -T0.0 | \
$V/writeout${SUFFIX} --Z21 --U1.0e-26 --G0 --T0 --B10000 --P$P --N3 --L${L}03 -A$P/monochromator-5_out.dat -a1 -h0 -f1 -F1 -S0 -I1.0 -c111111111 -C-1 -l-1.0 -L-1.0 --Fno_file
rm -f $P/result.txt
cat ${L}?? >> $P/result.txt
rm ${L}*
