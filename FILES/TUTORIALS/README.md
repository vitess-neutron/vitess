# Vitess tutorials

contains exercises that illustrate the usage and special features of different VITESS modules. Some exercises are taken from the joint Vitess/McStas workshop that took place in Berlin 18.-20.09.2013; the respective McStas examples can be found at http://2013.essworkshop.org/. These exercises are based on VITESS 3.1. Input files are given in the subdirectories, as well as .pdf files containing detailed description of the exercises as well as (some) solutions.

## BeginnerExample
  The [BeginnerExample](./BeginnerExample) folder contains an introduction to VITESS simulations and basic usage of the most common modules. It is divided in two parts: 
  - [Ex1](./BeginnerExample/Talk_Ex1_VitessComponents.pdf): `source`, `sample` and `detector` modules.
  - [Ex2](./BeginnerExample/Talk_Ex1_VitessComponents.pdf): `guide` and `chopper` modules. 

Useful material that can be found in the same folder:
  - [ESSpowder_common.pdf](./BeginnerExample/ESSpowder_common.pdf) describes the instrument setup to be simulated.
  - [ESSpowder_VITESS.pdf](./BeginnerExample/ESSpowder_VITESS.pdf) contains instructions on how to implement this in VITESS.
  - [Solutions](./BeginnerExample/Solutions) in GUI files for both [Ex1](./BeginnerExample/Solutions/Ex1.3.gui) and [Ex2](./BeginnerExample/Solutions/Ex2.2.gui) are provided.

## DetectorExample
In this [exercise](./DetectorExample/) we want to see how different kinds of neutron detectors can be
modeled in VITESS. We concentrate on artificial setups showing the detector
performance

  - [Ex3](./DetectorExample/Exercise_Detector.pdf): detection material and efficiency, resolution effects, tube geometry, detector array, event mode output file

## GuideExample
In the [GuideExample](./GuideExample/) directory, different type of guides are considered and their differences are compared.

 - [Ex4](./GuideExample/Exercises_GuidesAdvanced.pdf) Straight, curved, kinked, ballistic and elliptic guides

## RaytracingExample
In this [exercise](./RaytracingExample/), we assume we have optimized a guide system using m=6 mirror
coating. Now we want to find out where in the guide we can use a lower coating
without loosing flux on the sample. We don’t want to use a high coating to reflect
a lot of neutrons that get lost somewhere else in the system, or contribute to
background! Therefore, we use raytracing to select the neutrons we want, and
then choose the coating appropriate to transport them

  - [Ex5](./RaytracingExample/Raytracing.pdf): Use neutron raytracing to determine supermirror coating (m-value) necessary in different guide sections

## Samples
In this [excercise folder](./Samples/) we study in more detail the `sample` modules in Vitess, both for [diffraction](./Samples/Diffraction_SingleWavelength/) and [reflection](./Samples/Reflection/).

- [Ex6](./Samples/Sample-exercise.pdf): reconstruct powder samples, get the background from sample environment, create a sample for reflectometry and study the differences between  considering or not the incoherent scattering.

## SupermirrorSystems
Finally, the [SupermirrorSystems folder](./SupermirrorSystems/) contains an excercise to model complex systems of supermirrors using the `sm_ensemble` module. In this module, any user-defined coordinates and orientation of supermirror plates can be given. You will build a bi-spectral beam extraction system consisting of several, differently inclined mirrors surrounded by guide walls.

- [Ex7](./SupermirrorSystems/SupermissorSystems.pdf): Build a bi-spectral beam extraction system consisting of several mirrors inserted partly into a guide.
