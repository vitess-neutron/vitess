description O.K.

Pipe command would be :
C:/Programme/Vitess_2_5/MODULES/source.exe -S2 --Z2 --G1 --U1.0e-6 --B10000 --PC:/Programme/Vitess_2_5/FILES/S_Q_DIR --LC:/Programme/Vitess_2_5/FILES/S_Q_DIR/vpipelog1 -R50 -NESS -aC:/Programme/Vitess_2_5/FILES/S_Q_DIR/EssSPThermDec.mod -n12000000 -m0.75 -t0 -y0.8 -M3.0 -T0.8 -z0.5 -d0 -P0 -X1 -Y0 -V0 -D650 -w4 -h4 -i0 -A0 
 | C:/Programme/Vitess_2_5/MODULES/spacewindow.exe --Z2 --G1 --U1.0e-6 --B10000 --PC:/Programme/Vitess_2_5/FILES/S_Q_DIR --LC:/Programme/Vitess_2_5/FILES/S_Q_DIR/vpipelog2 -l250 -R0 -h-1.5 -H1.5 -w-1.5 -W1.5 -S0 -t0 -c6 
 | C:/Programme/Vitess_2_5/MODULES/monitor1.exe -k1 --Z2 --G1 --U1.0e-6 --B10000 --PC:/Programme/Vitess_2_5/FILES/S_Q_DIR --LC:/Programme/Vitess_2_5/FILES/S_Q_DIR/vpipelog3 -OC:/Programme/Vitess_2_5/FILES/S_Q_DIR/lambda.dat -n40 -m0.00 -M4.0 -p1 -e0 -t-1.e10 -T1.e10 
 | C:/Programme/Vitess_2_5/MODULES/sample_s_q.exe --Z2 --G1 --U1.0e-6 --B10000 --PC:/Programme/Vitess_2_5/FILES/S_Q_DIR --LC:/Programme/Vitess_2_5/FILES/S_Q_DIR/vpipelog4 -D30 -d30 -P180 -p180 -A40 -I0 -SC:/Programme/Vitess_2_5/FILES/S_Q_DIR/glass.psq 
 | C:/Programme/Vitess_2_5/MODULES/detector.exe --Z2 --G1 --U1.0e-6 --B10000 --PC:/Programme/Vitess_2_5/FILES/S_Q_DIR --LC:/Programme/Vitess_2_5/FILES/S_Q_DIR/vpipelog5 -Gcyl -h100 -w300 -t2 -e0.9 -T0 -P0 -D150 -c150 -r50 -A1 -M0 -g1 
 | C:/Programme/Vitess_2_5/MODULES/eval_elast.exe --Z2 --G1 --U1.0e-6 --B10000 --PC:/Programme/Vitess_2_5/FILES/S_Q_DIR --LC:/Programme/Vitess_2_5/FILES/S_Q_DIR/vpipelog6 -k2 -oC:/Programme/Vitess_2_5/FILES/S_Q_DIR/elast.eva -n50 -m0 -M10 -p1 -w1 -l1080 -T0 -e-1.e10 -E1.e10 --Fno_file


