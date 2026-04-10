#!/bin/sh
[ -z "$V" ] && V=/tmp/vitess/MODULES
[ -z "$P" ] && P=/tmp/vitess/FILES/EXAMPLES/ILL_Diffractometer_D20_Model
[ -z "$L" ] && L=/tmp/vitess673493a6vpipelog
[ -z "${SUFFIX}" ] && SUFFIX="_`uname -s`_`uname -m`"
GSL_RNG_SEED=1
export GSL_RNG_SEED
GSL_RNG_TYPE=ran3
export GSL_RNG_TYPE

$V/source${SUFFIX} -S1 --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N1 --L${L}01 -a$P/ILL_thml_2011.mod -n1e6 -l1 -m2.33 -M2.49 -d1 -b0.0 -c0.0 -y0.5 -z0.5 -D251.2 -w15 -h15 -i0 -s200 -X1 -Y0 -V0 -P0 -A0 -k0 | \
$V/frame${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N2 --L${L}02 -S1 -H0 -V0 -A45 -x0 -y0 -z0 -i0 -j0 -k0 | \
$V/window${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N3 --L${L}03 -l0 -R0 -r10 -y0 -z0 -w-8.85 -W8.85 -h-8.85 -H8.85 -S0 -F0 -c6 -t0.0 -T0 | \
$V/frame${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N4 --L${L}04 -S1 -H0 -V0 -A-45 -x0 -y0 -z0 -i0 -j0 -k0 | \
$V/mon2_pos${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N5 --L${L}05 -O$P/octogon.pos -w-10 -h-10 -W10 -H10 -y100 -z100 -p1 -e0 -F1 | \
$V/window${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N6 --L${L}06 -l102 -R1 -r5.65 -y0 -z0 -S0 -F0 -c6 -t0.0 -T0 | \
$V/window${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N7 --L${L}07 -l51 -R1 -r5.55 -y0 -z0 -S0 -F0 -c6 -t0.0 -T0 | \
$V/window${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N8 --L${L}08 -l95.7 -R1 -r5.75 -y0 -z0 -S0 -F0 -c6 -t0.0 -T0 | \
$V/window${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N9 --L${L}09 -l1140.1 -R0 -r10 -y0 -z0 -w-4.75 -W4.75 -h-15 -H15 -S0 -F0 -c6 -t0.0 -T0 | \
$V/collimator${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N10 --L${L}10 -w9.5 -h30 -W9.5 -H30 -l25.7 -n1 -s0.01 | \
$V/mon2_div${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N11 --L${L}11 -O$P/collim1.div -w-2 -h-2 -W2 -H2 -y80 -z80 -p1 -e0 -F0 | \
$V/monitor1${SUFFIX} -k3 --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N12 --L${L}12 -O$P/guide.mdy -n120 -C-1 -f0 -m-1 -M1 -p1 -e0 | \
$V/monitor1${SUFFIX} -k1 --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N13 --L${L}13 -O$P/guide.mtl -n50 -C-1 -m2.3 -M2.5 -f0 -p1 -e0 | \
$V/monochromator${SUFFIX} -O2 --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N14 --L${L}14 -P$P/Graphite.par -X1 -B0 -d1 -m0.8 -M0.6 -D0.0 -R0.9 -b0 -K0 -f0 -p0 -Q0 -n0 -q0 -w0 -c0 -C0 -A1 -G$P/lamb_foc.dat -g3 -H1 -V13 -I1 -s200 -r202 -a-3.4 -o0.0 -J0.0 -h0.0 -v0.0 -t0.0 -T0.0 | \
$V/monitor1${SUFFIX} -k1 --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N15 --L${L}15 -O$P/monochr.mtl -n50 -C-1 -m2.3 -M2.5 -f1 -p1 -e0 | \
$V/window${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N16 --L${L}16 -l40 -R0 -r1 -y0 -z0 -w-7.5 -W7.5 -h-15 -H15 -S0 -F0 -c6 -t0.0 -T0 | \
$V/monitor2D${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N17 --L${L}17 -O$P/PositionAfterFirstSpaceWindow.mon -X1 -Y2 -w-10 -h-10 -W10 -H10 -x100 -y100 -p1 -e0 -F1 -l-1.0 -L-1.0 -I0 -J0 -C0 -P0 -r1 -s0 -t0 | \
$V/window${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N18 --L${L}18 -l240 -R0 -r1 -y0 -z0 -w-1.5 -W1.5 -h-6 -H6 -S0 -F0 -c6 -t0.0 -T0 | \
$V/space${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N19 --L${L}19 -d39 | \
$V/mon2_pos${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N20 --L${L}20 -O$P/sample.pos -w-6 -h-6 -W6 -H6 -y120 -z120 -p1 -e0 -F1 | \
$V/monitor2D${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N21 --L${L}21 -O$P/Lambda_PosZ.mon -X5 -Y2 -w2.2 -h-6 -W2.6 -H6 -x80 -y120 -p1 -e0 -F1 -l-1.0 -L-1.0 -I0 -J0 -C0 -P0 -r1 -s0 -t0 | \
$V/monitor2D${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N22 --L${L}22 -O$P/Lambda_PosY.mon -X5 -Y1 -w2.2 -h-3 -W2.6 -H3 -x80 -y120 -p1 -e0 -F1 -l-1.0 -L-1.0 -I0 -J0 -C0 -P0 -r1 -s0 -t0 | \
$V/slit${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N23 --L${L}23 -d0.0 -W0.5 -H4.0 | \
$V/monitor1${SUFFIX} -k1 --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N24 --L${L}24 -O$P/sample.mtl -n50 -C-1 -m2.3 -M2.5 -f1 -p1 -e0 | \
$V/monitor1${SUFFIX} -k3 --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N25 --L${L}25 -O$P/DivY_AtSample.mon -n100 -C-1 -f0 -c0 -m-2 -M2 -p1 -e0 -l-1.0 -L-1.0 | \
$V/monitor2D${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N26 --L${L}26 -O$P/PositionBeforeSample.mon -X1 -Y2 -w-0.5 -h-3 -W0.5 -H3 -x50 -y60 -p1 -e0 -F1 -l-1.0 -L-1.0 -I0 -J0 -C0 -P0 -r1 -s0 -t0 | \
$V/sample_powder${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N27 --L${L}27 -D80 -d76.8 -P0 -p3.5 -A1 -I0 -S$P/SynE.pow -c1 -a0 | \
$V/detector${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N28 --L${L}28 -B1 -G1 -a1 -U0 -A1 -q-1 -Q-1 -d0 -P0 -T80 -D147 -h15 -w394 -t3 -r15 -c1536 -n1 -u0 -v0 -l0 -m5 -p4 -k293 -e0.9 -o0 -b0 -f0 -s0 -V0 -W0 -x2 -z0 | \
$V/eval_elast${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N29 --L${L}29 -k1 -o$P/elast_42_SynSample.dsp -n10000 -m0 -M10 -r2.405 -p1 -c0 -A-1 -w0 -t0 -T0 -e-1.e10 -E1.e10 --Fno_file
rm -f $P/result.txt
cat ${L}?? >> $P/result.txt
rm ${L}*
