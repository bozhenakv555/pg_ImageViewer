#include "ImageViewer.h"

ImageViewer::ImageViewer(QWidget* parent)
	: QMainWindow(parent), ui(new Ui::ImageViewerClass)
{
	ui->setupUi(this);
	vW = new ViewerWidget(QSize(500, 500), ui->scrollArea);
	ui->scrollArea->setWidget(vW);

	ui->scrollArea->setBackgroundRole(QPalette::Dark); //pre odlisnoost pozadia od vw 
	ui->scrollArea->setWidgetResizable(false); //aby sme nemohli menit velkost
	ui->scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded); //aby tie scrollbary sa objavovali iba ak potrebujeme (obrazok velky):)
	ui->scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

	vW->setObjectName("ViewerWidget");
	vW->installEventFilter(this);

	globalColor = Qt::blue; //default farba
	QString style_sheet = QString("background-color: %1;").arg(globalColor.name(QColor::HexRgb)); 
	ui->pushButtonSetColor->setStyleSheet(style_sheet); //aby ten pbutton bol defaultnej modrej farby

	//nastavime pozadie tlacidla T0 na cervenu (default)
	ui->pbT0Color->setStyleSheet(QString("background-color: %1;").arg(colorT0.name(QColor::HexRgb)));

	//nastavime pozadie tlacidla T1 na zelenu (default)
	ui->pbT1Color->setStyleSheet(QString("background-color: %1;").arg(colorT1.name(QColor::HexRgb)));

	//nastavime pozadie tlacidla T2 na modru (default)
	ui->pbT2Color->setStyleSheet(QString("background-color: %1;").arg(colorT2.name(QColor::HexRgb)));
}

// Event filters
//tu zachytavame vsetko, co sa deje v aplikacii
bool ImageViewer::eventFilter(QObject* obj, QEvent* event) //"kazda jedna blbost v Qt je zdedena s QObject:)"
{
	if (obj->objectName() == "ViewerWidget") { //chceme, aby iventy boli spracovane prave nad tym platnom vw pomocou ViewerWidgetEventFilter, tak kontrolujeme meno objektu - ci ta udalost sa stala prave na nom
		return ViewerWidgetEventFilter(obj, event); //ak ano, posleme to do nasho specialneho filtra pre platno
	}
	return QMainWindow::eventFilter(obj, event); //ak obj nie vw - default ivent
}

