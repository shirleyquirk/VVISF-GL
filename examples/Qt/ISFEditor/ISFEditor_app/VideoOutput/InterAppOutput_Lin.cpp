#include "InterAppOutput_Lin.h"

#include <QDebug>

#include "VVGL.hpp"

using namespace VVGL;

class InterAppOutput_LinOpaque{};

InterAppOutput_Lin::InterAppOutput_Lin(QObject *parent) : VideoOutput(parent) {}
InterAppOutput_Lin::~InterAppOutput_Lin(){}
	
void InterAppOutput_Lin::publishBuffer(const VVGL::GLBufferRef & inBuffer){
    if (inBuffer == nullptr)
        return;
    if (opaque == nullptr) {
        qDebug() << "unimplemented " << __PRETTY_FUNCTION__;
        return;
    }
}
void InterAppOutput_Lin::moveGLToThread(const QThread * n) {}


