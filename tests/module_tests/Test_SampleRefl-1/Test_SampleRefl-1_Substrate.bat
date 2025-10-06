subst V: /d
subst V: C:\Users\lieuten\VITESS\Vitess3_Tests\vitess\MODULES
subst P: /d
subst P: M:\SmplRefl\Test_SampleRefl-1_Substrate
set GSL_RNG_SEED=1
set GSL_RNG_TYPE=ran3
V:\read_in.exe --Z1 --U1.0e-25 --G1 --T0 --B10000 --PP:\ --N1 --LP:\lieuten680d2847vpipelog1 -f1 -F1 -AP:\sample_reflectom_in.dat -a1.0 -b0.0 -d0.0 -R1 -J0 -I1.0 -C-1 -t0 | V:\sample_reflectom.exe --Z1 --U1.0e-25 --G1 --T0 --B10000 --PP:\ --N2 --LP:\lieuten680d2847vpipelog2 -PP:\geom-1.ref -IP:\refl_Si-50Au.dat -O1 -RY -A1 -a1.2 -M0 -h0.2 -s4 -o0 -B0 -X0 -S1 -d0 -p0 -t0 | V:\writeout.exe --Z1 --U1.0e-25 --G1 --T0 --B10000 --PP:\ --N3 --LP:\lieuten680d2847vpipelog3 -AP:\sample_reflectom_out.dat -a1 -h0 -f1 -F1 -S0 -I1.0 -c111111111 -C-1 -l-1.0 -L-1.0 --Fno_file
type P:\lieuten680d2847vpipelog* > P:\result.txt
del P:\lieuten680d2847vpipelog*
