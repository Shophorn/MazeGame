from ctypes import *
from dataclasses import dataclass

idheader = open('src/game_assets.h')
lines = idheader.readlines()

class Enum:
	count 	= 0
	type 	= c_int

MeshAssetId 			= Enum()
TextureAssetId 			= Enum()
MaterialAssetId 		= Enum()
AnimationAssetId 		= Enum()
SkeletonAssetId 		= Enum()
CharacterSkeletonBone 	= Enum()
SoundAssetId 			= Enum()
FontAssetId 			= Enum()

for line in lines:
	line = line.strip()

	def try_parse(idName, target):
		if (line.startswith(idName)):
			id = line[len(idName):-1]
			setattr(target, id, c_int(target.count))
			target.count += 1

	try_parse('MeshAssetId_', MeshAssetId)
	try_parse('TextureAssetId_', TextureAssetId)
	try_parse('MaterialAssetId_', MaterialAssetId)
	try_parse('AnimationAssetId_', AnimationAssetId)
	try_parse('SkeletonAssetId_', SkeletonAssetId)
	try_parse('CharacterSkeletonBone_', CharacterSkeletonBone)
	try_parse('SoundAssetId_', SoundAssetId)
	try_parse('FontAssetId_', FontAssetId)


dll = CDLL('C:/Users/Leo/Work/friendsimulator/asset_cooker_for_python.dll')
dll.open_file(b'cooked_assets')	

### ---------------------------------------------------------------------------

@dataclass
class Mesh:
	id: 		MeshAssetId.type
	filename: 	c_char_p
	name: 		c_char_p

meshes = [
	Mesh(MeshAssetId.raccoon, 			b'raccoon.glb', 		b'raccoon'),
	Mesh(MeshAssetId.robot, 			b'Robot53.glb', 		b'model_rigged'),
	Mesh(MeshAssetId.character, 		b'cube_head_v4.glb', 	b'cube_head'),
	Mesh(MeshAssetId.skysphere, 		b'skysphere.glb', 		b'skysphere'),
	Mesh(MeshAssetId.totem, 			b'totem.glb', 			b'totem'),
	Mesh(MeshAssetId.small_pot, 		b'scenery.glb', 		b'small_pot'),
	Mesh(MeshAssetId.big_pot, 			b'scenery.glb', 		b'big_pot'),
	Mesh(MeshAssetId.seed, 				b'scenery.glb', 		b'acorn'),
	Mesh(MeshAssetId.water_drop, 		b'scenery.glb', 		b'water_drop'),
	Mesh(MeshAssetId.train, 			b'train.glb', 			b'train'),
	Mesh(MeshAssetId.monument_arcs, 	b'monuments.glb', 		b'monument_arches'),
	Mesh(MeshAssetId.monument_base,		b'monuments.glb', 		b'monument_base'),
	Mesh(MeshAssetId.monument_top_1,	b'monuments.glb', 		b'monument_ornament_01'),
	Mesh(MeshAssetId.monument_top_2, 	b'monuments.glb', 		b'monument_ornament_02'),
	Mesh(MeshAssetId.box, 				b'box.glb', 			b'box'),
	Mesh(MeshAssetId.box_cover, 		b'box.glb', 			b'cover'),
	Mesh(MeshAssetId.cloud, 			b'cloud.glb', 			b'cloud'),
	Mesh(MeshAssetId.rain,				b'cloud.glb', 			b'rain'),
]

for mesh in meshes:
	dll.cook_mesh(mesh.id, mesh.filename, mesh.name)

### ---------------------------------------------------------------------------

# Note(Leo): Check these every now and then that they are not changed, or parse them from header as well
TextureFormat = Enum()
TextureFormat.u8_srgb 	= TextureFormat.type(0)
TextureFormat.u8_linear = TextureFormat.type(1)

@dataclass
class Texture:
	id: 		TextureAssetId.type
	filename: 	c_char_p
	format: 	TextureFormat.type

