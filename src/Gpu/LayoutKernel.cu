/*!
 * LayoutKernel.cu
 * Projekt 3DVisual
 */
 
#ifndef GPU_LAYOUTKERNEL
#define GPU_LAYOUTKERNEL 1

#include <math.h>
#include <stdio.h>

#define NUM_REPULSE_THREADS 32
#define NUM_ATTRACT_THREADS 32

__constant__ float calmEdgeLength;
__constant__ float alpha = 0.005;
__constant__ float maxMovement = 30;

texture<float4, cudaTextureType1D, cudaReadModeElementType> texVertices;
texture<uint2, cudaTextureType1D, cudaReadModeElementType> texEdges;
texture<float4, cudaTextureType1D, cudaReadModeElementType> texForces;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DEVICE FUNCTIONS //////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
inline __device__ 
float4 operator+(float4 a, float4 b)
{
    return make_float4(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
}

inline __device__ 
float4 operator*(float4 a, float b)
{
    return make_float4(a.x * b, a.y * b, a.z * b, a.w * b);
}

inline __device__ 
float3 vertexVertexRepulsion(float4 u, float4 v, float3 fv)
{
	float3 r;  
	// r_uv  [3 FLOPS]  
	r.x = v.x - u.x;  
	r.y = v.y - u.y;  
	r.z = v.z - u.z;  
	// distSqr = dot(r_ij, r_ij) [5 FLOPS] 
	float distSqr = r.x * r.x + r.y * r.y + r.z * r.z;
	// distSqr = distSqr + small number [1 FLOP] -  to avoid division by zero
	distSqr = distSqr + 0.01f;
	// repForce =-k^2/distSqr  [3 FLOPS (2 mul, 1 inv)]  
	float repForce = -1.0f * calmEdgeLength * calmEdgeLength / distSqr;   
	// a_i =  a_i + s * r_ij [6 FLOPS]  
	fv.x += r.x * repForce;  
	fv.y += r.y * repForce;  
	fv.z += r.z * repForce;

	return fv;  
}

inline __device__ 
float3 tile_calculation(float4 vertexPosition, float3 fv)  
{   
	extern __shared__ float4 shPosition[];
	for (int i = 0; i < blockDim.x; i++) 
	{  
		fv = vertexVertexRepulsion(vertexPosition, shPosition[i], fv);  
	}  
	return fv;  
}

inline __device__ 
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
    extern __shared__ float4 shPosition[];  

	int vertexId = blockIdx.x * blockDim.x + threadIdx.x;  
	float4 vertexPosition = vertices[vertexId];  
	float3 fv = {0.0f, 0.0f, 0.0f};
	for (int i = 0, tile = 0; i < numVertices; i += NUM_REPULSE_THREADS, tile++) 
	{  
		int idx = tile * blockDim.x + threadIdx.x;  
		shPosition[threadIdx.x] = vertices[idx];  
		__syncthreads();  
		fv = tile_calculation(vertexPosition, fv);  
		__syncthreads();  
	}  
	// save the result in global memory for the integration step.  
	forceVectors[vertexId] = make_float4(fv.x, fv.y, fv.z, 0.0f);
}

__global__
void attractKernel( float4* forceVectors )
{
	//TODO: what happend if edge idx is out of range
    unsigned int edgeIdx = blockDim.x * blockIdx.x + threadIdx.x;
	uint2 edge = tex1Dfetch(texEdges, edgeIdx);

	float4 u = tex1Dfetch(texVertices, edge.x);
	float4 v = tex1Dfetch(texVertices, edge.y);

	float3 fv = edgeAttraction(u, v);

	atomicAdd(&(forceVectors[edge.x].x), fv.x);
	atomicAdd(&(forceVectors[edge.x].y), fv.y);
	atomicAdd(&(forceVectors[edge.x].z), fv.z);

	atomicAdd(&(forceVectors[edge.y].x), 0 - fv.x);
	atomicAdd(&(forceVectors[edge.y].y), 0 - fv.y);
	atomicAdd(&(forceVectors[edge.y].z), 0 - fv.z);

}

__global__
void applyKernel( float4* vertices )
{
	unsigned int vertexIdx = blockDim.x * blockIdx.x + threadIdx.x;
	float4 force = tex1Dfetch(texForces, vertexIdx) * alpha;
	float length = sqrtf((force.x * force.x) + (force.y * force.y) + (force.z * force.z));

	if(length > maxMovement)
	{
		force.x = force.x / length;
		force.y = force.y / length;
		force.z = force.z / length;
		force = force * 5;
	}

	vertices[vertexIdx] = vertices[vertexIdx] + force;
}

