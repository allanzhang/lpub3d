QT += core core5compat testlib

CONFIG += console c++17
CONFIG -= app_bundle

TARGET = studio_io_import_test

INCLUDEPATH += ../../mainApp ../../quazip

LIBS += ../../quazip/64bit_release/libQuaZIP.a -lz

SOURCES += \
    tst_studio_io_importer.cpp \
    ../../mainApp/studioioimporter.cpp \
    ../../mainApp/studiotexmap.cpp \
    ../../lclib/common/studio_mesh_processor.cpp

HEADERS += \
    ../../mainApp/studioioimporter.h \
    ../../mainApp/studiotexmap.h \
    ../../lclib/common/studio_mesh_processor.h

INCLUDEPATH += ../../lclib/common
