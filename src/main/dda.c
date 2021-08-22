#define DDA_DEF(dimensions)\
\
typedef struct {\
	byte step_count, side;\
	double dist;\
	const double origin[dimensions], dir[dimensions], unit_step_size[dimensions], ray_step[dimensions];\
	double ray_length[dimensions], curr_tile[dimensions];\
} DataDDA;\
\
DataDDA init_dda(const vec origin, const vec dir, const double step) {\
	const double unit_step_size[dimensions] = {fabs(step / dir[0]), fabs(step / dir[1])};\
	double ray_length[dimensions];\
	const double curr_tile[2] = {floor(origin[0]), floor(origin[1])};\
	double ray_step[dimensions];\
\
	for (byte i = 0; i < dimensions; i++) {\
		if (dir[i] < 0.0) {\
			ray_step[i] = -step;\
			ray_length[i] = (origin[i] - curr_tile[i]) * unit_step_size[i];\
		}\
		else {\
			ray_step[i] = step;\
			ray_length[i] = (curr_tile[i] + 1.0 - origin[i]) * unit_step_size[i];\
		}\
	}\
	return (DataDDA) {\
		0, 0, 0.0, {origin[0], origin[1]}, {dir[0], dir[1]},\
		{unit_step_size[0], unit_step_size[1]}, {ray_step[0], ray_step[1]},\
		{ray_length[0], ray_length[1]}, {curr_tile[0], curr_tile[1]}\
	};\
}\
\
inlinable DataDDA peek_dda(DataDDA d) {\
	d.side = d.ray_length[0] >= d.ray_length[1];\
\
	d.dist = d.ray_length[d.side];\
	d.curr_tile[d.side] += d.ray_step[d.side];\
	d.ray_length[d.side] += d.unit_step_size[d.side];\
\
	return d;\
}\
\
inlinable byte iter_dda(DataDDA* const d_ref) {\
	DataDDA d = peek_dda(*d_ref);\
	if (ivec_out_of_bounds((ivec) {d.curr_tile[0], d.curr_tile[1]})) return 0;\
\
	d.step_count++;\
	memcpy(d_ref, &d, sizeof(DataDDA));\
	return 1;\
}

DDA_DEF(2)

/* line 4: 3 different sides
line 11 and 13: vec_apply
lines 26 to 29: double[n] unpacker
line 34: compare 3 ray lengths
line 45: pass in a fn, along with a double[n] unpacker */
