#include "Model3D.h"
#include <string>
#include <fstream>

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