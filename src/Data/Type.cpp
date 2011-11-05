/*!
 * Type.cpp
 * Projekt 3DVisual
 */
#include "Data/Type.h"

Data::Type::Type(qlonglong id, QString name,Data::Graph* graph, QMap<QString, QString> * settings) 
{
	//konstruktor
    this->id = id;
    this->name = name;
	this->inDB = false;
	this->graph = graph;
	this->meta = false;
}

Data::Type::~Type(void)
{
	if(settings!=NULL) {
	    settings->clear();
	}
	
    delete settings;
    settings = NULL;
}
