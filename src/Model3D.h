#pragma once
#include<vector>
#include <QString>
#include <QColor>

struct Point3D {
    double x, y, z;

    // pre vektory a jednoduchost zivota:) :

    Point3D operator-(const Point3D& v) const {
        return { x - v.x, y - v.y, z - v.z };
    }

    Point3D operator*(const Point3D& v) const {
        return { x * v.x, y * v.y, z * v.z };
    }

    Point3D operator*(double c) const {
        return { x * c, y * c, z * c };
    }

    Point3D operator+(const Point3D& v) const {
        return { x + v.x, y + v.y, z + v.z };
    }

    // skalarny sucin (dot product)
    double dot(const Point3D& v) const {
        return x * v.x + y * v.y + z * v.z;
    }

    // normalizacia - spravi jednotkovu dlzku
    Point3D normalize() const {
        double len = std::sqrt(x * x + y * y + z * z);
        if (len > 0) return { x / len, y / len, z / len };
        return { 0, 0, 0 };
    }

    // vektorovy sucin (cross product) - pre vypocet normaly urcite
    Point3D cross(const Point3D& v) const {
        return {
            y * v.z - z * v.y,
            z * v.x - x * v.z,
            x * v.y - y * v.x
        };
    }
};
typedef Point3D Vector3D;

struct LightParams {
    Point3D lightPos;
    Vector3D I_L; //intenzita(vektor farby) luca dopadajuceho z hlavneho zdroja svetla
    Vector3D I_O; //intenzita(vektor farby) okoliteho svetla sceny
    //materialove koeficienty - trojzlozkove vektory farby (R,G,B) <- z UI
    Vector3D r_s; //zrkadlovy (s for specular) -> reflection 
    Vector3D r_d; //difuzny 
    Vector3D r_a; //ambientny 

    Point3D cameraPos;

    double h; //ostrost odlesku - zrkadloveho odrazu
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
    std::vector<Vector3D> normals; //normala pre kazdy vertex

	void createCube(double a);

	void createUVSphere(int P, int M, double r);

	bool saveToVTK(QString filename);
	bool loadFromVTK(QString filename);
};