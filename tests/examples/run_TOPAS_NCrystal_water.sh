#!/bin/sh
[ -z "$V" ] && V=/tmp/vitess/MODULES
[ -z "$P" ] && P=/tmp/vitess/FILES/EXAMPLES/TOPAS_NCrystal_water
[ -z "$L" ] && L=/tmp/vitess67926bb7vpipelog
[ -z "${SUFFIX}" ] && SUFFIX="_`uname -s`_`uname -m`"

$V/source${SUFFIX} -S1 --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N1 --L${L}01 -a$P/FrmTherm2.mod -n1e6 -l1 -m1.2 -t0 -M1.4 -T0.3 -d1 -b0.0 -c0.0 -y0.5 -z0.5 -D164 -w1.51 -h5.01 -i0 -s200 -X0 -Y0 -V1 -P100 -A0 -k0 | \
$V/guide${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N2 --L${L}02 -Y5 -Z5 -S$P/GuidevorBeamstop.txt -w6 -h10 -W6 -H10 -p0 -N1 -R0 -f0 -F0 -L1 -Q1 -G1 -i$P/superm2-.dat -I$P/superm2-.dat -j$P/superm2-.dat -M0 -m0 -a0 -g-1 -A0 -r0 -n0 -O-1 -v0 -e0 -E0 -c0 -C0 -d0 -D0 -B0 -t18 -T10 -V17 -k1000 -x0 -X10000 -K100 -u0 -U10 | \
$V/capture_flux${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N3 --L${L}03 -R1.798 -t2 -r0.5 -y0 -z0 -w-1.05 -W1.05 -h-2.45 -H2.45 -l0.0 -L0.0 | \
$V/beamstop${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N4 --L${L}04 -d0 -R0 -p0 -W2.1 -H4.9 | \
$V/sm_ensemble${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N5 --L${L}05 -P$P/sm_beamstop_1.txt -F1 -S1 -R0 -Q-1 -r100 -s0 -t0 -h0 -v0 -T0 -o2 -c1 -w-5 -W105 -a-10 -A10 -M1000 -C$P/collision.dat -m500 | \
$V/beamstop${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N6 --L${L}06 -d0 -R0 -p0 -W2.1 -H4.9 | \
$V/guide${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N7 --L${L}07 -Y5 -Z5 -S$P/GuideBeamstopFC1.txt -w6 -h10 -W6 -H10 -p0 -N1 -R0 -f0 -F0 -L1 -Q1 -G1 -i$P/superm2-.dat -I$P/superm2-.dat -j$P/superm2-.dat -M0 -m0 -a0 -g-1 -A0 -r0 -n0 -O-1 -v0 -e0 -E0 -c0 -C0 -d0 -D0 -B0 -t18 -T10 -V17 -k1000 -x0 -X10000 -K100 -u0 -U10 | \
$V/capture_flux${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N8 --L${L}08 -R1.798 -t2 -r0.5 -y0 -z0 -w-2.7 -W2.7 -h-4.15 -H4.15 -l0.0 -L0.0 | \
$V/chopper_fermi${SUFFIX} -O1 --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N9 --L${L}09 -X10 -Y0 -V0 -a8.5 -b5.5 -c2.5 -l54 -m0.012 -r6.1 -n300 -q-36.524 -z1 -p4 | \
$V/space${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N10 --L${L}10 -d10 -M0 -m0 | \
$V/monitor1${SUFFIX} -k1 --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N11 --L${L}11 -O$P/postFC1.lda -n100 -C-1 -m0.5 -M5.5 -f0 -c0 -p1 -e0 | \
$V/guide${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N12 --L${L}12 -Y5 -Z5 -S$P/GuideFC1HOR.txt -w6 -h10 -W6 -H10 -p0 -N1 -R0 -f0 -F0 -L1 -Q1 -G1 -i$P/superm2-.dat -I$P/superm2-.dat -j$P/superm2-.dat -M0 -m0 -a0 -g-1 -A0 -r0 -n0 -O-1 -v0 -e0 -E0 -c0 -C0 -d0 -D0 -B0 -t18 -T10 -V17 -k1000 -x0 -X10000 -K100 -u0 -U10 | \
$V/space${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N13 --L${L}13 -d2 -M0 -m0 | \
$V/guide${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N14 --L${L}14 -Y5 -Z5 -S$P/GuideHORFC2.txt -w6 -h10 -W6 -H10 -p0 -N1 -R0 -f0 -F0 -L1 -Q1 -G1 -i$P/superm2-.dat -I$P/superm2-.dat -j$P/superm2-.dat -M0 -m0 -a0 -g-1 -A0 -r0 -n0 -O-1 -v0 -e0 -E0 -c0 -C0 -d0 -D0 -B0 -t18 -T10 -V17 -k1000 -x0 -X10000 -K100 -u0 -U10 | \
$V/capture_flux${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N15 --L${L}15 -R1.798 -t2 -r0.5 -y0 -z0 -w-1.125 -W1.125 -h-2.21 -H2.21 -l0.0 -L0.0 | \
$V/chopper_fermi${SUFFIX} -O1 --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N16 --L${L}16 -X10 -Y0 -V0 -a5 -b2.5 -c2 -l51 -m0.004 -r4 -n300 -q-177.450 -z0 -p4 | \
$V/space${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N17 --L${L}17 -d10 -M0 -m0 | \
$V/monitor1${SUFFIX} -k2 --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N18 --L${L}18 -O$P/postFC2.tim -n200 -C-1 -f1 -c0 -m1.5 -M1.7 -p1 -e0 -l-1.0 -L-1.0 | \
$V/spacewindow${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N19 --L${L}19 -l90 -R1 -r10 -y0 -z0 -w-1 -W1 -h-2.5 -H2.5 -A0.0 -S0 -F0 -c6 -t0.0 -T0 | \
$V/monitor1${SUFFIX} -k1 --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N20 --L${L}20 -O$P/sample.lda -n220 -C-1 -m0.8 -M3 -f1 -c0 -p1 -e0 | \
$V/capture_flux${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N21 --L${L}21 -t2 -r0.5 -y0 -z0 -w-0.5 -W0.5 -h-2 -H2 -l0.0 -L0.0 | \
$V/writeout${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N22 --L${L}22 -A$P/noutascii_before.dat -a1 -h1 -f1 -F1 -S0 -I1.0 -c111111111 -C-1 | \
$V/sample_ncrystal${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N23 --L${L}23 -f$P/LiquidWaterH2O_T293.6K.ncmat -T293.6 -d0.5 -m0.2 -R1 -G2 -X50 -Y0 -Z0 -t3 -g3 -w2.8 -o0 -O0 -M1.8 -h1 -k1 -l1 -a1 -b1 -c1 -H1 -K1 -L0 -A1 -B1 -C0 -x50 -y0 -z0 -u0 -U0 | \
$V/writeout${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N24 --L${L}24 -A$P/noutascii_after.dat -a1 -h1 -f1 -F1 -S0 -I1.0 -c111111111 -C-1 | \
$V/detector${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N25 --L${L}25 -B0 -G1 -a1 -U0 -A10 -q-1 -Q-1 -S-1 -d0 -P0 -T60 -D250 -h200 -w785 -t2.5 -r100 -c314 -n1 -u0 -v0 -l0 -m1 -p10 -k293 -e1 -o1 -b0 -f0 -s0 -V0 -W0 -x2 -z0 -O$P/inelastic.dat | \
$V/eval_inelast${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N26 --L${L}26 -E$P/tofsp_up.eva -G$P/energysp_up.eva -A0 -t1 -a600 -b250 -c1.3 -d0 -f-1 -m-50 -M50 -C50 -j60 -k180 -H$P/tofsp_down.eva -T$P/energysp_down.eva --Fno_file
rm -f $P/result.txt
cat ${L}?? >> $P/result.txt
rm ${L}*
