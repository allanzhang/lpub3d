# This means build Qt GUI - it does not mean 'using Qt!!'
contains(DEFINES, _OSMESA): \
DEFINES -= _OSMESA
DEFINES	+= _QT
include(TCFoundation.pri)

MISC_HEADER = $$shell_path( $$absolute_path( $$_PRO_FILE_PWD_/../../LDVMisc.h ) )
UTILS_DIR = $$shell_path( $$absolute_path( $$_PRO_FILE_PWD_/../Utilities ) )
# LPub3D Mod - quote the paths interpolated into the shell command below.
# $$shell_path() escapes for the shell but does NOT quote, so a checkout living
# under a path that contains a space (e.g. '.../IO Enhancement/...') truncates
# the command at the first space and fails the build with
#   /bin/sh: cd: /Users/.../IO: No such file or directory
MISC_HEADER_Q = \"$${MISC_HEADER}\"
UTILS_DIR_Q = \"$${UTILS_DIR}\"
if (mingw:ide_qtcreator)|win32-arm64-msvc|win32-msvc*: \
LINK_CMD = cd $${UTILS_DIR_Q} & if not exist \"misc.h\" \( mklink misc.h $${MISC_HEADER_Q} \)
else: \
LINK_CMD = cd $${UTILS_DIR_Q}; if ! test -f misc.h; then ln -s $${MISC_HEADER_Q} misc.h; fi
linkMiscHeader.target = linkMiscHeaderFile
linkMiscHeader.depends = $${MISC_HEADER}
linkMiscHeader.commands = $${LINK_CMD}
QMAKE_EXTRA_TARGETS += linkMiscHeader
PRE_TARGETDEPS += linkMiscHeaderFile

QMAKE_CLEAN += $$_PRO_FILE_PWD_/../Utilities/misc.h
