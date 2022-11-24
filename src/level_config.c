#include "level_config.h"
#include "utils/failure.h" // For `FAIL`
#include "utils/texture.h" // For texture creation utils
#include "data/constants.h" // For `max_byte_value`
#include "rendering/entities/billboard.h" // For `Billboard`

void validate_all_materials(const List* const all_materials) {
	#define VALIDATE_MATERIAL_PROPERTY_RANGE(property) do {\
		if (material.lighting.property < 0.0f || material.lighting.property > 1.0f)\
			FAIL(InitializeMaterial,\
				"Material property '" #property "' for texture path '%s' "\
				"is %g, and outside of the expected 0-1 range", material.albedo_texture_path,\
				(GLdouble) material.lighting.property);\
	} while (false)

	LIST_FOR_EACH(0, all_materials, untyped_material,
		const MaterialPropertiesPerObjectInstance material =
			*(MaterialPropertiesPerObjectInstance*) untyped_material;	

		VALIDATE_MATERIAL_PROPERTY_RANGE(metallicity);
		VALIDATE_MATERIAL_PROPERTY_RANGE(min_roughness);
		VALIDATE_MATERIAL_PROPERTY_RANGE(max_roughness);

		if (material.lighting.min_roughness > material.lighting.max_roughness) FAIL(InitializeMaterial,
			"The min roughness for the material with texture path '%s' exceeds its max roughness (%g > %g)",
			material.albedo_texture_path, (GLdouble) material.lighting.min_roughness, (GLdouble) material.lighting.max_roughness
		);
	);

	#undef VALIDATE_MATERIAL_PROPERTY_RANGE
}

static void copy_matching_material_to_dest_materials(const GLchar* const texture_path,
	const List* const all_materials, List* const dest_materials, const material_index_t dest_index) {

	#define SET_TO_RGB_PIXEL(index, property) dest[index] = (sdl_pixel_component_t)\
		(material -> lighting.property * constants.max_byte_value)

	LIST_FOR_EACH(0, all_materials, untyped_material,
		const MaterialPropertiesPerObjectInstance* const material =
			(MaterialPropertiesPerObjectInstance*) untyped_material;

		if (!strcmp(texture_path, material -> albedo_texture_path)) {
			sdl_pixel_component_t* const dest = ptr_to_list_index(dest_materials, dest_index);

			SET_TO_RGB_PIXEL(0, metallicity);
			SET_TO_RGB_PIXEL(1, min_roughness);
			SET_TO_RGB_PIXEL(2, max_roughness);

			return;
		}
	);

	#undef SET_TO_RGB_PIXEL

	FAIL(InitializeMaterial, "No material definition found for texture path %s", texture_path);
}

//////////

/* List subtypes:
`all_materials` -> MaterialPropertiesPerObjectInstance
`sector_face_texture_paths` -> GLchar*
`still_billboard_texture_paths` -> GLchar*
`billboard_animation_layouts` -> AnimationLayout

Note: this mutates the billboard list by setting each billboard's material index.

This function returns a 1D texture of material lighting properties (each property is a `sdl_pixel_component_t[3]`).
This texture is indexed by a material index when shading world-shaded objects.

- For a sector face texture, its material index = its texture id.

Let S = the number of sector face textures,
	B = the number of still billboard textures,
	V = the number of billboard animation layouts (each animation layout is a spritesheet description).

- For a billboard, the material index is a little bit more involved:
	If the billboard is still, its material index = S + its still texture id.
	If the billboard is animated, its material index = S + B + its animation layout id.

- For a weapon sprite, the material index equals S + B + V + its animation layout id among the other weapon sprites. */
GLuint init_materials_texture(const List* const all_materials, const List* const sector_face_texture_paths,
	const List* const still_billboard_texture_paths, const List* const billboard_animation_layouts,
	List* const billboards, const AnimationLayout* const weapon_sprite_animation_layout,
	material_index_t* const weapon_sprite_material_index) {

	////////// Variable initialization
	
	const byte num_sector_face_textures = (byte) sector_face_texture_paths -> length;
	const texture_id_t num_still_billboard_texture_paths = (texture_id_t) still_billboard_texture_paths -> length;

	const texture_id_t num_material_lighting_properties = (texture_id_t) (num_sector_face_textures +
		num_still_billboard_texture_paths + billboard_animation_layouts -> length) + 1u; // 1 more for the weapon sprite

	// Lighting properties are stored as 3 pixel components (just 24 bytes each)
	List material_lighting_properties = init_list(num_material_lighting_properties, sdl_pixel_component_t[3]);
	material_lighting_properties.length = num_material_lighting_properties;

	////////// First, inserting still face lighting properties

	const GLchar* const* const typed_sector_face_texture_paths = sector_face_texture_paths -> data;

	for (byte i = 0; i < num_sector_face_textures; i++)
		copy_matching_material_to_dest_materials(typed_sector_face_texture_paths[i],
			all_materials, &material_lighting_properties, i);

	////////// Then, inserting billboard lighting properties

	material_index_t last_dest_material_index = 0u;
	const Billboard* const first_billboard = billboards -> data;

	LIST_FOR_EACH(0, billboards, untyped_billboard,
		Billboard* const billboard = (Billboard*) untyped_billboard;
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
			LIST_FOR_EACH(0, billboard_animation_layouts, untyped_animation_layout,
				const AnimationLayout* const animation_layout = (AnimationLayout*) untyped_animation_layout;
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
				&material_lighting_properties, dest_material_index);

			last_dest_material_index = dest_material_index;
		}
	);

	////////// Then, inserting weapon sprite lighting properties

	*weapon_sprite_material_index = num_material_lighting_properties - 1u;

	copy_matching_material_to_dest_materials(weapon_sprite_animation_layout -> spritesheet_path,
		all_materials, &material_lighting_properties, *weapon_sprite_material_index);
	
	////////// Making the materials texture

	const GLuint materials_texture = preinit_texture(TexPlain1D, TexNonRepeating, TexNearest, TexNearest, false);

	init_texture_data(TexPlain1D, (GLsizei[]) {num_material_lighting_properties},
		GL_RGB, OPENGL_MATERIALS_MAP_INTERNAL_PIXEL_FORMAT,
		OPENGL_COLOR_CHANNEL_TYPE, material_lighting_properties.data);

	//////////

	deinit_list(material_lighting_properties);

	return materials_texture;
}
