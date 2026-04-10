subst V: /d
subst V: C:\Users\lieuten\VITESS\Vitess3_Tests\vitess\MODULES
subst P: /d
subst P: M:\SmplRefl\Test_SampleRefl-3_Incoherent
set GSL_RNG_SEED=1
set GSL_RNG_TYPE=ran3
V:\read_in.exe --Z1 --U1.0e-25 --G1 --T0 --B10000 --PP:\ --N1 --LP:\lieuten68177ba7vpipelog1 -f1 -F1 -AP:\sample_reflectom-3_in.dat -a1.0 -b0.0 -d0.0 -R1 -J0 -I1.0 -C-1 -t0 | V:\sample_reflectom.exe --Z1 --U1.0e-25 --G1 --T0 --B10000 --PP:\ --N2 --LP:\lieuten68177ba7vpipelog2 -PP:\geom-3.ref -IP:\refl_d2o.dat -O1 -RZ -A0 -a1.0 -M0 -s0 -m0 -o0 -B1 -X0.05 -S50 -d50 -p11 -t7 | V:\writeout.exe --Z1 --U1.0e-25 --G1 --T0 --B10000 --PP:\ --N3 --LP:\lieuten68177ba7vpipelog3 -AP:\sample_reflectom-3_out.dat -a1 -h0 -f1 -F1 -S0 -I1.0 -c111111111 -C-1 --Fno_file
type P:\lieuten68177ba7vpipelog* > P:\result.txt
del P:\lieuten68177ba7vpipelog*
