static const double actor_box_side_len = 0.4; // Actor = player or thing

#define DEF_BOUNDING_BOX_TYPE(name, suffix, vec_type, vec_type_filler, subtype, num_axes)\
\
typedef struct {const vec_type origin, size;} name##suffix; /* Origin for left = top-left corner */ \
\
byte aabb_axis_collision##suffix(const name##suffix a, const name##suffix b, const byte axis) {\
	return (a.origin[axis] < b.origin[axis] + b.size[axis]) && (a.origin[axis] + a.size[axis] > b.origin[axis]);\
}\
\
byte aabb_collision##suffix(const name##suffix a, const name##suffix b) {\
	byte axes_hit = 0;\
	for (byte axis = 0; axis < num_axes; axis++) {\
		if (aabb_axis_collision##suffix(a, b, axis)) axes_hit++;\
	}\
	return axes_hit == num_axes;\
}\
inlinable name##suffix init_bounding_box##suffix(const vec_type pos, const subtype size) {\
	return (name##suffix) {pos - size * vec_type_filler(0.5), vec_type_filler(size)};\
}\

DEF_BOUNDING_BOX_TYPE(BoundingBox, , vec, vec_fill, double, 2)
DEF_BOUNDING_BOX_TYPE(BoundingBox, _3D, vec3D, vec_fill_3D, float, 3)

inlinable BoundingBox_3D init_actor_bounding_box(const vec pos_2D, const double height) {
	const BoundingBox part_2D = init_bounding_box(pos_2D, actor_box_side_len);

	return (BoundingBox_3D) {
		{part_2D.origin[0], part_2D.origin[1], height},
		{part_2D.size[0], part_2D.size[1], actor_height}
	};
}