//ViewerWidget Events
bool ImageViewer::ViewerWidgetEventFilter(QObject* obj, QEvent* event) //tu triedime udalosti nad platnom podla typu - klik, pohyb, koliesko...
{
	ViewerWidget* w = static_cast<ViewerWidget*>(obj); //pretypovanie smernika na parent triedu na smernik na child triedu - QObject na ViewerWidget

	if (!w) {
		return false;
	}

	//podla toho co sa stalo, zavolame konkretnu funkciu
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
	//pretypovanie vseobecneho QEvent na QMouseEvent
	QMouseEvent* e = static_cast<QMouseEvent*>(event); //aby sme mohli ziskavat info o kliku (akym tlacidlom, suradnice(pos))
	
	//ak user klikne mimo bieleho platna, ignorujeme to
	if (!w->isInside(e->pos().x(), e->pos().y())) {
		return;
	}

	if (e->button() == Qt::LeftButton) {
		w->setStartMousePos(e->pos()); //ulozime pociatocnu polohu mysi pre vypocet posunu (SHIFT)
	}

	//LINE 
	if (ui->toolButtonDrawLine->isChecked())
	{
		if (e->button() == Qt::LeftButton) {
			if (w->getDrawLineActivated()) { //uz je aktivovane kreslenie - mame uz zaciatocny bod v pamati -> kreslime usecku
				w->drawLine(w->getDrawLineBegin(), e->pos(), globalColor, ui->comboBoxLineAlg->currentIndex());

				w->clearPolygonPoints(); //vymazeme predchadzajuci objekt (podla zadania len jeden objekt naraz)
				w->addPolygonPoint(w->getDrawLineBegin());
				w->addPolygonPoint(e->pos()); //ulozime body usecky do zoznamu bodov (ako 2-bodovy polygon)
				w->setPolygonClosed(false); //usecka nie je uzavrety tvar

				w->setDrawLineActivated(false);
			}
			else { //lineActivated vratilo false - usecka sa prave zacina
				w->clear(); //+!
				w->clearPolygonPoints();

				w->setDrawLineBegin(e->pos()); //ulozime pociatocny bod usecky - volame funkciu vw a tam posielame objekt typu QPoint (uchovava int suradnice, ale QPointF float suradnice)
				w->setDrawLineActivated(true);
				w->setPixel(e->pos().x(), e->pos().y(), globalColor);
				w->update(); //sa vyvola paintEvent - prekresli obrazok
			}
		}
	}
	//CIRCLE 
	else if (ui->toolButtonDrawCircle->isChecked())
	{
		if (e->button() == Qt::LeftButton) {
			if (w->getDrawCircleActivated()) {
				w->drawCircleBresenham(w->getDrawCircleBegin(), e->pos(), globalColor);
				w->setDrawCircleActivated(false);
			}
			else {
				w->clear(); //+!
				w->clearPolygonPoints();

				w->setDrawCircleBegin(e->pos());
				w->setDrawCircleActivated(true);
				w->setPixel(e->pos().x(), e->pos().y(), globalColor);
				w->update();
			}
		}
	}
	//POLYGON
	else if (ui->toolButtonDrawPolygon->isChecked())
	{
		if (e->button() == Qt::LeftButton) {
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
		else if (e->button() == Qt::RightButton) {
			if (w->getdrawPolygonActivated() && w->getPolygonPoints().size() > 2) {
				w->setPolygonClosed(true); //povieme vw, ze ma spojit posledny bod s prvym

				w->clear();
				w->drawPolygon(globalColor);
				w->setDrawPolygonActivated(false);
				w->update();
			}
		}
	}
	//CURVE - hermite
	else if (ui->toolButtonHermite->isChecked()) {
		//lavym tlacidlom len pridavame body
		if (e->button() == Qt::LeftButton) {

			if (hermiteVectorAngles.empty()) {
				w->clearPolygonPoints();
			}

			w->addHermitePoint(e->pos());
			hermiteVectorAngles.push_back(0.0); //nainizializujeme pre bod uhol

			//vykreslime bod
			w->setPixel(e->pos().x(), e->pos().y(), globalColor);
			w->update();

			//aktualizujeme spinbox pre indexy
			ui->sbPointIndex->setMaximum(w->getHermitePoints().size());
		}
		//prave tlacidlo - vykreslime samotnu krivku
		else if (e->button() == Qt::RightButton) {
			if (w->getHermitePoints().size() >= 2) {
				w->clear();
				w->drawHermiteCurve(hermiteVectorAngles, hermiteVectorLength, globalColor);
				w->update();
			}
		}
	}
	else if (ui->toolButtonBezier->isChecked()) {
		//lavym tlacidlom len pridavame body
		if (e->button() == Qt::LeftButton) {

			w->addBezierPoint(e->pos());

			//vykreslime bod
			w->setPixel(e->pos().x(), e->pos().y(), globalColor);
			w->update();
		}
		//prave tlacidlo - vykreslime samotnu krivku
		else if (e->button() == Qt::RightButton) {
			if (w->getBezierPoints().size() >= 2) {
				w->clear();
				w->drawBezierCurve(globalColor);
				w->update();
			}
		}
	}
	else if (ui->toolButtonBSpline->isChecked()) {
		//lavym tlacidlom len pridavame body
		if (e->button() == Qt::LeftButton) {

			w->addBSplinePoint(e->pos());

			//vykreslime bod
			w->setPixel(e->pos().x(), e->pos().y(), globalColor);
			w->update();
		}
		//prave tlacidlo - vykreslime samotnu krivku
		else if (e->button() == Qt::RightButton) {
			if (w->getBSplinePoints().size() >= 4) {
				w->clear();
				w->drawBSplineCurve(globalColor);
				w->update();
			}
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
	if ((e->buttons() == Qt::LeftButton) && !ui->toolButtonDrawLine->isChecked() && !ui->toolButtonDrawPolygon->isChecked() && !ui->toolButtonDrawCircle->isChecked() && !ui->toolButtonHermite->isChecked()) {
		QPoint delta = e->pos() - w->getStartMousePos(); //smer a vzdialenost pohybu mysi
		std::vector<QPoint> points = w->getPolygonPoints();

		if (points.empty()) return;

		for (QPoint& point : points) {
			point += delta;
		}

		w->setPolygonPoints(points); //ulozime transformovane body spat

		if (points.size() == 3) {
			w->setTriangleVertices({ points[0], colorT0 }, { points[1], colorT1 }, { points[2], colorT2 });
		}

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
	if (original.empty()) return;
	std::vector<QPoint> scaled = w->scale(original, factor, factor);
	w->setPolygonPoints(scaled);

	if (scaled.size() == 3) {
		w->setTriangleVertices({ scaled[0], colorT0 }, { scaled[1], colorT1 }, { scaled[2], colorT2 });
	}

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
	vW->clearAll();
	hermiteVectorAngles.clear(); //vymazeme aj uhly v ImageVieweri
	ui->sbPointIndex->setMaximum(0); //resetneme UI prvok
	currentPointIndex = -1;
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

	if (scaled.size() == 3) {
		vW->setTriangleVertices({ scaled[0], colorT0 }, { scaled[1], colorT1 }, { scaled[2], colorT2 });
	}

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

	if (sheared.size() == 3) {
		vW->setTriangleVertices({ sheared[0], colorT0 }, { sheared[1], colorT1 }, { sheared[2], colorT2 });
	}

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

	if (rotated.size() == 3) {
		vW->setTriangleVertices({ rotated[0], colorT0 }, { rotated[1], colorT1 }, { rotated[2], colorT2 });
	}

	vW->setPolygonPoints(rotated);
	vW->clear();
	vW->drawPolygon(globalColor);
	vW->update();
}

void ImageViewer::on_tbSymmetry_clicked()
{
	std::vector<QPoint> original = vW->getPolygonPoints();
	if (original.size() < 2) return;

	QPoint A, B;

	if (original.size() == 2) {
		//ak pracujeme s useckou - zvolime horizontalnu os prechadzajucu prvym bodom
		A = original[0]; //prvy bod
		B = QPoint(original[0].x() + 10, original[0].y()); //bod vytvoreny posunutim prveho vpravo o 10 pixelov
	}
	else {
		//ak s polygonom - os lezi na prvej hrane
		A = original[0];
		B = original[1];
	}
	std::vector<QPoint> reflected = vW->reflect(original, A, B);
	vW->setPolygonPoints(reflected);

	if (reflected.size() == 3) {
		//ak je to trojuholnik, musime mu prepisat vrcholy, nech sa farby a vypln nezaseknu na starom mieste
		vW->setTriangleVertices({ reflected[0], colorT0 }, { reflected[1], colorT1 }, { reflected[2], colorT2 });
	}

	vW->clear();
	vW->drawPolygon(globalColor);
	vW->update();
}

void ImageViewer::on_tbFill_clicked() {
	std::vector<QPoint> points = vW->getPolygonPoints();

	bool isChecked = ui->tbFill->isChecked();
	int fillType = ui->cbFillType->currentIndex();

	vW->setFillEnabled(isChecked);
	vW->setFillType(fillType);

	if (points.size() == 3) {
	
		Vertex t0 = { points[0], colorT0 };
		Vertex t1 = { points[1], colorT1 };
		Vertex t2 = { points[2], colorT2 };
		vW->setTriangleVertices(t0, t1, t2); 
	}

	vW->clear();
	vW->drawPolygon(globalColor); 
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

void ImageViewer::on_sbPointIndex_valueChanged(int i) {
	currentPointIndex = i - 1; // -1, lebo v UI zaciname od 1, ale v vector od 0

	//ak mame vybraty validny index, vytiahneme uhol z pola a nastavime ho do UI
	if (currentPointIndex >= 0 && currentPointIndex < hermiteVectorAngles.size()) {
		double angleDegrees = hermiteVectorAngles[currentPointIndex] * (180.0 / M_PI); //prevod z radianov na stupne pre UI spinbox
		ui->dsbVectorAngle->setValue(angleDegrees);
	}
}


void ImageViewer::on_dsbVectorAngle_valueChanged(double value) {
	//ak mame vybraty nejaky bod, prepiseme mu uhol v poli a prekreslime
	if (currentPointIndex >= 0 && currentPointIndex < hermiteVectorAngles.size()) {
		hermiteVectorAngles[currentPointIndex] = value * (M_PI / 180.0); //prevod spat na radiany pre vypocty
		vW->clear(); //zmazeme stare
		vW->drawHermiteCurve(hermiteVectorAngles, hermiteVectorLength, globalColor); //vykreslime s novym uhlom
		vW->update();
	}
}

void ImageViewer::on_dsbVectorLength_valueChanged(double value) {
	hermiteVectorLength = value; //zmenime globalnu dlzku vektorov
	vW->clear();
	vW->drawHermiteCurve(hermiteVectorAngles, hermiteVectorLength, globalColor); //prekreslime celu krivku s novou dlzkou
	vW->update();
}

//3D BUDE CELE DOLE, LEBO SA STRACAM))

void ImageViewer::on_tbCreateCube_clicked()
{
	double size = ui->dsbCubeSize->value();
	if (size <= 0) {
		QMessageBox::warning(this, "Error", "Enter a valid parameter");
		return;
	}
	model3D.createCube(size);
	QMessageBox::information(this, "Cube", "Cube was created successfully!");
	render3D();
}

void ImageViewer::on_tbCreateUVSphere_clicked()
{
	int P = ui->sbParallels->value();
	int M = ui->sbMeridians->value();
	double radius = ui->dsbRadius->value();
	if (P < 3 || M < 3 || radius <= 0) {
		QMessageBox::warning(this, "Error", "Enter valid parameters");
		return;
	}
	model3D.createUVSphere(P, M, radius);
	QMessageBox::information(this, "Sphere", "UV Sphere was created successfully!");
	render3D();
}


void ImageViewer::render3D()
{
	vW->clear();
	double theta = ui->sliderZenith_theta->value() * (M_PI / 180.0);
	double phi = ui->sliderAzimuth_phi->value() * (M_PI / 180.0);
	double dz = ui->dsb_distance->value();
	double R = ui->dsb_cameraDistance->value();
	int projectionType = ui->cb_projectionType->currentIndex();
	int representationType = ui->chbFill3D->isChecked() ? 1 : 0;

	vW->draw3DModel(model3D, phi, theta, projectionType, representationType, dz, R);

	vW->update();
}

void ImageViewer::on_actionSave_3D_to_VTK_triggered()
{
	QString fileName = QFileDialog::getSaveFileName(this, "Save 3D model", "", "VTK files (*.vtk)");
	if (fileName.isEmpty()) return;

	if (model3D.saveToVTK(fileName)) {
		QMessageBox::information(this, "Success!", "VTK file was saved!");
	}
	else {
		QMessageBox::critical(this, "Error!", "VTK file was not saved!");
	}
}

void ImageViewer::on_actionLoad_3D_from_VTK_triggered()
{
	QString fileName = QFileDialog::getOpenFileName(this,
		tr("Open VTK Model"), "", tr("VTK Files (*.vtk);;All Files (*)"));

	if (fileName.isEmpty()) {
		return;
	}

	if (model3D.loadFromVTK(fileName)) {
		QMessageBox::information(this, "Success!", "VTK file was loaded!");
		render3D();

	}
	else {
		QMessageBox::critical(this, "Error!", "Could not load VTK file");
	}

}

void ImageViewer::on_sliderZenith_theta_valueChanged(int value)
{
	render3D();
}

void ImageViewer::on_sliderAzimuth_phi_valueChanged(int value)
{
	render3D();
}

void ImageViewer::on_dsb_distance_valueChanged(double value)
{
	render3D();
}


