#include "ImageViewer.h"

ImageViewer::ImageViewer(QWidget* parent)
	: QMainWindow(parent), ui(new Ui::ImageViewerClass)
{
	ui->setupUi(this);
	vW = new ViewerWidget(QSize(500, 500), ui->scrollArea);
	ui->scrollArea->setWidget(vW);

	ui->scrollArea->setBackgroundRole(QPalette::Dark); //pre odlisnoost pozadia od vw asi
	ui->scrollArea->setWidgetResizable(false); //aby sme nemohli menit velkost
	ui->scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded); //aby tie scrollbary sa objavovali iba sk potrebujeme (obrazok vrlky) asi:)
	ui->scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

	vW->setObjectName("ViewerWidget");
	vW->installEventFilter(this);

	globalColor = Qt::blue; //default farba
	QString style_sheet = QString("background-color: %1;").arg(globalColor.name(QColor::HexRgb));
	ui->pushButtonSetColor->setStyleSheet(style_sheet);
}

// Event filters
bool ImageViewer::eventFilter(QObject* obj, QEvent* event) //kazda jedna blbost v Qt je zdedena s QObject:)
{
	if (obj->objectName() == "ViewerWidget") { //chceme, aby iventy boli spracovane prave nad tym platnom vw pomocou ViewerWidgetEventFilter, tak kontrolujeme menu objektu
		return ViewerWidgetEventFilter(obj, event);
	}
	return QMainWindow::eventFilter(obj, event); //ak obj nie vw - default ivent
}

//ViewerWidget Events
bool ImageViewer::ViewerWidgetEventFilter(QObject* obj, QEvent* event)
{
	ViewerWidget* w = static_cast<ViewerWidget*>(obj); //pretypovanie smernika na parent triedu na smernik na child triedu +NAUC VIAC

	if (!w) {
		return false;
	}

	if (event->type() == QEvent::MouseButtonPress) {
		ViewerWidgetMouseButtonPress(w, event);
	}
	else if (event->type() == QEvent::MouseButtonRelease) {
		ViewerWidgetMouseButtonRelease(w, event);
	}
	else if (event->type() == QEvent::MouseMove) {
		ViewerWidgetMouseMove(w, event);
	}
	else if (event->type() == QEvent::Leave) {
		ViewerWidgetLeave(w, event);
	}
	else if (event->type() == QEvent::Enter) {
		ViewerWidgetEnter(w, event);
	}
	else if (event->type() == QEvent::Wheel) {
		ViewerWidgetWheel(w, event);
	}
	return QObject::eventFilter(obj, event);
}

