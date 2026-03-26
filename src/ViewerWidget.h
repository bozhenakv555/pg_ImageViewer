#pragma once
#include <QtWidgets>

struct Vertex {
	QPoint pos;
	QColor color;
};

class ViewerWidget :public QWidget {
	Q_OBJECT

private:
	QSize areaSize = QSize(0, 0);
	QImage* img = nullptr; //samotny vw obrazok neobsahuje, takze pridame *img na ktorom uz budeme kreslit (QPainter nepouzivame, tam je naprogramovane to, co budeme programovat lol)
	uchar* data = nullptr; //ukazuje ake data su uloxene v QImage

	bool drawLineActivated = false;
	QPoint drawLineBegin = QPoint(0, 0);

	bool drawCircleActivated = false;
	QPoint drawCircleBegin = QPoint(0, 0);

	bool drawPolygonActivated = false;
	std::vector<QPoint> polygonPoints;
	bool polygonClosed = false;
	QPoint startMousePos;

	Vertex base_t0, base_t1, base_t2;

	bool fillEnabled = false; 
	int currentFillType = 0;

	std::vector<QPoint> hermitePoints;
	std::vector<QPoint> bezierPoints;

public:
	ViewerWidget(QSize imgSize, QWidget* parent = Q_NULLPTR);
	~ViewerWidget();
	void resizeWidget(QSize size);

	//Image functions
	bool setImage(const QImage& inputImg);
	QImage* getImage() { return img; };
	bool isEmpty();
	bool changeSize(int width, int height);

	void setPixel(int x, int y, int r, int g, int b, int a = 255);
	void setPixel(int x, int y, double valR, double valG, double valB, double valA = 1.);
	void setPixel(int x, int y, const QColor& color);
	bool isInside(int x, int y);

	//Draw functions TOTO BUDEME MAT V 2. TYZDNI - FUNKCIE NA RASTERIZACIU USECKY
	void drawLine(QPoint start, QPoint end, QColor color, int algType = 0);
	void setDrawLineBegin(QPoint begin) { drawLineBegin = begin; }
	QPoint getDrawLineBegin() { return drawLineBegin; }
	void setDrawLineActivated(bool state) { drawLineActivated = state; }
	bool getDrawLineActivated() { return drawLineActivated; }

	//Get/Set functions
	uchar* getData() { return data; }
	void setDataPtr() { data = img ? img->bits() : nullptr; }

	int getImgWidth() { return img ? img->width() : 0; };
	int getImgHeight() { return img ? img->height() : 0; };

	void clear();

	void clearAll();

	//Algorithms
	void drawLineDDA(QPoint start, QPoint end, QColor color);
	void drawLineBresenham(QPoint start, QPoint end, QColor color);

	void drawCircleBresenham(QPoint center, QPoint edge, QColor color);
	void setDrawCircleBegin(QPoint begin) { drawCircleBegin = begin; }
	QPoint getDrawCircleBegin() { return drawCircleBegin; }
	void setDrawCircleActivated(bool state) { drawCircleActivated = state; }
	bool getDrawCircleActivated() { return drawCircleActivated; }

	void addPolygonPoint(QPoint p);
	void drawPolygon(QColor color);
	void setPolygonClosed(bool state) { polygonClosed = state; }
	bool getPolygonClosed() { return polygonClosed; }
	void setDrawPolygonActivated(bool state) { drawPolygonActivated = state; }
	bool getdrawPolygonActivated() { return drawPolygonActivated; }
	std::vector<QPoint> getPolygonPoints() { return polygonPoints; }
	void setPolygonPoints(std::vector<QPoint> points) { polygonPoints = points; }
	void clearPolygonPoints() { polygonPoints.clear(); }

	void setStartMousePos(QPoint pos) { startMousePos=pos; }
	QPoint getStartMousePos() { return startMousePos; }

	//void scale(double factorx, double factory);
	std::vector<QPoint> rotate(const std::vector<QPoint>& points, double angle_deg, QPoint pivot);
	std::vector<QPoint> rotate(const std::vector<QPoint>& points, double angle);
	std::vector<QPoint> scale(const std::vector<QPoint>& points, double factorx, double factory);
	std::vector<QPoint> shear(const std::vector<QPoint>& points, double koefx);
	std::vector<QPoint> reflect(const std::vector<QPoint>& points, QPoint A, QPoint B);

	//orezanie
	QPoint intersection(QPoint S, QPoint V, int xmin);
	std::vector<QPoint> clipEdgeSH(const std::vector<QPoint>& points, int xmin);
	std::vector<QPoint> clipSutherlandHodgman(const std::vector<QPoint>& points);

	std::vector<QPoint> clipCyrusBeck(QPoint P1, QPoint P2);

	//vyplnanie
	void fillScanLine(std::vector<QPoint> points, QColor color);

	void fillTriangle(Vertex t0, Vertex t1, Vertex t2, int fillType);
	void fillTrianglePart(int y1, int y2, double x1, double x2, double w1, double w2, int fillType);
	void fillBottomTriangle(Vertex t0, Vertex t1, Vertex t2, int fillType);
	void fillTopTriangle(Vertex t0, Vertex t1, Vertex t2, int fillType);
	QColor  getNearestColor(int x, int y, Vertex t0, Vertex t1, Vertex t2);
	QColor getBarycentricColor(int x, int y, Vertex t0, Vertex t1, Vertex t2);
	QColor getColor(int x, int y, int fillType);

	void setTriangleVertices(Vertex t0, Vertex t1, Vertex t2) {
		base_t0 = t0;
		base_t1 = t1;
		base_t2 = t2;
	}
	void setFillEnabled(bool enabled) { fillEnabled = enabled; }
	void setFillType(int type) { currentFillType = type; }

	//krivky
	//hermite
	void drawHermiteCurve(const std::vector<double>& angles, double length, QColor color);
	void addHermitePoint(QPoint p) { hermitePoints.push_back(p); }
	void clearHermitePoints() { hermitePoints.clear(); }
	std::vector<QPoint>& getHermitePoints() { return hermitePoints; }

	void drawBezierCurve(QColor color, int NSegments = 100);
	void addBezierPoint(QPoint p) { bezierPoints.push_back(p);  };

public slots:
	void paintEvent(QPaintEvent* event) Q_DECL_OVERRIDE;
};