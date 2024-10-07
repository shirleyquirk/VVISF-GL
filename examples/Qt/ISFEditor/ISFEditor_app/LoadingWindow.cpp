#include "LoadingWindow.h"
#include "ui_LoadingWindow.h"

#include <QDebug>
#include <QFile>
#include <QDir>
//#include <QFileSystemModel>
//#include <QTimer>
#include <QSettings>
#include <QItemSelection>
//#include <QErrorMessage>
#include <QScrollArea>
#include <QDir>
#include <QStandardItemModel>
#include <QAbstractEventDispatcher>
#include <QScreen>
#include <QMessageBox>

#include "DocWindow.h"
#include "ISFController.h"
#include "JGMTop.h"
#include "DynamicVideoSource.h"
#include "LoadingWindowFileListModel.h"
#include "MediaFile.h"




using namespace std;

static LoadingWindow * globalLoadingWindow = nullptr;




LoadingWindow::LoadingWindow(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::LoadingWindow)
{
	//qDebug() << __PRETTY_FUNCTION__;
	
	//	disable the close button!
	setWindowFlags((windowFlags() | Qt::CustomizeWindowHint) & (~Qt::WindowCloseButtonHint));
	
	globalLoadingWindow = this;
	
	ui->setupUi(this);
	
	//	the spinboxes for setting rendering res need sane min/maxes
	ui->renderResWidthWidget->blockSignals(true);
	ui->renderResHeightWidget->blockSignals(true);
	ui->renderResWidthWidget->setMinimum(1);
	ui->renderResWidthWidget->setMaximum(16384);
	ui->renderResHeightWidget->setMinimum(1);
	ui->renderResHeightWidget->setMaximum(16384);
	ui->renderResWidthWidget->blockSignals(false);
	ui->renderResHeightWidget->blockSignals(false);
	
	//	the DynamicVideoSource class has a signal which is emitted when its list of sources change
	//connect(GetDynamicVideoSource(), &DynamicVideoSource::listOfStaticSourcesUpdated, this, &LoadingWindow::listOfVideoSourcesUpdated);
	connect(GetDynamicVideoSource(), &DynamicVideoSource::listOfStaticSourcesUpdated, this, &LoadingWindow::listOfVideoSourcesUpdated);
	//connect(GetDynamicVideoSource(), SIGNAL(listOfStaticSourcesUpdated), this, SLOT(listOfVideoSourcesUpdated));
	connect(ui->videoSourceComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &LoadingWindow::videoSourceChanged);
	
	//	we do this later, on an even that fires when the window opens, because i think it's screwing things up
	/*
	//	bump the slot to populate the pop-up button with the list of sources
	listOfVideoSourcesUpdated(GetDynamicVideoSource());
	*/
	
	//	save window position on app quit
	connect(qApp, &QCoreApplication::aboutToQuit, this, &LoadingWindow::appQuitEvent);
	
	
	//qDebug() << "\t" << __PRETTY_FUNCTION__ << " - FINISHED";
}

LoadingWindow::~LoadingWindow()
{
	delete ui;
}

