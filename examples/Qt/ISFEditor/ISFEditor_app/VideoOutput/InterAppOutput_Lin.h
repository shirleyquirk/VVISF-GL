#pragma once

#include <QObject>

#include "VideoOutput.h"

class InterAppOutput_LinOpaque;




class InterAppOutput_Lin : public VideoOutput
{
	Q_OBJECT
	
public:
	explicit InterAppOutput_Lin(QObject *parent = nullptr);
	~InterAppOutput_Lin();
	
	virtual void publishBuffer(const VVGL::GLBufferRef & inBuffer) override;
	virtual void moveGLToThread(const QThread * n) override;

private:
	InterAppOutput_LinOpaque		*opaque = nullptr;	//	used to store objective-c stuff
};