void ImageViewer::ViewerWidgetMouseButtonPress(ViewerWidget* w, QEvent* event)
{
	QMouseEvent* e = static_cast<QMouseEvent*>(event); //pretypovanie vseobecneho QEvent na QMouseEvent

	if (e->button() == Qt::LeftButton) {
		w->setStartMousePos(e->pos()); //ulozime pociatocnu polohu mysi pre vypocet posunu (SHIFT)
	}

	//LINE
	if (e->button() == Qt::LeftButton && ui->toolButtonDrawLine->isChecked())
	{
		if (w->getDrawLineActivated()) { //ciaru kreslime iba ak mame aktivovane jej kreslenie
			w->drawLine(w->getDrawLineBegin(), e->pos(), globalColor, ui->comboBoxLineAlg->currentIndex());
			
			w->clearPolygonPoints(); //vymazeme predchadzajuci objekt (podla zadania len jeden objekt naraz)
			w->addPolygonPoint(w->getDrawLineBegin());
			w->addPolygonPoint(e->pos()); //ulozime body usecky do zoznamu bodov (ako 2-bodovy polygon)
			w->setPolygonClosed(false); //usecka nie je uzavrety tvar
			
			w->setDrawLineActivated(false);
		}
		else {
			w->clear(); // +!
			w->clearPolygonPoints(); 

			w->setDrawLineBegin(e->pos()); //volame funkciu vw a tam posielame objekt typu QPoint (uchovava int suradnice, ale QPointF float suradnice)
			w->setDrawLineActivated(true);
			w->setPixel(e->pos().x(), e->pos().y(), globalColor);
			w->update(); //sa vyvola paintEvent - prekresli obrazok
		}
	}
	//CIRCLE
	if (e->button() == Qt::LeftButton && ui->toolButtonDrawCircle->isChecked())
	{
		if (w->getDrawCircleActivated()) {
			w->drawCircleBresenham(w->getDrawCircleBegin(), e->pos(), globalColor);
			w->setDrawCircleActivated(false);
		}
		else {
			w->clear(); // +!
			w->clearPolygonPoints();

			w->setDrawCircleBegin(e->pos()); 
			w->setDrawCircleActivated(true);
			w->setPixel(e->pos().x(), e->pos().y(), globalColor);
			w->update(); 
		}
	}
	//POLYGON
	if (e->button() == Qt::LeftButton && ui->toolButtonDrawPolygon->isChecked())
	{
		if (w->getdrawPolygonActivated()) { //klikame uz nie prvykrat
			w->setPolygonClosed(false);
			w->addPolygonPoint(e->pos()); 

			w->clear();
			w->drawPolygon(globalColor); //vykresli vsetko, co je v polygonPoints
			w->update();
		}
		else {
			w->clear();
			w->clearPolygonPoints(); 
			w->setDrawPolygonActivated(true); 
			w->setPolygonClosed(false); 
			w->setPixel(e->pos().x(), e->pos().y(), globalColor);
			w->addPolygonPoint(e->pos());
			w->update();
		}
	}
	if (e->button() == Qt::RightButton && ui->toolButtonDrawPolygon->isChecked()) {
		if (w->getdrawPolygonActivated() && w->getPolygonPoints().size()>2) {
			w->setPolygonClosed(true);

			w->clear();
			w->drawPolygon(globalColor); 
			w->setDrawPolygonActivated(false);
			w->update();
		}
	}
}
void ImageViewer::ViewerWidgetMouseButtonRelease(ViewerWidget* w, QEvent* event)
{
	QMouseEvent* e = static_cast<QMouseEvent*>(event);
}
void ImageViewer::ViewerWidgetMouseMove(ViewerWidget* w, QEvent* event)
{
	QMouseEvent* e = static_cast<QMouseEvent*>(event);
	//SHIFT
	if ((e->buttons() == Qt::LeftButton) && !ui->toolButtonDrawLine->isChecked() && !ui->toolButtonDrawPolygon->isChecked() && !ui->toolButtonDrawCircle->isChecked()) {
		QPoint delta = e->pos() - w->getStartMousePos();
		std::vector<QPoint> points = w->getPolygonPoints();

		for (QPoint& point : points) {
			point += delta;
		}

		w->setPolygonPoints(points); //ulozime transformovane body spat
		w->setStartMousePos(e->pos()); //vktualizujeme polohu mysi pre plynuly pohyb v dalsom kroku

		w->clear(); //vymazeme stare platno
		w->drawPolygon(globalColor); //vykreslime objekt na novej pozicii
		w->update(); //prekreslime widget
	}

}
void ImageViewer::ViewerWidgetLeave(ViewerWidget* w, QEvent* event)
{
}
void ImageViewer::ViewerWidgetEnter(ViewerWidget* w, QEvent* event)
{
}
void ImageViewer::ViewerWidgetWheel(ViewerWidget* w, QEvent* event)
{
	QWheelEvent* e = static_cast<QWheelEvent*>(event);

	double factor = (e->angleDelta().y() > 0) ? 1.25 : 0.75; //ak je y-ova (koliesko hore-dole iba) suradnica angleDelta (vector otocenia kolieska) kladna, scrollovali sme hore (faktor 1.25), inak sme scrollovali dole (faktor 0.75)

	std::vector<QPoint> original = w->getPolygonPoints();
	std::vector<QPoint> scaled = w->scale(original, factor, factor);
	w->setPolygonPoints(scaled);

	w->clear();
	w->drawPolygon(globalColor); 
	w->update();
}

//ImageViewer Events
void ImageViewer::closeEvent(QCloseEvent* event) //iba na konci dava otazku, ci naozaj chceme zavriet aplikaciu
{
	if (QMessageBox::Yes == QMessageBox::question(this, "Close Confirmation", "Are you sure you want to exit?", QMessageBox::Yes | QMessageBox::No))
	{
		event->accept();
	}
	else {
		event->ignore();
	}
}

//Image functions
bool ImageViewer::openImage(QString filename)
{
	QImage loadedImg(filename);
	if (!loadedImg.isNull()) {
		return vW->setImage(loadedImg);
	}
	return false;
}
bool ImageViewer::saveImage(QString filename)
{
	QFileInfo fi(filename);
	QString extension = fi.completeSuffix();

	QImage* img = vW->getImage();
	return img->save(filename, extension.toStdString().c_str());
}

