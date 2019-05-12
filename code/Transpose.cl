
__kernel void transpose1(__global float *a,
						__global float *at,
						unsigned int m, unsigned int n)
{
    int gid = get_global_id(0);

    result[gid] = a[gid] + b[gid];
}
