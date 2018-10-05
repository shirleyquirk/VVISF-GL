#ifndef JSONGUIINPUTLONG_H
#define JSONGUIINPUTLONG_H

#include <QWidget>

#include "JSONGUIInput.h"




namespace Ui {
	class JSONGUIInputLong;
}




class JSONGUIInputLongWidget : public QWidget, public JSONGUIInput
{
	Q_OBJECT

public:
	explicit JSONGUIInputLongWidget(const JGMInputRef & inRef, QWidget *parent = nullptr);
	~JSONGUIInputLongWidget();
	
	virtual void prepareUIItems() override;
	virtual void refreshUIItems() override;

private:
	Ui::JSONGUIInputLong *ui;
};

#endif // JSONGUIINPUTLONG_H
