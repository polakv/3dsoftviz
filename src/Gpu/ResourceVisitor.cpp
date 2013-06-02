/*!
 * ResourceVisitor.cpp
 * Projekt 3DVisual
 */
#ifdef HAVE_CUDA

#include "Gpu/ResourceVisitor.h"
#include "Data/Node.h"
#include <osg/AutoTransform>
#include <osgCuda/Memory>
#include <QDebug>

#include <stdlib.h>
#include <time.h>

Gpu::ResourceVisitor::ResourceVisitor(bool randomize)
{	
	_nodeCount = 0;
	_vertexOffsets = new QMap<qlonglong, unsigned int>;	
	_edges = new QMap<qlonglong, osg::ref_ptr<Data::Edge> >;
	_vertexBuffer = NULL;		
	_randomize = randomize;

	srand ( time(NULL) );
}

void Gpu::ResourceVisitor::collect(osg::Node& node)
{
	osg::ref_ptr<Data::Node> dataNode = dynamic_cast<Data::Node*> ( &node );
	if(NULL != dataNode)
	{
		_vertexOffsets->insert(dataNode->getId(), _nodeCount);
		_nodeCount++;

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
	
	float graphScale = Util::ApplicationConfig::get()->getValue("Viewer.Display.NodeDistanceScale").toFloat();
	Data::Node* dataNode = dynamic_cast<Data::Node*> ( &node );
	if(NULL != dataNode && _vertexBuffer != NULL && _vertexOffsets->contains(dataNode->getId()))
	{
		osg::Vec3f targetPosition = _randomize ? this->getRandomPosition() : dataNode->getTargetPosition();
        dataNode->setTargetPositionPtr((osg::Vec3*) _vertexBuffer->map(osgCompute::MAP_HOST_TARGET, _vertexOffsets->value(dataNode->getId()) * sizeof(osg::Vec4)));
		dataNode->setTargetPosition(targetPosition);
		dataNode->setCurrentPosition(dataNode->getTargetPosition() * graphScale);

        dataNode->setGpuFlagsPtr((float*) _vertexBuffer->map(osgCompute::MAP_HOST_TARGET, _vertexOffsets->value(dataNode->getId()) * sizeof(osg::Vec4) + sizeof(osg::Vec3)));
	}

	osgCompute::ResourceVisitor::distribute(node);
}

void Gpu::ResourceVisitor::exchange(osg::Node& node)
{
	osgCompute::ResourceVisitor::exchange(node);
}

void Gpu::ResourceVisitor::addPositionResource()
{
	if(_vertexBuffer == NULL && _nodeCount > 0)
	{

		//vertex positions
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

		//vertex velocities
		osg::ref_ptr<osgCompute::Memory> velocityBuffer = new osgCuda::Memory;
		velocityBuffer->addIdentifier("VELOCITY_BUFFER");
		velocityBuffer->setElementSize( sizeof(osg::Vec4) );
		velocityBuffer->setDimension(0, _nodeCount);

		if(velocityBuffer->init())
		{
			this->addResource(*velocityBuffer);
		}
		else
		{
			qDebug() <<  "[Gpu::ResourceVisitor::addPositionResource] Problem with creating memory object - velocities.";
		}

		//edges
		osg::ref_ptr<osgCompute::Memory> edgeBuffer = new osgCuda::Memory;
		edgeBuffer->addIdentifier("EDGE_BUFFER");
		edgeBuffer->setElementSize( sizeof(unsigned int) );
		edgeBuffer->setDimension(0, 2 * _edges->size());

		//create vector with src and dst vertex offsets
		std::vector<unsigned int>* edgeVertexOffsets = new std::vector<unsigned int>;
		QMapIterator<qlonglong, osg::ref_ptr<Data::Edge> > iter(*_edges);
		while (iter.hasNext()) 
		{
			osg::ref_ptr<Data::Edge> edge = iter.next().value();
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

		if(_randomize)
			qDebug() <<  "Nodes: " << _nodeCount << "Edges: " << _edges->size();
	}
}

osg::Vec3f Gpu::ResourceVisitor::getRandomPosition() 
{
	double PI = acos((double) - 1);
	double l = getRandomDouble() * 30;	
	double alpha = getRandomDouble() * 2 * PI;
	double beta = getRandomDouble() * 2 * PI;
	
	return osg::Vec3f((float) (l * sin(alpha)), (float) (l * cos(alpha) * cos(beta)), (float) (l	* cos(alpha) * sin(beta)));
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

#endif //HAVE_CUDA