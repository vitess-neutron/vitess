description O.K.

Pipe command would be :
C:/Programme/Vitess_2_5/MODULES/source.exe -S1 --Z1 --G1 --U1.0e-12 --B10000 --PC:/Programme/Vitess_2_5/FILES/SANS_DIR --LC:/temp/vpipelog1 -n5000000 -k0 -aC:/Programme/Vitess_2_5/FILES/SANS_DIR/Reactor.mod -m4.0 -M6.0 -t0.0 -T0.0 -d1 -P0 -X1 -Y0 -V0 -D600 -w3.0 -h3.0 
 | C:/Programme/Vitess_2_5/MODULES/velselect.exe --Z1 --G1 --U1.0e-12 --B10000 --PC:/Programme/Vitess_2_5/FILES/SANS_DIR --LC:/temp/vpipelog2 -l25 -s420 -w72 -c48.3 -r14.5 -o11.5 -d0.04 
 | C:/Programme/Vitess_2_5/MODULES/monitor1.exe -k1 --Z1 --G1 --U1.0e-12 --B10000 --PC:/Programme/Vitess_2_5/FILES/SANS_DIR --LC:/temp/vpipelog3 -OC:/Programme/Vitess_2_5/FILES/SANS_DIR/sans_lambda.dat -n60 -m3.5 -M6.5 -p1 -e0 -t-1.e10 -T1.e10 
 | C:/Programme/Vitess_2_5/MODULES/spacewindow.exe --Z1 --G1 --U1.0e-12 --B10000 --PC:/Programme/Vitess_2_5/FILES/SANS_DIR --LC:/temp/vpipelog4 -l0 -R0 -r1 -y0 -z0 -h-1.5 -H1.5 -w-1.5 -W1.5 -S0 -t0.01 -c6 
 | C:/Programme/Vitess_2_5/MODULES/spacewindow.exe --Z1 --G1 --U1.0e-12 --B10000 --PC:/Programme/Vitess_2_5/FILES/SANS_DIR --LC:/temp/vpipelog5 -l800 -R0 -r1 -y0 -z0 -h-0.75 -H0.75 -w-0.75 -W0.75 -S0 -t0.01 -c6 
 | C:/Programme/Vitess_2_5/MODULES/sample_sans.exe --Z1 --G1 --U1.0e-12 --B10000 --PC:/Programme/Vitess_2_5/FILES/SANS_DIR --LC:/temp/vpipelog6 -D1.55 -d1.55 -P180 -p180 -A1 -SC:/Programme/Vitess_2_5/FILES/SANS_DIR/sphere.san 
 | C:/Programme/Vitess_2_5/MODULES/detector.exe --Z1 --G1 --U1.0e-12 --B10000 --PC:/Programme/Vitess_2_5/FILES/SANS_DIR --LC:/temp/vpipelog7 -Gcub -h60 -w60 -t1 -e0.9 -T0 -P90 -D800 -c120 -r120 -A1 -M0 -g1 
 | C:/Programme/Vitess_2_5/MODULES/eval_elast.exe --Z1 --G1 --U1.0e-12 --B10000 --PC:/Programme/Vitess_2_5/FILES/SANS_DIR --LC:/temp/vpipelog8 -k2 -oC:/Programme/Vitess_2_5/FILES/SANS_DIR/spectsans.dat -n75 -m0 -M0.075 -p1 -d0.215 -w0 -r5.0 -C0 --Fno_file


