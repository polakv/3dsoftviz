/*!
 * ResourceVisitor.cpp
 * Projekt 3DVisual
 */
#include "Gpu/ResourceVisitor.h"
#include <osg/AutoTransform>
#include <QDebug>

Gpu::ResourceVisitor::ResourceVisitor()
{	
	_positionsResource = new Gpu::NodePositions;
	_positionsResource->addIdentifier("NODE_POSITIONS");
	this->addResource(*_positionsResource);
}

void Gpu::ResourceVisitor::collect(osg::Node& node)
{
	osg::AutoTransform* transform = dynamic_cast<osg::AutoTransform*>( &node );
	if(NULL != transform)
	{
		_positionsResource->_positions->push_back(transform->getPosition());
	}

	osgCompute::ResourceVisitor::collect(node);
}

void Gpu::ResourceVisitor::clearLocal()
{
    _positionsResource = NULL;
}