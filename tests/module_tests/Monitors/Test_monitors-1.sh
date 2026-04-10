#!/bin/sh
[ -z "$V" ] && V=/tmp/vitess/MODULES
[ -z "$P" ] && P=/tmp/vitess/tests/module_tests/Test_Monitors
[ -z "$L" ] && L=/tmp/vitess7c9635cvpipelog
[ -z "${SUFFIX}" ] && SUFFIX="_`uname -s`_`uname -m`"
GSL_RNG_SEED=1
export GSL_RNG_SEED
GSL_RNG_TYPE=ran3
export GSL_RNG_TYPE

$V/source${SUFFIX} -S1 --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N1 --L${L}01 -a$P/SrcConst.mod -n1e4 -l1 -m1 -M10 -d1 -b0.0 -c0.0 -y0.5 -z0.5 -D200 -w2 -h2 -i0 -s200 -X1 -Y1 -V1 -P100 -A0 -k0 | \
$V/monitor1${SUFFIX} -k2 --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N2 --L${L}02 -O$P/monitor-1_out.dat -n400 -C-1 -f0 -c0 -m0 -M10 -p1 -e0 -l1 -L10 -y-2 -Y2 -z-2 -Z2 | \
$V/monitor1${SUFFIX} -k1 --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N3 --L${L}03 -O$P/monitor-2_out.dat -n100 -C-1 -m1 -M10 -f0 -c0 -p1 -e0 -y-2 -Y2 -z-2 -Z2 | \
$V/monitor1${SUFFIX} -k7 --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N4 --L${L}04 -O$P/monitor-3_out.dat -n100 -C-1 -f0 -c0 -m0 -M20 -p1 -e0 -t-1.e10 -T1.e10 -l-1.0 -L-1.0 -y-2 -Y2 -z-2 -Z2 | \
$V/monitor1${SUFFIX} -k5 --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N5 --L${L}05 -O$P/monitor-4_out.dat -n100 -C-1 -f0 -c0 -m-2 -M2 -p1 -e0 -z-2 -Z2 -l1 -L10 | \
$V/monitor1${SUFFIX} -k6 --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N6 --L${L}06 -O$P/monitor-5_out.dat -n100 -C-1 -f0 -c0 -m-2 -M2 -p1 -e0 -y-2 -Y2 -l1 -L10 | \
$V/monitor1${SUFFIX} -k3 --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N7 --L${L}07 -O$P/monitor-6_out.dat -n100 -C-1 -f0 -c0 -m-1 -M1 -p1 -e0 -l1 -L10 -y-2 -Y2 -z-2 -Z2 | \
$V/monitor1${SUFFIX} -k4 --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N8 --L${L}08 -O$P/monitor-7_out.dat -n100 -C-1 -f0 -c0 -m-1 -M1 -p1 -e0 -l1 -L10 -y-2 -Y2 -z-2 -Z2 | \
$V/monitor1${SUFFIX} -k8 --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N9 --L${L}09 -O$P/monitor-8_out.dat -n100 -C-1 -f0 -c0 -m-1 -M1 -a0.0 -A0.5 -s0.1 -p1 -e0 -P1 -l1 -L10 -y-2 -Y2 -z-2 -Z2 | \
$V/mon1_brl${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N10 --L${L}10 -O$P/monitor-9_out.dat -n100 -C-1 -k1 -N1 -B0 -l1 -L10 -y-2 -Y2 -z-2 -Z2 -h-1 -H1 -v-1 -V1 | \
$V/mon2_pos${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N11 --L${L}11 -O$P/monitor-10_out.dat -w-3 -h-3 -W3 -H3 -y100 -z100 -p1 -e0 -F1 -l1 -L10 | \
$V/mon2_div${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N12 --L${L}12 -O$P/monitor-11_out.dat -w-1 -h-1 -W1 -H1 -y100 -z100 -p1 -e0 -F1 -l1 -L10 -u-2 -U2 -v-2 -V2 | \
$V/mon2_kdiv${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N13 --L${L}13 -O$P/monitor-12_out.dat -w-0.01 -h-0.01 -W0.01 -H0.01 -y10 -z10 -p1 -e0 -F3 | \
$V/mon2_rdiv${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N14 --L${L}14 -O$P/monitor-13_out.dat -w0.0 -h0.0 -W2.0 -H1.0 -y100 -z100 -p1 -e0 -F3 -l1 -L10 -u-2 -U2 -v-2 -V2 | \
$V/mon2_tofwl${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N15 --L${L}15 -O$P/monitor-14_out.dat -w0 -m1 -W10 -M10 -y100 -z100 -p1 -e0 -F3 | \
$V/mon2_wldiv${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N16 --L${L}16 -O$P/monitor-15_out.dat -w1 -h-1 -W10 -H1 -y100 -z100 -c-90 -C90 -q1 -p1 -e0 -F3 -u-2 -U2 -v-2 -V2 | \
$V/mon2_posdiv${SUFFIX} -q1 --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N17 --L${L}17 -O$P/monitor-16_out.dat -w-3 -h-1 -W3 -H1 -y100 -z100 -p1 -e0 -F1 -l1 -L10 -u-2 -U2 -v-2 -V2 | \
$V/mon2_posdiv${SUFFIX} -q2 --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N18 --L${L}18 -O$P/monitor-17_out.dat -w-3 -h-1 -W2 -H1 -y100 -z100 -p1 -e0 -F1 -l1 -L10 -u-2 -U2 -v-2 -V2 | \
$V/mon1_pol${SUFFIX} -k2 --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N19 --L${L}19 -O$P/monitor-18_out.dat -n100 -C-1 -m1 -M10 -p1 -e0 -a1 -b1 -c1 | \
$V/mon1_pol${SUFFIX} -k1 --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N20 --L${L}20 -O$P/monitor-19_out.dat -n100 -C-1 -m1 -M10 -p1 -e0 -a1 -b1 -c1 | \
$V/mon1_pol${SUFFIX} -k5 --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N21 --L${L}21 -O$P/monitor-20_out.dat -n100 -C-1 -m-3 -M3 -p1 -e0 -a1 -b1 -c1 | \
$V/mon1_pol${SUFFIX} -k6 --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N22 --L${L}22 -O$P/monitor-21_out.dat -n100 -C-1 -m-3 -M3 -p1 -e0 -a1 -b1 -c1 | \
$V/mon1_pol${SUFFIX} -k3 --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N23 --L${L}23 -O$P/monitor-22_out.dat -n100 -C-1 -m-1 -M1 -p1 -e0 -a1 -b1 -c1 | \
$V/mon1_pol${SUFFIX} -k4 --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N24 --L${L}24 -O$P/monitor-23_out.dat -n100 -C-1 -m-1 -M1 -p1 -e0 -a1 -b1 -c1 | \
$V/monitorpol_pos${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N25 --L${L}25 -O$P/monitor-24_out.dat -w-3 -h-3 -W3 -H3 -y100 -z100 -p1 -e0 -a1 -b1 -c1 | \
$V/monitor1D${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N26 --L${L}26 -O$P/monitor-25_out.dat -X5 -w0 -W11 -x110 -p0 -e0 -I1 -J2 -C1 -u-1 -v-1 -U1 -V1 -P1 -r1 -s1 -t1 | \
$V/monitor2D${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N27 --L${L}27 -O$P/monitor-26_out.dat -X1 -Y2 -w-3 -h-3 -W3 -H3 -x100 -y100 -p1 -e0 -F1 -l1 -L10 -I3 -J4 -C1 -u0 -v0 -U0.5 -V0.5 -P1 -r1 -s1 -t1 --Fno_file
rm -f $P/result.txt
cat ${L}?? >> $P/result.txt
rm ${L}*
