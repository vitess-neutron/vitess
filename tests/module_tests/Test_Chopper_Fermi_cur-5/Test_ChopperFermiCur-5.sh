#!/bin/sh
[ -z "$V" ] && V=/tmp/vitess/MODULES
[ -z "$P" ] && P=/tmp/vitess/tests/module_tests/Test_Chopper_Fermi_cur-5
[ -z "$L" ] && L=/tmp/vitess75976f3vpipelog
[ -z "${SUFFIX}" ] && SUFFIX="_`uname -s`_`uname -m`"

$V/read_in${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N1 --L${L}01 -f1 -F1 -A$P/chopper_fermi_cur-5_in.dat -a1.0 -b0.0 -d0.0 -R1 -J0 -I1.0 -C-1 -t0 | \
$V/chopper_fermi_parallel${SUFFIX} -O2 --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N2 --L${L}02 -X10 -Y0 -V0 -a5 -b5 -c3 -l20 -m0.02 -r7.1 -n300 -q-74.529 -L1.3 -z1 -g1 -G$P/ch_fermi_geom.dat -p8 | \
$V/writeout${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N3 --L${L}03 -A$P/chopper_fermi_cur-5_out.dat -a1 -h0 -f1 -F1 -S0 -I1.0 -c111111111 -C-1 --Fno_file
rm -f $P/result.txt
cat ${L}?? >> $P/result.txt
rm ${L}*
