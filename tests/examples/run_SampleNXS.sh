#!/bin/sh
[ -z "$V" ] && V=/tmp/vitess/MODULES
[ -z "$P" ] && P=/tmp/vitess/FILES/EXAMPLES/SampleNXS
[ -z "$L" ] && L=/tmp/vitess6661890dvpipelog
SUFFIX="$(uname -s)_$(uname -m)"
$V/source_${SUFFIX} -S1 --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N1 --L${L}01 -a$P/ReactorCold.mod -n1e8 -l1 -m0.5 -M2.5 -d1 -b0.0 -c0.0 -y0.5 -z0.5 -D100 -w1 -h1 -i0 -s200 -X0 -Y0 -V1 -P0 -A0 -k0 | $V/slit_${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N2 --L${L}02 -d100 -W0.2 -H1 | $V/frame_${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N3 --L${L}03 -S1 -H0 -V0 -A0 -x10 -y0 -z0 -i0 -j0 -k0 | $V/monitor1_${SUFFIX} -k1 --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N4 --L${L}04 -O$P/lambda_open.dat -n300 -C-1 -m0.5 -M5 -f0 -c0 -p1 -e0 | $V/sample_nxs_${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N5 --L${L}05 -D90 -d1 -P0 -p1 -A1 -I1 -S$P/nxs_sample.par -a1 -T0 | $V/frame_${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N6 --L${L}06 -S1 -H90 -V0 -A0 -x100 -y0 -z0 -i0 -j0 -k0 | $V/slit_${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N7 --L${L}07 -d0 -W0.5 -H10 | $V/monitor1_${SUFFIX} -k1 --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N8 --L${L}08 -O$P/lambda_2.dat -n3000 -C-1 -m0.5 -M2.5 -f0 -c0 -p1 -e0 --F$P/output.dat
rm -f $P/result.txt
cat ${L}?? >> $P/result.txt
rm ${L}*
