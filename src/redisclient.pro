QT       -= core gui

TARGET = redisclient
TEMPLATE = lib
CONFIG += staticlib

include(../globals.pri)


HEADERS += \
    redisclient/config.h \
    redisclient/redisclient.h \
    redisclient/redisparser.h \
    redisclient/redisvalue.h \
    redisclient/version.h \
    redisclient/impl/redisclientimpl.h

SOURCES += \
    redisclient/impl/redisclient.cpp \
    redisclient/impl/redisclientimpl.cpp \
    redisclient/impl/redisparser.cpp \
    redisclient/impl/redisvalue.cpp
