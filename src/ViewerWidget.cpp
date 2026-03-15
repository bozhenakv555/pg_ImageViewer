#include   "ViewerWidget.h"

ViewerWidget::ViewerWidget(QSize imgSize, QWidget* parent)
	: QWidget(parent)
{
	setAttribute(Qt::WA_StaticContents);
	setMouseTracking(true);
	if (imgSize != QSize(0, 0)) { //kontrolujeme, ci ma nejaku velkost
		img = new QImage(imgSize, QImage::Format_ARGB32); //ak nie, alokujeme pamat na novy obrazok (dynam)
		img->fill(Qt::white);
		resizeWidget(img->size());
		setDataPtr();
	}
}
ViewerWidget::~ViewerWidget()
{
	delete img;
	img = nullptr;
	data = nullptr;
}
void ViewerWidget::resizeWidget(QSize size)
{
	this->resize(size);
	this->setMinimumSize(size);
	this->setMaximumSize(size);
	this->setFixedSize(size);
}

//Image functions
bool ViewerWidget::setImage(const QImage& inputImg)
{
	if (img) {
		delete img;
		img = nullptr;
		data = nullptr;
	}
	img = new QImage(inputImg.convertToFormat(QImage::Format_ARGB32));
	if (!img || img->isNull()) {
		return false;
	}
	resizeWidget(img->size());
	setDataPtr();
	update();

	return true;
}
bool ViewerWidget::isEmpty() //ci obrazok existuje, ci ma nulovu velkost
{
	if (img == nullptr) {
		return true;
	}

	if (img->size() == QSize(0, 0)) {
		return true;
	}
	return false;
}

bool ViewerWidget::changeSize(int width, int height)
{
	QSize newSize(width, height);

	if (newSize != QSize(0, 0)) {
		if (img != nullptr) {
			delete img;
		}

		img = new QImage(newSize, QImage::Format_ARGB32);
		if (!img || img->isNull()) {
			return false;
		}
		img->fill(Qt::white);
		resizeWidget(img->size());
		setDataPtr();
		update();
	}

	return true;
}

void ViewerWidget::setPixel(int x, int y, int r, int g, int b, int a)
{
	if (!img || !data) return;
	if (!isInside(x, y)) return;

	r = r > 255 ? 255 : (r < 0 ? 0 : r); //kontrolujeme ci hodnoty nie su pod 0 a nad 255 - validne, ak nie - riezme
	g = g > 255 ? 255 : (g < 0 ? 0 : g);
	b = b > 255 ? 255 : (b < 0 ? 0 : b);
	a = a > 255 ? 255 : (a < 0 ? 0 : a);

	size_t startbyte = y * img->bytesPerLine() + x * 4; //kazda stvorica bajtov prislucha jednemu pixelu - zapisujeme udaje do obrazku priamo ako 4*bajt
	data[startbyte] = static_cast<uchar>(b); //zapisujemem v opacnom poradi - bgr
	data[startbyte + 1] = static_cast<uchar>(g);
	data[startbyte + 2] = static_cast<uchar>(r);
	data[startbyte + 3] = static_cast<uchar>(a);
}
void ViewerWidget::setPixel(int x, int y, double valR, double valG, double valB, double valA)
{
	valR = valR > 1 ? 1 : (valR < 0 ? 0 : valR); //orezavame, musi byt >0 a <1
	valG = valG > 1 ? 1 : (valG < 0 ? 0 : valG);
	valB = valB > 1 ? 1 : (valB < 0 ? 0 : valB);
	valA = valA > 1 ? 1 : (valA < 0 ? 0 : valA);

	setPixel(x, y, static_cast<int>(255 * valR + 0.5), static_cast<int>(255 * valG + 0.5), static_cast<int>(255 * valB + 0.5), static_cast<int>(255 * valA + 0.5));
}
void ViewerWidget::setPixel(int x, int y, const QColor& color) //na nejake miesto v obrazku (x,y) dame pixel danej farby
{
	if (color.isValid()) { //ak je validna
		setPixel(x, y, color.red(), color.green(), color.blue(), color.alpha()); //zavolame horny setPixel pre (x,y) a RGB hodnoty tejto farby
	}
}

bool ViewerWidget::isInside(int x, int y)
{
	return img && x >= 0 && y >= 0 && x < img->width() && y < img->height();
}

