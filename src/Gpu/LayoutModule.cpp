/*!
 * LayoutModule.cpp
 * Projekt 3DVisual
 */
#include "Gpu/LayoutModule.h"
#include <QDebug>

extern "C"
void computeLayout( unsigned int numBlocks, unsigned int numThreads, void* positions );

bool Gpu::LayoutModule::init()
{
	if(!_buffer)
	{
        qDebug() << "[Gpu::LayoutModule::init] Memory object with positions is missing.";
		this->disable();
        return false;
    }

	_numBlocks = _buffer->getDimension(0) / 128;
    if( _buffer->getDimension(0) % 128 != 0 )
	{
		_numBlocks+=1;
	}
    _numThreads = 128;

	return osgCompute::Module::init();
}

void Gpu::LayoutModule::launch()
{	
	computeLayout(_numBlocks, _numThreads, _buffer->map());
	_buffer->map( osgCompute::MAP_HOST_SOURCE );
}

void Gpu::LayoutModule::acceptResource(osgCompute::Resource& resource)
{
	if( resource.isIdentifiedBy("NODE_POSITIONS") )
	{
		Gpu::NodePositions* positionsResource = dynamic_cast<Gpu::NodePositions*>( &resource );
		if(NULL != positionsResource && positionsResource->_positions->size() > 0)
		{
			osg::ref_ptr<osg::Vec3Array> positions = positionsResource->_positions;
			_buffer = new osgCuda::Memory;
			_buffer->setElementSize( 3 * sizeof(float) );
			_buffer->setDimension(0, positions->size());

			memcpy(_buffer->map(osgCompute::MAP_HOST_TARGET), positions->getDataPointer(), _buffer->getByteSize());
		}
	}  
}

void Gpu::LayoutModule::clearLocal()
{
	_buffer = NULL;
}