#!/bin/sh
[ -z "$V" ] && V=/tmp/vitess/MODULES
[ -z "$P" ] && P=/tmp/vitess/tests/module_tests/Test_Source-1
[ -z "$L" ] && L=/tmp/vitess8305e68vpipelog
[ -z "${SUFFIX}" ] && SUFFIX="_`uname -s`_`uname -m`"
GSL_RNG_SEED=1
export GSL_RNG_SEED
GSL_RNG_TYPE=ran3
export GSL_RNG_TYPE

$V/source${SUFFIX} -S1 --Z1 --U1.0e-26 --G0 --T0 --B10000 --P$P --N1 --L${L}01 -a$P/SrcConst.mod -n500 -l1 -m1 -M6 -d1 -b0.0 -c0.0 -y0.5 -z0.5 -D200 -w3 -h3 -i0 -s200 -X0 -Y0 -V1 -P0 -A0 -k0 | \
$V/writeout${SUFFIX} --Z1 --U1.0e-26 --G0 --T0 --B10000 --P$P --N2 --L${L}02 -A$P/source-1_out.dat -a1 -h0 -f1 -F1 -S0 -I1.0 -c111111111 -C-1 --Fno_file
rm -f $P/result.txt
cat ${L}?? >> $P/result.txt
rm ${L}*
