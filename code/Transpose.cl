
__kernel void transpose1(__global float *a,
						__global float *at,
						unsigned int m, unsigned int n)
{
    int i = get_global_id(0);
	int j = get_global_id(1);
	
	float x = a[j * k + i];
    at[i * m + j] = x;
}
