# -*- coding: utf-8 -*-

import os
import sys
import numpy as np
from numpy import ma
import matplotlib.pyplot as plt
from matplotlib import ticker, cm

def read_mfile(fn):
    try:
        fin = open(fn,'r')
        hline = fin.readline().rstrip()
    except:
        return 0,0,0,0,0,0,0
    sitems = hline.split(':')
    if sitems[0].find('#Monitor matrix weight') != 0: return 0,0,0,0,0,0,0
    line = fin.readline()
    if line[0] == '#': return 0,0,0,0,0,0,0
    try:
        by = np.fromstring(line, dtype=float, sep=' ')
        if len(sitems) == 5:
            nbiny = int(sitems[1])
            nbinz = int(sitems[3])
            xaxis = sitems[2]
            yaxis = sitems[4]
            if nbiny != len(by): nbiny = 0
        else:
            n = os.popen('wc ' + fn).read().split()[0]
            nbiny = len(by)
            nbinz = int(n) - 1
            xaxis = ''
            yaxis = ''
    except:
        nbiny = 0

    if nbiny <= 0: return 0,0,0,0,0,0,0
    
    bz = np.empty(nbinz)
    counts = np.zeros([nbinz, nbiny])
    for i in range(nbinz):
        z = np.fromstring(fin.readline(), dtype=float, sep=' ')
        bz[i] = z[0]
        counts[:][i] = z[1:]
    return nbiny, nbinz, by, bz, counts, xaxis,yaxis

def contshow(fn, x, y, counts, xlabel,ylabel):
    a,b = plt.subplots()
    z = ma.masked_where(counts <= 0, counts)
    cs = b.contourf(x, y, z, locator=ticker.LogLocator(), cmap=cm.PuBu_r)
    a.suptitle(fn)
    if xlabel != "": b.set_xlabel(xlabel)
    if ylabel != "": b.set_ylabel(ylabel)
    a.colorbar(cs)
    plt.show()

def readshow(fn):
    nbiny, nbinz, by, bz, counts, xlabel,ylabel = read_mfile(fn)
    if nbiny > 0:
        contshow(fn, by, bz, counts, xlabel,ylabel)
    else:
        print fn + ' is no Vitess mon2 file'

readshow(sys.argv[1])
