# Vitess FILES directory

Several different type of files can be found in this directory including:

- [Example files](./EXAMPLES/) of different neutron instruments and modules that can aid users on how to properly use Vitess.
- [moderator](./moderators/) files describing the moderators of a variety of neutron sources, both reactor based as well as spallation sources. These files are needed when adding a source module in vitess and are use to describe the neutron phase-space variables to be sampled on each neutron generation.
- [Plot](./Plot/) files that provide commands to the GUI for the plotting software to be used. Read documentation of [show.tcl](../GUI/show.tcl) to further understand how to use this Plot folder to create your own plotting tool from a template.
- Examples of [reflectivity](./reflectivity_files/) files necessary when defining neutron guides or mirrors.
- A set of [sample](./sample_files/) files that can be used when defining sample modules. These serve as reference also on the format accepted by the modules.
- Examples of auxiliary [scripts](./Scripts/) that can be used to load and plot Vitess data in `python`.