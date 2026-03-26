#pragma once

#include <QtWidgets/QMainWindow>
#include <QtWidgets>
#include "ui_ImageViewer.h"
#include "ViewerWidget.h"

class ImageViewer : public QMainWindow
{
	Q_OBJECT

public:
	ImageViewer(QWidget* parent = Q_NULLPTR);
	~ImageViewer() { delete ui; }
private:
	Ui::ImageViewerClass* ui;
	ViewerWidget* vW; //pre kazdy jeden ImageViewer sa dynamicky alokuje jeden ViewerWidget

	QColor globalColor; //sa vyberie z QColorDialog
	QSettings settings;
	QMessageBox msgBox;

	QColor colorT0 = Qt::red;
	QColor colorT1 = Qt::green;
	QColor colorT2 = Qt::blue;

	//Event filters
	bool eventFilter(QObject* obj, QEvent* event);

	//ViewerWidget Events
	bool ViewerWidgetEventFilter(QObject* obj, QEvent* event);
	void ViewerWidgetMouseButtonPress(ViewerWidget* w, QEvent* event);
	void ViewerWidgetMouseButtonRelease(ViewerWidget* w, QEvent* event);
	void ViewerWidgetMouseMove(ViewerWidget* w, QEvent* event);
	void ViewerWidgetLeave(ViewerWidget* w, QEvent* event);
	void ViewerWidgetEnter(ViewerWidget* w, QEvent* event);
	void ViewerWidgetWheel(ViewerWidget* w, QEvent* event);

	//ImageViewer Events
	void closeEvent(QCloseEvent* event);

	//Image functions
	bool openImage(QString filename);
	bool saveImage(QString filename);

private slots:
	void on_actionOpen_triggered();
	void on_actionSave_as_triggered();
	void on_actionClear_triggered();
	void on_actionExit_triggered();

	//Tools slots
	void on_pushButtonSetColor_clicked();
public slots:
	void on_tbScale_clicked();
	void on_tbShear_clicked();
	void on_tbRotate_clicked();
	void on_tbSymmetry_clicked();
	void on_tbFill_clicked();
	void on_pbT0Color_clicked();
	void on_pbT1Color_clicked();
	void on_pbT2Color_clicked();
};
