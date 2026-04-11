#include "Model3D.h"
#include <string>
#include <fstream>
#include <sstream>
#include <math.h>

void Model3D::createCube(double a) {
	vertices.clear();
	faces.clear();

	//Vrcholy

	//horny stvorec: y=a
	vertices.push_back({ a, a, a });   //[0]
	vertices.push_back({ -a, a, a });  //[1]
	vertices.push_back({ -a, a, -a }); //[2]
	vertices.push_back({ a, a, -a });  //[3]

	//dolny stvorec: y=-a
	vertices.push_back({ a, -a, a });   //[4]
	vertices.push_back({ -a, -a, a });  //[5]
	vertices.push_back({ -a, -a, -a }); //[6]
	vertices.push_back({ a, -a, -a });  //[7]

	//horna stena: y=a (diag:0-2)
	addFace(0, 1, 2);
	addFace(0, 2, 3);
	//predna stena: x=a (diag:0-7)
	addFace(0, 3, 7);
	addFace(0, 7, 4);
	//prava stena: z=a (diag:0-5)
	addFace(0, 4, 5);
	addFace(0, 5, 1);
	//dolna stena y=-a (diag: 4-6)
	addFace(4, 6, 7);
	addFace(4, 5, 6);
	//zadna stena x=-a (diag: 1-6)
	addFace(1, 2, 6);
	addFace(1, 6, 5);
	//lava stena z=-a (diag: 3-6)
	addFace(3, 7, 6);
	addFace(3, 6, 2);
}

void Model3D::createSphere(int P, int M, double r)
{
	vertices.clear();
	faces.clear();

	//definicia bodov mriezky
	double dtheta = M_PI / P;
	double dphi = 2*M_PI / M;
	for (int i = 0; i <= P; i++) {
		double theta = i*dtheta;
		for (int j = 0; j <= M; i++) {
			double phi = j * dphi;

			double x = r * sin(theta) * cos(phi);
			double y = r * cos(theta);
			double z = r * sin(theta) * sin(phi);

			vertices.push_back({ x,y,z });
		}
	}
}

bool Model3D::saveToVTK(QString filename) {
	std::string path = filename.toStdString();
	std::ofstream file(path);
	if (!file.is_open()) {
		return false;
	}
	file << "# vtk DataFile Version 3.0" << "\n";
	file << "meow kocka" << "\n";
	file << "ASCII" << "\n";
	file << "DATASET POLYDATA" << "\n";
	file << "POINTS " << vertices.size() << " double" << "\n";
	for (const Point3D& v : vertices) {
		file << v.x << " " << v.y << " " << v.z << "\n";
	}
	file << "POLYGONS " << faces.size() << " " << faces.size() * 4 << "\n";
	for (const Triangle& f : faces) {
		file << "3 " 
			<< f.vertex_indexes[0] << " "
			<< f.vertex_indexes[1] << " "
			<< f.vertex_indexes[2] << "\n";
	}
	file.close();
	return true;
}

bool Model3D::loadFromVTK(QString filename)
{
	std::string path = filename.toStdString();
	std::ifstream file(path);
	if (!file.is_open()) {
		return false;
	}
	std::string line;
	while (std::getline(file, line)) {
		std::stringstream ss(line);
		std::string keyword;
		ss >> keyword;
		if (keyword == "POINTS") {
			int count;
			std::string datatype;
			ss >> count >> datatype;
			vertices.resize(count);
			for (int i = 0; i < count; i++) {
				file >> vertices[i].x >> vertices[i].y >> vertices[i].z;
			}
		}
		else if (keyword == "POLYGONS") {
			faces.clear();
			int facesSize, totalSize;
			ss >> facesSize >> totalSize;
			for (int i = 0; i < facesSize; i++) {
				int face_vert;
				file >> face_vert;
				if (face_vert == 3) {
					Triangle t;
					file >> t.vertex_indexes[0] >> t.vertex_indexes[1] >> t.vertex_indexes[2];
					faces.push_back(t);
				}
			}
		}
	}
	file.close();
	return !vertices.empty();
}