QScrollArea * LoadingWindow::getScrollArea()	{
	return ui->scrollArea;
}
QSpinBox * LoadingWindow::getWidthSB() { return ui->renderResWidthWidget; }
QSpinBox * LoadingWindow::getHeightSB() { return ui->renderResHeightWidget; }
void LoadingWindow::on_createNewFile(const bool & inShowTypePicker, const VVISF::ISFFileType & inFileType)	{
	//qDebug() << __PRETTY_FUNCTION__ << " ... " << inShowTypePicker << ", " << QString(ISFFileTypeString(inFileType).c_str());
	
	DocWindow		*dw = GetDocWindow();
	ISFFileType		fileType = ISFFileType_None;
	if (inShowTypePicker)	{
		//	before we show the "new file type" picker, pause the auto-save timer...
		if (dw != nullptr)
			dw->pauseAutoSaveTimer();
		//	figure out what kind of file the user wants to create
		QMessageBox		*question = new QMessageBox(this);
		question->addButton("Cancel", QMessageBox::ButtonRole::NoRole);
		question->addButton("Generator", QMessageBox::ButtonRole::YesRole);
		question->addButton("Filter", QMessageBox::ButtonRole::YesRole);
		question->addButton("Transition", QMessageBox::ButtonRole::YesRole);
		question->setText("What kind of ISF would you like to create?");
		//question->setDetailedText("This is the detailed text");
		//question->setWindowTitle("This is the title");
		question->setWindowModality(Qt::ApplicationModal);
		int			ret = question->exec();
		if (ret == 0)
			return;
		switch (ret)	{
		case 1:		fileType = ISFFileType_Source;		break;
		case 2:		fileType = ISFFileType_Filter;		break;
		case 3:		fileType = ISFFileType_Transition;	break;
		default:	fileType = ISFFileType_None;		break;
		}
	}
	//	if there's a passed filetype, use that instead of the value displayed by the file picker
	if (inFileType != ISFFileType_None)
		fileType = inFileType;
	
	
	
	
	//	check doc window, see if contents need to be saved
	if (dw != nullptr && dw->contentsNeedToBeSaved())	{
		//	open a message box asking the user if they want to save the changes to this isf file
		QMessageBox::StandardButton		reply;
		reply = QMessageBox::question(
			this, 
			"Warning: Unsaved changes", 
			"Do you want to save your changes to this shader before proceeding?", 
			QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel);
		//	if user wants to save the contents...
		if (reply == QMessageBox::Yes)	{
			//	call DocWindow::saveOpenFile()
			if (!dw->saveOpenFile())	{
				QMessageBox::warning(GetDocWindow(), "", "There was a problem saving the file", QMessageBox::Ok);
				return;
			}
		}
		else if (reply == QMessageBox::Cancel)	{
			return;
		}
	}
	
	
	
	
	//QThread		*mainThread = QApplication::instance()->thread();
	//qDebug() << "main thread is currently " << mainThread;
	
	//	make new tmp file, populate its contents
	QString			tmpFilePath = QString();
	QFile			tmpFragShaderFile( QString("%1/ISFTesterTmpFile.fs").arg(QDir::tempPath()) );
	if (tmpFragShaderFile.open(QIODevice::WriteOnly))	{
		tmpFilePath = tmpFragShaderFile.fileName();
		//qDebug() << "\ttmpFragShaderFile's path is " << tmpFilePath;
	
		//QFile			newFileTemplate(":/resources/NewFileTemplate.txt");
		QFile			*newFileTemplate;
		switch (fileType)	{
		case ISFFileType_Source:
			newFileTemplate = new QFile(":/resources/NewGeneratorTemplate.txt");
			qDebug() << "\tshould be using file " << newFileTemplate;
			break;
		case ISFFileType_Filter:
			newFileTemplate = new QFile(":/resources/NewFilterTemplate.txt");
			break;
		case ISFFileType_Transition:
			newFileTemplate = new QFile(":/resources/NewTransitionTemplate.txt");
			break;
		default:
			newFileTemplate = new QFile(":/resources/NewFileTemplate.txt");
			break;
		}
		
		if (newFileTemplate->open(QFile::ReadOnly))	{
			QTextStream		rStream(newFileTemplate);
			QString			tmpString = rStream.readAll();
			
			QTextStream		wStream(&tmpFragShaderFile);
			wStream << tmpString;
		
			newFileTemplate->close();
		}
		delete newFileTemplate;
	
		tmpFragShaderFile.close();
	}
	//	load the temp file!
	on_loadFile(tmpFilePath);
}
void LoadingWindow::on_loadFile(const QString & n)	{
	
	//	make sure the window's visible
	if (!isVisible())
		show();
	
	//	tell the ISF controller to load the passed file- the ISF controller will tell the doc window (and myself) to load when it's ready
	ISFController		*isfc = GetISFController();
	if (isfc != nullptr)
		isfc->loadFile(n);
}
void LoadingWindow::on_saveFile()	{
	qDebug() << __PRETTY_FUNCTION__;
}




void LoadingWindow::finishedConversionDisplayFile(const QString & n)	{
	qDebug() << __PRETTY_FUNCTION__ << "... " << n;
	//on_loadFile(n);
	
	//	set the base directory- this should manually create a new data model with an updated file list
	setBaseDirectory(baseDirectory);
	//	get the filter list view's data model, figure out the index corresponding to the file path i was just passed
	QFileSystemModel		*tmpModel = qobject_cast<QFileSystemModel*>(ui->filterListView->model());
	if (tmpModel != nullptr)	{
		QModelIndex				tmpIndex = tmpModel->index(n);
		if (!tmpIndex.isValid())
			qDebug() << "\tERR: tmpIndex not valid in " << __PRETTY_FUNCTION__;
		else	{
			ui->filterListView->setCurrentIndex(tmpIndex);
		}
	}
	else
		qDebug() << "\tERR: model null in " << __PRETTY_FUNCTION__;
}




