
__kernel void matmul1(__global float *a,
		__global float *b,
		__global float *c,
		unsigned int M, unsigned int K, unsigned int N)
{
    int i = get_global_id(0);
	int j = get_global_id(1);
	
	float sum = 0.0f;
	for (int k = 0; k < K; k++) {
		sum += a[j * K + k] * b[k * N + i];
	}
	c[j * N * i] = sum;
}
