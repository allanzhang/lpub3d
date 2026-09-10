# myLPub3D Translation Source Project
# ---------------------------------------------------------------------------
# Purpose
#   Single-invocation extraction of every user-visible string in myLPub3D.
#   lupdate cannot be given both .pro files and source directories in one call,
#   and running it repeatedly against the same .ts with -no-obsolete would drop
#   entries contributed by the previous module. Therefore every module's source
#   and form files are aggregated here and extracted exactly once.
#
# Usage
#   lupdate mainApp/translations/translations.pro \
#           -ts mainApp/translations/lpub3d_zh_CN.ts -no-obsolete
#
# This file is a translation-source manifest only. It is never built.
# ---------------------------------------------------------------------------

TEMPLATE = app
TARGET   = translations
QT      += core gui widgets
CONFIG  += console
CONFIG  -= app_bundle

# ---------------------------------------------------------------------------
# mainApp - main application UI
# ---------------------------------------------------------------------------
SOURCES += \
    $$files($$PWD/../*.cpp) \
    $$files($$PWD/../*.h) \
    $$files($$PWD/../commands/*.cpp) \
    $$files($$PWD/../commands/*.h) \
    $$files($$PWD/../commands/snippets/*.cpp) \
    $$files($$PWD/../commands/snippets/*.h)

FORMS += \
    $$files($$PWD/../*.ui)

# ---------------------------------------------------------------------------
# lclib - visual editor library (reachable from the application)
# ---------------------------------------------------------------------------
SOURCES += \
    $$files($$PWD/../../lclib/*.cpp) \
    $$files($$PWD/../../lclib/*.h) \
    $$files($$PWD/../../lclib/common/*.cpp) \
    $$files($$PWD/../../lclib/common/*.h) \
    $$files($$PWD/../../lclib/qt/*.cpp) \
    $$files($$PWD/../../lclib/qt/*.h)

FORMS += \
    $$files($$PWD/../../lclib/common/*.ui) \
    $$files($$PWD/../../lclib/qt/*.ui)

# ---------------------------------------------------------------------------
# ldvlib / LDVQt - LDView renderer panels and messages
# ---------------------------------------------------------------------------
SOURCES += \
    $$files($$PWD/../../ldvlib/LDVQt/*.cpp) \
    $$files($$PWD/../../ldvlib/LDVQt/*.h) \
    $$files($$PWD/../../ldvlib/LDVQt/LDView/LDExporter/*.cpp) \
    $$files($$PWD/../../ldvlib/LDVQt/LDView/LDExporter/*.h) \
    $$files($$PWD/../../ldvlib/LDVQt/LDView/LDLib/*.cpp) \
    $$files($$PWD/../../ldvlib/LDVQt/LDView/LDLib/*.h) \
    $$files($$PWD/../../ldvlib/LDVQt/LDView/LDLoader/*.cpp) \
    $$files($$PWD/../../ldvlib/LDVQt/LDView/LDLoader/*.h) \
    $$files($$PWD/../../ldvlib/LDVQt/LDView/TCFoundation/*.cpp) \
    $$files($$PWD/../../ldvlib/LDVQt/LDView/TCFoundation/*.h) \
    $$files($$PWD/../../ldvlib/LDVQt/LDView/TRE/*.cpp) \
    $$files($$PWD/../../ldvlib/LDVQt/LDView/TRE/*.h)

FORMS += \
    $$files($$PWD/../../ldvlib/LDVQt/*.ui)

# ---------------------------------------------------------------------------
# qsimpleupdater - update notification dialogs
# ---------------------------------------------------------------------------
SOURCES += \
    $$files($$PWD/../../qsimpleupdater/src/*.cpp) \
    $$files($$PWD/../../qsimpleupdater/src/*.h) \
    $$files($$PWD/../../qsimpleupdater/src/progress_bar/*.cpp) \
    $$files($$PWD/../../qsimpleupdater/src/progress_bar/*.h)

FORMS += \
    $$files($$PWD/../../qsimpleupdater/src/*.ui) \
    $$files($$PWD/../../qsimpleupdater/src/progress_bar/*.ui)

# ---------------------------------------------------------------------------
# quazip - archive error messages
# ---------------------------------------------------------------------------
SOURCES += \
    $$files($$PWD/../../quazip/*.cpp) \
    $$files($$PWD/../../quazip/*.h)
