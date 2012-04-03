/*!
 * LayoutKernel.cu
 * Projekt 3DVisual
 */
 
#ifndef GPU_LAYOUTKERNEL
#define GPU_LAYOUTKERNEL 1

#include <stdio.h>

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DEVICE FUNCTIONS //////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
inline __device__ 
float3 operator+(float3 a, float3 b)
{
    return make_float3(a.x + b.x, a.y + b.y, a.z + b.z);
}

//------------------------------------------------------------------------------
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
void layoutKernel( float3* positions )
{
    unsigned int ptclIdx = thIdx();

    // perform a euler step
	positions[ptclIdx] = positions[ptclIdx] + make_float3(0,0.5f,0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HOST FUNCTIONS ////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------

//checkCudaError
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
void computeLayout( unsigned int numBlocks, unsigned int numThreads, void* positions )
{
    dim3 blocks( numBlocks, 1, 1 );
    dim3 threads( numThreads, 1, 1 );

    layoutKernel<<< blocks, threads >>>(reinterpret_cast<float3*>(positions));
//	checkCudaError("Kernel Execution Failed!");
}

#endif
