#ifndef INTERAPPOUTPUT_H
#define INTERAPPOUTPUT_H

#include <QObject>

#include "VideoOutput.h"

#if defined(Q_OS_MAC)
#include "InterAppOutput_Mac.h"
#elif defined(Q_OS_WIN)
#include "InterAppOutput_Win.h"
#elif defined(Q_OS_LINUX)
#include "InterAppOutput_Lin.h"
#endif




class InterAppOutput : public VideoOutput
{
	Q_OBJECT
	
public:
	explicit InterAppOutput(QObject *parent = nullptr);
	
	virtual void publishBuffer(const VVGL::GLBufferRef & inBuffer) override;
	
	virtual void moveGLToThread(const QThread * n) override;

private:
#if defined(Q_OS_MAC)
	InterAppOutput_Mac			output;
#elif defined(Q_OS_WIN)
	InterAppOutput_Win			output;
#elif defined(Q_OS_LINUX)
	InterAppOutput_Lin			output;
#endif
};




#endif // INTERAPPOUTPUT_H
