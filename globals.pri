CONFIG += c++11

win32:LIBS += -Lc:/local/boost_1_56_0/lib32-msvc-12.0/

win32:INCLUDEPATH += c:/local/boost_1_56_0
win32:DEPENDPATH += c:/local/boost_1_56_0

unix:LIBS += -lboost_system
