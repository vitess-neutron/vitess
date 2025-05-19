#!/usr/bin/env python3
import os
import glob
import platform

os.system("rm *.dat *.inf *.out Trc")


def pwrite(fn, pattern):
    f = open(fn, 'w')
    for name in sorted(glob.glob(pattern + '*')):
        for line in open(name):
            f.write(line)
        os.remove(name)
    f.close


if 'VITESS_DIR' in os.environ:
    print("VITESS_DIR is defined:", os.environ['VITESS_DIR'])
else:
    print("VITESS_DIR is not defined, please define it by runing 'export VITESS_DIR=/path/to/vitess'.")
    exit()

VPATH = '$VITESS_DIR/MODULES'
LPATH = '/tmp/temp_vitess'
PPATH = '.'
SETTS = '--Z1 --U1.0e-25 --G1 --T1 --B10000 --P{:s}'.format(PPATH)
ARCH  = f'{platform.system()}_{platform.machine()}'


##### Run with the xml file

source = '{:s}/read_in_{:s} {:s} --N1 --L{:s}01 '.format(VPATH, ARCH, SETTS, LPATH)
# source += '-f3 -F1 -A{:s}/read_in.mcpl -a1.0 -b0.0 -d0.0 -R1 -I1.0 -C-1 -t0'.format(PPATH)
source += '-f7 -F1 -A{:s}/read_in.xml -a1.0 -b0.0 -d0.0 -R1 -I1.0 -C-1 -t0 -M100 -K0'.format(PPATH)
source += ' | '

space = '{:s}/space_{:s} {:s} --N2 --L{:s}02 '.format(VPATH, ARCH, SETTS, LPATH)
space += '-d10 -M0 -m0'
space += '|'

monitor = '{:s}/writeout_{:s} {:s} --N3 --L{:s}03 '.format(VPATH, ARCH, SETTS, LPATH)
monitor += '-A{:s}/tracks_kds.mcpl -a1 -h1 -f3 -F0 -S0 -I1.0 -c111111111 -C-1'.format(PPATH)
monitor += ' | '

detector = '{:s}/detector_{:s} {:s} --N4 --L{:s}04 '.format(VPATH, ARCH, SETTS, LPATH)
detector += '-B0 -G2 -a0 -U0 -A1 -q-1 -Q-1 -S-1 -d0 -P0 -T0 -D1.19 -h10 -w2.38 -t2.38 -r1 -c1 -n1 -u0 -v0 -l0 -m1 '
detector += '-p4 -k293 -e1 -o1 -b0 -f0.5 -s0 -V0 -W0 -x2 -z0 -O{:s}/detector_kds.dat --Fno_file'.format(PPATH)

r = ''
for instr in [source, space, monitor, detector]:
    r += instr
r += ' --Fno_file'

os.system("export GSL_RNG_SEED='1' GSL_RNG_TYPE='ran3' ;"+r)
pwrite('{:s}/results_kds.out'.format(PPATH), LPATH)
print('Develop run: done.')

##### Run with the mcpl file

source = '{:s}/read_in_{:s} {:s} --N1 --L{:s}01 '.format(VPATH, ARCH, SETTS, LPATH)
source += '-f3 -F1 -A{:s}/read_in.mcpl -a1.0 -b0.0 -d0.0 -R1 -I1.0 -C-1 -t0 -M100 -K0'.format(PPATH)
# source += '-f7 -F1 -A{:s}/read_in.xml -a1.0 -b0.0 -d0.0 -R1 -I1.0 -C-1 -t0'.format(PPATH)
source += ' | '

space = '{:s}/space_{:s} {:s} --N2 --L{:s}02 '.format(VPATH, ARCH, SETTS, LPATH)
space += '-d10 -M0 -m0'
space += '|'

monitor = '{:s}/writeout_{:s} {:s} --N3 --L{:s}03 '.format(VPATH, ARCH, SETTS, LPATH)
monitor += '-A{:s}/tracks_mcpl.mcpl -a1 -h1 -f3 -F0 -S0 -I1.0 -c111111111 -C-1'.format(PPATH)
monitor += ' | '

detector = '{:s}/detector_{:s} {:s} --N4 --L{:s}04 '.format(VPATH, ARCH, SETTS, LPATH)
detector += '-B0 -G2 -a0 -U0 -A1 -q-1 -Q-1 -S-1 -d0 -P0 -T0 -D1.19 -h10 -w2.38 -t2.38 -r1 -c1 -n1 -u0 -v0 -l0 -m1 '
detector += '-p4 -k293 -e1 -o1 -b0 -f0.5 -s0 -V0 -W0 -x2 -z0 -O{:s}/detector_mcpl.dat --Fno_file'.format(PPATH)

r = ''
for instr in [source, space, monitor, detector]:
    r += instr
r += ' --Fno_file'

os.system("export GSL_RNG_SEED='1' GSL_RNG_TYPE='ran3' ;"+r)
pwrite('{:s}/results_mcpl.out'.format(PPATH), LPATH)
print('Master run: done.')

# import kdsource as kds

# ssf0 = kds.SurfaceSourceFile('tracks_mcpl.mcpl')
# ssf1 = kds.SurfaceSourceFile('tracks_kds.mcpl')

# ssf0.plot_distribution(['E'], [100], label='mcpl')
# ssf1.plot_distribution(['E'], [100], label='kds')