//Draw functions
void ViewerWidget::drawLine(QPoint start, QPoint end, QColor color, int algType)
{
	if (!img || !data) return;

	if (algType == 0) {
		drawLineDDA(start, end, color);
	}
	else {
		drawLineBresenham(start, end, color);
	}
	update();

	//Po implementovani drawLineDDA a drawLineBresenham treba vymazat
	/*QPainter painter(img);
	painter.setPen(QPen(color));
	painter.drawLine(start, end);
	update();*/
}

void ViewerWidget::clear()
{
	if (!img) return;
	img->fill(Qt::white);
	update();
}

void ViewerWidget::drawLineDDA(QPoint start, QPoint end, QColor color)
{
	double x1 = start.x();
	double y1 = start.y();
	double x2 = end.x();
	double y2 = end.y();

	double dx = x2 - x1;
	double dy = y2 - y1;

	double maxDif = std::max(std::abs(dx), std::abs(dy)); //rozmer riadiacej osi - ak y:dy, ak x:dx

	if (maxDif == 0) {
		setPixel((int)(x1 + 0.5), (int)(y1 + 0.5), color);
		return;
	}

	double x_inc = dx / maxDif;
	double y_inc = dy / maxDif;

	for (int i = 0; i <= maxDif; i++) {
		setPixel((int)(x1 + 0.5), (int)(y1 + 0.5), color);
		x1 += x_inc;
		y1 += y_inc;
	}

}

void ViewerWidget::drawLineBresenham(QPoint start, QPoint end, QColor color)
{
	// Zaciatocny bod usecky
	int x1 = start.x();
	int y1 = start.y();
	// Koncovy bod usecky
	int x2 = end.x();
	int y2 = end.y();

	// Vypocet rozdielov
	int dx = x2 - x1; 
	int dy = y2 - y1;
	//hovoria o posune vseobecne - o kolko pixelov a v akom smere 
	
	// Urcenie smeru (namiesto kontroly podla smernice: m > 0 alebo m < 0)
	int smer_x = (x2 >= x1) ? 1 : -1; //dx>=0; 1 - doprava, -1 - dolava
	int smer_y = (y2 >= y1) ? 1 : -1; //dy>=0; 1 - nadol, -1 - nahor
	//kombinacie (1,1),(-1,-1),(-1,1),(1,-1) pokryju vsetky 4 kvadranty - vpravo nadol(standartny), vlavo nahor, vlavo nadol, vpravo nahor

	// Absolutne hodnoty dx a dy (pre vypocet parametra p)
	int adx = std::abs(dx); //horizontalna vzd - pocet krokov od x1 do x2 po osi x
	int ady = std::abs(dy); //vertikalna vzd
	//predstavuju len velkost posunu - vzdialenosti, bez smeru
	
	// Rozdelenie len na dva pripady podla sklonu (ciara je plocha-viac do sirky alebo strma-viac do vysky)
	if (adx >= ady) { // RIADIACA OS JE X - x menime vzdy, pri kazdej iteracie
		int p = 2 * ady - adx; //rozhodovaci parameter - ci je priamka blizsia k dolnemu alebo hornemu pixelu - ci menime y a akym smerom
		//p predstavuje mieru chyby medzi spojitou priamkou a aktualne kreslenym pixelom 
		int k1 = 2 * ady; //konst-zmena(+) p ak sa posunee iba po osi x pri kresleni akt pixela a y sa nemeni - urcuje hodnotu pi+1
		int k2 = 2 * (ady - adx); //ak sa posuneme po oboch osiach

		for (int i = 0; i <= adx; i++) { //prechadzame po adx+1 pixelov
			setPixel(x1, y1, color); //nakreslime aktualny pixel na suradnici (x1, y1).
			x1 += smer_x; //posun v smere x - urcite (menime xovu suradnicu - akt pixelom stava dalsi)
			if (p > 0) { //ak priamka je blizsia k susednemu pixelu v smere y
				y1 += smer_y; //posun v smere y 
				p += k2;
			}
			else {
				p += k1;
			}
		}
	}
	else { // RIADIACA OS JE Y 
		int p = 2 * adx - ady;
		int k1 = 2 * adx;
		int k2 = 2 * (adx - ady);

		for (int i = 0; i <= ady; i++) {
			setPixel(x1, y1, color);
			y1 += smer_y; 
			if (p > 0) {
				x1 += smer_x;
				p += k2;
			}
			else {
				p += k1;
			}
		}
	}
}


