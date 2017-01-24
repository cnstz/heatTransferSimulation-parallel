#include <lcutil.h>

__global__ void updateGPU(float *u0, float *u1, int NXPROB, int NYPROB, int N){
	const unsigned int i = blockIdx.x * blockDim.x + threadIdx.x;

	if(i < N)
		if(u0[i] != 0.0)
			u1[i] = u0[i] + 0.1 * (u0[i+NYPROB]+u0[i-NYPROB]-2*u0[i]) + 0.1 * (u0[i+1]+u0[i-1]-2*u0[i]);
}

extern "C" void update(float *u0, float *u1, int NXPROB, int NYPROB){
	float* dev_u0, *dev_u1;

	/* malloc device memory */
	CUDA_SAFE_CALL(cudaMalloc((void**) &dev_u0, NXPROB * NYPROB * sizeof(float)));
	CUDA_SAFE_CALL(cudaMalloc((void**) &dev_u1, NXPROB * NYPROB * sizeof(float)));

	/* Copy from host memory to device memory */
	CUDA_SAFE_CALL(cudaMemcpy(dev_u0, u0, NXPROB * NYPROB * sizeof(float), cudaMemcpyHostToDevice));
	CUDA_SAFE_CALL(cudaMemcpy(dev_u1, u1, NXPROB * NYPROB * sizeof(float), cudaMemcpyHostToDevice));
	
	const int BLOCK_SIZE = 1024;
	dim3 dimBl(BLOCK_SIZE);  
	dim3 dimGr(FRACTION_CEILING((NXPROB * NYPROB), BLOCK_SIZE)); 
	
	/* Update */
	updateGPU<<<dimGr, dimBl>>>(dev_u0, dev_u1, NXPROB, NYPROB, (NXPROB * NYPROB));
	CUDA_SAFE_CALL(cudaGetLastError());
	CUDA_SAFE_CALL(cudaThreadSynchronize());

	/* Copy from device memory to host memory */
	CUDA_SAFE_CALL(cudaMemcpy(u0, dev_u0, NXPROB * NYPROB * sizeof(float), cudaMemcpyDeviceToHost) );
	CUDA_SAFE_CALL(cudaMemcpy(u1, dev_u1, NXPROB * NYPROB * sizeof(float), cudaMemcpyDeviceToHost) );

	/* Free device memory */
	CUDA_SAFE_CALL(cudaFree(dev_u0));
	CUDA_SAFE_CALL(cudaFree(dev_u1));
}

