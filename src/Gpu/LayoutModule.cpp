/*!
 * LayoutModule.cpp
 * Projekt 3DVisual
 */
#include "Gpu/LayoutModule.h"
#include <QDebug>

extern "C"
void computeLayout(unsigned int numBlocks, 
				   unsigned int numThreads, 
				   void* nodes, 
				   void* edgeIndexes, 
				   unsigned int edgeIndexesSize, 
				   void* edgeValues, 
				   unsigned int edgeValuesSize);
extern "C"
void createExplosion( unsigned int numBlocks, unsigned int numThreads, void* nodes);

bool Gpu::LayoutModule::init()
{
	if(!_vertexBuffer || !_edgeIndexBuffer || !_edgeValueBuffer)
	{
        qDebug() << "[Gpu::LayoutModule::init] Resources are missing.";
		this->disable();
        return false;
    }

	_numThreads = 128;
	_numBlocks = _vertexBuffer->getDimension(0) / _numThreads;
    if( _vertexBuffer->getDimension(0) % _numThreads != 0 )
	{
		_numBlocks+=1;
	}    

	return osgCompute::Module::init();
}

void Gpu::LayoutModule::launch()
{	
	/*computeLayout(_numBlocks, 
		_numThreads, 
		_vertexBuffer->map(), 
		_edgeIndexBuffer->map(),
		_edgeIndexBuffer->getDimension(0),
		_edgeValueBuffer->map(),
		_edgeValueBuffer->getDimension(0));*/

	createExplosion(_numBlocks, _numThreads, _vertexBuffer->map());

	_vertexBuffer->map( osgCompute::MAP_HOST );
}

void Gpu::LayoutModule::acceptResource(osgCompute::Resource& resource)
{
	if( resource.isIdentifiedBy("NODE_POSITIONS") )
	{
		_vertexBuffer = dynamic_cast<osgCompute::Memory*>( &resource );
	}

	if( resource.isIdentifiedBy("EDGE_INDEXES") )
	{
		_edgeIndexBuffer = dynamic_cast<osgCompute::Memory*>( &resource );
	}

	if( resource.isIdentifiedBy("EDGE_VALUES") )
	{
		_edgeValueBuffer = dynamic_cast<osgCompute::Memory*>( &resource );
	}
}

void Gpu::LayoutModule::clearLocal()
{
	_vertexBuffer = NULL;
	_edgeIndexBuffer = NULL;
	_edgeValueBuffer = NULL;
}