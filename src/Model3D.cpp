#include "Model3D.h"

void Model3D::createCube(double a) {
	vertices.clear();
	faces.clear();

	//Vrcholy

	//horny stvorec - y=a
	vertices.push_back({ a, a, a });   //[0]
	vertices.push_back({ -a, a, a });  //[1]
	vertices.push_back({ -a, a, -a }); //[2]
	vertices.push_back({ a, a, -a });  //[3]

	//dolny stvorec - y=
	vertices.push_back({ a, -a, a });   //[4]
	vertices.push_back({ -a, -a, a });  //[5]
	vertices.push_back({ -a, -a, -a }); //[6]
	vertices.push_back({ a, -a, -a });  //[7]
}