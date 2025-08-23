#!/bin/sh
[ -z "$V" ] && V=/tmp/vitess/MODULES
[ -z "$P" ] && P=/tmp/vitess/FILES/EXAMPLES/FlipperGradient
[ -z "$L" ] && L=/tmp/vitess66854501vpipelog
SUFFIX="$(uname -s)_$(uname -m)"
$V/source_${SUFFIX} -S1 --Z1 --U1.e-25 --G1 --T0 --B10000 --P$P --N1 --L${L}01 -a$P/s.dat -n20000 -l1 -m1 -t0 -M20 -T0 -d0 -b0.0 -c0.0 -y0.5 -z0.5 -D0 -w10 -h10 -i0 -s200 -X0 -Y0 -V1 -P100 -A0 -k0 | $V/monitorpol_1d_${SUFFIX} -k1 --Z1 --U1.e-25 --G1 --T0 --B10000 --P$P --N2 --L${L}02 -O$P/pin_lambdax.dat -n100 -C-1 -m0 -M20 -p1 -e0 -a1 -b0 -c0 | $V/monitorpol_1d_${SUFFIX} -k1 --Z1 --U1.e-25 --G1 --T0 --B10000 --P$P --N3 --L${L}03 -O$P/pin_lambday.dat -n100 -C-1 -m0 -M20 -p1 -e0 -a0 -b1 -c0 | $V/monitorpol_1d_${SUFFIX} -k1 --Z1 --U1.e-25 --G1 --T0 --B10000 --P$P --N4 --L${L}04 -O$P/pin_lambdaz.dat -n100 -C-1 -m0 -M20 -p1 -e0 -a0 -b0 -c1 | $V/flipper_gradient_${SUFFIX} --Z1 --U1.e-25 --G1 --T0 --B10000 --P$P --N5 --L${L}05 -k5 -l0 -m0 -X10 -Y10 -V10 -C50 -D25 -E25 -i0 -p10 -r0 -s0 -d15 -w-288723.6 -z0 -M2 -h0 -y0 -a0 -b0 -e0 -v0 -n1 -I0 -A0 -K84 -P0 -Q0 -R114 -q0 -u1 -t0 -S0 -O$P/revp.dat -N$P/revm.dat | $V/monitorpol_1d_${SUFFIX} -k1 --Z1 --U1.e-25 --G1 --T0 --B10000 --P$P --N6 --L${L}06 -O$P/pex_lambdax.dat -n100 -C-1 -m0 -M20 -p1 -e0 -a1 -b0 -c0 | $V/monitorpol_1d_${SUFFIX} -k1 --Z1 --U1.e-25 --G1 --T0 --B10000 --P$P --N7 --L${L}07 -O$P/pex_lambday.dat -n100 -C-1 -m0 -M20 -p1 -e0 -a0 -b1 -c0 | $V/monitorpol_1d_${SUFFIX} -k1 --Z1 --U1.e-25 --G1 --T0 --B10000 --P$P --N8 --L${L}08 -O$P/pex_lambdaz.dat -n100 -C-1 -m0 -M20 -p1 -e0 -a0 -b0 -c1 --F$P/noutput.dat
rm -f $P/result.txt
cat ${L}?? >> $P/result.txt
rm ${L}*
