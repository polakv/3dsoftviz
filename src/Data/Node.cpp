/*!
 * Node.cpp
 * Projekt 3DVisual
 *
 * TODO - reload configu sa da napisat aj efektivnejsie. Pri testoch na hranach priniesol vsak podobny prepis len male zvysenie vykonu. Teraz na to
 * nemam cas, takze sa raz k tomu vratim 8)
 */
#include "Data/Node.h"

typedef osg::TemplateIndexArray<unsigned int, osg::Array::UIntArrayType,4,1> ColorIndexArray;

Data::Node::Node(qlonglong id, QString name, Data::Type* type, float scaling, Data::Graph* graph, osg::Vec3f position) 
{
	//konstruktor
	//scaling je potrebne na zmensenie uzla ak je vnoreny
    this->id = id;
	this->name = name;
	this->type = type;
	this->targetPosition = position;
	this->graph = graph;
	this->inDB = false;
	this->edges = new QMap<qlonglong, Data::Edge* >;



	settings = new QMap<QString, QString>();
	//APA

	settings->insert("Velkost","4242");
	settings->insert("Farba","ruzova");
	//APA

	int pos = 0;
	int cnt = 0;

	labelText = this->name;

	while ((pos = labelText.indexOf(QString(" "), pos + 1)) != -1)
	{
		if (++cnt % 3 == 0)
			labelText = labelText.replace(pos, 1, "\n");
	}
	
	//vytvorenie grafickeho zobrazenia ako label
	//this->square = createSquare(this->type->getScale(), Node::createStateSet());
	//this->label = createLabel(this->type->getScale(), labelText);

	this->force = osg::Vec3f();
	this->velocity = osg::Vec3f(0,0,0);
	this->fixed = false;
	this->ignore = false;
	this->positionCanBeRestricted = true;
	this->removableByUser = true;
	this->selected = false;
	this->usingInterpolation = true;

	//nastavenie farebneho typu
	float r = type->getSettings()->value("color.R").toFloat();
	float g = type->getSettings()->value("color.G").toFloat();
	float b = type->getSettings()->value("color.B").toFloat();
	float a = type->getSettings()->value("color.A").toFloat();

	//this->setColor(osg::Vec4(r, g, b, a));
}

Data::Node::~Node(void)
{
	foreach(qlonglong i, edges->keys()) {
		edges->value(i)->unlinkNodes();
	}
    edges->clear(); //staci to ?? netreba spravit delete/remove ??

	delete edges;
}

void Data::Node::addEdge(Data::Edge* edge) { 
	//pridanie napojenej hrany na uzol
	edges->insert(edge->getId(), edge);
}



void Data::Node::removeEdge(Edge* edge )
{
	//odobranie napojenej hrany zo zoznamu
	edges->remove(edge->getId());
}

void Data::Node::removeAllEdges()
{
	//odpojenie od vsetkych hran
	foreach(qlonglong i, edges->keys()) {
		//edges->value(i)->unlinkNodesAndRemoveFromGraph();
	}
	edges->clear();
}

bool Data::Node::equals(Node* node) 
{
	//porovnanie s inym uzlom
	if (this == node)
	{
		return true;
	}
	if (node == NULL)
	{
		return false;
	}		
	if (id != node->getId())
	{
		return false;
	}
	if((graph==NULL && node->getGraph()!=NULL) || (graph!=NULL && node->getGraph()==NULL)) {
	    return false;
	}
    if(graph==NULL && node->getGraph()==NULL) {
        return true;
    }
	return true;
}

osg::Vec3f Data::Node::getCurrentPosition(bool calculateNew, float interpolationSpeed)  
{ 
	//zisime aktualnu poziciu uzla v danom okamihu
	if (calculateNew)
	{
		float graphScale = 1; 

		osg::Vec3 directionVector = osg::Vec3(targetPosition.x(), targetPosition.y(), targetPosition.z()) * graphScale - currentPosition;
		this->currentPosition = osg::Vec3(directionVector * (usingInterpolation ? interpolationSpeed : 1) + this->currentPosition);
	}

	return osg::Vec3(this->currentPosition); 
}
