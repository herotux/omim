# Expat xml parser.

TARGET = expat
TEMPLATE = lib
CONFIG += staticlib

ROOT_DIR = ../..
DEPENDENCIES =

include($$ROOT_DIR/common.pri)

unix|win32-g++ {
  QMAKE_CFLAGS_WARN_ON += -Wno-unused -Wno-missing-field-initializers -Wno-switch -Wno-uninitialized
}

SOURCES += \
    lib/xmlparse.c \
    lib/xmlrole.c \
    lib/xmltok.c \
    lib/xmltok_impl.c \
    lib/xmltok_ns.c \

HEADERS += \

