description O.K.

Pipe command would be :
C:/Programme/Vitess_2_5/MODULES/source.exe -S2 --Z1 --G1 --U1.0e-6 --B10000 --PC:/Programme/Vitess_2_5/FILES/BEND_DIR --LC:/Programme/Vitess_2_5/FILES/BEND_DIR/vpipelog1 -R60.0 -NSNS -aC:/Programme/Vitess_2_5/FILES/BEND_DIR/SnsColdCpld.mod -n1000 -m0.1 -t0 -M5 -T1.2 -d1 -P0 -X0 -Y0 -V1 -D1000 -w2.8 -h5 -k1 -rfermi.out 
 | C:/Programme/Vitess_2_5/MODULES/bender.exe --Z1 --G1 --U1.0e-6 --B10000 --PC:/Programme/Vitess_2_5/FILES/BEND_DIR --LC:/Programme/Vitess_2_5/FILES/BEND_DIR/vpipelog2 -o3 -h5 -H5 -s0.0001 -l17.8 -R927 -iC:/Programme/Vitess_2_5/FILES/BEND_DIR/mirr0.dat -mC:/Programme/Vitess_2_5/FILES/BEND_DIR/mirr2linear.dat -kC:/Programme/Vitess_2_5/FILES/BEND_DIR/mirr1b.dat -IC:/Programme/Vitess_2_5/FILES/BEND_DIR/mirr0.dat -MC:/Programme/Vitess_2_5/FILES/BEND_DIR/mirr2linear.dat -KC:/Programme/Vitess_2_5/FILES/BEND_DIR/mirr1b.dat -uC:/Programme/Vitess_2_5/FILES/BEND_DIR/SurfaceS1_1.dat -g0 -c6 -z1 -w1 -r0.005 -a0 -y1 -p0 -V2 -t0 
 | C:/Programme/Vitess_2_5/MODULES/monitor1.exe -k5 --Z1 --G1 --U1.0e-6 --B10000 --PC:/Programme/Vitess_2_5/FILES/BEND_DIR --LC:/Programme/Vitess_2_5/FILES/BEND_DIR/vpipelog3 -OC:/Programme/Vitess_2_5/FILES/BEND_DIR/mon_pos_y.dat -n23 -m-0.2875 -M0.2875 -p1 -e0 
 | C:/Programme/Vitess_2_5/MODULES/monitor1.exe -k3 --Z1 --G1 --U1.0e-6 --B10000 --PC:/Programme/Vitess_2_5/FILES/BEND_DIR --LC:/Programme/Vitess_2_5/FILES/BEND_DIR/vpipelog4 -OC:/Programme/Vitess_2_5/FILES/BEND_DIR/mon_div_y.dat -n40 -m-2 -M2 -p1 -e0 
 | C:/Programme/Vitess_2_5/MODULES/chopper_fermi.exe -O1 --Z1 --G1 --U1.0e-6 --B10000 --PC:/Programme/Vitess_2_5/FILES/BEND_DIR --LC:/Programme/Vitess_2_5/FILES/BEND_DIR/vpipelog5 -X10 -Y0 -V0 -a5 -b4 -c3 -l16 -m0.02 -r8 -n30 -q-55.5 
 | C:/Programme/Vitess_2_5/MODULES/writeout.exe --Z1 --G1 --U1.0e-6 --B10000 --PC:/Programme/Vitess_2_5/FILES/BEND_DIR --LC:/Programme/Vitess_2_5/FILES/BEND_DIR/vpipelog6 -AC:/Programme/Vitess_2_5/FILES/BEND_DIR/fermi.out 
 | C:/Programme/Vitess_2_5/MODULES/space.exe --Z1 --G1 --U1.0e-6 --B10000 --PC:/Programme/Vitess_2_5/FILES/BEND_DIR --LC:/Programme/Vitess_2_5/FILES/BEND_DIR/vpipelog7 -d200 
 | C:/Programme/Vitess_2_5/MODULES/sample_powder.exe --Z1 --G1 --U1.0e-6 --B10000 --PC:/Programme/Vitess_2_5/FILES/BEND_DIR --LC:/Programme/Vitess_2_5/FILES/BEND_DIR/vpipelog8 -D135 -d45 -P180 -p180 -A1 -I0 -SC:/Programme/Vitess_2_5/FILES/BEND_DIR/ptest.pow --Fno_file


