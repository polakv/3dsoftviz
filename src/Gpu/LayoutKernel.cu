/*!
 * LayoutKernel.cu
 * Projekt 3DVisual
 */
 
#ifndef GPU_LAYOUTKERNEL
#define GPU_LAYOUTKERNEL 1

#include <math.h>
#include <stdio.h>

texture<uint2, cudaTextureType1D, cudaReadModeElementType> texEdgeIndexes;
texture<uint1, cudaTextureType1D, cudaReadModeElementType> texEdgeValues;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DEVICE FUNCTIONS //////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
inline __device__ 
float3 operator+(float3 a, float3 b)
{
    return make_float3(a.x + b.x, a.y + b.y, a.z + b.z);
}

inline __device__ 
float vectorLength(float3 position)
{
    return sqrtf(powf(position.x,2) + powf(position.y,2) + powf(position.z,2));
}


inline __device__
unsigned int thIdx()
{
    unsigned int x = blockIdx.x * blockDim.x + threadIdx.x;
    unsigned int y = blockIdx.y * blockDim.y + threadIdx.y;
    unsigned int width = gridDim.x * blockDim.x;

    return y*width + x;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS //////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
__global__
void layoutKernel( float3* nodes )
{
    unsigned int ptclIdx = thIdx();

	uint2 edgeIndex = tex1Dfetch(texEdgeIndexes, ptclIdx);
	uint1 edgeValue = tex1Dfetch(texEdgeValues, edgeIndex.x);

	float3 node =  nodes[ptclIdx];
	float length = vectorLength(node);
  
	nodes[ptclIdx] = node + make_float3(node.x / length, node.y / length, node.z / length);
}

__global__
void explosionKernel( float3* nodes )
{
    unsigned int ptclIdx = thIdx();

	float3 node =  nodes[ptclIdx];
	float length = vectorLength(node);
  
	nodes[ptclIdx] = node + make_float3(node.x / length, node.y / length, node.z / length);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HOST FUNCTIONS ////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------

void checkCudaError(const char* message) 
{
	cudaError_t error = cudaGetLastError();
	if (error != cudaSuccess)
	{
		fprintf(stderr, "CUDA error. %s. %s.\n", message, cudaGetErrorString(error));
		exit(EXIT_FAILURE);
	}
}

extern "C" __host__
void computeLayout( unsigned int numBlocks, unsigned int numThreads, void* nodes, void* edgeIndexes, unsigned int edgeIndexesSize, void* edgeValues, unsigned int edgeValuesSize)
{
    dim3 blocks( numBlocks, 1, 1 );
    dim3 threads( numThreads, 1, 1 );

	cudaBindTexture(0, texEdgeIndexes, reinterpret_cast<uint2*>(edgeIndexes), (edgeIndexesSize / 2) * sizeof(uint2));
	cudaBindTexture(0, texEdgeValues, reinterpret_cast<uint1*>(edgeValues), edgeValuesSize * sizeof(uint1));

    layoutKernel<<< blocks, threads >>>(reinterpret_cast<float3*>(nodes));

	cudaUnbindTexture(texEdgeIndexes);
	cudaUnbindTexture(texEdgeValues);

	//checkCudaError("Kernel Execution Failed!");
}

extern "C" __host__
void createExplosion( unsigned int numBlocks, unsigned int numThreads, void* nodes)
{
    dim3 blocks( numBlocks, 1, 1 );
    dim3 threads( numThreads, 1, 1 );

    explosionKernel<<< blocks, threads >>>(reinterpret_cast<float3*>(nodes));
}

#endif
