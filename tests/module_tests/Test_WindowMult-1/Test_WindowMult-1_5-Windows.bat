subst V: /d
subst V: C:\Users\lieuten\VITESS\Vitess_3-6a\MODULES
subst P: /d
subst P: M:\WindowMult\Test_WindowMult-1_5-Windows
set GSL_RNG_SEED=1
set GSL_RNG_TYPE=ran3
V:\read_in.exe --Z1 --U1.0e-26 --G0 --T0 --B10000 --PP:\ --N1 --LP:\lieuten6751d0edvpipelog1 -f1 -F1 -AP:\spacewindow_multiple_in.dat -a1.0 -b0.0 -d0.0 -R1 -I1.0 -C-1 -t0 | V:\spacewindow_multiple.exe --Z1 --U1.0e-26 --G0 --T0 --B10000 --PP:\ --N2 --LP:\lieuten6751d0edvpipelog2 -IP:\5_windows.dat -D2 -r6 -S0 -c6 | V:\writeout.exe --Z1 --U1.0e-26 --G0 --T0 --B10000 --PP:\ --N3 --LP:\lieuten6751d0edvpipelog3 -AP:\spacewindow_multiple_out.dat -a1 -h0 -f1 -F1 -S0 -I1.0 -c111111111 -C-1 -l-1.0 -L-1.0 --Fno_file
type P:\lieuten6751d0edvpipelog* > P:\result.txt
del P:\lieuten6751d0edvpipelog*
