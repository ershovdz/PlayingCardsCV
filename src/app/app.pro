TEMPLATE = app
FORMS = ui/mainwindow.ui ui/aboutdialog.ui ui/addobjectdialog.ui
TARGET = cards
DESTDIR = ../build/bin
CONFIG += debug
BINDIR = /usr/bin
target.path = $$BINDIR

INCLUDEPATH += $(OPENCV_DIR)/build/include

QMAKE_RPATHDIR += ../build/lib

QMAKE_LIBDIR += ../build/lib $(OPENCV_DIR)/lib/Release

HEADERS += mainwindow.h \
	aboutdialog.h \
	addobjectdialog.h \
	camera.h \
	keypointitem.h \
	objwidget.h \
	parameterstoolbox.h \
	qtipl.h \
	settings.h

SOURCES += main.cpp \
	mainwindow.cpp \
	aboutdialog.cpp \
	addobjectdialog.cpp \
	camera.cpp \
	keypointitem.cpp \
	objwidget.cpp \
	parameterstoolbox.cpp \
	qtipl.cpp \
	settings.cpp 

LIBS += -lopencv_core231 -lopencv_features2d231 -lopencv_flann231 -lopencv_imgproc231 -lopencv_calib3d231 -lopencv_highgui231  


OBJECTS_DIR += ../build/bin

RESOURCES += resources.qrc
INSTALLS += target
