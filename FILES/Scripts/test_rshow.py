from rshow import read_m2Dfile, contshow
import matplotlib.pyplot as plt
import sys

fname = sys.argv[1]

print(f"This script generates two plots for mon2D data file: {fname}.")

nbiny, nbinz, by, bz, counts, xaxis, yaxis = read_m2Dfile(sys.argv[1])

# Make your own plot
plt.figure()
plt.imshow(counts, origin="lower", extent=[min(by), max(by), min(bz), max(bz)])
plt.xlabel(xaxis)
plt.ylabel(yaxis)
plt.title(fname)
plt.colorbar()
plt.axis("equal")
plt.tight_layout()
plt.show()

# Use contour plot from rshow
contshow(sys.argv[1], by, bz, counts, xaxis, yaxis)
