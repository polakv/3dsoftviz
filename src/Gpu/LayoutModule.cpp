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
void computeLayout(void* vertexBuffer, unsigned int vertexBufferSize,
				   void* edgeBuffer, unsigned int edgeBufferSize);
extern "C"
void createExplosion(void* nodes, unsigned int numNodes);

bool Gpu::LayoutModule::init()
{
	if(!_vertexBuffer || !_edgeBuffer)
	{
        qDebug() << "[Gpu::LayoutModule::init] Resources are missing.";
		this->disable();
        return false;
    }

	initKernelConstants(_vertexBuffer->getDimension(0), 10);
	return osgCompute::Module::init();
}

void Gpu::LayoutModule::launch()
{	
	
	computeLayout(_vertexBuffer->map(), _vertexBuffer->getDimension(0),
					_edgeBuffer->map(), _edgeBuffer->getDimension(0));

	/*createExplosion(_vertexBuffer->map(), _vertexBuffer->getDimension(0));*/

	_vertexBuffer->map( osgCompute::MAP_HOST );
}

void Gpu::LayoutModule::acceptResource(osgCompute::Resource& resource)
{
	if( resource.isIdentifiedBy("VERTEX_BUFFER") )
	{
		_vertexBuffer = dynamic_cast<osgCompute::Memory*>( &resource );
	}

	if( resource.isIdentifiedBy("EDGE_BUFFER") )
	{
		_edgeBuffer = dynamic_cast<osgCompute::Memory*>( &resource );
	}
}

void Gpu::LayoutModule::clearLocal()
{
	_vertexBuffer = NULL;
	_edgeBuffer = NULL;
}