void LoadingWindow::closeEvent(QCloseEvent * event)	{
	QSettings		settings;
	settings.setValue("LoadingWindowGeometry", saveGeometry());
	
	QWidget::closeEvent(event);
}
void LoadingWindow::showEvent(QShowEvent * event)	{
	//qDebug() << __PRETTY_FUNCTION__;
	
	//Q_UNUSED(event);
	QWidget::showEvent(event);
	
	//	bump the slot to populate the pop-up button with the list of sources.
	listOfVideoSourcesUpdated(GetDynamicVideoSource());
	
	//	select a source in the combo box- let's start rendering a built-in ISF source
	ui->videoSourceComboBox->setCurrentIndex(ui->videoSourceComboBox->findText("Cellular"));
	ui->videoSourceComboBox->update();

	//	hid the 'load system ISFs' button if we're on windows ("load user ISFs" loads the "sample_ISFs" directory)
#if defined(Q_OS_WIN)
	ui->loadUserISFsButton->setVisible(false);
#endif
	
	//	figure out what directory's contents we want to display, and use it to set the base directory
	QString			defaultDirToLoad;
#ifdef Q_OS_MAC
	defaultDirToLoad = QString("~/Library/Graphics/ISF");
	//defaultDirToLoad = QString("~/Documents/VDMX5/VDMX5/supplemental resources/ISF tests+tutorials");
	defaultDirToLoad.replace("~", QDir::homePath());
#else
	//defaultDirToLoad = QDir::toNativeSeparators("C:/Users/operator/Desktop/ISF tests+tutorials");
	//QString		desktopDir = QDesktopServices::storageLocation(QDesktopServices::DesktopLocation);
	//defaultDirToLoad = QCoreApplication::applicationDirPath() + "/sample_ISFs";
	QDir			tmpDir = QDir::root();
	if (!tmpDir.cd("ProgramData"))	{
		tmpDir.mkdir("ProgramData");
		tmpDir.cd("ProgramData");
	}
	if (!tmpDir.cd("ISF"))	{
		tmpDir.mkdir("ISF");
		tmpDir.cd("ISF");
	}
	defaultDirToLoad = tmpDir.path();
#endif
	QSettings		settings;
	QVariant		lastUsedPath = settings.value("baseDir");
	if (!lastUsedPath.isNull())	{
		//qDebug() << "\tfound a path stored in the user settings! (" << lastUsedPath.toString() << ")";
		QString			tmpStr = lastUsedPath.toString();
		tmpStr.replace("~", QDir::homePath());
		if (QDir(tmpStr).exists())	{
			setBaseDirectory(tmpStr);
		}
		else	{
			//qDebug() << "\terr: the dir to load doesn't exist, falling back to default dir";
			setBaseDirectory(defaultDirToLoad);
		}
	}
	else
		setBaseDirectory(defaultDirToLoad);
	
	//	restore the window position
	//QSettings		settings;
	if (settings.contains("LoadingWindowGeometry"))	{
		restoreGeometry(settings.value("LoadingWindowGeometry").toByteArray());
	}
	else	{
		QWidget			*window = this->window();
		if (window != nullptr)	{
			QRect		winFrame = window->frameGeometry();
			QRect		contentFrame = window->geometry();
			QPoint		contentOffset = contentFrame.topLeft() - winFrame.topLeft();
			QScreen		*screen = QGuiApplication::screenAt(winFrame.center());
			if (screen != nullptr)	{
				QRect		screenFrame = screen->geometry();
				winFrame.moveTopLeft(screenFrame.topLeft());
				winFrame.translate(contentOffset.x(), contentOffset.y());
				window->setGeometry(winFrame);
			}
		}
	}
}
void LoadingWindow::appQuitEvent()	{
	//qDebug() << __PRETTY_FUNCTION__;
	
	QSettings		settings;
	settings.setValue("LoadingWindowGeometry", saveGeometry());
}




