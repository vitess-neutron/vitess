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
