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
	_vertexBufferOffsets = new QMap<qlonglong, unsigned int>;	
	_edgeIndexes = new std::vector<unsigned int>;
	_edgeValues = new std::vector<unsigned int>;

	srand ( time(NULL) );
}

void Gpu::ResourceVisitor::collect(osg::Node& node)
{
	Data::Node* dataNode = dynamic_cast<Data::Node*> ( &node );
	if(NULL != dataNode)
	{
		_vertexBufferOffsets->insert(dataNode->getId(), _nodeCount);
		_nodeCount++;

		//qDebug() << "nodeId: " << dataNode->getId() << " (" << _edgeValues->size() << "," << dataNode->getEdges()->size() << ")";
		_edgeIndexes->push_back(_edgeValues->size());
		_edgeIndexes->push_back(dataNode->getEdges()->size());

		QMap<qlonglong, osg::ref_ptr<Data::Edge> >::iterator j;
		for (j = dataNode->getEdges()->begin(); j != dataNode->getEdges()->end(); j++)
		{
			osg::ref_ptr<Data::Edge> edge = j.value();
			if(edge->getSrcNode()->getId() == dataNode->getId())
			{
				_edgeValues->push_back(edge->getDstNode()->getId());
			}
			else
			{
				_edgeValues->push_back(edge->getSrcNode()->getId());
			}
			//qDebug() << "neighbourId: " << neighbourNode->getId();
		}
	}

	osgCompute::ResourceVisitor::collect(node);
}

void Gpu::ResourceVisitor::distribute(osg::Node& node)
{
	this->addPositionResource();

	Data::Node* dataNode = dynamic_cast<Data::Node*> ( &node );
	if(NULL != dataNode && _vertexBuffer != NULL && _vertexBufferOffsets->contains(dataNode->getId()))
	{
		dataNode->setCurrentPositionPointer((osg::Vec3*) _vertexBuffer->map(osgCompute::MAP_HOST_TARGET, _vertexBufferOffsets->value(dataNode->getId()) * sizeof(osg::Vec3)));
		
		//initial random position
		osg::Vec3 randomPosition(((float)(rand()*2)/(float)RAND_MAX) - 1, ((float)(rand()*2)/(float)RAND_MAX) - 1, ((float)(rand()*2)/(float)RAND_MAX) - 1);
		randomPosition.normalize();
		dataNode->setCurrentPosition(randomPosition * 2);
	}

	osgCompute::ResourceVisitor::distribute(node);
}

void Gpu::ResourceVisitor::exchange(osg::Node& node)
{
	this->addPositionResource();

	Data::Node* dataNode = dynamic_cast<Data::Node*> ( &node );
	if(NULL != dataNode && _vertexBuffer != NULL && _vertexBufferOffsets->contains(dataNode->getId()))
	{
		dataNode->setCurrentPositionPointer((osg::Vec3*) _vertexBuffer->map(osgCompute::MAP_HOST_TARGET, _vertexBufferOffsets->value(dataNode->getId()) * sizeof(osg::Vec3)));
	}

	osgCompute::ResourceVisitor::exchange(node);
}

void Gpu::ResourceVisitor::addPositionResource()
{
	if(_vertexBuffer == NULL && _nodeCount > 0)
	{
		_vertexBuffer = new osgCuda::Memory;
		_vertexBuffer->addIdentifier("NODE_POSITIONS");
		_vertexBuffer->setElementSize( sizeof(osg::Vec3) );
		_vertexBuffer->setDimension(0, _nodeCount);

		if(_vertexBuffer->init())
		{
			this->addResource(*_vertexBuffer);
		}
		else
		{
			qDebug() <<  "[Gpu::ResourceVisitor::addPositionResource] Problem with creating memory object - vertices.";
		}	

		osg::ref_ptr<osgCompute::Memory> edgeIndexBuffer = new osgCuda::Memory;
		edgeIndexBuffer->addIdentifier("EDGE_INDEXES");
		edgeIndexBuffer->setElementSize( sizeof(unsigned int) );
		edgeIndexBuffer->setDimension(0, _edgeIndexes->size());

		if(edgeIndexBuffer->init())
		{
			std::copy(_edgeIndexes->begin(), _edgeIndexes->end(), (unsigned int*) edgeIndexBuffer->map(osgCompute::MAP_HOST_TARGET));
			this->addResource(*edgeIndexBuffer);
		}
		else
		{
			qDebug() <<  "[Gpu::ResourceVisitor::addPositionResource] Problem with creating memory object - edge indexes.";
		}

		osg::ref_ptr<osgCompute::Memory> edgeValueBuffer = new osgCuda::Memory;
		edgeValueBuffer->addIdentifier("EDGE_VALUES");
		edgeValueBuffer->setElementSize( sizeof(unsigned int) );
		edgeValueBuffer->setDimension(0, _edgeValues->size());

		//translate from node ids to offsets
		std::vector<unsigned int>::iterator iter;
		for(iter = _edgeValues->begin(); iter != _edgeValues->end(); iter++)
		{
			unsigned int nodeId = *iter;
			*iter = _vertexBufferOffsets->value(nodeId);
		}

		if(edgeValueBuffer->init())
		{
			std::copy(_edgeValues->begin(), _edgeValues->end(), (unsigned int*) edgeValueBuffer->map(osgCompute::MAP_HOST_TARGET));
			this->addResource(*edgeValueBuffer);
		}
		else
		{
			qDebug() <<  "[Gpu::ResourceVisitor::addPositionResource] Problem with creating memory object - edge values.";
		}
	}
}

void Gpu::ResourceVisitor::clearLocal()
{
	_nodeCount = 0;
	_vertexBuffer = NULL;
	_vertexBufferOffsets->clear();
	_edgeIndexes->clear();
	_edgeValues->clear();
}