textures = [
	Texture(TextureAssetId.ground_albedo, 		b'ground.png', 				TextureFormat.u8_srgb),
	Texture(TextureAssetId.tiles_albedo, 		b'tiles.png', 				TextureFormat.u8_srgb),
	Texture(TextureAssetId.red_tiles_albedo, 	b'tiles_red.png', 			TextureFormat.u8_srgb),
	Texture(TextureAssetId.seed_albedo, 		b'Acorn_albedo.png', 		TextureFormat.u8_srgb),
	Texture(TextureAssetId.bark_albedo, 		b'bark.png', 				TextureFormat.u8_srgb),
	Texture(TextureAssetId.raccoon_albedo, 		b'RaccoonAlbedo.png', 		TextureFormat.u8_srgb),
	Texture(TextureAssetId.robot_albedo, 		b'Robot_53_albedo_4k.png', 	TextureFormat.u8_srgb),
	Texture(TextureAssetId.leaves_mask, 		b'leaf_mask.png', 			TextureFormat.u8_srgb),
	Texture(TextureAssetId.ground_normal, 		b'ground_normal.png', 		TextureFormat.u8_linear),
	Texture(TextureAssetId.tiles_normal, 		b'tiles_normal.png', 		TextureFormat.u8_linear),
	Texture(TextureAssetId.bark_normal, 		b'bark_normal.png', 		TextureFormat.u8_linear),
	Texture(TextureAssetId.robot_normal, 		b'Robot_53_normal_4k.png', 	TextureFormat.u8_linear),
	Texture(TextureAssetId.heightmap, 			b'heightmap_island.png', 	TextureFormat.u8_srgb),
	Texture(TextureAssetId.menu_background, 	b'keyart.png', 				TextureFormat.u8_srgb),
]

for texture in textures:
	dll.cook_texture(texture.id, texture.filename, texture.format)

dll.cook_skeleton(SkeletonAssetId.character, b'cube_head_v4.glb', b'cube_head')

### ---------------------------------------------------------------------------

@dataclass
class Animation:
	id: 		AnimationAssetId.type
	filename: 	c_char_p
	name:		c_char_p

animations = [
	Animation(AnimationAssetId.character_idle,		 b'cube_head_v3.glb', 	b'Idle'),
	Animation(AnimationAssetId.character_walk,		 b'cube_head_v3.glb', 	b'Walk'),
	Animation(AnimationAssetId.character_run,		 b'cube_head_v3.glb', 	b'Run'),
	Animation(AnimationAssetId.character_jump,		 b'cube_head_v3.glb', 	b'JumpUp'),
	Animation(AnimationAssetId.character_fall,		 b'cube_head_v3.glb', 	b'JumpDown'),
	Animation(AnimationAssetId.character_crouch, 	 b'cube_head_v3.glb', 	b'Crouch'),
]

for animation in animations:
	dll.cook_animation(animation.id, animation.filename, animation.name)

### ---------------------------------------------------------------------------

@dataclass
class Audio:
	id:			SoundAssetId.type
	filename: 	c_char_p

audios = [
	Audio(SoundAssetId.background, 	b'Wind-Mark_DiAngelo-1940285615.wav'),
	Audio(SoundAssetId.step_1, 		b'step_9.wav'),
	Audio(SoundAssetId.step_2, 		b'step_10.wav'),
	Audio(SoundAssetId.birds, 		b'Falcon-Mark_Mattingly-169493032.wav'),
	# Audio(SoundAssetId_electric_1, 'arc1_freesoundeffects_com_NO_LICENSE.wav')
	# Audio(SoundAssetId_electric_2, 'earcing_freesoundeffects_com_NO_LICENSE.wav')
]

for audio in audios:
	dll.cook_audio(audio.id, audio.filename)

# -------------------------------------------------------------

# Todo(Leo): There is some fuckery going around here >:(
dll.cook_font(FontAssetId.game, b'SourceCodePro-Regular.ttf')
# dll.cook_font(FontAssetId.menu, 'SourceCodePro-Regular.ttf')
# dll.cook_font(FontAssetId.menu, 'TheStrangerBrush.ttf')

dll.close_file()