![Logo](./BITMAPS/vitess_logo.png){height=100px}

# Virtual Instrumentation Tool for Neutron Scattering at Pulsed and Continuous Sources

![Version](https://img.shields.io/badge/Current_Version-3.7-green.svg)  ![License: GNU](https://img.shields.io/badge/License-GNU-blue.svg)

Welcome to the GitLab repository of **VITESS** (Virtual Instrumentation Tool for the European Spallation Source).

**VITESS** is a tool for the simulation of neutron scattering instruments and experiments at pulsed and continuous sources. Using **VITESS**, you can simulate a large variety of instruments at any of the major current and future neutron sources, including the European Spallation Source, which is currently being designed. **VITESS** is supported by a graphical user interface (GUI), making it simple for you to compose your instrument and run simulations. You can use **VITESS** on a **Windows**, **Linux** or **Macintosh** computer. Simply choose the appropriate [download file](https://iffgit.fz-juelich.de/vitess/vitess/-/releases) and install **VITESS** on your computer. The **VITESS** installation includes some example instruments to demonstrate the main features of the program. You can also use the [examples](./FILES/EXAMPLES/) as an initial aid for your own instruments. For any questions and feedback of all kind you may contact the **VITESS** developers team, vitess@fz-juelich.de, and we will help you as quickly as possible.

Version 3.8 contains a many improvements and corrections.
Plotting is done with [GR](https://gr-framework.org) by default; python/matplotlib and gnuplot options are also available if installed.
The modules `kdsource` and `sample_ncrystal` are now compiled for windows.
There are major fixes for some options of the `monochromator` and `bender` modules.

**VITESS** 4.0 is in active development. If you are interested in testing it, checkout the develop branch or download an [up-to-date snapshot](https://iffgit.fz-juelich.de/vitess/vitess/-/jobs/artifacts/develop/browse?job=package). Feel free to contact us if you encounter any issues.


## Installation

### Download Links

Download the current stable version: [VITESS 3.7](https://iffgit.fz-juelich.de/vitess/vitess/-/releases).

Download a snapshot of the development version: [VITESS develop](https://iffgit.fz-juelich.de/vitess/vitess/-/jobs/artifacts/develop/browse?job=package).


### Windows
Download the and run the installer from the links above.

### MacOS
- Install [XQuartz](https://xquartz.org)
- Download and unpack VITESS: run `curl -L https://iffgit.fz-juelich.de/vitess/vitess/-/jobs/1178196/artifacts/raw/Downloads/Vitess3.8-Darwin-universal.tar.gz | tar xz` in the terminal.
- If you used a download link from above, you might get an error message about running unknown code. In that case, run `xattr -c -r vitess3.8/`
- Run the script `Vitess-MacOS` to start the GUI
- For older versions (Vitess 3.7 or below): Install Tcl/Tk 8.6, e.g. from homebrew: `brew install tcl-tk@8.6`

### Linux

To run the Vitess-GUI you need [Tcl/TK](https://www.tcl-lang.org) 8.6 and [LibGD](https://libgd.org) 2.3.

- Debian, Ubuntu etc: `sudo apt install -y libgd3 tk`
- RedHat, Fedora, etc: `sudo dnf install -y gd tk`

Install a visualization programm: [Grplot](https://gr-framework.org), python with matplotlib, or gnuplot.


### Optional: Compile from source

To build Vitess on various operating systems you need to install the following packages:

- Mac OS: [Xcode Command Line Tools](https://developer.apple.com/xcode/resources), [XQuartz](https://www.xquartz.org), [LibGD](https://libgd.org), [Tcl/Tk](https://www.tcl-lang.org/) and [CMake](https://cmake.org/)
- Debian, Ubuntu: `apt-get install make gcc g++ cmake libgd-dev libxml2-dev`
- RedHat, CentOS: `dnf install make gcc gcc-c++ cmake gd-devel libxml2-devel`

Compilation Commands:
- Linux/MacOS: `cd SRC; make && make install`
- Windows: `cd SRC & nmake -f vitess.mak && nmake -f vitess.mak install`


## Concept of VITESS

The simulation of a neutron instrument comprises several modules, which represent instrument components like guides or detectors, various helper modules, e.g. frames modifying the coordinate system between two instrument components, and modules for data visualisation and evaluation. All modules can be selected within the GUI. When you run a simulation, such modules are co-working sequentially embedded in a pipe structure:

- Each module processes and then passes neutron data to the following one.
- The first module must be either a neutron source module, or it must read information about the neutron trajectories from a file typically created in an earlier simulation of the preceding part of your instrument, which includes the source module.
- The last module may be set up to write a file of neutron trajectories.  This makes sense if you wish to use these trajectories in further simulations  as input, e.g., for future studies of parameter distributions of these neutrons.


## Documentation, Examples and Tutorials

The HTML documentation of the Vitess Modules can be found at [vitess.iffgit.fz-juelich.de/vitess](https://vitess.iffgit.fz-juelich.de/vitess)

A directory containing a compendium of over 20 examples illustrating the utilization of `Vitess` for Monte Carlo simulations on diverse existing neutron instruments,
as well as explications of the distinctive `Vitess` Modules can be found in [FILES/EXAMPLES](FILES/EXAMPLES/).
It is recommended as a starting point for those new to the software.

A series of excercises (most with solutions) designed to enhance the utilization of the most common `Vitess` Modules
can be found in the [FILES/TUTORIALS](./FILES/TUTORIALS/) folder.


## Python package

The [Python package](https://iffgit.fz-juelich.de/vitess/vitess-python) is being developed as an alternative to the GUI.
It enables editing Vitess Instruments from the Python command line or Jupyter Notebooks. 


## References

The VITESS package is described in the following publications:

[1] D. Wechsler, G. Zsigmond, F. Streffer, F. Mezei, [VITESS: Virtual instrumentation tool for pulsed and continuous sources](http://www.tandfonline.com/doi/abs/10.1080/10448630008233764), Neutron News 11/4 (2000) 25-28.

[2] G. Zsigmond, K. Lieutenant, F. Mezei, [Monte Carlo simulations of neutron scattering instruments by VITESS - Virtual Instrumentation Tool for ESS](http://www.tandfonline.com/doi/abs/10.1080/10448630208218488), Neutron News 13/4 (2002) 11-14.

[3] G. Zsigmond, K. Lieutenant, S. Manoshin, H. N. Bordallo, J. D. M. Champion, J. Peters, J.M. Carpenter, F. Mezei, [A survey of simulations of complex neutronic systems by VITESS](http://www.sciencedirect.com/science/article/pii/S0168900204009015), Nucl. Instrum. Methods Phys. Res A 529 (2004) 218-222.

[4] K. Lieutenant, G. Zsigmond, S. Manoshin, M. Fromme, H. N. Bordallo, J. D. M. Champion, J. Peters, F. Mezei, [Neutron instrument simulation and optimization using the software package VITESS](https://www.spiedigitallibrary.org/conference-proceedings-of-spie/5536/0000/Neutron-instrument-simulation-and-optimization-using-the-software-package-VITESS/10.1117/12.562814.short), Proc. SPIE Int. Soc. Opt. Eng. 5536, ed. M. Sanchez del Rio (2004) 134 - 145.

[5] C. Zendler, K. Lieutenant, D. Nekrassov, M. Fromme, [Vitess 3 – Virtual Instrumentation Tool for the European Spallation](http://iopscience.iop.org/article/10.1088/1742-6596/528/1/012036), J. Phys. Conf. Ser. 528 (2014) 012036.

Additionally there is a large number of publications based on VITESS simulations. A list of them is easiest obtained using [Google Scholar](https://scholar.google.com/scholar?q=VITESS+neutron+-vitesse+-vitesses&btnG=&hl=en&num=20&as_sdt=0%2C5).
