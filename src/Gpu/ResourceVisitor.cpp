/*!
 * ResourceVisitor.cpp
 * Projekt 3DVisual
 */
#include "Gpu/ResourceVisitor.h"
#include "Data/Node.h"
#include <osg/AutoTransform>
#include <osgCuda/Memory>
#include <QDebug>

#include <stdlib.h>
#include <time.h>

Gpu::ResourceVisitor::ResourceVisitor()
{	
	_nodeCount = 0;
	_vertexBuffer = NULL;
	_vertexOffsets = new QMap<qlonglong, unsigned int>;	
	_edges = new QMap<qlonglong, osg::ref_ptr<Data::Edge>>;

	srand ( time(NULL) );
}

void Gpu::ResourceVisitor::collect(osg::Node& node)
{
	osg::ref_ptr<Data::Node> dataNode = dynamic_cast<Data::Node*> ( &node );
	if(NULL != dataNode)
	{
		_vertexOffsets->insert(dataNode->getId(), _nodeCount);
		_nodeCount++;

		//qDebug() << "nodeId: " << dataNode->getId() << "[" << (_nodeCount - 1) << "]";
		QMap<qlonglong, osg::ref_ptr<Data::Edge> >::iterator j;
		for (j = dataNode->getEdges()->begin(); j != dataNode->getEdges()->end(); j++)
		{
			if(!_edges->contains(j.value()->getId()))
			{
				_edges->insert(j.value()->getId(), j.value());
			}
		}
	}

	osgCompute::ResourceVisitor::collect(node);
}

void Gpu::ResourceVisitor::distribute(osg::Node& node)
{
	this->addPositionResource();

	Data::Node* dataNode = dynamic_cast<Data::Node*> ( &node );
	if(NULL != dataNode && _vertexBuffer != NULL && _vertexOffsets->contains(dataNode->getId()))
	{
		dataNode->setCurrentPositionPointer((osg::Vec3*) _vertexBuffer->map(osgCompute::MAP_HOST_TARGET, _vertexOffsets->value(dataNode->getId()) * sizeof(osg::Vec4)));
		
		dataNode->setCurrentPosition(this->getRandomLocation());
	}

	osgCompute::ResourceVisitor::distribute(node);
}

void Gpu::ResourceVisitor::exchange(osg::Node& node)
{
	this->addPositionResource();

	Data::Node* dataNode = dynamic_cast<Data::Node*> ( &node );
	if(NULL != dataNode && _vertexBuffer != NULL && _vertexOffsets->contains(dataNode->getId()))
	{
		dataNode->setCurrentPositionPointer((osg::Vec3*) _vertexBuffer->map(osgCompute::MAP_HOST_TARGET, _vertexOffsets->value(dataNode->getId()) * sizeof(osg::Vec3)));
	}

	osgCompute::ResourceVisitor::exchange(node);
}

void Gpu::ResourceVisitor::addPositionResource()
{
	if(_vertexBuffer == NULL && _nodeCount > 0)
	{
		_vertexBuffer = new osgCuda::Memory;
		_vertexBuffer->addIdentifier("VERTEX_BUFFER");
		_vertexBuffer->setElementSize( sizeof(osg::Vec4) );
		_vertexBuffer->setDimension(0, _nodeCount);

		if(_vertexBuffer->init())
		{
			this->addResource(*_vertexBuffer);
		}
		else
		{
			qDebug() <<  "[Gpu::ResourceVisitor::addPositionResource] Problem with creating memory object - vertices.";
		}	

		osg::ref_ptr<osgCompute::Memory> edgeBuffer = new osgCuda::Memory;
		edgeBuffer->addIdentifier("EDGE_BUFFER");
		edgeBuffer->setElementSize( sizeof(unsigned int) );
		edgeBuffer->setDimension(0, 2 * _edges->size());

		//create vector with src and dst vertex offsets
		std::vector<unsigned int>* edgeVertexOffsets = new std::vector<unsigned int>;
		QMapIterator<qlonglong, osg::ref_ptr<Data::Edge>> iter(*_edges);
		while (iter.hasNext()) 
		{
			osg::ref_ptr<Data::Edge> edge = iter.next().value();
			//qDebug() << "Edge: " << _vertexOffsets->value(edge->getSrcNode()->getId()) << "<->" << _vertexOffsets->value(edge->getDstNode()->getId());
			edgeVertexOffsets->push_back(_vertexOffsets->value(edge->getSrcNode()->getId()));
			edgeVertexOffsets->push_back(_vertexOffsets->value(edge->getDstNode()->getId()));
		}

		if(edgeBuffer->init())
		{
			std::copy(edgeVertexOffsets->begin(), edgeVertexOffsets->end(), (unsigned int*) edgeBuffer->map(osgCompute::MAP_HOST_TARGET));
			this->addResource(*edgeBuffer);
		}
		else
		{
			qDebug() <<  "[Gpu::ResourceVisitor::addPositionResource] Problem with creating memory object - edge values.";
		}

		qDebug() <<  "Nodes: " << _nodeCount << "Edges: " << _edges->size();
	}
}

osg::Vec3f Gpu::ResourceVisitor::getRandomLocation() 
{
	double PI = acos((double) - 1);
	double l = getRandomDouble() * 300;	
	double alpha = getRandomDouble() * 2 * PI;
	double beta = getRandomDouble() * 2 * PI;
	osg::Vec3f newPos =  osg::Vec3f((float) (l * sin(alpha)), (float) (l * cos(alpha) * cos(beta)), (float) (l	* cos(alpha) * sin(beta)));
	return newPos;
}

double Gpu::ResourceVisitor::getRandomDouble()
{
	int p = rand();

	return (double)rand() / (double)RAND_MAX;
}

void Gpu::ResourceVisitor::clearLocal()
{
	_nodeCount = 0;
	_vertexBuffer = NULL;
	_vertexOffsets->clear();
	_edges->clear();
}