__global__
void explosionKernel( float4* nodes )
{
    unsigned int ptclIdx = blockDim.x * blockIdx.x + threadIdx.x;

	float4 node =  nodes[ptclIdx];
	float length = sqrtf(powf(node.x,2) + powf(node.y,2) + powf(node.z,2));
  
	nodes[ptclIdx] = node + make_float4(node.x / length, node.y / length, node.z / length, 0.0f);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HOST FUNCTIONS ////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------

void checkCudaError(const char* message);
float computeCalm(unsigned int numVertices, float sizeFactor);

extern "C" __host__
void initKernelConstants(unsigned int numVertices, float sizeFactor)
{
	float calmEdgeLength = computeCalm(numVertices, sizeFactor);
	cudaMemcpyToSymbol("calmEdgeLength", &calmEdgeLength, sizeof(float));
}

float computeCalm(unsigned int numVertices, float sizeFactor)
{
	float R = 30;
	float PI = acos((float) - 1);

	return sizeFactor * pow((4 * PI * R * R * R)/(numVertices * 3), (float) 1/3);
}

extern "C" __host__
void computeLayout(void* vertexBuffer, unsigned int vertexBufferSize,
				   void* edgeBuffer, unsigned int edgeBufferSize)
{
	//init
	unsigned int numBlocks, numThreads;
	dim3 blocks, threads;

	float4* forceVectorBuffer;
	size_t forceVectorSize = vertexBufferSize * sizeof(float4);
	cudaMalloc(&forceVectorBuffer, forceVectorSize);


	//repulse kernel
    numThreads = NUM_REPULSE_THREADS;
	numBlocks = (unsigned int) ceil((float) vertexBufferSize / (float) numThreads);
	unsigned int sharedMemSize = numThreads * sizeof(float4);

	//printf("Repulse - blocks: %d threads: %d\n", numBlocks, numThreads);
	
	blocks = dim3( numBlocks, 1, 1 );
	threads = dim3( numThreads, 1, 1 );

    repulseKernel<<< blocks, threads, sharedMemSize >>>(reinterpret_cast<float4*>(vertexBuffer), forceVectorBuffer, vertexBufferSize);

	//attract kernel
	numThreads = NUM_ATTRACT_THREADS;
	numBlocks = (unsigned int) ceil((float) edgeBufferSize / (float) (numThreads * 2));

	//printf("Attract - blocks: %d threads: %d\n", numBlocks, numThreads);

	blocks = dim3( numBlocks, 1, 1 );
	threads = dim3( numThreads, 1, 1 );

	cudaBindTexture(0, texVertices, reinterpret_cast<float4*>(vertexBuffer), vertexBufferSize * sizeof(float4));
	cudaBindTexture(0, texEdges, reinterpret_cast<uint2*>(edgeBuffer), (edgeBufferSize / 2) * sizeof(uint2));

	attractKernel<<< blocks, threads >>>(forceVectorBuffer);

	cudaUnbindTexture(texVertices);
	cudaUnbindTexture(texEdges);

	//apply forces
	numThreads = NUM_REPULSE_THREADS;
	numBlocks = (unsigned int) ceil((float) vertexBufferSize / (float) numThreads);

	//printf("Apply - blocks: %d threads: %d\n", numBlocks, numThreads);
	
	blocks = dim3( numBlocks, 1, 1 );
	threads = dim3( numThreads, 1, 1 );

	cudaBindTexture(0, texForces, reinterpret_cast<float4*>(forceVectorBuffer), vertexBufferSize * sizeof(float4));

    applyKernel<<< blocks, threads >>>(reinterpret_cast<float4*>(vertexBuffer));

	cudaUnbindTexture(texForces);

	checkCudaError("Kernel Execution Failed!");
}

extern "C" __host__
void createExplosion(void* nodes, unsigned int numNodes)
{
	unsigned int numThreads = 128;
	unsigned int numBlocks = (unsigned int) ceil((float) numNodes / (float) numThreads);
	
    dim3 blocks( numBlocks, 1, 1 );
    dim3 threads( numThreads, 1, 1 );

    explosionKernel<<< blocks, threads >>>(reinterpret_cast<float4*>(nodes));
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

#endif
