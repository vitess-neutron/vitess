description O.K.

Pipe command would be :
C:/Programme/Vitess_2_5/MODULES/source.exe -S3 --Z1 --G1 --U1.0e-7 --B10000 --PC:/Programme/Vitess_2_5/FILES/REFL_DIR --LC:/temp/vpipelog1 -NESS -R16.667 -p2.0 -n12000000 -k0 -aC:/Programme/Vitess_2_5/FILES/REFL_DIR/EssLPTherm.mod -y2.1 -z0.065 -m1.8 -M6.5 -t0.0 -T4.7 -d0 -P0 -X1 -Y0 -V0 -D600 -w6 -h10 -i0 
 | C:/Programme/Vitess_2_5/MODULES/chopper_disc.exe --Z1 --G1 --U1.0e-7 --B10000 --PC:/Programme/Vitess_2_5/FILES/REFL_DIR --LC:/temp/vpipelog2 -s1000 -o-43.9 -g1 -z0 -p1 -c0 -CC:/Programme/Vitess_2_5/FILES/REFL_DIR/chop_045.dat -l0 
 | C:/Programme/Vitess_2_5/MODULES/guide.exe --Z1 --G1 --U1.0e-7 --B10000 --PC:/Programme/Vitess_2_5/FILES/REFL_DIR --LC:/temp/vpipelog3 -w6 -h10 -W6 -H10 -p100 -N27 -R1000 -iC:/Programme/Vitess_2_5/FILES/REFL_DIR/mirr1b.dat -IC:/Programme/Vitess_2_5/FILES/REFL_DIR/mirr2linear.dat -jC:/Programme/Vitess_2_5/FILES/REFL_DIR/mirr1b.dat -JC:/Programme/Vitess_2_5/FILES/REFL_DIR/mirr1b.dat -b3 -s0.05 -r0.0 -a1 
 | C:/Programme/Vitess_2_5/MODULES/chopper_disc.exe --Z1 --G1 --U1.0e-7 --B10000 --PC:/Programme/Vitess_2_5/FILES/REFL_DIR --LC:/temp/vpipelog4 -s1000 -o152.3 -g1 -z0 -p1 -c0 -CC:/Programme/Vitess_2_5/FILES/REFL_DIR/chop_180.dat -l0 
 | C:/Programme/Vitess_2_5/MODULES/guide.exe --Z1 --G1 --U1.0e-7 --B10000 --PC:/Programme/Vitess_2_5/FILES/REFL_DIR --LC:/temp/vpipelog5 -w6 -h10 -W6 -H10 -p2974 -N1 -R0 -iC:/Programme/Vitess_2_5/FILES/REFL_DIR/mirr1b.dat -IC:/Programme/Vitess_2_5/FILES/REFL_DIR/mirr1b.dat -jC:/Programme/Vitess_2_5/FILES/REFL_DIR/mirr1b.dat -JC:/Programme/Vitess_2_5/FILES/REFL_DIR/mirr1b.dat -r0.005 -a1 
 | C:/Programme/Vitess_2_5/MODULES/spacewindow.exe --Z1 --G1 --U1.0e-7 --B10000 --PC:/Programme/Vitess_2_5/FILES/REFL_DIR --LC:/temp/vpipelog6 -l0 -R0 -h-0.1 -H0.1 -w-2 -W2 -S0 -t0.01 -c6 
 | C:/Programme/Vitess_2_5/MODULES/spacewindow.exe --Z1 --G1 --U1.0e-7 --B10000 --PC:/Programme/Vitess_2_5/FILES/REFL_DIR --LC:/temp/vpipelog7 -l94 -R0 -h-0.1 -H0.1 -w-2 -W2 -S0 -t0.01 -c6 
 | C:/Programme/Vitess_2_5/MODULES/monitor1.exe -k1 --Z1 --G1 --U1.0e-7 --B10000 --PC:/Programme/Vitess_2_5/FILES/REFL_DIR --LC:/temp/vpipelog8 -OC:/Programme/Vitess_2_5/FILES/REFL_DIR/lambda.dat -n100 -C0 -f1 -m0 -M10 -p1 -e0 
 | C:/Programme/Vitess_2_5/MODULES/sample_reflectom.exe --Z1 --G1 --U1.0e-7 --B10000 --PC:/Programme/Vitess_2_5/FILES/REFL_DIR --LC:/temp/vpipelog9 -O1 -PC:/Programme/Vitess_2_5/FILES/REFL_DIR/refl.ref -IC:/Programme/Vitess_2_5/FILES/REFL_DIR/refl_Si-50Au.dat -RY -a-2 
 | C:/Programme/Vitess_2_5/MODULES/frame.exe --Z1 --G1 --U1.0e-7 --B10000 --PC:/Programme/Vitess_2_5/FILES/REFL_DIR --LC:/temp/vpipelog10 -S1 -H0 -V0 -A90 -x0 -y0 -z0 -i0 -j0 -k0 
 | C:/Programme/Vitess_2_5/MODULES/detector.exe --Z1 --G1 --U1.0e-7 --B10000 --PC:/Programme/Vitess_2_5/FILES/REFL_DIR --LC:/temp/vpipelog11 -Gcyl -h25 -w80 -t2 -e0.9 -T0 -P0 -D200 -c48 -r1 -A1 -M1 -g0 
 | C:/Programme/Vitess_2_5/MODULES/monitor1.exe -k2 --Z1 --G1 --U1.0e-7 --B10000 --PC:/Programme/Vitess_2_5/FILES/REFL_DIR --LC:/temp/vpipelog12 -OC:/Programme/Vitess_2_5/FILES/REFL_DIR/time.dat -n120 -f1 -m0 -M120 -p1 -e0 
 | C:/Programme/Vitess_2_5/MODULES/eval_elast.exe --Z1 --G1 --U1.0e-7 --B10000 --PC:/Programme/Vitess_2_5/FILES/REFL_DIR --LC:/temp/vpipelog13 -k2 -oC:/Programme/Vitess_2_5/FILES/REFL_DIR/eval_q.dat -n100 -m0 -M0.25 -p1 -w1 -l6600 -T0 -e41.53 -E92.24 --Fno_file


