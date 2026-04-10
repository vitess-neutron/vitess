#!/bin/sh
[ -z "$V" ] && V=/tmp/vitess/MODULES
[ -z "$P" ] && P=/tmp/vitess/tests/module_tests/Test_detector-2
[ -z "$L" ] && L=/tmp/vitess75befb3vpipelog
[ -z "${SUFFIX}" ] && SUFFIX="_`uname -s`_`uname -m`"
GSL_RNG_SEED=1
export GSL_RNG_SEED
GSL_RNG_TYPE=ran3
export GSL_RNG_TYPE

$V/read_in${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N1 --L${L}01 -f1 -F1 -A$P/detector-2_in.dat -a1.0 -b0.0 -d0.0 -R1 -J0 -I1.0 -C-1 -t0 | \
$V/detector${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N2 --L${L}02 -B0 -G2 -a1 -U1 -A1 -q-1 -Q-1 -S-1 -d0 -P0 -T0 -D250 -h100 -w100 -t2 -r100 -c100 -n1 -u0.1 -v0.1 -l0.1 -m1 -p4 -k293 -e1 -o1 -b0 -f0 -s0 -V0 -W0 -x2 -z0 -O$P/detector-2_out.dat --Fno_file
rm -f $P/result.txt
cat ${L}?? >> $P/result.txt
rm ${L}*
