description O.K.

Pipe command would be :
C:/Programme/VITESS_2_5_1/MODULES/source.exe -S1 --Z1 --G1 --U1.0e-12 --B10000 --PC:/Programme/VITESS_2_5_1/FILES/SANS_DIR --LC:/Programme/VITESS_2_5_1/FILES/SANS_DIR/vpipelog1 -aC:/Programme/VITESS_2_5_1/FILES/SANS_DIR/Reactor.mod -n5000000 -m4.0 -t0.0 -M6.0 -T0.0 -d1 -P0 -X1 -Y0 -V0 -D600 -w3.0 -h3.0 -k0 
 | C:/Programme/VITESS_2_5_1/MODULES/velselect.exe --Z1 --G1 --U1.0e-12 --B10000 --PC:/Programme/VITESS_2_5_1/FILES/SANS_DIR --LC:/Programme/VITESS_2_5_1/FILES/SANS_DIR/vpipelog2 -l25 -s420 -w72 -c48.3 -r14.5 -o11.5 -d0.04 
 | C:/Programme/VITESS_2_5_1/MODULES/monitor1.exe -k1 --Z1 --G1 --U1.0e-12 --B10000 --PC:/Programme/VITESS_2_5_1/FILES/SANS_DIR --LC:/Programme/VITESS_2_5_1/FILES/SANS_DIR/vpipelog3 -OC:/Programme/VITESS_2_5_1/FILES/SANS_DIR/sans_lambda.dat -n60 -m3.5 -M6.5 -p1 -e0 -t-1.e10 -T1.e10 
 | C:/Programme/VITESS_2_5_1/MODULES/spacewindow.exe --Z1 --G1 --U1.0e-12 --B10000 --PC:/Programme/VITESS_2_5_1/FILES/SANS_DIR --LC:/Programme/VITESS_2_5_1/FILES/SANS_DIR/vpipelog4 -l0 -R0 -r1 -y0 -z0 -h-1.5 -H1.5 -w-1.5 -W1.5 -S0 -t0.01 -c6 
 | C:/Programme/VITESS_2_5_1/MODULES/spacewindow.exe --Z1 --G1 --U1.0e-12 --B10000 --PC:/Programme/VITESS_2_5_1/FILES/SANS_DIR --LC:/Programme/VITESS_2_5_1/FILES/SANS_DIR/vpipelog5 -l800 -R0 -r1 -y0 -z0 -h-0.75 -H0.75 -w-0.75 -W0.75 -S0 -t0.01 -c6 
 | C:/Programme/VITESS_2_5_1/MODULES/sample_sans.exe --Z1 --G1 --U1.0e-12 --B10000 --PC:/Programme/VITESS_2_5_1/FILES/SANS_DIR --LC:/Programme/VITESS_2_5_1/FILES/SANS_DIR/vpipelog6 -D1.55 -d1.55 -P180 -p180 -A1 -SC:/Programme/VITESS_2_5_1/FILES/SANS_DIR/sphere.san 
 | C:/Programme/VITESS_2_5_1/MODULES/detector.exe --Z1 --G1 --U1.0e-12 --B10000 --PC:/Programme/VITESS_2_5_1/FILES/SANS_DIR --LC:/Programme/VITESS_2_5_1/FILES/SANS_DIR/vpipelog7 -Gcub -h60 -w60 -t1 -e0.9 -T0 -P90 -D800 -c120 -r120 -A1 -M0 -g1 
 | C:/Programme/VITESS_2_5_1/MODULES/eval_elast.exe --Z1 --G1 --U1.0e-12 --B10000 --PC:/Programme/VITESS_2_5_1/FILES/SANS_DIR --LC:/Programme/VITESS_2_5_1/FILES/SANS_DIR/vpipelog8 -k2 -oC:/Programme/VITESS_2_5_1/FILES/SANS_DIR/spectsans.dat -n75 -m0 -M0.075 -p1 -d0.215 -w0 -r5.0 -C0 --Fno_file