void LoadingWindow::loadUserISFsButtonClicked()	{
	qDebug() << __PRETTY_FUNCTION__;
#if defined(Q_OS_MAC)
	QString			dirToLoad("~/Library/Graphics/ISF");
#elif defined(Q_OS_LINUX)
	QString			dirToLoad("~/.cache/ISF");
#endif
	dirToLoad.replace("~", QDir::homePath());
	setBaseDirectory(dirToLoad);
}
void LoadingWindow::loadSystemISFsButtonClicked()	{
	qDebug() << __PRETTY_FUNCTION__;
#if defined(Q_OS_WIN)
	QDir			tmpDir = QDir::root();
	if (!tmpDir.cd("ProgramData"))	{
		tmpDir.mkdir("ProgramData");
		tmpDir.cd("ProgramData");
	}
	if (!tmpDir.cd("ISF"))	{
		tmpDir.mkdir("ISF");
		tmpDir.cd("ISF");
	}
	if (!tmpDir.exists())
		return;
	QString			dirToLoad(tmpDir.path());
#elif defined(Q_OS_MAC)
	QString			dirToLoad("/Library/Graphics/ISF");
#elif defined(Q_OS_LINUX)
	QDir			tmpDir = QDir::home();
	if (!tmpDir.cd(".cache"))	{
		tmpDir.mkdir(".cache");
		tmpDir.cd(".cache");
	}
	if (!tmpDir.cd("ISF"))	{
		tmpDir.mkdir("ISF");
		tmpDir.cd("ISF");
	}
	if (!tmpDir.exists())
		return;
	QString			dirToLoad(tmpDir.path());
#endif
	setBaseDirectory(dirToLoad);
	
}
void LoadingWindow::halveRenderResClicked()	{
	qDebug() << __PRETTY_FUNCTION__;
	ISFController		*isfc = GetISFController();
	DynamicVideoSource	*dvs = GetDynamicVideoSource();
	if (isfc == nullptr)
		return;
	Size			origSize = isfc->renderSize();
	Size			newSize(origSize.width/2., origSize.height/2.);
	
	ui->renderResWidthWidget->blockSignals(true);
	ui->renderResHeightWidget->blockSignals(true);
	
	ui->renderResWidthWidget->setValue(newSize.width);
	ui->renderResHeightWidget->setValue(newSize.height);
	
	ui->renderResWidthWidget->blockSignals(false);
	ui->renderResHeightWidget->blockSignals(false);
	
	ui->renderResWidthWidget->update();
	ui->renderResHeightWidget->update();
	
	isfc->setRenderSize(newSize);
	if (dvs != nullptr)
		dvs->setRenderSize(newSize);
}
void LoadingWindow::doubleRenderResClicked()	{
	qDebug() << __PRETTY_FUNCTION__;
	ISFController		*isfc = GetISFController();
	DynamicVideoSource	*dvs = GetDynamicVideoSource();
	if (isfc == nullptr)
		return;
	Size			origSize = isfc->renderSize();
	Size			newSize(origSize.width*2., origSize.height*2.);
	
	ui->renderResWidthWidget->blockSignals(true);
	ui->renderResHeightWidget->blockSignals(true);
	
	ui->renderResWidthWidget->setValue(newSize.width);
	ui->renderResHeightWidget->setValue(newSize.height);
	
	ui->renderResWidthWidget->blockSignals(false);
	ui->renderResHeightWidget->blockSignals(false);
	
	ui->renderResWidthWidget->update();
	ui->renderResHeightWidget->update();
	
	isfc->setRenderSize(newSize);
	if (dvs != nullptr)
		dvs->setRenderSize(newSize);
}
void LoadingWindow::renderResWidthWidgetValueChanged()	{
	qDebug() << __PRETTY_FUNCTION__;
	ISFController		*isfc = GetISFController();
	DynamicVideoSource	*dvs = GetDynamicVideoSource();
	
	Size			tmpSize(ui->renderResWidthWidget->value(), ui->renderResHeightWidget->value());
	if (isfc != nullptr)
		isfc->setRenderSize(tmpSize);
	if (dvs != nullptr)
		dvs->setRenderSize(tmpSize);
}
void LoadingWindow::renderResHeightWidgetValueChanged()	{
	qDebug() << __PRETTY_FUNCTION__;
	ISFController		*isfc = GetISFController();
	DynamicVideoSource	*dvs = GetDynamicVideoSource();
	
	Size			tmpSize(ui->renderResWidthWidget->value(), ui->renderResHeightWidget->value());
	if (isfc != nullptr)
		isfc->setRenderSize(tmpSize);
	if (dvs != nullptr)
		dvs->setRenderSize(tmpSize);
}
void LoadingWindow::saveUIValsToDefaultClicked()	{
	qDebug() << __PRETTY_FUNCTION__;
}
/*
void LoadingWindow::newFileSelected(const QItemSelection &selected, const QItemSelection &deselected)	{
	QList<QModelIndex>		selectedIndexes = selected.indexes();
	if (selectedIndexes.count() < 1)
		return;
	QModelIndex				firstIndex = selectedIndexes.first();
	if (!firstIndex.isValid())
		return;
	QVariant				selectedPath = firstIndex.data(QFileSystemModel::FilePathRole);
	if (selectedPath.isNull())
		return;
	//qDebug() << "\tshould be loading file " << selectedPath.toString();
	
	GetISFController()->loadFile(selectedPath.toString());
}
*/
void LoadingWindow::listOfVideoSourcesUpdated(DynamicVideoSource * inSrc)	{
	//qDebug() << __PRETTY_FUNCTION__;
	
	if (inSrc == nullptr)	{
		qDebug() << "ERR: bailing, inSrc null, " << __PRETTY_FUNCTION__;
		return;
	}
	
	//if (qApp->thread() == QThread::currentThread())
	//	qDebug() << "reloading list of sources on main thread";
	//else
	//	qDebug() << "ERR: reloading list of sources on NON-MAIN thread!";
	
	
	ui->videoSourceComboBox->blockSignals(true);
	//QObject::disconnect(ui->videoSourceComboBox, 0, 0, 0);
	
	//	clear the list of video sources
	ui->videoSourceComboBox->clear();
	
	//	get a new list of video source menu items, use them to populate the combo box
	QList<MediaFile>		newFiles = inSrc->createListOfStaticMediaFiles();
	MediaFile::Type			lastType = MediaFile::Type_None;
	//	run through every media file in the list
	for (const MediaFile & newFile : newFiles)	{
		MediaFile::Type			thisType = newFile.type();
		//	if the file type has changed, add a disabled "label" item to the menu...
		if (lastType != thisType)	{
			lastType = thisType;
			QString					tmpName("    ???");
			switch (thisType)	{
			case MediaFile::Type_App:
				tmpName = QString("    Other Apps:");
				break;
			case MediaFile::Type_Mov:
				tmpName = QString("    Movies:");
				break;
			case MediaFile::Type_Img:
				tmpName = QString("    Images:");
				break;
			case MediaFile::Type_Cam:
				tmpName = QString("    Cameras:");
				break;
			case MediaFile::Type_ISF:
				tmpName = QString("    ISFs:");
				break;
			case MediaFile::Type_None:
			default:
				tmpName = QString("    ???");
				break;
			}
			ui->videoSourceComboBox->addItem(tmpName, QVariant());
			QStandardItemModel		*model = qobject_cast<QStandardItemModel*>(ui->videoSourceComboBox->model());
			if (model != nullptr)	{
				QStandardItem			*tmpItem = model->item(model->rowCount()-1);
				if (tmpItem != nullptr)	{
					tmpItem->setFlags(tmpItem->flags() & ~Qt::ItemIsEnabled);
				}
			}
		}
		
		//	add the menu item for the file
		ui->videoSourceComboBox->addItem(newFile.name(), QVariant::fromValue(newFile));
		
		//	manually enable the items- turns out to be unnecessary, doesn't work around that weird bug where items in the pop-up button are not selectable.
		/*
		if (tmpModel == nullptr)
			tmpModel = qobject_cast<QStandardItemModel*>( ui->videoSourceComboBox->model() );
		if (tmpModel == nullptr)
			continue;
		
		int			lastItemNumber = tmpModel->rowCount() - 1;
		QStandardItem		*lastItem = tmpModel->item(lastItemNumber);
		if (lastItem == nullptr)
			continue;
		lastItem->setFlags( lastItem->flags() | Qt::ItemIsEnabled );
		*/
	}
	
	//	run through the items in the combo box, select the first item that appears to be playing back
	int				foundIndex = -1;
	if (GetDynamicVideoSource()->srcType() != MediaFile::Type_None)	{
		for (int i=0; i<ui->videoSourceComboBox->count(); ++i)	{
			QString			tmpItemString = ui->videoSourceComboBox->itemText(i);
			QVariant		tmpItemVariant = ui->videoSourceComboBox->itemData(i);
			//qDebug() << "\tcomparing against camera " << tmpInfo.description();
			//qDebug() << "\tchecking against item name " << tmpItemString << ", which is media file " << tmpItemVariant.value<MediaFile>();
			if (GetDynamicVideoSource()->playingBackItem(tmpItemVariant.value<MediaFile>()))	{
				foundIndex = i;
				break;
			}
		}
	}
	ui->videoSourceComboBox->setCurrentIndex(foundIndex);
	ui->videoSourceComboBox->blockSignals(false);
	//connect(ui->videoSourceComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &LoadingWindow::videoSourceChanged);
}
void LoadingWindow::videoSourceChanged(int arg1)	{
	//qDebug() << __PRETTY_FUNCTION__;
	Q_UNUSED(arg1);
	
	DynamicVideoSource		*dvs = GetDynamicVideoSource();
	if (dvs != nullptr)	{
		//	the combo box stores a QVariant<MediaFile> for each item
		QVariant		selectedData = ui->videoSourceComboBox->currentData();
		if (selectedData.isValid())	{
			MediaFile		selectedMediaFile = selectedData.value<MediaFile>();
			dvs->loadFile(selectedMediaFile);
		}
	}
	else
		qDebug() << "ERR: DVS null in " << __PRETTY_FUNCTION__;
}




