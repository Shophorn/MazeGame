// #define CONCAT(A,B) A ## B

// #define VARIABLE(name) int _##name Num = 0
// VARIABLE(a);

// #define LOAD_TEXTURE(NAME) TextureAsset NAME ## Texture = load_texture_asset("textures/" #NAME ".jpg", &state->persistentMemoryArena);

//   LOAD_TEXTURE (lava),
//   LOAD_TEXTURE (tiles),

// works
// #define PUSH_TEXTURE(name) TextureHandle name ## Texture = platformInfo->graphicsContext->PushTexture(&name ## TextureAsset);
// PUSH_TEXTURE(tiles)



MaterialAsset characterMaterialAsset = {MaterialType::Character, lavaTexture, textureTexture, blackTexture};
state->materials.character = platformInfo->graphicsContext->PushMaterial(&characterMaterialAsset);
