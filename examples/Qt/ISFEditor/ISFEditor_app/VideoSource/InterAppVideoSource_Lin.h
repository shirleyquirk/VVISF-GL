#pragma once

#include <QObject>
#include "VideoSource.h"

class InterAppVideoSource_LinOpaque;

class InterAppVideoSource_Lin : public VideoSource
{
    Q_OBJECT
public:
    InterAppVideoSource_Lin(QObject *parent = nullptr);
    ~InterAppVideoSource_Lin();

    virtual QList<MediaFile> createListOfStaticMediaFiles() override;
    virtual void start() override;
    virtual void stop() override;
    virtual bool playingBackItem(const MediaFile & n) override;
    virtual void loadFile(const MediaFile & n) override;

private:
    InterAppVideoSource_LinOpaque *opaque = nullptr;
};