//Slots
void ImageViewer::on_actionOpen_triggered() //NAUC VIAC! regedit do startu -> registory editor -> v MPM v ImageViewer (toto je nastavene v maine) je folder_img_load_path - kluc, ak nenajedeny, folder je prazdny
{
	QString folder = settings.value("folder_img_load_path", "").toString();

	QString fileFilter = "Image data (*.bmp *.gif *.jpg *.jpeg *.png *.pbm *.pgm *.ppm *.xbm *.xpm);;All files (*)";
	QString fileName = QFileDialog::getOpenFileName(this, "Load image", folder, fileFilter);
	if (fileName.isEmpty()) { return; }

	QFileInfo fi(fileName);
	settings.setValue("folder_img_load_path", fi.absoluteDir().absolutePath()); //ked sme vybrali subor, nastavime cestu k registrom

	if (!openImage(fileName)) {
		msgBox.setText("Unable to open image.");
		msgBox.setIcon(QMessageBox::Warning);
		msgBox.exec();
	}
}
void ImageViewer::on_actionSave_as_triggered()
{
	QString folder = settings.value("folder_img_save_path", "").toString();

	QString fileFilter = "Image data (*.bmp *.gif *.jpg *.jpeg *.png *.pbm *.pgm *.ppm *.xbm *.xpm);;All files (*)";
	QString fileName = QFileDialog::getSaveFileName(this, "Save image", folder, fileFilter);
	if (!fileName.isEmpty()) {
		QFileInfo fi(fileName);
		settings.setValue("folder_img_save_path", fi.absoluteDir().absolutePath());

		if (!saveImage(fileName)) {
			msgBox.setText("Unable to save image.");
			msgBox.setIcon(QMessageBox::Warning);
		}
		else {
			msgBox.setText(QString("File %1 saved.").arg(fileName));
			msgBox.setIcon(QMessageBox::Information);
		}
		msgBox.exec();
	}
}
void ImageViewer::on_actionClear_triggered()
{
	vW->clear();
}
void ImageViewer::on_actionExit_triggered()
{
	this->close();
}

void ImageViewer::on_pushButtonSetColor_clicked()
{
	QColor newColor = QColorDialog::getColor(globalColor, this); //funkcia QColorDialog, na vstup dostane povodnu, vrati novu
	if (newColor.isValid()) {
		QString style_sheet = QString("background-color: %1;").arg(newColor.name(QColor::HexRgb));
		ui->pushButtonSetColor->setStyleSheet(style_sheet);
		globalColor = newColor;
	}
}

void ImageViewer::on_tbScale_clicked() 
{
	double factorx = ui->dspinbox_x->value();
	double factory = ui->dspinbox_y->value();

	std::vector<QPoint> original = vW->getPolygonPoints();
	std::vector<QPoint> scaled = vW->scale(original, factorx, factory);
	vW->setPolygonPoints(scaled);

	vW->clear();
	vW->drawPolygon(globalColor);
	vW->update();
}

void ImageViewer::on_tbShear_clicked()
{
	double koefx = ui->dspb_koefx->value();
	std::vector<QPoint> original = vW->getPolygonPoints();
	std::vector<QPoint> sheared = vW->shear(original, koefx);
	vW->setPolygonPoints(sheared);

	vW->clear();
	vW->drawPolygon(globalColor);
	vW->update();
}

void ImageViewer::on_tbRotate_clicked() // Nazov podla tvojho ToolButtonu
{
	//ziskame uhol zo SpinBoxu (ktory ma rozsah -360 az 360)
	double angle = ui->dspb_angle->value();

	//ziskame aktualne body (usecku alebo polygon)
	std::vector<QPoint> original = vW->getPolygonPoints();
	//vypocitame rotaciu
	std::vector<QPoint> rotated = vW->rotate(original, angle);

	vW->setPolygonPoints(rotated);
	vW->clear();
	vW->drawPolygon(globalColor);
	vW->update();
}

void ImageViewer::on_tbSymmetry_clicked()
{
	std::vector<QPoint> original = vW->getPolygonPoints();
	QPoint A = original[0];
	QPoint B = original[1];

	std::vector<QPoint> reflected = vW->reflect(original, A, B);
	vW->setPolygonPoints(reflected);

	vW->clear();
	vW->drawPolygon(globalColor);
	vW->update();
}

void ImageViewer::on_tbFill_clicked() {
	std::vector<QPoint> points = vW->getPolygonPoints();

	if (points.size() == 3) {
		Vertex t0 = { points[0], colorT0 };
		Vertex t1 = { points[1], colorT1 };
		Vertex t2 = { points[2], colorT2 };

		int fillType = ui->cbFillType->currentIndex();

		vW->fillTriangle(t0, t1, t2, fillType);
	}
	else if (points.size() > 3) {
		vW->fillScanLine(points, globalColor);
	}

	vW->update();
}


void ImageViewer::on_pbT0Color_clicked()
{
	QColor newColor = QColorDialog::getColor(colorT0, this);

	if (newColor.isValid()) {
		colorT0 = newColor;

		QString style = QString("background-color: %1;")
			.arg(colorT0.name(QColor::HexRgb));
		ui->pbT0Color->setStyleSheet(style);
	}
}

void ImageViewer::on_pbT1Color_clicked()
{
	QColor newColor = QColorDialog::getColor(colorT1, this);

	if (newColor.isValid()) {
		colorT1 = newColor;

		QString style = QString("background-color: %1;")
			.arg(colorT1.name(QColor::HexRgb));
		ui->pbT1Color->setStyleSheet(style);
	}
}

void ImageViewer::on_pbT2Color_clicked()
{
	QColor newColor = QColorDialog::getColor(colorT2, this);

	if (newColor.isValid()) {
		colorT2 = newColor;

		QString style = QString("background-color: %1;")
			.arg(colorT2.name(QColor::HexRgb));
		ui->pbT2Color->setStyleSheet(style);
	}
}
