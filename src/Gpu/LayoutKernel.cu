/*!
 * LayoutKernel.cu
 * Projekt 3DVisual
 */
 
#ifndef GPU_LAYOUTKERNEL
#define GPU_LAYOUTKERNEL 1

#include <stdio.h>
#include <math.h>

#define NUM_THREADS 256 

__constant__ float alpha;
__constant__ float minMovement;
__constant__ float maxMovement;
__constant__ float flexibility;
__constant__ float calmEdgeLength;

texture<float4, cudaTextureType1D, cudaReadModeElementType> texVertices;
texture<uint2, cudaTextureType1D, cudaReadModeElementType> texEdges;
texture<float4, cudaTextureType1D, cudaReadModeElementType> texForces;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DEVICE FUNCTIONS //////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
__device__ 
float4 operator+(float4 a, float4 b)
{
    return make_float4(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
}

__device__ 
float4 operator*(float4 a, float b)
{
    return make_float4(a.x * b, a.y * b, a.z * b, a.w * b);
}

__device__
float3 vertexVertexRepulsion(float4 u, float4 v, float3 fv)
{
	float3 r;  
	// r_uv  [3 FLOPS]  
	r.x = v.x - u.x;  
	r.y = v.y - u.y;  
	r.z = v.z - u.z;
	// distSqr = dot(r_ij, r_ij) [5 FLOPS] 
	float distSqr = r.x * r.x + r.y * r.y + r.z * r.z;
	// repForce = -k^2/distSqr  [4 FLOPS (2 mul, 1 add, 1 inv)]  
	float repForce = -1.0f * calmEdgeLength * calmEdgeLength / (distSqr + 0.0001f);  
	// ignore meta nodes
	unsigned int metaFlag = ((unsigned int) v.w) & 1;
	repForce *= (metaFlag ^ 1);
	// a_i =  a_i + s * r_ij [6 FLOPS]  
	fv.x += r.x * repForce;  
	fv.y += r.y * repForce;  
	fv.z += r.z * repForce;

	return fv;  
}

__device__
float3 tile_calculation(float4 vertexPosition, float3 fv, unsigned int arrayBound)  
{   
	extern __shared__ float4 shPosition[];
	#pragma unroll 32
	for (unsigned int i = 0; i < arrayBound; i++) 
	{  
		fv = vertexVertexRepulsion(vertexPosition, shPosition[i], fv); 
	}  
	return fv;  
}

__device__
float3 edgeAttraction(float4 u, float4 v)
{
	float3 fv;  
	// r_uv  [3 FLOPS]  
	fv.x = v.x - u.x;  
	fv.y = v.y - u.y;  
	fv.z = v.z - u.z;   

	// dist [6 FLOPS (3 mul, 2 add, 1sqrt)]
	float dist = sqrtf(fv.x * fv.x + fv.y * fv.y + fv.z * fv.z);
	// attrForce = dist / calmEdgeLength [1 FLOP] 
	float attrForce = dist / calmEdgeLength;

	// fv =  fv * attrForce [3 FLOPS]  
	fv.x = fv.x * attrForce;  
	fv.y = fv.y * attrForce;  
	fv.z = fv.z * attrForce;

	return fv;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS //////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
__global__ 
void repulseKernel( float4* vertices, float4* forceVectors, unsigned int numVertices )
{
	int vertexIdx = blockIdx.x * blockDim.x + threadIdx.x; 
	if(vertexIdx >= numVertices)
	{
		return; 
	}

	extern __shared__ float4 shPosition[]; 
	float4 vertexPosition = vertices[vertexIdx];  
	float3 fv = {0.0f, 0.0f, 0.0f};
	for (int tile = 0; tile * blockDim.x < numVertices; tile++) 
	{
		shPosition[threadIdx.x] = tile * blockDim.x + threadIdx.x < numVertices ? vertices[tile * blockDim.x + threadIdx.x] : vertexPosition;
		__syncthreads();  
		unsigned int arrayBound = (numVertices - tile * blockDim.x) > blockDim.x ? blockDim.x : (numVertices - tile * blockDim.x);
		fv = tile_calculation(vertexPosition, fv, arrayBound);  
		__syncthreads();  
	}  
	// save the result in global memory for apply kernel  
	forceVectors[vertexIdx] = make_float4(fv.x, fv.y, fv.z, 0.0f);
}

__global__
void attractKernel( float4* forceVectors, unsigned int numEdges )
{
    unsigned int edgeIdx = blockDim.x * blockIdx.x + threadIdx.x;
	if(edgeIdx >= numEdges)
	{
		return; 
	}

	uint2 edge = tex1Dfetch(texEdges, edgeIdx);

	float4 u = tex1Dfetch(texVertices, edge.x);
	float4 v = tex1Dfetch(texVertices, edge.y);

	float3 fv = edgeAttraction(u, v);

	atomicAdd(&(forceVectors[edge.x].x), fv.x);
	atomicAdd(&(forceVectors[edge.x].y), fv.y);
	atomicAdd(&(forceVectors[edge.x].z), fv.z);

	atomicAdd(&(forceVectors[edge.y].x), fv.x * -1);
	atomicAdd(&(forceVectors[edge.y].y), fv.y * -1);
	atomicAdd(&(forceVectors[edge.y].z), fv.z * -1);

}

__global__
void applyKernel( float4* vertices, unsigned int numVertices, float4* velocities )
{
	unsigned int vertexIdx = blockDim.x * blockIdx.x + threadIdx.x;
	if(vertexIdx >= numVertices)
	{
		return;
	}

	float4 force = tex1Dfetch(texForces, vertexIdx) * alpha;
	float length = sqrtf((force.x * force.x) + (force.y * force.y) + (force.z * force.z));

	force.x = force.x / length;
	force.y = force.y / length;
	force.z = force.z / length;
	
	float optimalLength = length < maxMovement ?  length : maxMovement;
	force = length > minMovement ? (force * optimalLength) + velocities[vertexIdx] : make_float4(0.0f, 0.0f, 0.0f, 0.0f);

	unsigned int fixedFlag = ((unsigned int) vertices[vertexIdx].w) >> 1 & 1;
	force = force * (fixedFlag ^ 1);

	vertices[vertexIdx] = vertices[vertexIdx] + force;
	velocities[vertexIdx] = force * flexibility;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HOST FUNCTIONS ////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------

float computeCalm(unsigned int numVertices, float sizeFactor)
{
	float R = 300;
	float PI = acos((float) - 1);

	return sizeFactor * pow((4 * PI * R * R * R)/(numVertices * 3), (float) 1/3);
}

extern "C" __host__
void initKernelConstants(float alphaValue, float minMovementValue, float maxMovementValue, float flexibilityValue, float sizeFactor, unsigned int numVertices)
{
	cudaMemcpyToSymbol(alpha, &alphaValue, sizeof(float));
	cudaMemcpyToSymbol(minMovement, &minMovementValue, sizeof(float));
	cudaMemcpyToSymbol(maxMovement, &maxMovementValue, sizeof(float));
	cudaMemcpyToSymbol(flexibility, &flexibilityValue, sizeof(float));

	float calmEdgeLengthValue = computeCalm(numVertices, sizeFactor);
	cudaMemcpyToSymbol(calmEdgeLength, &calmEdgeLengthValue, sizeof(float));
}

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
void computeLayout(void* vertexBuffer, unsigned int vertexBufferSize, void* velocityBuffer,
				   void* edgeBuffer, unsigned int edgeBufferSize)
{
	//init
	unsigned int numBlocks, numThreads;
	dim3 blocks, threads;

	float4* forceVectorBuffer;
	size_t forceVectorSize = vertexBufferSize * sizeof(float4);
	cudaMalloc(&forceVectorBuffer, forceVectorSize);

	//repulse kernel
    numThreads = NUM_THREADS;
	numBlocks = (unsigned int) ceil((float) vertexBufferSize / (float) numThreads);
	unsigned int sharedMemSize = numThreads * sizeof(float4);
	
	blocks = dim3( numBlocks, 1, 1 );
	threads = dim3( numThreads, 1, 1 );

    repulseKernel<<< blocks, threads, sharedMemSize >>>(reinterpret_cast<float4*>(vertexBuffer), forceVectorBuffer, vertexBufferSize);

	//attract kernel
	unsigned int numEdges = edgeBufferSize / 2;
	numThreads = NUM_THREADS;
	numBlocks = (unsigned int) ceil((float) numEdges / (float) numThreads);

	blocks = dim3( numBlocks, 1, 1 );
	threads = dim3( numThreads, 1, 1 );

	cudaBindTexture(0, texVertices, reinterpret_cast<float4*>(vertexBuffer), vertexBufferSize * sizeof(float4));
	cudaBindTexture(0, texEdges, reinterpret_cast<uint2*>(edgeBuffer), numEdges * sizeof(uint2));

	attractKernel<<< blocks, threads >>>(forceVectorBuffer, numEdges);

	cudaUnbindTexture(texVertices);
	cudaUnbindTexture(texEdges);

	//apply forces
	numThreads = NUM_THREADS;
	numBlocks = (unsigned int) ceil((float) vertexBufferSize / (float) numThreads);
	
	blocks = dim3( numBlocks, 1, 1 );
	threads = dim3( numThreads, 1, 1 );

	cudaBindTexture(0, texForces, reinterpret_cast<float4*>(forceVectorBuffer), vertexBufferSize * sizeof(float4));

    applyKernel<<< blocks, threads >>>(reinterpret_cast<float4*>(vertexBuffer), vertexBufferSize, reinterpret_cast<float4*>(velocityBuffer));

	cudaUnbindTexture(texForces);

	//cleanup
	cudaFree(forceVectorBuffer);
	checkCudaError("Kernel Execution Failed!");
}

#endif
