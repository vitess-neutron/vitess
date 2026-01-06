#!/bin/sh
[ -z "$V" ] && V=/tmp/vitess/MODULES
[ -z "$P" ] && P=/tmp/vitess/tests/module_tests/Test_Mono-4
[ -z "$L" ] && L=/tmp/vitess8305bacvpipelog
[ -z "${SUFFIX}" ] && SUFFIX="_`uname -s`_`uname -m`"
GSL_RNG_SEED=1
export GSL_RNG_SEED
GSL_RNG_TYPE=ran3
export GSL_RNG_TYPE

$V/read_in${SUFFIX} --Z21 --U1.0e-26 --G0 --T0 --B10000 --P$P --N1 --L${L}01 -f1 -F1 -A$P/monochromator-4_in.dat -a1.0 -b0.0 -d0.0 -R1 -J0 -I1.0 -C-1 -t0 | \
$V/monochromator${SUFFIX} -O2 --Z21 --U1.0e-26 --G0 --T0 --B10000 --P$P --N2 --L${L}02 -P$P/crys_Silicon111.par -X1 -B0 -d2 -m0.1 -M0.1 -D0.0 -R1.0 -b3 -K1 -f10 -p0 -Q7.5 -n0 -q0 -w0 -c0 -C0 -A1 -G$P/Doppler.dat -g2 -H7 -V7 -I1 -s218 -r218 -a0 -o0 -h0.0 -v0.0 -t0.0 -T0.0 | \
$V/writeout${SUFFIX} --Z21 --U1.0e-26 --G0 --T0 --B10000 --P$P --N3 --L${L}03 -A$P/monochromator-4_out.dat -a1 -h0 -f1 -F1 -S0 -I1.0 -c111111111 -C-1 -l-1.0 -L-1.0 --Fno_file
rm -f $P/result.txt
cat ${L}?? >> $P/result.txt
rm ${L}*
