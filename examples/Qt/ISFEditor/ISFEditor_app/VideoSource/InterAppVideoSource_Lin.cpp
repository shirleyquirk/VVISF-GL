// dummy implementation because no syphon nor spout
#include "InterAppVideoSource_Lin.h"
#include "VVGL.hpp"

using namespace VVGL;

class InterAppVideoSource_LinOpaque{};

InterAppVideoSource_Lin::InterAppVideoSource_Lin(QObject *parent) : VideoSource(parent) {}

InterAppVideoSource_Lin::~InterAppVideoSource_Lin(){
    stop();
}

void InterAppVideoSource_Lin::stop(){ VideoSource::stop(); }
void InterAppVideoSource_Lin::start(){
    std::lock_guard<std::recursive_mutex> tmpLock(_lock);
    if (_running)
        return;
    if (_file.type() != MediaFile::Type_App)
        return;
    _running = true;
}
void InterAppVideoSource_Lin::loadFile(const MediaFile & n){
    std::lock_guard<std::recursive_mutex> tmpLock(_lock);
    if (_file == n)
        return;
    stop();
    _file = n;
    start();
}
bool InterAppVideoSource_Lin::playingBackItem(const MediaFile & n){
    return (_file == n);
}

QList<MediaFile> InterAppVideoSource_Lin::createListOfStaticMediaFiles(){
    QList<MediaFile> result;
    return result;
}


