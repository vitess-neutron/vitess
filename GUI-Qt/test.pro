#-------------------------------------------------
#
# Project created by QtCreator 2020-03-18T11:23:19
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = test
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++11

SOURCES += \
        basedialog.cpp \
        basemodule.cpp \
#        beamstop.cpp \
#        capture_flux.cpp \
        chopper_disc.cpp \
#        chopper_fermi_cur.cpp \
#        chopper_fermi_str.cpp \
        chopper_para.cpp \
#        collimator.cpp \
#        collimator_radial.cpp \
#        detector.cpp \
        dummy.cpp \
        elasticisotr_para.cpp \
        environment_para.cpp \
#        filter.cpp \
        flipper_coil.cpp \
        flipper_gradient.cpp \
        frame.cpp \
        guide.cpp \
        help.cpp \
        inelast_para.cpp \
        main.cpp \
        mainwindow.cpp \
        mirror.cpp \
        moderator.cpp \
        modultable.cpp \
        monitor1d.cpp \
        monitor2d.cpp \
        monochr_analyser.cpp \
        monochromator.cpp \
        monochromator_para.cpp \
        polariser_he3.cpp \
        polariser_sm.cpp \
        polar_sm_para.cpp \
        powder_para.cpp \
        precessionfield.cpp \
        reflectom_para.cpp \
        resonator_drabkin.cpp \
        rotating_field.cpp \
        s_q_para.cpp \
        sample_elasticisotr.cpp \
        sample_environment.cpp \
        sample_inelast.cpp \
        sample_nxs.cpp \
        sample_powder.cpp \
        sample_reflectom.cpp \
        sample_s_q.cpp \
        sample_sans.cpp \
        sample_singcryst.cpp \
        sans_para.cpp \
        singcryst_para.cpp \
        slit.cpp \
        sm_ensemble.cpp \
        source.cpp \
        space.cpp \
        spacewindow.cpp \
        velselect.cpp

HEADERS += \
        basedialog.h \
        basemodule.h \
#        beamstop.h \
#        capture_flux.h \
        chopper_disc.h \
#        chopper_fermi_cur.h \
#        chopper_fermi_str.h \
        chopper_para.h \
#        collimator.h \
#        collimator_radial.h \
#        detector.h \
        dummy.h \
        defines.h \
        elasticisotr_para.h \
        environment_para.h \
#        filter.h \
        flipper_coil.h \
        flipper_gradient.h \
        frame.h \
        guide.h \
        help.h \
        inelast_para.h \
        mainwindow.h \
        mirror.h \
        moderator.h \
        modultable.h \
        monitor1d.h \
        monitor2d.h \
        monochr_analyser.h \
        monochromator.h \
        monochromator_para.h \
        polariser_he3.h \
        polariser_sm.h \
        polar_sm_para.h \
        powder_para.h \
        precessionfield.h \
        reflectom_para.h \
        resonator_drabkin.h \
        rotating_field.h \
        s_q_para.h \
        sample_elasticisotr.h \
        sample_environment.h \
        sample_inelast.h \
        sample_nxs.h \
        sample_powder.h \
        sample_reflectom.h \
        sample_s_q.h \
        sample_sans.h \
        sample_singcryst.h \
        sans_para.h \
        singcryst_para.h \
        slit.h \
        sm_ensemble.h \
        source.h \
        space.h \
        spacewindow.h \
        velselect.h

FORMS += \
#        beamstop.ui \
#        capture_flux.ui \
        chopper_disc.ui \
#        chopper_fermi_cur.ui \
#        chopper_fermi_str.ui \
        chopper_para.ui \
#        collimator.ui \
#        collimator_radial.ui \
#        detector.ui \
        dummy.ui \
        elasticisotr_para.ui \
        environment_para.ui \
#        filter.ui \
        flipper_coil.ui \
        flipper_gradient.ui \
        frame.ui \
        guide.ui \
        help.ui \
        inelast_para.ui \
        mainwindow.ui \
        mirror.ui \
        moderator.ui \
        modultable.ui \
        monitor1d.ui \
        monitor2d.ui \
        monochr_analyser.ui \
        monochromator.ui \
        monochromator_para.ui \
        polariser_he3.ui \
        polariser_sm.ui \
        polar_sm_para.ui \
        powder_para.ui \
        precessionfield.ui \
        reflectom_para.ui \
        resonator_drabkin.ui \
        rotating_field.ui \
        s_q_para.ui \
        sample_elasticisotr.ui \
        sample_environment.ui \
        sample_inelast.ui \
        sample_nxs.ui \
        sample_powder.ui \
        sample_reflectom.ui \
        sample_s_q.ui \
        sample_sans.ui \
        sample_singcryst.ui \
        sans_para.ui \
        singcryst_para.ui \
        slit.ui \
        sm_ensemble.ui \
        source.ui \
        space.ui \
        spacewindow.ui \
        velselect.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resource.qrc

unix: CONFIG += link_pkgconfig
unix: PKGCONFIG += yaml-cpp
