#define TILE_SIZE 32
__kernel void matmul2(__global float *a,
		__global float *b,
		__global float *c,
		unsigned int M, unsigned int K, unsigned int N)
{
    int i = get_global_id(0);
	int j = get_global_id(1);
	int local_i = get_local_id(0);
	int local_j = get_local_id(1);
	__local float tileA[TILE_SIZE][TILE_SIZE];
	__local float tileB[TILE_SIZE][TILE_SIZE];
	
	float sum = 0.0f;
	for (int tileK = 0; tileK * TILE_SIZE < K; tileK++) {
		tileA[local_j][local_i] = a[j * K + (tileK * TILE_SIZE + local_i)];
		tileB[local_j][local_i] = b[j * K + (tileK * TILE_SIZE + local_i)];
		barrier(CLK_LOCAL_MEM_FENCE);
		for (int k = 0; k < TILE_SIZE; k++) {
			sum += tileA[local_j * TILE_SIZE][k] 
					* tileB[local_i * TILE_SIZE][k];
		}
	}
	c[j * N * i] = sum;
}
