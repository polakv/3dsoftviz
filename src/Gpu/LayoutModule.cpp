/*!
 * LayoutModule.cpp
 * Projekt 3DVisual
 */
#include <cuda_runtime.h>
#include "Gpu/LayoutModule.h"
#include <QDebug>

extern "C"
void initKernelConstants(unsigned int numVertices, float sizeFactor);

extern "C"
void computeLayout(void* vertexBuffer, unsigned int vertexBufferSize, void* velocityBuffer,
				   void* edgeBuffer, unsigned int edgeBufferSize);

bool Gpu::LayoutModule::init()
{
	if(!_vertexBuffer || !_velocityBuffer || !_edgeBuffer)
	{
        qDebug() << "[Gpu::LayoutModule::init] Resources are missing.";
		this->disable();
        return false;
    }

	initKernelConstants(_vertexBuffer->getDimension(0), 0.4f);
	return osgCompute::Module::init();
}

void Gpu::LayoutModule::launch()
{	
	computeLayout(_vertexBuffer->map(), _vertexBuffer->getDimension(0), _velocityBuffer->map(),
					_edgeBuffer->map(), _edgeBuffer->getDimension(0));

	//mapping position data back to CPU
	_vertexBuffer->map( osgCompute::MAP_HOST );
}

void Gpu::LayoutModule::acceptResource(osgCompute::Resource& resource)
{
	if( resource.isIdentifiedBy("VERTEX_BUFFER") )
	{
		_vertexBuffer = dynamic_cast<osgCompute::Memory*>( &resource );
	}

	if( resource.isIdentifiedBy("VELOCITY_BUFFER") )
	{
		_velocityBuffer = dynamic_cast<osgCompute::Memory*>( &resource );
	}

	if( resource.isIdentifiedBy("EDGE_BUFFER") )
	{
		_edgeBuffer = dynamic_cast<osgCompute::Memory*>( &resource );
	}
}

void Gpu::LayoutModule::clearLocal()
{
	_vertexBuffer = NULL;
	_velocityBuffer = NULL;
	_edgeBuffer = NULL;
}