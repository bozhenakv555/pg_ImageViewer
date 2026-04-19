#include   "ViewerWidget.h"
#include <climits>

#include <QtGlobal> //pre qRound

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

	//hned orezeme
	std::vector<QPoint> clipped = clipCyrusBeck(start, end);
	if (clipped.size() < 2) return; //usecka je uplne mimo, nepokracujeme

	QPoint p1 = clipped[0];
	QPoint p2 = clipped[1];

	if (algType == 0) {
		drawLineDDA(p1, p2, color);
	}
	else {
		drawLineBresenham(p1, p2, color);
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

void ViewerWidget::clearAll()
{
	polygonPoints.clear(); //vymazeme zoznam bodov polygona
	hermitePoints.clear(); //aj krivky
	bezierPoints.clear();
	bSplinePoints.clear();
	polygonClosed = false; //resetneme stav uzavretia
	fillEnabled = false; //vypneme vypln
	drawPolygonActivated = false; //vypneme rezim kreslenia
	clear(); //vymazeme samotne biele platno (img->fill)
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

	//najprv si pripravime orezane body pre obrys aj pre scanline
	//clipSutherlandHodgman nam vrati body, ktore su uz vnutri platna
	std::vector<QPoint> pointsToDraw = polygonPoints;
	if (polygonClosed) {
		pointsToDraw = clipSutherlandHodgman(this->polygonPoints);
	}

	if (pointsToDraw.empty()) return;

	//vykreslenie vyplne, ale iba ak je zapnuta a polygon je uz uzavrety
	if (polygonClosed && fillEnabled) {
		// ak je to trojuholnik, pouzijeme povodne vrcholy (base_t0, t1, t2), farby tak zostanu spravne
		if (polygonPoints.size() == 3 & currentFillType != 0) {
			fillTriangle(base_t0, base_t1, base_t2, currentFillType);
		}
		// ak je to standartny polygon, vyplname pomocou ScanLine s orezanymi bodmi
		else {
			fillScanLine(pointsToDraw, color);
		}

	}

	// vykreslenie obrysu
	if (!polygonClosed) {
		for (size_t i = 0; i < polygonPoints.size() - 1; i++) {
			drawLine(polygonPoints[i], polygonPoints[i + 1], color);
		}
	}
	else {
		//tu uz pouzivame orezane body, aby ciary sa nekoncili mimo obrazka
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

std::vector<QPoint> ViewerWidget::rotate(const std::vector<QPoint>& points, double angle) { //pretazovanie rotate
	return rotate(points, angle, points[0]); 
}

std::vector<QPoint> ViewerWidget::scale(const std::vector<QPoint>& points, double factorx, double factory)
{
	std::vector<QPoint> scaled;
	QPoint scaledpoint;
	QPoint pivot = points[0];
	for (QPoint point : points) {
		scaledpoint = QPoint(qRound((point.x() - pivot.x()) * factorx + pivot.x()), qRound((point.y() - pivot.y()) * factory + pivot.y()));
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
		shearedpoint = QPoint(qRound((point.x() - pivot.x())+ koefx * (point.y() - pivot.y()) +pivot.x()), point.y()); //posunieme bod k nule (odcitame pivot), tam ho "skrivime" (*koefx) a potom ho vratime spat tam kde bol (pripocitame pivot)
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
		//zaokruhlime na najblizsi pixel, nech netratime presnost (inak by 10.9 bolo 10)
		int xnew = qRound(point.x() - 2 * a * d);
		int ynew = qRound(point.y() - 2 * b * d);
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
	if (points.empty()) return std::vector<QPoint>(); //ak nemame co orezavat, vratime prazdny vektor

	std::vector<QPoint> W; //vrcholy orezaneho polygony, vrcholy orig - V - points
	QPoint S = points.back(); //posledny vrchol points(V) - V_(n-1) 
	//S je start bod iteracie - zaciatocny bod aktualne spracovavanej hranyS->Vi(su orientovane) (*teda vzdy prva spracovavana hrana je medzi prvym a poslednym bodom z points)
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
			if (S.x() >= xmin) { //ale zaciatocny vnutri - hrana vychadza z okna
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

	/*int xmin[4] = {5,5,-(img->width() - 5),-(img->height() - 5)};*/
	int xmin[4] = { 0, 0, -(img->width() - 1), -(img->height() - 1) };
	for (int i = 0; i < 4; i++) {
		clipped = clipEdgeSH(clipped, xmin[i]);

		if (clipped.empty()) break;

		clipped = rotate(clipped, -90, origin);
	}

	return clipped;
}

std::vector<QPoint> ViewerWidget::clipCyrusBeck(QPoint P1, QPoint P2) {
	std::vector<QPoint> newpoints;
	
	double tLow = 0;
	double tUp = 1;
	QPoint d = P2 - P1;

	/*QPoint edges[4] = {QPoint(5,5), QPoint(5, img->height()) - 5, QPoint(img->width() - 5, img->height() - 5), QPoint(img->width() - 5, 5)};*/
	QPoint edges[4] = { QPoint(0,0), QPoint(0, img->height() - 1), QPoint(img->width() - 1, img->height() - 1), QPoint(img->width() - 1, 0) };

	for (int i = 0; i < 4; i++) {
		double u, v;
		if (i == 3) {
			u = edges[0].x() - edges[i].x();
			v = edges[0].y() - edges[i].y();
		}
		else {
			u = edges[i + 1].x() - edges[i].x();
			v = edges[i + 1].y() - edges[i].y();
		}
		
		QPoint n(v, -u);

		QPoint w = P1 - edges[i];

		int dn = d.x() * n.x() + d.y() * n.y();
		int wn = w.x() * n.x() + w.y() * n.y();

		if (dn != 0) {
			double t = -(double)wn / dn;
			if (dn > 0) {
				tLow = std::max(t, tLow);
			}
			else {
				tUp = std::min(t, tUp);
			}
		}
		else { //usecka je rovnobezna s hranou
			if (wn < 0) return std::vector<QPoint>(); //ak je bod vonku (wn < 0), cela usecka je mimo
			continue;
		}

	}

	if (tLow == 0 && tUp == 1) {
		bool P1_inside = (P1.x() >= 0 && P1.x() <= img->width() - 1 &&
			P1.y() >= 0 && P1.y() <= img->height() - 1);
		if (P1_inside) {
			newpoints.push_back(P1);
			newpoints.push_back(P2);
		}
	}
	else if (tLow <= tUp) {
		QPoint P1_new(qRound(P1.x() + d.x() * tLow), qRound(P1.y() + d.y() * tLow));
		QPoint P2_new(qRound(P1.x() + d.x() * tUp), qRound(P1.y() + d.y() * tUp));

		newpoints.push_back(P1_new);
		newpoints.push_back(P2_new);
	}

	return newpoints;
}

void ViewerWidget::fillScanLine(std::vector<QPoint> points, QColor color)
{
	if (points.size() < 3) return;

	struct Edge { //sluzi na definiciu geometrie hrany polygpnu
		int y_z, y_k; //y_z = horny koniec hrany, y_k = dolny koniec hrany 
		int x_z; // x suradnica horneho konca hrany
		double w; // w = 1/m , m = delta_y/delta_x; prirastok x pri posune o jeden riadok
		bool operator<(const Edge& e) const {
			return y_z < e.y_z;
		}
	};

	//Priprava gran - asi je to ta tabulka
	std::vector<Edge> edges;

	int ymin = INT_MAX;
	int ymax = INT_MIN;

	for (int i = 0; i < points.size(); i++) {
		QPoint z(points[i].x(), points[i].y());
		QPoint k;
		if (i == points.size() - 1) {
			/*p_k.x() = points[0].x();
			p_k.y() = points[0].y();*/
			k = points[0];
		}
		else {
			k = points[i+1];
		}

		if (z.y() == k.y()) continue; //vynechavanie vodorovnych hran
		
		if (k.y() < z.y()) std::swap(z,k); //zorientovanie zhora nadol

		Edge e;
		e.y_z = z.y();
		e.y_k = k.y() - 1; //skratenie zdola
		e.x_z = z.x();
		e.w = (double)(k.x()-z.x())/(k.y()-z.y()); //ORIG M

		edges.push_back(e);

		//Hladanie extremov - budeme prechadzat od ymin do ymax - TH velkosti ymax-ymin
		if (e.y_z < ymin) ymin = e.y_z;
		if (e.y_k > ymax) ymax = e.y_k;
	}

	//Zoradenie hran
	std::sort(edges.begin(), edges.end());

	struct scanlineEdge { //sluzi na vykreslovanie(rasterizaciu) :)
		int dy; //pocet riadkov do ktorych hrana zasahuje
		double x; //aktualny suradnice priesecniku s rozkladovym riadkom - kde hrana pretina riadok
		double w; //prirastok x pri prechodu na dalsi riadok - o kolko sa posunie
	};

	std::vector<QList<scanlineEdge>> TH;
	TH.resize(ymax - ymin + 1);
	for (int i = 0; i < edges.size(); i++) {
		scanlineEdge se;
		se.dy = edges[i].y_k - edges[i].y_z;
		se.x = edges[i].x_z;
		se.w = edges[i].w;

		TH[edges[i].y_z - ymin].append(se); //ulozenie hrany do tabulky TH pod index, ktory zacina od 0 pre ymin, aby sme mohli prechadzat riadky od 0 po TH.size()-1
	}

	QList<scanlineEdge> ZAH;
	int y = ymin;

	for (int i = 0; i < TH.size(); i++) {
		if (!TH[i].isEmpty()) {
			for (const scanlineEdge& edge : TH[i]) {
				ZAH.push_back(edge);
			}
		}
		std::sort(ZAH.begin(), ZAH.end(), [](const scanlineEdge& a, const scanlineEdge& b) {
			return a.x < b.x;
		});
		for (int j = 0; j+1 < ZAH.size(); j += 2) { //po dvojiciach - krok 2
			int x_start = std::ceil(ZAH[j].x);    
			int x_end = std::floor(ZAH[j + 1].x); 

			if (x_start <= x_end) {
				for (int x = x_start; x <= x_end; x++) {
					setPixel(x, y, color);
				}
			}
		}
		for (int j = 0; j < ZAH.size(); ) {
			if (ZAH[j].dy <= 0) {
				ZAH.removeAt(j);
			}
			else {
				ZAH[j].x += ZAH[j].w;
				ZAH[j].dy--;
				j++;
			}
		}
		y++;
	}
}

void ViewerWidget::fillTriangle(Vertex t0, Vertex t1, Vertex t2, int fillType) {
	base_t0 = t0;
	base_t1 = t1;
	base_t2 = t2;
	
	std::vector<Vertex> points = { t0, t1, t2 };

	std::sort(points.begin(), points.end(), [](const Vertex& a, const Vertex& b) {
		if (a.pos.y() != b.pos.y()) { //ak y sa nerovnaju - teda mozme ich porovnavat <>
			return a.pos.y() < b.pos.y(); //tak primarne usporiadame podla y
		}
		else { //ak nemozme rozhodnut, ako usporiadat podla y, pretoze sa rovnaju
			return a.pos.x() < b.pos.x(); //sekindarne podla x
		}
	});

	t0 = points[0];
	t1 = points[1];
	t2 = points[2];

	if (t0.pos.y() == t1.pos.y()) {
		//pripad: vodorovna horna hrana
		fillBottomTriangle(t0, t1, t2, fillType);
	}
	else if (t1.pos.y() == t2.pos.y()) {
		//pripad: vodorovna spodna hrana
		fillTopTriangle(t0, t1, t2, fillType);
	}
	else {
		QPoint pos_p;
		pos_p = QPoint((t0.pos.x()+(t1.pos.y()- t0.pos.y())*((double)(t2.pos.x()- t0.pos.x())/(t2.pos.y() - t0.pos.y()))), t1.pos.y());
		Vertex p = {pos_p, t1.color};

		if (t1.pos.x() < p.pos.x()) {
			fillTopTriangle(t0, t1, p, fillType);
			fillBottomTriangle(t1, p, t2, fillType);
		}
		else {
			fillTopTriangle(t0, p, t1, fillType);
			fillBottomTriangle(p, t1, t2, fillType);
		}
	}
}

void ViewerWidget::fillTrianglePart(int y1, int y2, double x1, double x2, double w1, double w2, int fillType)
{
	for (int y = y1; y <= y2; y++) {

		int startX = (int)std::ceil(std::min(x1, x2));
		int endX = (int)std::floor(std::max(x1, x2));

		for (int x = startX; x <= endX; x++) {
			setPixel(x, y, getColor(x, y, fillType));
		}

		x1 += w1;
		x2 += w2;
	}
}


void ViewerWidget::fillBottomTriangle(Vertex t0, Vertex t1, Vertex t2, int fillType)
{
	double w1 = (double)(t2.pos.x() - t0.pos.x()) / (t2.pos.y() - t0.pos.y());
	double w2 = (double)(t2.pos.x() - t1.pos.x()) / (t2.pos.y() - t1.pos.y());

	double x1 = t0.pos.x();
	double x2 = t1.pos.x();

	int y1 = t0.pos.y();
	int y2 = t2.pos.y();

	fillTrianglePart(y1, y2, x1, x2, w1, w2, fillType);
}

void ViewerWidget::fillTopTriangle(Vertex t0, Vertex t1, Vertex t2, int fillType)
{
	//hrany idu zhora nadol: e1 spaja t0-t1, e2 spaja t0-t2
	double w1 = (double)(t1.pos.x() - t0.pos.x()) / (t1.pos.y() - t0.pos.y());
	double w2 = (double)(t2.pos.x() - t0.pos.x()) / (t2.pos.y() - t0.pos.y());

	//zaciname na vrchole t0
	double x1 = t0.pos.x();
	double x2 = t0.pos.x();

	int y1 = t0.pos.y();
	int y2 = t1.pos.y();

	fillTrianglePart(y1, y2, x1, x2, w1, w2, fillType);
}

QColor ViewerWidget::getNearestColor(int x, int y, Vertex t0, Vertex t1, Vertex t2)
{
	// vzdialenosti od pixelu k jednotlivym vrcholom
	int d0 = (x - t0.pos.x()) * (x - t0.pos.x()) +
		(y - t0.pos.y()) * (y - t0.pos.y());

	int d1 = (x - t1.pos.x()) * (x - t1.pos.x()) +
		(y - t1.pos.y()) * (y - t1.pos.y());

	int d2 = (x - t2.pos.x()) * (x - t2.pos.x()) +
		(y - t2.pos.y()) * (y - t2.pos.y());

	// vyberieme farbu najblizsieho vrcholu
	if (d0 <= d1 && d0 <= d2) {
		return t0.color;
	}
	else if (d1 <= d0 && d1 <= d2) {
		return t1.color;
	}
	else {
		return t2.color;
	}
}

QColor ViewerWidget::getBarycentricColor(int x, int y, Vertex t0, Vertex t1, Vertex t2)
{
	// celkova plocha trojuholnika T0,T1,T2
	double A = abs((t1.pos.x() - t0.pos.x()) * (t2.pos.y() - t0.pos.y()) -
		(t1.pos.y() - t0.pos.y()) * (t2.pos.x() - t0.pos.x())) / 2;

	// plochy podtrojuholnikov s bodom P(x,y)
	double A0 = abs((t1.pos.x() - x) * (t2.pos.y() - y) -
		(t1.pos.y() - y) * (t2.pos.x() - x)) / 2;

	double A1 = abs((t0.pos.x() - x) * (t2.pos.y() - y) -
		(t0.pos.y() - y) * (t2.pos.x() - x)) / 2;

	double A2 = A - A0 - A1; // tretia plocha (aby sme nemuseli pocitat znova)

	// vahy (barycentricke suradnice)
	double l0 = A0 / A;
	double l1 = A1 / A;
	double l2 = A2 / A;

	// interpolacia farby
	int r = (int)(l0 * t0.color.red() + l1 * t1.color.red() + l2 * t2.color.red());
	int g = (int)(l0 * t0.color.green() + l1 * t1.color.green() + l2 * t2.color.green());
	int b = (int)(l0 * t0.color.blue() + l1 * t1.color.blue() + l2 * t2.color.blue());

	// orezanie na rozsah 0–255
	r = qBound(0, r, 255);
	g = qBound(0, g, 255);
	b = qBound(0, b, 255);

	return QColor(r, g, b);
}

QColor ViewerWidget::getColor(int x, int y, int fillType) {
	QColor color;
	if (fillType == 0) {
		color = base_t0.color; 
	}
	else if (fillType == 1) {
		color = getNearestColor(x, y, base_t0, base_t1, base_t2);
	}
	else {
		color = getBarycentricColor(x, y, base_t0, base_t1, base_t2);
	}
	return color;

}

void ViewerWidget::drawHermiteCurve(const std::vector<double>& angles, double length, QColor color)
{
	if (hermitePoints.size() < 2) return;

	double N = 100.0;
	double dt = 1.0 / N;
	double t;

	for (size_t i = 1; i < hermitePoints.size(); i++) {
		QPoint Pi_minus = hermitePoints[i - 1];
		QPoint Pi = hermitePoints[i];

		QPointF Pi_minus_dot(cos(angles[i - 1]) * length, sin(angles[i - 1]) * length);
		QPointF Pi_dot(cos(angles[i]) * length, sin(angles[i]) * length);

		QPoint Q0(Pi_minus.x(), Pi_minus.y());
		t = dt;

		while (t < 1.0) {
			double t2 = t * t;
			double t3 = t2 * t;

			//Hermitovske polynomy
			double F0 = 2.0 * t3 - 3.0 * t2 + 1.0;
			double F1 = -2.0 * t3 + 3.0 * t2;
			double F2 = t3 - 2.0 * t2 + t;
			double F3 = t3 - t2;

			double q1x = Pi_minus.x() * F0 + Pi.x() * F1 + Pi_minus_dot.x() * F2 + Pi_dot.x() * F3;
			double q1y = Pi_minus.y() * F0 + Pi.y() * F1 + Pi_minus_dot.y() * F2 + Pi_dot.y() * F3;

			QPoint Q1(qRound(q1x), qRound(q1y));

			drawLineBresenham(Q0, Q1, color);

			Q0 = Q1;
			t += dt;
		}

		drawLineBresenham(Q0, Pi, color);

		drawLineBresenham(Pi_minus, QPoint(Pi_minus.x() + qRound(Pi_minus_dot.x()),
			Pi_minus.y() + qRound(Pi_minus_dot.y())), Qt::red);

		if (i == hermitePoints.size() - 1) {
			drawLineBresenham(Pi, QPoint(Pi.x() + qRound(Pi_dot.x()),
				Pi.y() + qRound(Pi_dot.y())), Qt::red);
		}
	}
}

void ViewerWidget::drawBezierCurve(QColor color)
{
	// ak mame menej ako dva body tak nemame co spajat
	if (bezierPoints.size() < 2) return;

	int n = bezierPoints.size();
	int NSegments = 100; //na kolko kuskov rozsekame krivku aby bola hladka
	double dt = 1.0 / NSegments; //krok o kolko sa posunieme v kazdom cykle od 0 po 1

	QPointF Q0 = bezierPoints[0]; //zacneme kreslit v prvom riadiacom bode

	for (double t = dt; t < 1.001; t += dt) { //while (t < 1.0)
		//ale ide az do 1.0001 aby sme kvoli zaokruhlovaniu urcite trafili aj posledny bod
		std::vector<std::vector<QPointF>> P(n); //vyrobime si pomocnu tabulku n riadkov na vypocty podla prednasky
		for (int i = 0; i < n; i++) {
			P[i].resize(n - i); //kazdy dalsi riadok bude mat o jeden bod menej az kym nezostane jeden - "trojuholnikovy tvar"
		}
		for (int j = 0; j < n; j++) {
			P[0][j] = bezierPoints[j]; //do prveho riadku si len skopirujeme body co sme naklikali
		}
		for (int i = 1; i < n; i++) {
			for (int j = 0; j < n - i; j++) {
				//vzorec co robi linearnu interpolaciu medzi bodmi z predchadzajuceho riadku; (1-t) je vaha laveho bodu a t je vaha praveho bodu
				//berieme susedne body z riadku nad nami a vytvarame bod medzi nimi podla toho, k akemu s nich blizsi - teda ich zmesanim podla ich vah; ak t=0.1 sme blizko laveho, ak t=0.9 sme uz skoro pri pravom
				//P[i,j] = (1 - t) * P[i-1,j] + t * P[i-1,j+1]
				P[i][j].setX((1.0 - t) * P[i - 1][j].x() + t * P[i - 1][j + 1].x());
				P[i][j].setY((1.0 - t) * P[i - 1][j].y() + t * P[i - 1][j + 1].y());
			}
		}
		QPointF Q1 = P[n - 1][0]; //bod na krivke pre dane t; n-1 je posledny riadok naseho trojuholnika a 0 je ten jediny bod co v nom ostal
		//spojime predchadzajuci bod s novym a zaokruhlime pre pixely
		drawLine(QPoint(qRound(Q0.x()), qRound(Q0.y())), QPoint(qRound(Q1.x()), qRound(Q1.y())), color);

		Q0 = Q1; //aktualizujeme bod aby sme v dalsej iteracii vedeli kde sme skoncili
	}

	//prejdeme vsetky nase naklikane body a nakreslime ich ako cervene stvorceky 5*5 aby boli viac viditelne
	for (int i = 0; i < n; i++) {
		QPoint p = bezierPoints[i]; //vyberieme si jeden konkretny bod zo zoznamu
		for (int dx = -2; dx <= 2; dx++) { //dvoma cyklami zostrojme pixely okolo neho
			for (int dy = -2; dy <= 2; dy++) { // dx a dy idu od -2 po 2 (5 pixelov sirka/vyska)
				setPixel(p.x() + dx, p.y() + dy, Qt::red);
			}
		}
	}
}

void ViewerWidget::drawBSplineCurve(QColor color)
{
	//a jeden segment potrebujeme stvoricu bodov takze pod 4 nic nekreslime
	if (bSplinePoints.size() < 4) return;

	int n = bSplinePoints.size();
	int NSegments = 100; //kolko ciarok urobime v ramci jedneho segmentu
	double dt = 1.0 / NSegments;

	//prechadzame body tak aby sme vzdy mali stvoricu (i-3, i-2, i-1, i)
	for (int i = 3; i < n; i++) {
		QPoint P0 = bSplinePoints[i - 3];
		QPoint P1 = bSplinePoints[i - 2];
		QPoint P2 = bSplinePoints[i - 1];
		QPoint P3 = bSplinePoints[i];

		//vypocitame uplne prvy bod segmentu pre t = 0
		//Q(0) = (P0 + 4*P1 + P2) / 6
		QPointF Q0((P0.x() + 4.0 * P1.x() + P2.x()) / 6.0,
			(P0.y() + 4.0 * P1.y() + P2.y()) / 6.0);

		for (double t = dt; t < 1.001; t += dt) { 
			double t2 = t * t;
			double t3 = t2 * t;

			//Coonsove bazicke funkcie 
			double b0 = (-t3 + 3.0 * t2 - 3.0 * t + 1.0) / 6.0;
			double b1 = (3.0 * t3 - 6.0 * t2 + 4.0) / 6.0;
			double b2 = (-3.0 * t3 + 3.0 * t2 + 3.0 * t + 1.0) / 6.0;
			double b3 = t3 / 6.0;

			//vypocitame polohu noveho bodu Q1
			QPointF Q1(b0 * P0.x() + b1 * P1.x() + b2 * P2.x() + b3 * P3.x(),
				b0 * P0.y() + b1 * P1.y() + b2 * P2.y() + b3 * P3.y());

			//spojime Q0 a Q1 a zaokruhlime na pixely
			drawLine(QPoint(qRound(Q0.x()), qRound(Q0.y())),
				QPoint(qRound(Q1.x()), qRound(Q1.y())), color);

			Q0 = Q1; //posunieme sa dalej
		}
	}

	//nakreslime cervene stvorceky pre vsetky riadiace body
	for (int i = 0; i < n; i++) {
		QPoint p = bSplinePoints[i];
		for (int dx = -2; dx <= 2; dx++) {
			for (int dy = -2; dy <= 2; dy++) {
				setPixel(p.x() + dx, p.y() + dy, Qt::red);
			}
		}
	}
}


void ViewerWidget::draw3DModel(Model3D model, double phi, double theta, int projection_type, int representation_type, double dz, double R)
{
	if (model.vertices.empty()) return;


	//Transformacia do pohladovej suradnicovej sustavy (View Space asi). Ju tvoria: suradnice vsetkych objektov v scene, pozicia kamery, priemetna, orientacia kamery
	Point3D n, u, v; //pozicia kamery tvorena troma bazovymi vektormi 

	//n je znormovany normalovy vektor priemetne
	n.x = sin(theta) * sin(phi);
	n.y = sin(theta) * cos(phi);
	n.z = cos(theta);

	//u je vektor kolmy na n (takze iba pridame 90 stupnov k uhlu) - uruje orientaciu kamery(zmena or. je rotacia v priemetne)
	u.x = sin(theta + M_PI / 2) * sin(phi);
	u.y = sin(theta + M_PI / 2) * cos(phi);
	u.z = cos(theta + M_PI / 2);

	//v je vektor kolmy na u a v - ziskame vektorovym sucinom
	v.x = u.y * n.z - u.z * n.y;
	v.y = u.z * n.x - u.x * n.z;
	v.z = u.x * n.y - u.y * n.x;

	std::vector<Point3D> viewSpacePoints;

	for (const Point3D& P : model.vertices) {
		Point3D viewPoint;
		viewPoint.x = P.x * v.x + P.y * v.y + P.z * v.z;
		viewPoint.y = P.x * u.x + P.y * u.y + P.z * u.z;
		viewPoint.z = P.x * n.x + P.y * n.y + P.z * n.z;

		viewSpacePoints.push_back(viewPoint);
	}

	std::vector<QPoint> projectedPoints;
	int scale = 1; //pre viditelnost, iba skusam
	double centerX = getImgWidth() / 2.0; //aby objekt bol v strede obrazovky, nie v (0,0) (lavom hornom rohu)
	double centerY = getImgHeight() / 2.0;

	for (const Point3D& VP : viewSpacePoints) {
		float x_proj, y_proj;
	
		//z_proj = 0, ale pamatame si povodne z pre z-buffer asi
		if (projection_type == 0) { //rovnobezne priemetanie
			x_proj = centerX + VP.x * scale;
			y_proj = centerY - VP.y * scale;
			projectedPoints.push_back(QPoint(qRound(x_proj), qRound(y_proj)));
		}
		else if (projection_type == 1) { //perspektivne priemetanie
			double point_distance = R - VP.z;
			//R je vzdialenost kamery od objektu, v podstate radius tej sfery, v strede ktorej sa nachadza nas objekt
			//dz je vzdialenost kamery od priemetne - obrazovky
			//point_distance je vzdialenost kamery od jednotlivych bodov objektu
			if (point_distance > 0.1) { //0.1 je minimalna vzdialenost konkretneho bodu od nasho "oka" aby bod bol viditelny
				x_proj = centerX + ((dz * VP.x) / point_distance);
				y_proj = centerY - ((dz * VP.y) / point_distance);
				projectedPoints.push_back(QPoint(qRound(x_proj), qRound(y_proj)));
			}
			else {
				//ak je bod prilis blizko, pridame "mimo" suradnice, aby sa zachoval pocet bodov v poli, ale nic sa nevykreslilo
				projectedPoints.push_back(QPoint(-10000, -10000));
			}
		}
	}

	//Vykreslenie plosok
	for (const Triangle& t : model.faces) {
		//ziskame "sprojektovane" - 2D body trojuholnikov
		std::vector<QPoint> facePoints = {
			projectedPoints[t.vertex_indexes[0]],
			projectedPoints[t.vertex_indexes[1]],
			projectedPoints[t.vertex_indexes[2]]
		};
		std::vector<QPoint> clipped = clipSutherlandHodgman(facePoints);

		if (representation_type == 0) { //Wireframe
			for (size_t i = 0; i < clipped.size(); i++) {
				QPoint p1 = clipped[i];
				QPoint p2;
				if (i == clipped.size() - 1) {
					p2 = clipped[0];
				}
				else {
					p2 = clipped[i + 1];
				}
				drawLineDDA(p1, p2, Qt::black);
			}
		}

	//test bez orezavania
	//	QPoint p1 = projectedPoints[t.vertex_indexes[0]];
	//	QPoint p2 = projectedPoints[t.vertex_indexes[1]];
	//	QPoint p3 = projectedPoints[t.vertex_indexes[2]];

	//	drawLineDDA(p1, p2, Qt::black);
	//	drawLineDDA(p2, p3, Qt::black);
	//	drawLineDDA(p3, p1, Qt::black);
	}
}

//Slots
void ViewerWidget::paintEvent(QPaintEvent* event)
{
	QPainter painter(this);
	if (!img || img->isNull()) return;

	QRect area = event->rect();
	painter.drawImage(area, *img, area);
}