void ViewerWidget::drawCircleBresenham(QPoint center, QPoint edge, QColor color)
{
	// Zakladne data
	int sx = center.x();
	int sy = center.y();
	double dx = edge.x() - sx;
	double dy = edge.y() - sy;
	int r = (int)(std::sqrt(dx * dx + dy * dy) + 0.5);

	// Inicializacia premennych  (str. 33)
	int x = 0;
	int y = r;
	int p = 1 - r;
	int dvaX = 3;
	int dvaY = 2 * r - 2;

	// Kym sme v jednom oktante)
	while (x <= y) {
		// Vykreslenie 8 symetrickych bodov
		setPixel(sx + x, sy + y, color);
		setPixel(sx - x, sy + y, color);
		setPixel(sx + x, sy - y, color);
		setPixel(sx - x, sy - y, color);
		setPixel(sx + y, sy + x, color);
		setPixel(sx - y, sy + x, color);
		setPixel(sx + y, sy - x, color);
		setPixel(sx - y, sy - x, color);

		// Rozhodovacia podmienka
		if (p > 0) {
			p = p + dvaX - dvaY + 5;
			y--;
			dvaY -= 2;
		}
		else {
			p = p + dvaX + 3;
		}
		dvaX += 2;
		x++;
	}
	update();
}

void ViewerWidget::addPolygonPoint(QPoint p)
{
	polygonPoints.push_back(p);
}

void ViewerWidget::drawPolygon(QColor color)
{
	if (polygonPoints.size() < 2) return;

	if (!polygonClosed) {
		for (size_t i = 0; i < polygonPoints.size() - 1; i++) { //teda zacneme od 1.
			drawLine(polygonPoints[i], polygonPoints[i + 1], color);
		}
	} else {
		std::vector<QPoint> pointsToDraw = clipSutherlandHodgman(this->polygonPoints);

		if (pointsToDraw.size() >= 2) {
			for (size_t i = 0; i < pointsToDraw.size(); i++) {
				QPoint start = pointsToDraw[i];
				QPoint end;

				if (i == pointsToDraw.size() - 1) {
					end = pointsToDraw[0];
				}
				else {
					end = pointsToDraw[i + 1];
				}
				drawLine(start, end, color);
			}
		}
	}
	
}

std::vector<QPoint> ViewerWidget::rotate(const std::vector<QPoint>& points, double angle_deg, QPoint pivot) {
	std::vector<QPoint> rotated;
	double angle_rad = angle_deg * (M_PI / 180.0);
	double cosA = std::cos(angle_rad);
	double sinA = std::sin(angle_rad);
	for (QPoint point : points) {
		double xnew = (point.x() - pivot.x()) * cosA - (point.y() - pivot.y()) * sinA + pivot.x();
		double ynew = (point.x() - pivot.x()) * sinA + (point.y() - pivot.y()) * cosA + pivot.y();
		rotated.push_back(QPoint(qRound(xnew), qRound(ynew)));
	}
	return rotated;
}

std::vector<QPoint> ViewerWidget::rotate(const std::vector<QPoint>& points, double angle) {
	return rotate(points, angle, points[0]); 
}


std::vector<QPoint> ViewerWidget::scale(const std::vector<QPoint>& points, double factorx, double factory)
{
	std::vector<QPoint> scaled;
	QPoint scaledpoint;
	QPoint pivot = points[0];
	for (QPoint point : points) {
		scaledpoint = QPoint((point.x() - pivot.x()) * factorx + pivot.x(), (point.y() - pivot.y()) * factory + pivot.y());
		scaled.push_back(scaledpoint);
	}
	return scaled;
}

std::vector<QPoint> ViewerWidget::shear(const std::vector<QPoint>& points, double koefx)
{
	std::vector<QPoint> sheared;
	QPoint shearedpoint;
	QPoint pivot = points[0];
	for (QPoint point : points) {
		shearedpoint = QPoint((point.x() - pivot.x())+ koefx * (point.y() - pivot.y()) +pivot.x(), point.y()); //posunieme bod k nule (odcitame pivot), tam ho "skrivime" (*koefx) a potom ho vratime spat tam kde bol (pripocitame pivot)
		sheared.push_back(shearedpoint);
	}
	return sheared;
}

