# -*- coding: utf-8 -*-

import sys
import numpy as np
from numpy import ma
import matplotlib.pyplot as plt

def plot2D(counts, xaxis, yaxis, by, bz, fname):
    plt.figure()
    # counts = ma.masked_where(counts <= 0, counts)
    plt.imshow(counts, origin="lower", extent=[min(by), max(by), min(bz), max(bz)])
    plt.xlabel(xaxis)
    plt.ylabel(yaxis)
    plt.title(fname)
    plt.colorbar()
    plt.axis("equal")
    plt.tight_layout()
    plt.show()

def plot1D(x, counts, error, xaxis, yaxis, fname):
    plt.figure()
    plt.bar(x, counts, width=x[1]-x[0], color="black", alpha=0.3)
    if (any(error<0)):
        print("Check your data because some error values are below zero!")
        error = np.abs(error)
    plt.errorbar(x, counts, yerr=error, ls="dotted", marker=None, capsize=2,alpha=0.2)
    plt.xlabel(xaxis.capitalize())
    plt.ylabel(yaxis.capitalize())
    if "loglog_" in fname:
        plt.xscale('log')
        plt.yscale('log')
    elif "log_" in fname:
        plt.yscale('log')
    plt.title(fname)
    plt.grid()
    plt.tight_layout()
    plt.show()

def get_info(line):
    parts = line.split(":")
    nbin = parts[1].strip().split(" ")[0]
    axis = parts[2]
    return float(nbin), axis

def read_mfile(fn):
    """
    Reads data from a 2D monitor file and extracts relevant information.

    Parameters:
    - fn (str): File path of the monitor file to be read.

    Returns:
    - nbiny (int): Number of bins in the y axis.
    - nbinz (int): Number of bins in the z axis.
    - by (numpy.ndarray): Array of bin values for the y axis.
    - bz (numpy.ndarray): Array of bin values for the z axis.
    - counts (numpy.ndarray): 2D array of counts for each bin.
    - xaxis (str): Label for the x axis.
    - yaxis (str): Label for the y axis.

    If an error occurs during file reading or if the file format is invalid,
    the function returns zeros for all output parameters.
    """
    try:
        fin = open(fn, "r")
        print("read correctly")
    except:
        print("Could not open file.")
        return 0

    header_lines = []
    content_lines = []
    for line in fin.readlines():
        if line.startswith("#"):
            header_lines.append(line.rstrip())
        elif line.rstrip():
            content_lines.append(line.rstrip())

    hline = header_lines[0]
    sitems = hline.split(":")
    if "2D" in sitems[0]:
        ftype = "mon2D"
    elif "1D" in sitems[0]:
        ftype = "mon1D"
    else:
        print("Wrong type of file!")
        return 0

    nbiny, nbinz = 0, 0
    # parse
    try:
        if hline.startswith("#Monitor 2D Intensity"):
            split1 = hline.split("bins:")
            nbiny = int(split1[0].split(":")[-1])
            xaxis = split1[1].strip().split(" ")[0]
            nbinz = split1[1].strip().split(" ")[-1]
            yaxis = split1[2]

        for line in header_lines:
            if line.startswith("# x-axis"):
                nbiny, xaxis = get_info(line)
                continue
            if line.startswith("# y-axis"):
                nbinz, yaxis = get_info(line)
                continue
    except:
        print("Error parsing file, no labels will be used.")
        xaxis, yaxis = "x", "y"

    bz = []
    counts = []
    if ftype == "mon2D":
        by = np.fromstring(content_lines[1], dtype=float, sep=" ")
    for line in content_lines[2:]:
        z = np.fromstring(line, dtype=float, sep=" ")
        bz.append(z[0])
        counts.append(z[1:])


    if ftype == "mon2D":
        by = np.array(by)
        bz = np.array(bz)
        plot2D(np.array(counts), xaxis, yaxis, by, bz, fn)

    elif ftype == "mon1D":
        counts = np.array(counts)
        plot1D(bz, counts[:,0], counts[:,1], xaxis, r"Intensity [n/s]", fn)

if __name__ == "__main__":
    read_mfile(sys.argv[1])