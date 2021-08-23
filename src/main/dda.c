// this could really use templates

#define BIGGEST_IND_OF_2(a) a[0] >= a[1]

inlinable byte biggest_ind_of_3(const double a[3]) {
	if (a[0] >= a[1] && a[0] >= a[2]) return 1;
	else if (a[1] >= a[0] && a[1] >= a[2]) return 2;
	else return 3;
}

#define UNPACK_2(v) {v[0], v[1]}
#define APPLY_2(v, f) {f(v[0]), f(v[1])}

#define UNPACK_3(v) {v[0], v[1], v[2]}
#define APPLY_3(v, f) {f(v[0]), f(v[1]), f(v[2])}

#define DDA_DEF(dimensions, typename, init_fn, peek_fn, iter_fn, applier, unpacker, component_cmp)\
\
typedef struct {\
	byte step_count, side;\
	double dist;\
	const double origin[dimensions], dir[dimensions], unit_step_size[dimensions], ray_step[dimensions];\
	double ray_length[dimensions], curr_tile[dimensions];\
} typename;\
\
typename init_fn(const double origin[dimensions], const double dir[dimensions], const double step) {\
	const double curr_tile[dimensions] = applier(origin, floor);\
	double unit_step_size[dimensions], ray_length[dimensions], ray_step[dimensions];\
\
	for (byte i = 0; i < dimensions; i++) {\
		unit_step_size[i] = fabs(step / dir[i]);\
\
		if (dir[i] < 0.0) {\
			ray_step[i] = -step;\
			ray_length[i] = (origin[i] - curr_tile[i]) * unit_step_size[i];\
		}\
		else {\
			ray_step[i] = step;\
			ray_length[i] = (curr_tile[i] + 1.0 - origin[i]) * unit_step_size[i];\
		}\
	}\
	return (typename) {\
		0, 0, 0.0, unpacker(origin), unpacker(dir), unpacker(unit_step_size),\
		unpacker(ray_step), unpacker(ray_length), unpacker(curr_tile)\
	};\
}\
\
inlinable typename peek_fn(typename d) {\
	d.side = component_cmp(d.ray_length);\
\
	d.dist = d.ray_length[d.side];\
	d.curr_tile[d.side] += d.ray_step[d.side];\
	d.ray_length[d.side] += d.unit_step_size[d.side];\
\
	return d;\
}\
\
inlinable byte iter_fn(DataDDA* const d_ref) {\
	typename d = peek_dda(*d_ref);\
	if (ivec_out_of_bounds((ivec) unpacker(d.curr_tile))) return 0;\
\
	d.step_count++;\
	memcpy(d_ref, &d, sizeof(DataDDA));\
	return 1;\
}

DDA_DEF(2, DataDDA, init_dda, peek_dda, iter_dda, APPLY_2, UNPACK_2, BIGGEST_IND_OF_2)

// line 50: out of bounds checker
