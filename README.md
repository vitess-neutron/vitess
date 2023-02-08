## Virtual Instrumentation Tool for Neutron Scattering at Pulsed and Continuous Sources

Welcome to GitLab Page of the simulation program '**VITESS**' (Virtual Instrumentation Tool for the European Spallation Source).


**VITESS** is a tool for simulation of neutron scattering instruments and experiments at pulsed and continuous sources. Using **VITESS**, you can simulate a large variety of instruments at all major current and future neutron sources, including the European Spallation Source, which is currently being designed. **VITESS** is supported by a graphical user interface (GUI), making it simple for you to compose your instrument and run simulations. You can use **VITESS** on a **Windows**, **Linux** or **Macintosh** computer. Simply choose the appropriate [download file](https://iffgit.fz-juelich.de/vitess/vitess/-/releases) and install **VITESS** on your computer. The **VITESS** installation includes some example instruments to demonstrate the main features of the program. You can also use the examples as an initial aid for your own instruments. For any questions and feedback of all kind you may contact the **VITESS** developers team, vitess@fz-juelich.de, and we will help you as quickly as possible.

### Latest versions

The current **VITESS** version is 3.4.

As new features, it allows reading and writing neutron data in MCLP and MCNPX format and contains the description of the ESS Butterfly-1 moderator. We invite you to download and install the new release and give us feedback on your experiences. Please also take a look at the version history.


### Concept of VITESS
The simulation of a neutron instrument comprises several modules, which represent instrument components like guides or detectors, various helper modules, e.g. frames modifying the coordinate system between two instrument components, and modules for data visualisation and evaluation. All modules can be selected within the GUI. When you run a simulation, such modules are co-working sequentially embedded in a pipe structure:

- Each module processes and then passes neutron data to the following one.
- The first module must be either a neutron source module, or it must read information about the neutron trajectories from a file typically created in an earlier simulation of the preceding part of your instrument, which includes the source module.
- The last module may be set up to write a file of neutron trajectories.  This makes sense if you wish to use these trajectories in further simulations  as input, e.g., for future studies of parameter distributions of these neutrons.

### References

The VITESS package is described in the following publications:

[1] D. Wechsler, G. Zsigmond, F. Streffer, F. Mezei, [VITESS: Virtual instrumentation tool for pulsed and continuous sources](http://www.tandfonline.com/doi/abs/10.1080/10448630008233764), Neutron News 11/4 (2000) 25-28.

[2] G. Zsigmond, K. Lieutenant, F. Mezei, [Monte Carlo simulations of neutron scattering instruments by VITESS - Virtual Instrumentation Tool for ESS](http://www.tandfonline.com/doi/abs/10.1080/10448630208218488), Neutron News 13/4 (2002) 11-14.

[3] G. Zsigmond, K. Lieutenant, S. Manoshin, H. N. Bordallo, J. D. M. Champion, J. Peters, J.M. Carpenter, F. Mezei, [A survey of simulations of complex neutronic systems by VITESS](http://www.sciencedirect.com/science/article/pii/S0168900204009015), Nucl. Instrum. Methods Phys. Res A 529 (2004) 218-222.

[4] K. Lieutenant, G. Zsigmond, S. Manoshin, M. Fromme, H. N. Bordallo, J. D. M. Champion, J. Peters, F. Mezei, [Neutron instrument simulation and optimization using the software package VITESS](https://www.spiedigitallibrary.org/conference-proceedings-of-spie/5536/0000/Neutron-instrument-simulation-and-optimization-using-the-software-package-VITESS/10.1117/12.562814.short), Proc. SPIE Int. Soc. Opt. Eng. 5536, ed. M. Sanchez del Rio (2004) 134 - 145.

[5] C. Zendler, K. Lieutenant, D. Nekrassov, M. Fromme, [Vitess 3 – Virtual Instrumentation Tool for the European Spallation](http://iopscience.iop.org/article/10.1088/1742-6596/528/1/012036), J. Phys. Conf. Ser. 528 (2014) 012036.

Additionally there is a large number of publications based on VITESS simulations. A list of them is easiest obtained using [Google Scholar](https://scholar.google.com/scholar?q=VITESS+neutron+-vitesse+-vitesses&btnG=&hl=en&num=20&as_sdt=0%2C5).
