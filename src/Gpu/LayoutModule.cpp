/*!
 * LayoutModule.cpp
 * Projekt 3DVisual
 */
#include <QDebug>
#include "Gpu/LayoutModule.h"
#include "Util/ApplicationConfig.h"

extern "C"
void initKernelConstants(float alphaValue, float minMovementValue, float maxMovementValue, 
						 float flexibilityValue, float sizeFactor, unsigned int numVertices);

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

	this->initAlgorithmParameters();
	return osgCompute::Module::init();
}

void Gpu::LayoutModule::initAlgorithmParameters()
{
	float alpha = Util::ApplicationConfig::get()->getValue("Gpu.LayoutAlgorithm.Alpha").toFloat();
	float minMovement = Util::ApplicationConfig::get()->getValue("Gpu.LayoutAlgorithm.MinMovement").toFloat();
	float maxMovement = Util::ApplicationConfig::get()->getValue("Gpu.LayoutAlgorithm.MaxMovement").toFloat();
	float flexibility = Util::ApplicationConfig::get()->getValue("Gpu.LayoutAlgorithm.Flexibility").toFloat();
	float sizeFactor = Util::ApplicationConfig::get()->getValue("Gpu.LayoutAlgorithm.GraphSize").toFloat();
	initKernelConstants(alpha, minMovement, maxMovement, flexibility, sizeFactor, _vertexBuffer->getDimension(0));
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