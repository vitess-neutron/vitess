#!/bin/sh
[ -z "$V" ] && V=/tmp/vitess/MODULES
[ -z "$P" ] && P=/tmp/vitess/FILES/EXAMPLES/SampleSQ
[ -z "$L" ] && L=/tmp/vitess66618964vpipelog
SUFFIX="$(uname -s)_$(uname -m)"
$V/source_${SUFFIX} -S3 --Z2 --U1.0e-6 --G1 --T0 --B10000 --P$P --N1 --L${L}01 -v3 -R14.0 -p2.857 -NESS -L5.0 -a$P/EssSPThermDec.mod -n1000000 -l1 -m0.75 -t0 -M3 -T1 -d1 -b0.0 -c0.0 -y0.5 -z0.5 -D650 -w3.1 -h3.1 -i0 -s200 -X0 -Y0 -V1 -P0 -A0 -k0 | $V/spacewindow_${SUFFIX} --Z2 --U1.0e-6 --G1 --T0 --B10000 --P$P --N2 --L${L}02 -l0 -R0 -r10 -y0 -z0 -w-1.5 -W1.5 -h-1.5 -H1.5 -A0.0 -S0 -F0 -p-1 -P-1 -f-1 -d0 -c6 -t0 -T0 | $V/monitor1_${SUFFIX} -k1 --Z2 --U1.0e-6 --G1 --T0 --B10000 --P$P --N3 --L${L}03 -O$P/lambda.dat -n40 -C-1 -m0.00 -M4.0 -f0 -c0 -p1 -e0 -t-1.e10 -T1.e10 | $V/sample_s_q_${SUFFIX} --Z2 --U1.0e-6 --G1 --T0 --B10000 --P$P --N4 --L${L}04 -D30 -d30 -P180 -p180 -A40 -I0 -S$P/glass.psq -f0.0 -o0.0 | $V/detector_${SUFFIX} --Z2 --U1.0e-6 --G1 --T0 --B10000 --P$P --N5 --L${L}05 -B0 -G1 -a1 -U0 -A1 -q-1 -Q-1 -S-1 -d0 -P0 -T0 -D150 -h100 -w300 -t2 -r50 -c150 -n1 -u0 -v0 -l0 -m1 -p4 -k293 -e1 -o0 -b0 -f0 -s0 -V0 -W0 -x2 -z0 | $V/eval_elast_${SUFFIX} --Z2 --U1.0e-6 --G1 --T0 --B10000 --P$P --N6 --L${L}06 -k2 -o$P/elast.eva -n50 -m0 -M10 -p1 -c0 -A-1 -w1 -t0 -l1080 -T0 -e-1.e10 -E1.e10 -C-1 --Fno_file
rm -f $P/result.txt
cat ${L}?? >> $P/result.txt
rm ${L}*
