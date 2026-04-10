#!/bin/sh
[ -z "$V" ] && V=/tmp/vitess/MODULES
[ -z "$P" ] && P=/tmp/vitess/tests/module_tests/Test_Source-8
[ -z "$L" ] && L=/tmp/beule685157acvpipelog
[ -z "${SUFFIX}" ] && SUFFIX="_`uname -s`_`uname -m`"
GSL_RNG_SEED=21
export GSL_RNG_SEED
GSL_RNG_TYPE=ran3
export GSL_RNG_TYPE

$V/source${SUFFIX} -S2 --Z21 --U1.0e-26 --G0 --T0 --B10000 --P$P --N1 --L${L}01 -R50 -N- -L1 -a$P/IpnsThermPois.mod -n1000 -l1 -m0.1 -t0.001 -M20 -T0.045 -d2 -D150 -w6 -h10 -i0 -X0 -Y0 -V-1 -P0 -A0 -k0 | \
$V/writeout${SUFFIX} --Z21 --U1.0e-26 --G0 --T0 --B10000 --P$P --N2 --L${L}02 -A$P/source-8_out.dat -a1 -h1 -f1 -F1 -S0 -I1.0 -c111111111 -C-1 --Fno_file
rm -f $P/result.txt
cat ${L}?? >> $P/result.txt
rm ${L}*
