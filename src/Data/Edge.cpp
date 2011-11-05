/*!
 * Edge.cpp
 * Projekt 3DVisual
 */
#include "Data/Edge.h"

Data::Edge::Edge(qlonglong id, QString name, Data::Graph* graph, Data::Node* srcNode, Data::Node* dstNode, Data::Type* type, bool isOriented, float scaling, int pos) //: osg::DrawArrays(osg::PrimitiveSet::QUADS, pos, 4)
{
    this->id = id;
    this->name = name;
    this->graph = graph;
    this->srcNode = srcNode;
    this->dstNode = dstNode;
    this->type = type;
    this->oriented = isOriented;
    //this->camera = camera;
    this->selected = false;
	this->inDB = false;
	this->scale = scaling;
    float r = type->getSettings()->value("color.R").toFloat();
    float g = type->getSettings()->value("color.G").toFloat();
    float b = type->getSettings()->value("color.B").toFloat();
    float a = type->getSettings()->value("color.A").toFloat();
    
    //this->edgeColor = osg::Vec4(r, g, b, a);
    	
    /*coordinates = new Data::VectorArray();*/
    //edgeTexCoords = new osg::Vec2Array();
       
    updateCoordinates(getSrcNode()->getTargetPosition(), getDstNode()->getTargetPosition());
}


Data::Edge::~Edge(void)
{
    this->graph = NULL;
	if(this->srcNode!=NULL) {
		this->srcNode->removeEdge(this);
		this->srcNode = NULL;
	}
	
	if(this->dstNode!=NULL) {
		this->dstNode->removeEdge(this);
		this->dstNode = NULL;
	}
	
    this->type = NULL;
}

void Data::Edge::linkNodes(QMap<qlonglong, Data::Edge* > *edges)
{
    edges->insert(this->id, this);
    this->dstNode->addEdge(this);
    this->srcNode->addEdge(this);
}

void Data::Edge::unlinkNodes()
{
	this->dstNode->removeEdge(this);
	this->srcNode->removeEdge(this);
	this->srcNode = NULL;
	this->dstNode = NULL;
}

void Data::Edge::updateCoordinates(Data::Vector srcPos, Data::Vector dstPos)
{
	/*coordinates->clear();*/
	//edgeTexCoords->clear();

	Data::Vector viewVec(0, 0, 1);
	Data::Vector up;

	//if (camera != 0)
	//{
	//	Data::Vector eye;
	//	Data::Vector center;

	//	//camera->getViewMatrixAsLookAt(eye,center,up);

	//	viewVec = eye - center;

	////	std::cout << eye.x() << " " << eye.y() << " " << eye.z() << "\n";
	////	std::cout << center.x() << " " << center.y() << " " << center.z() << "\n";
	//}
	
	viewVec.normalize();

	Data::Vector x, y;
	x.set(srcPos);
	y.set(dstPos);

	Data::Vector edgeDir = x - y;
	length = edgeDir.length();

	up = edgeDir ^ viewVec;
	up.normalize();

	up *= this->scale;

	//updating edge coordinates due to scale
	//coordinates->push_back(Data::Vector(x.x() + up.x(), x.y() + up.y(), x.z() + up.z()));
	//coordinates->push_back(Data::Vector(x.x() - up.x(), x.y() - up.y(), x.z() - up.z()));
	//coordinates->push_back(Data::Vector(y.x() - up.x(), y.y() - up.y(), y.z() - up.z()));
	//coordinates->push_back(Data::Vector(y.x() + up.x(), y.y() + up.y(), y.z() + up.z()));

	int repeatCnt = length / (2 * this->scale);

	//init edge-text (label) coordinates
	/*edgeTexCoords->push_back(osg::Vec2(0,1.0f));
	edgeTexCoords->push_back(osg::Vec2(0,0.0f));
	edgeTexCoords->push_back(osg::Vec2(repeatCnt,0.0f));
	edgeTexCoords->push_back(osg::Vec2(repeatCnt,1.0f));*/

	/*if (label != NULL)
		label->setPosition((srcPos + dstPos) / 2 );*/
}

