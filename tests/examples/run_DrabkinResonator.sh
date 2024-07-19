#!/bin/sh
[ -z "$V" ] && V=/tmp/vitess/MODULES
[ -z "$P" ] && P=/tmp/vitess/FILES/EXAMPLES/DrabkinResonator
[ -z "$L" ] && L=/tmp/vitess66618011vpipelog
$V/source_Linux_x86_64 -S1 --Z1 --U1.e-25 --G1 --T0 --B10000 --P$P --N1 --L${L}01 -a$P/s.dat -n20000 -l1 -m1 -t0 -M6 -T0 -d0 -b0.0 -c0.0 -y0.5 -z0.5 -D0 -w10 -h10 -i0 -s200 -X0 -Y0 -V1 -P100 -A0 -k0 | $V/monitorpol_1d_Linux_x86_64 -k1 --Z1 --U1.e-25 --G1 --T0 --B10000 --P$P --N2 --L${L}02 -O$P/pin_lambdax.dat -n1000 -C-1 -m1 -M6 -p1 -e0 -a1 -b0 -c0 | $V/monitorpol_1d_Linux_x86_64 -k1 --Z1 --U1.e-25 --G1 --T0 --B10000 --P$P --N3 --L${L}03 -O$P/pin_lambday.dat -n1000 -C-1 -m1 -M6 -p1 -e0 -a0 -b1 -c0 | $V/monitorpol_1d_Linux_x86_64 -k1 --Z1 --U1.e-25 --G1 --T0 --B10000 --P$P --N4 --L${L}04 -O$P/pin_lambdaz.dat -n1000 -C-1 -m1 -M6 -p1 -e0 -a0 -b0 -c1 | $V/resonator_drabkin_Linux_x86_64 --Z1 --U1.e-25 --G1 --T0 --B10000 --P$P --N5 --L${L}05 -X40 -Y10 -V10 -k20 -l0 -m0 -p40 -r0 -s0 -C400 -D20 -E20 -M0 -d1.05 -v1 -a0 -e1 -x1 -I0 -A0 -K170 -q0 -S0 -O$P/revp.dat -N$P/revm.dat | $V/monitorpol_1d_Linux_x86_64 -k1 --Z1 --U1.e-25 --G1 --T0 --B10000 --P$P --N6 --L${L}06 -O$P/pex_lambdax.dat -n1000 -C-1 -m1 -M6 -p1 -e0 -a1 -b0 -c0 | $V/monitorpol_1d_Linux_x86_64 -k1 --Z1 --U1.e-25 --G1 --T0 --B10000 --P$P --N7 --L${L}07 -O$P/pex_lambday.dat -n1000 -C-1 -m1 -M6 -p1 -e0 -a0 -b1 -c0 | $V/monitorpol_1d_Linux_x86_64 -k1 --Z1 --U1.e-25 --G1 --T0 --B10000 --P$P --N8 --L${L}08 -O$P/pex_lambdaz.dat -n1000 -C-1 -m1 -M6 -p1 -e0 -a0 -b0 -c1 --F$P/noutput.dat
rm -f $P/result.txt
cat ${L}?? >> $P/result.txt
rm ${L}*
