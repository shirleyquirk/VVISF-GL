#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>




namespace Ui {
	class MainWindow;
}




class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = nullptr);
	~MainWindow();

private slots:
	void on_actionNew_triggered();

	void on_actionOpen_triggered();

	void on_actionSave_triggered();
	
	void widgetDrewItsFirstFrame();

private:
	Ui::MainWindow *ui;
};




//	this function is called when the GL environment has been created and the app is ready to finish launching
void FinishLaunching();




#endif // MAINWINDOW_H
