__kernel void maxprefix(__global const float a,
		__global float sums,
		__global float prefs) {
	int work_group_size = get_local_size(0);
	__local float al[work_group_size];
	
	int i = get_global_id(0);
	int local_i = get_local_id(0);
	al[local_i] = a[i];
	
	barrier(CLK_GLOBAL_MEM_FENCE);
	if (local_i == 0) {
		g_id = get_group_id(0);
		for (int ind = 0; ind < work_group_size; ++ind) {	
			sum[g_id] += al[ind];
			if (sum[g_id] > prefs[g_id]) {
				prefs[g_id] = sum[g_id];
			}
		}
	}
}
