# Tutorial: D20 Two-Axis Diffractometer

## Preparation
- Download the current version of the program VITESS from the website: https://iffgit.fz-juelich.de/vitess/vitess
- Follow the instructions in the "Installation" section of the README file
- Start the 'Vitess' program and begin a new instrument simulation by defining the parameter directory, where the simulation is executed

## Exercise 1: Source
- Use 'source\_ILL' as the first module and create neutrons in the wavelength range 2.4+/-0.1 Ang.
- Choose the file 'ILL\_thml\_2011.mod' to define the thermal source of the D2O diffractometer at ILL.
  (Its size is assumed to be 20x20 cm², and the wavelength dependence of the brilliance is given in 'ILL\_thml\_2011.dat'.)
  The files can be found in 'VITESS\_installation\_directory'/FILES/moderators/ILL.)
- Direct the neutron beam to an area of 15x15 cm² in 2.512 m distance using 'direction defined by window'.
- Put a 1D monitor there to show the wavelength distribution (using 'mon1\_lambda' or 'Monitor1D')
- Start 1 Mio. trajectories to determine the wavelength distribution at the target
Question:
- Do wavelength and spatial distribution look as expected?

## Exercise 2: Apertures
The aperture in 2.512 m distance from the source has in fact the octogonal shape of 'Stop' sign.
- Add another aperture 45° rotated. It is directly possible within the 'window' module or using the module 'frame' before and after the module
- Add a circular window of 5.55 cm radius is located 4.042 m from the moderator
- Add 2D position monitors after each aperture
- Run the simulation
Tasks and questions:
- Check the position of all components by comparing the output file 'instrument.inf' with the description in 'D20.inf'.
- Are size and shape of apertures as expected?

## Exercise 3: Collimator
The beam divergence is reduced by a collimator of 9.5 cm width, 30 cm height and 25.7 cm length beginning 16.4 m from the moderator. It consists of 25 channels separated by aborbing material of 0.1 mm thickness.
- Add the collimator using the module 'collimator'.
- Run the simulation and check its influence on the horizontal divergence by adding a monitor ('mon1\_divy' or 'Monitor1D') before and after this component (proper range: -1° to +1°)
Questions
- Does the collimator work ?
- What are the expected and measured FWHM widths?

## Exercise 4: Monochromator
A monochromator selects a narrow wavelength band out of the white neutron beam by Bragg reflection of crystals varying slightly in their orientation, which is called mosaicity or mosaic spread. In this case, the (2 0 0) reflection of HOPG graphite crystals are used to reflect the beam 42° out of the incoming direction (lattice distance 6.71 Ang). It consists of 13 elements of 9.5 cm width, 2.0 cm height and 0.2 cm thickness, located without gaps on a vertical cylinder of 202 cm radius.
- Add the monochromator using the module 'ma\_focus\_new' assuming  a peak reflectivity of 0.9, no d\_spread and no monochromator movement
- The center of the monochromator is 17.2 m from the moderator.
- Set the central monochromator element to the equatorial plane by adjusting the orientation of hte downmost element in 'angle vert.'
  (The setting can be checked in the 'focus file', which is generated in the beginning of the simulation.)
- Check the wavelength incoming and outgoing wavelength bands by suitable monitors.
Questions:
- What is the average wavelength?
- Was the wavelength band at the source properly chosen?

## Exercise 5: Sample
A powder sample will scatter the neutron beam. Its structure can be determined from orientation and intensities of the rings in which the neutrons are scattered
- Choose aluminum and/or Na2Ca3Al2F14 (NAC). 'NAC.pow' and 'Al.pow' contain the properties of the sample, 'NAC.str' and 'Al.str' the reflections and their intensities
- The sample is cylindrical with 0.5 cm diameter and 4 cm height
- Scatter first into 4 pi solid angle by leaving 'Theta','dTheta', 'Phi' and 'dPhi' empty
Questions:
- Which percentage of the neutrons is scattered?

## Exercise 6: Detector
The detector has a height of 15 cm and covers 153.6° in a distance of 1.471 m with cells of 0.1° width and 1 cm height. In our experiment, it is centered at 80° scattering angle.
- Set up the detector using the module 'screen' to get the diffracion pattern in the 'monitor file'.
  (Alternatively, the module 'detector' in the option 'monitor only' can be used in combination with 'Monitor2D' to visualize the diffraction pattern.)
- Run a first simulation to see the diffraction pattern
- Reduce the solid angle into which neutrons are scattered in 'sample\_powder' to the detector area (to improve simulation efficiency).
  Take care that the full detector range is covered by the angular range defined this way.
Questions:
- Is the percentage of scattered neutrons still the same?
- What is the total detector count rate?

Comment to using 'screen' or 'detector':
In both cases, neither the detection efficiency nor line broadening inside the detector are considered. For this, the module 'detector'has to be used in the option 'usage normal', which requires defining several detector details and is beyond the scope of this virtual experiment

## Exercise 7: Evaluate
VITESS enables first step of data evaluation by calculating the detector count rate as a function of scattering anlge, d-spacing or momentum transfer using the average wavelength (or the time-of-flight on pulsed sources).
- Determine the count rate as a function of scattering angle using the module 'eval\_elast'?
- Determine the count rate as a function of d-spacing with the same module?
Questions:
- Is the resolution good?
- If not, how can it be improved?
- Can you identify the crystal structure?
- If not, how can this be done?