void LoadingWindow::setBaseDirectory(const QString & inBaseDir)	{
	//qDebug() << __PRETTY_FUNCTION__ << ", " << inBaseDir;
	
	if (qApp->thread() != QThread::currentThread()) qDebug() << "ERR: thread is not main! " << __PRETTY_FUNCTION__;
	/*
	if (baseDirectory == inBaseDir)
		return;
	*/
	QDir		tmpDir(inBaseDir);
	if (!tmpDir.exists() || !tmpDir.isReadable())	{
		qDebug() << "\tERR: passed dir doesn't exist or is not readable (" << inBaseDir << "), " << __PRETTY_FUNCTION__;
		return;
	}
	
	baseDirectory = inBaseDir;
	
	
	//	store the base directory in the settings for next time (try to store the relative path)
	QSettings		settings;
	if (baseDirectory.startsWith(QDir::homePath(), Qt::CaseSensitive))	{
		QString			relativePath = baseDirectory;
		relativePath.replace(QDir::homePath(), "~");
		settings.setValue("baseDir", relativePath);
	}
	else	{
		settings.setValue("baseDir", baseDirectory);
	}
	settings.sync();
	
	
	//	get the existing directory model from the list view
	QAbstractItemModel		*oldModel = ui->filterListView->model();
	
	//	make a new model, pass it to the list view (or pass null if we couldn't make a model)
	LoadingWindowFileListModel		*newModel = new LoadingWindowFileListModel(this);
	newModel->setReadOnly(true);
	//newModel->setFilter(QDir::Files | QDir::NoSymLinks | QDir::NoDotAndDotDot | QDir::Readable);
	newModel->setNameFilters(QStringList(QString("*.fs")));
	newModel->setNameFilterDisables(false);
	QItemSelectionModel		*oldSelModel = ui->filterListView->selectionModel();
	ui->filterListView->setModel(newModel);
	if (oldSelModel != nullptr)	{
		delete oldSelModel;
		oldSelModel = nullptr;
	}
	ui->filterListView->setRootIndex(newModel->setRootPath(baseDirectory));
	
	//	delete the old model
	if (oldModel != nullptr)
		delete oldModel;
	
	//	set a new selection model
	//	we need to load a new file as the selection changes
	QItemSelectionModel		*selModel = ui->filterListView->selectionModel();
	if (selModel != nullptr)	{
		//connect(selModel, &QItemSelectionModel::selectionChanged, this, &LoadingWindow::newFileSelected);
		connect(selModel, &QItemSelectionModel::selectionChanged, [&](const QItemSelection &selected, const QItemSelection &deselected)	{
			Q_UNUSED(deselected);
			//qDebug() << __FUNCTION__ << "->&QItemSelectionModel::selectionChanged";
			QList<QModelIndex>		selectedIndexes = selected.indexes();
			if (selectedIndexes.count() < 1)
				return;
			QModelIndex		firstIndex = selectedIndexes.first();
			if (!firstIndex.isValid())
				return;
			QVariant		selectedPath = firstIndex.data(QFileSystemModel::FilePathRole);
			if (selectedPath.isNull())
				return;
			//qDebug() << "\tshould be loading file " << selectedPath.toString();
			
			QString			selectedPathString = selectedPath.toString();
			//GetISFController()->loadFile(selectedPathString);
			//GetDocWindow()->loadFile(selectedPathString);
			QString			deselectedPathString("");
			
			//	if the selected path string is already on display, bail
			ISFController		*isfc = GetISFController();
			if (isfc != nullptr)	{
				ISFDocRef			currentDoc = isfc->getCurrentDoc();
				if (currentDoc != nullptr)	{
					QString				currentDocPath(currentDoc->path().c_str());
					if (selectedPathString == currentDocPath)
						return;
					deselectedPathString = currentDocPath;
				}
			}
			
			//	check doc window, see if contents need to be saved
			DocWindow		*dw = GetDocWindow();
			if (dw != nullptr && dw->contentsNeedToBeSaved())	{
				//	open a message box asking the user if they want to save the changes to this isf file
				QMessageBox::StandardButton		reply;
				reply = QMessageBox::question(
					this, 
					"Warning: Unsaved changes", 
					"Do you want to save your changes to this shader before proceeding?", 
					QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel);
				//	if user wants to save the contents...
				if (reply == QMessageBox::Yes)	{
					//	call DocWindow::saveOpenFile()
					if (!dw->saveOpenFile())	{
						QMessageBox::warning(GetDocWindow(), "", "The file could not be saved", QMessageBox::Ok);
						//	re-select the currently-selected file
						QFileSystemModel		*tmpModel = qobject_cast<QFileSystemModel*>(ui->filterListView->model());
						if (tmpModel != nullptr)	{
							QModelIndex				tmpIndex = tmpModel->index(deselectedPathString);
							if (tmpIndex.isValid())
								ui->filterListView->setCurrentIndex(tmpIndex);
						}
						return;
					}
				}
				else if (reply == QMessageBox::Cancel)	{
					//	re-select the currently-selected file
					QFileSystemModel		*tmpModel = qobject_cast<QFileSystemModel*>(ui->filterListView->model());
					if (tmpModel != nullptr)	{
						QModelIndex				tmpIndex = tmpModel->index(deselectedPathString);
						if (tmpIndex.isValid())
							ui->filterListView->setCurrentIndex(tmpIndex);
					}
					return;
				}
			}
			
			
			on_loadFile(selectedPathString);
		});
	}
}
void LoadingWindow::selectFile(const QString & inFileToSelect)	{
	qDebug() << __PRETTY_FUNCTION__ << ", " << inFileToSelect;
	QFileInfo		fileInfo(inFileToSelect);
	//	if the file doesn't exist, bail
	if (!fileInfo.exists())
		return;
	//	if the file we were told to select is a directory...
	if (fileInfo.isDir())	{
		//	update the base directory only if necessary
		QString			newBaseDir = inFileToSelect;
		if (newBaseDir == getBaseDirectory())
			return;
		setBaseDirectory(newBaseDir);
	}
	//	else the file we were told to select isn't a directory (it's a file)
	else	{
		//	update the base directory only if necessary
		QString			newBaseDir = fileInfo.dir().path();
		if (newBaseDir != getBaseDirectory())
			setBaseDirectory(newBaseDir);
		//	get the filter list view's data model, figure out if the index corresponding to the file path
		QFileSystemModel		*tmpModel = qobject_cast<QFileSystemModel*>(ui->filterListView->model());
		if (tmpModel != nullptr)	{
			QModelIndex				tmpIndex = tmpModel->index(inFileToSelect);
			if (tmpIndex.isValid())	{
				ui->filterListView->setCurrentIndex(tmpIndex);
			}
		}
	}
}




LoadingWindow * GetLoadingWindow()	{
	return globalLoadingWindow;
}