std::vector<QPoint> ViewerWidget::reflect(const std::vector<QPoint>& points, QPoint A, QPoint B)
{
	std::vector<QPoint> reflected;
	QPoint reflectedpoint;

	//vektor AB
	double u = B.x() - A.x(); 
	double v = B.y() - A.y();

	//vektor normaly
	double a = v;
	double b = -u;
	//c pre plnu rovnicu priamky aX+bY+c
	double c = - a * A.x() - b * A.y();

	for (QPoint point : points) {
		double d = (a * point.x() + b * point.y() + c) / (a * a + b * b);
		int xnew = point.x() - 2 * a * d;
		int ynew = point.y() - 2 * b * d;
		reflectedpoint = QPoint(xnew, ynew);
		reflected.push_back(reflectedpoint);
	}
	return reflected;
}

QPoint ViewerWidget::intersection(QPoint S, QPoint V, int xmin)
{
	if (V.x() == S.x()) return QPoint(xmin, S.y());
	double m = (double)(V.y() - S.y()) / (V.x() - S.x());
	int Py = qRound(S.y() + m * (xmin - S.x()));
	return QPoint(xmin, Py);
}

std::vector<QPoint> ViewerWidget::clipEdgeSH(const std::vector<QPoint>& points, int xmin)
{
	std::vector<QPoint> W; //vrcholy orezaneho polygony, vrcholy orig - V - points
	QPoint S = points.back(); //posledny vrchol points(V) - V_(n-1) 
	//S je start bod iteracie - zaciatocny bod aktualne spracovavanej hranyS->Vi(su orientovane) (*teda vzdy prva srpacovavana hrana je medzi prvym a poslednym bodom z points)
	for (const QPoint& Vi : points) { //Vi su koncove body hran S->Vi
		if (Vi.x() >= xmin) { //koncovy je vnutri okna
			if (S.x() >= xmin) { //aj zaciatocny je vnutri - obidva
				W.push_back(Vi); //pridavame posledny do orezaneho polygona (S nepridavame kvoli duplikaciam, pridame kedy bude poslednym)
			}
			else { //S je vonku, V vnutri - hrana vchadza v okno
				QPoint Pi = intersection(S, Vi, xmin); //vypocitavame priesecnik i-tej hrany s oknom
				W.push_back(Pi); //do vysledku pridame aj priesecmik, aj koncovy
				W.push_back(Vi);
			}
		}
		else { //koncovy je vonku
			if (S.x() >= xmin) { //ale zaciatocny vnutri - hrana vychadza s okna
				QPoint Pi = intersection(S, Vi, xmin);
				W.push_back(Pi); //do vysledku ide vypocitany priesecnik (lebo zaciatocne nepridavame hned, pokial nedojdeme k nim ako koncovym))
			}
		}
		S = Vi; //posunieme na dalsiu hranu - aktualny koncovy sa stava zaciatocnym
	}

	return W;
}

std::vector<QPoint> ViewerWidget::clipSutherlandHodgman(const std::vector<QPoint>& points)
{
	std::vector<QPoint> clipped = points;
	QPoint origin(0, 0);

	////Orezanie LEFT (xmin=0)
	//clipped = clipEdgeSH(clipped, 0);
	//clipped = rotate(clipped, -90, origin);
	////Orezanie TOP - otocime sa o 90 a orezeme podla xmin=0 stale
	//clipped = clipEdgeSH(clipped, 0);
	//clipped = rotate(clipped, -90, origin);
	////Orezanie RIGHT (xmin=-(sirka_okna))
	//clipped = clipEdgeSH(clipped, -(img->width()-1));
	//clipped = rotate(clipped, -90, origin);
	////Orezanie BOTTOM
	//clipped = clipEdgeSH(clipped, -(img->height()-1));
	//clipped = rotate(clipped, -90, origin);

	int xmin[4] = {5,5,-(img->width() - 5),-(img->height() - 5)};
	for (int i = 0; i < 4; i++) {
		clipped = clipEdgeSH(clipped, xmin[i]);
		clipped = rotate(clipped, -90, origin);
	}

	return clipped;
}

//Slots
void ViewerWidget::paintEvent(QPaintEvent* event)
{
	QPainter painter(this);
	if (!img || img->isNull()) return;

	QRect area = event->rect();
	painter.drawImage(area, *img, area);
}