#pragma once
#include<vector>

struct Point3D {
	double x, y, z;
};

struct Triangle {
	int vertex_indexes[3];
};

class Model3D {
public:
	std::vector<Point3D> vertices;
	std::vector<Triangle> faces;
};