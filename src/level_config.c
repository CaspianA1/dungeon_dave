#include "level_config.h"
#include "utils/failure.h" // For `FAIL`
#include "utils/texture.h" // For texture creation utils
#include "rendering/entities/billboard.h" // For `Billboard`
#include "utils/opengl_wrappers.h" // For `deinit_texture`

//////////

static void copy_matching_material_to_dest_materials(const GLchar* const texture_path,
	const Dict* const all_materials, packed_material_properties_t* const dest_material_properties,
	const material_index_t dest_index) {

	dest_material_properties[dest_index] = typed_read_from_dict(all_materials, texture_path, string, unsigned_int);
}

//////////

/* List subtypes:
`all_materials` -> MaterialPropertiesPerObjectInstance
`sector_face_texture_paths` -> GLchar*
`still_billboard_texture_paths` -> GLchar*
`billboard_animation_layouts` -> AnimationLayout

Note: this mutates the billboard list by setting each billboard's material index.

This function returns a 1D texture of material lighting properties (each property is a `packed_material_lighting_properties_t`).
This texture is indexed by a material index when shading world-shaded objects.

- For a sector face texture, its material index = its texture id.

Let S = the number of sector face textures,
	B = the number of still billboard textures,
	V = the number of billboard animation layouts (each animation layout is a spritesheet description).

- For a billboard, the material index is a little bit more involved:
	If the billboard is still, its material index = S + its still texture id.
	If the billboard is animated, its material index = S + B + its animation layout id.

- For a weapon sprite, the material index equals S + B + V + its animation layout id among the other weapon sprites. */
MaterialsTexture init_materials_texture(const Dict* const all_materials, const List* const sector_face_texture_paths,
	const List* const still_billboard_texture_paths, const List* const billboard_animation_layouts,
	List* const billboards, const AnimationLayout* const weapon_sprite_animation_layout,
	material_index_t* const weapon_sprite_material_index) {

	////////// Variable initialization
	
	const byte num_sector_face_textures = (byte) sector_face_texture_paths -> length;
	const texture_id_t num_still_billboard_texture_paths = (texture_id_t) still_billboard_texture_paths -> length;

	const texture_id_t num_material_properties = (texture_id_t) (num_sector_face_textures +
		num_still_billboard_texture_paths + billboard_animation_layouts -> length) + 1u; // 1 more for the weapon sprite

	////////// Making a GPU buffer of material lighting properties

	const GLuint material_properties_buffer = init_gpu_buffer();
	use_gpu_buffer(TexBuffer, material_properties_buffer);
	init_gpu_buffer_data(TexBuffer, num_material_properties, sizeof(packed_material_properties_t), NULL, GL_STATIC_DRAW);

	packed_material_properties_t* const material_properties_mapping = init_gpu_buffer_memory_mapping(
		material_properties_buffer, TexBuffer, num_material_properties * sizeof(packed_material_properties_t), true
	);

	////////// First, inserting still face lighting properties

	const GLchar* const* const typed_sector_face_texture_paths = sector_face_texture_paths -> data;

	for (byte i = 0; i < num_sector_face_textures; i++)
		copy_matching_material_to_dest_materials(typed_sector_face_texture_paths[i],
			all_materials, material_properties_mapping, i);

	////////// Then, inserting billboard lighting properties

	material_index_t last_dest_material_index = 0u;
	const Billboard* const first_billboard = billboards -> data;

	LIST_FOR_EACH(billboards, Billboard, billboard,
		const texture_id_t billboard_texture_id = billboard -> texture_id; // TODO: rename `texture_id` to `texture_index`

		////////// Finding the texture path and the dest material index (for the output list) for the current billboard
	
		const GLchar* texture_path = NULL;
		material_index_t dest_material_index = num_sector_face_textures;

		/* This works because within a given texture set, the still textures always precede the slices of animated frames.
		If the texture id here is less than the number of still texture paths, that texture id must correspond to
		a still texture. Therefore, this texture id can be used as an index into the still billboard texture paths. */
		if (billboard_texture_id < num_still_billboard_texture_paths) {
			texture_path = *(GLchar**) ptr_to_list_index(still_billboard_texture_paths, billboard_texture_id);
			dest_material_index += billboard_texture_id;
		}
		else {
			texture_id_t frame_index_start = num_still_billboard_texture_paths;

			/* The animation layout array is naturally ordered because it also specifies an order
			of which textures will be placed into their set - so this frame index math is valid. */
			LIST_FOR_EACH(billboard_animation_layouts, AnimationLayout, animation_layout,
				const texture_id_t next_frame_index_start = frame_index_start + animation_layout -> total_frames;

				if (billboard_texture_id >= frame_index_start && billboard_texture_id < next_frame_index_start) {
					const billboard_index_t layout_index = (billboard_index_t) (animation_layout -
						(AnimationLayout*) (billboard_animation_layouts -> data));

					texture_path = animation_layout -> spritesheet_path;
					dest_material_index += num_still_billboard_texture_paths + layout_index;
					break;
				}
				frame_index_start = next_frame_index_start;
			);

			if (texture_path == NULL) FAIL(InvalidTextureID,
				"Billboard #%lld has an invalid texture ID of %hu",
				(long long) (billboard - first_billboard + 1),
				billboard_texture_id
			);
		}

		billboard -> material_index = dest_material_index;

		const bool skipping_material_lookup_and_copy =
			last_dest_material_index == dest_material_index &&
			billboard != first_billboard; // No skipping for the first billboard

		if (!skipping_material_lookup_and_copy) {
			copy_matching_material_to_dest_materials(texture_path, all_materials,
				material_properties_mapping, dest_material_index);

			last_dest_material_index = dest_material_index;
		}
	);

	////////// Then, inserting weapon sprite lighting properties

	*weapon_sprite_material_index = num_material_properties - 1u;

	copy_matching_material_to_dest_materials(weapon_sprite_animation_layout -> spritesheet_path,
		all_materials, material_properties_mapping, *weapon_sprite_material_index);
	
	////////// Making the materials texture buffer

	deinit_gpu_buffer_memory_mapping(TexBuffer);

	GLuint materials_texture;
	glGenTextures(1, &materials_texture);
	use_texture(TexBuffer, materials_texture);
	glTexBuffer(TexBuffer, OPENGL_MATERIALS_MAP_INTERNAL_PIXEL_FORMAT, material_properties_buffer);

	//////////


	return (MaterialsTexture) {material_properties_buffer, materials_texture};
}

void deinit_materials_texture(const MaterialsTexture* const materials_texture) {
	deinit_texture(materials_texture -> buffer_texture);
	deinit_gpu_buffer(materials_texture -> material_properties_buffer);
}
