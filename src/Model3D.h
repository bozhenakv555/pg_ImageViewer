#pragma once
#include<vector>
#include <QString>

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

	void createCube(double a);

	bool saveToVTK(QString filename);
};