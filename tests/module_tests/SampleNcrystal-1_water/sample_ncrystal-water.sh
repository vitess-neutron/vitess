#!/bin/sh
[ -z "$V" ] && V=/tmp/vitess/MODULES
[ -z "$P" ] && P=/tmp/vitess/tests/module_tests/sample_ncrystal-water
[ -z "$L" ] && L=/tmp/vitess67926bb7vpipelog
[ -z "${SUFFIX}" ] && SUFFIX="_`uname -s`_`uname -m`"

$V/read_in${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N1 --L${L}01 -f1 -F1 -A$P/sample_ncrystal-water_in.dat -a1.0 -b0.0 -d0.0 -R1 -J0 -I1.0 -C-1 -t0 | \
$V/sample_ncrystal${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N2 --L${L}02 -f$P/LiquidWaterH2O_T293.6K.ncmat -T293.6 -d0.5 -m0.2 -R1 -G2 -X50 -Y0 -Z0 -t3 -g3 -w2.8 -o0 -O0 -M1.8 -h1 -k1 -l1 -a1 -b1 -c1 -H1 -K1 -L0 -A1 -B1 -C0 -x50 -y0 -z0 -u0 -U0 | \
$V/writeout${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N3 --L${L}03 -A$P/sample_ncrystal-water_out.dat -a1 -h0 -f1 -F1 -S0 -I1.0 -c111111111 -C-1 --Fno_file
rm -f $P/result.txt
cat ${L}?? >> $P/result.txt
rm ${L}*
