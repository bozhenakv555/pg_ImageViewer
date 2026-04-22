#pragma once
#include<vector>
#include <QString>
#include <QColor>

struct Point3D {
	double x, y, z;
};

struct Triangle {
	int vertex_indexes[3];
};

class Model3D {
private:
	void addFace(int v1, int v2, int v3) {
		Triangle t;
		t.vertex_indexes[0] = v1;
		t.vertex_indexes[1] = v2;
		t.vertex_indexes[2] = v3;
		faces.push_back(t);
	}
public:
	std::vector<Point3D> vertices;
	std::vector<Triangle> faces;
	std::vector<QColor> facesColors;

	void createCube(double a);

	void createUVSphere(int P, int M, double r);

	bool saveToVTK(QString filename);
	bool loadFromVTK(QString filename);
};