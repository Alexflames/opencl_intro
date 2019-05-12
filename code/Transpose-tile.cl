#define TILE_SIZE 32
__kernel void transpose2(__global float *a,
						__global float *at,
						unsigned int m, unsigned int n)
{
	int i = get_global_id(0);
	int j = get_global_id(1);
	
    _local float tile[TILE_SIZE][TILE_SIZE];
	int local_i = get_local_id(0);
	int local_j = get_local_id(1);
	
	tile[local_j][local_i] = a[j * k + i]
	
	float tmp = tile[local_j][i];
	tile[local_j][local_i] = tile[local_i][local_j];
	tile[local_i][local_j] = tmp;
	
	at[i * m + j] = tile[j * TILE_SIZE][i];
}
