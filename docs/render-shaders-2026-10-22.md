# PCJ render layer: shaders and the focus blur (2026-10-22)

Follows claude/pcj-xbwnd-decomp.md (frame order, XBDisplay, XBRenderTexture).

**Correction to the composite in pcj-xbwnd-decomp.md:** the focus blur pixel shader does not sample stage 0 (its texture mode is NONE), so the composite never reads the surface it draws onto. It reads only the depth texture (t1) and the blurred copy (t2).

**XBPixelShader.obj** 3/3, data 11/11 (src/enginex/XBPixelShader.cpp, commit 3288ea7). Handle index: 1 focusblur, 2 alphafog, 3 negative, 4 enhance, 5 BloomMask, 6 BaseLightShadow, 7 ShadowMapWithId (0 unused). **Focus blur program, decoded from the register combiner words:** five alpha stages with a per-stage c0 (D3D c0 maps to stage 0 only):
- r0.a = t1.a - focus_hi (signed); r0.a += 0.5 (stage 1's own c0 = 0x80); r1.a = r0.a >= 0.5 ? 2*r0.a - 1 : 0 (mux on the MSB); r1.a *= 4; r1.a *= 4.
- Final combiner: rgb = t2.rgb * r1.a, alpha = r1.a; drawn with ONE / INVSRCALPHA.
- So blur = clamp(32 * (depth_hi - focus_hi), 0, 1): pixels in front of the focus depth stay sharp, pixels behind fade to the blurred copy within 1/32 of the depth range. depth_hi is the top byte of the 24-bit W-buffer value; focus_hi is the top byte of the 16-bit quantised focus depth (view w x 0.001, so far = 1000 units).

**Smoothing passes depend on the frame's alpha:** XBBlurFocusBlurTextures copies the back buffer into the 256 texture, then BiLinearSmoothTexture draws 256 -> 128 -> 256 GetFocusBlurFactor() times with XBRenderTexture mode 0, i.e. SRCALPHA / INVSRCALPHA blending with white diffuse. Every copy therefore lands only as far as the source texel's alpha allows, and the source alpha is the back buffer's alpha. On the Xbox that alpha is whatever the window clear wrote (geometry draws with colour write RGB only, 0x10101; only glow textures and a few effects write alpha). If the PC back end lets draws write alpha, or clears it differently, the copies only partly replace the previous blur (or the CPU's initial black), which gives exactly a dark or stale blurred background that changes from run to run. Checked locally (Linux, Mesa, route fx4, new CJ_GL_DUMP_SRC_MAX): at the blur copy the back buffer's alpha is 255 everywhere and the 256 copy and its smoothing come out right, so the local build does not show this problem. Earlier in the frame (the bloom chain) the alpha is 0 on 98% of the screen, as expected with RGB-only colour writes. On the NVIDIA test build the same dump (CJ_GL_DUMP_SRC=1 CJ_GL_DUMP_SRC_MAX=400 CJ_GL_DUMP_SRC_SKIPVA=80EA9000, dump trigger while the weapon list is open) would show whether his frames differ at this step.

**XBVertexShader.obj** 27/28, data 541/541 (src/enginex/XBVertexShader.cpp, commits f03a7fb, d18fe7d): all 528 shader programs as data, lights, fog, environment matrix, streams. DeclareAllVertexShaders: see the update below. Vertex constants (shader registers):
- c4/c5/c42 directional light directions (negated, view space, w 1), c6/c7/c43 their colours.
- c9-c10 sphere-map texgen matrix (camera normal matrix x 0.5 / -0.5; flag 0x40000 drops U, 0x80000 drops V).
- c11 global diffuse, c12 global ambient, c13 fog (scale, offset from start/end 0..1 and near/far, times XBVertexShader+0x1e08).
- c14/c15, c18/c19, c21/c22, c32/c33 point lights (position relative to the camera in view space, colour); c17 1/range of each.
- c52-c67 spot lights: fixed 15/30 degree cones (c52, c67 cosines; c54 1/(cos inner - cos outer); c53 1/|direction|; position/direction/colour per light in c55-c66).
- XBRenderTexture also sets c29 = 0. EXDisplay::SetViewPort sets c72, c-38, c-37.
- Vertex declarations (from DeclareAllVertexShaders): the main model layout is stream 0 = v0 FLOAT3 position, v1 NORMPACKED3 normal, v3 FLOAT2 uv; stream 1 = v2 D3DCOLOR; stream 2 = v5 D3DCOLOR; streams 3-10 = v6-v13 FLOAT4 (morph/deformer inputs). 259 shaders use it, 131 a short form without the morph streams.

**Particles (XBParticle.obj):** SetParticleRenderStates matched (z write off, alpha ref 0, cull off, alpha test and blend on, SRCALPHA/INVSRCALPHA; system flag 8 additive, 0x10 reverse subtract). DrawQuadParticles writes each particle straight into the push buffer (BeginPush: two vertex constant loads, c(0x88-96)=position and a 4x4 at c(0x84-96), then BEGIN QUADS, an inline array of 4 x 7 dwords, END); not decompiled yet.

## Update (later 2026-10-22): DeclareAllVertexShaders and XBGeoEntity

**DeclareAllVertexShaders written** (commit e2c9492; XBVertexShader.obj 28/29, data 546/546). It comes to 14527 of 14528 bytes and differs only in how the store-heavy attribute blocks are scheduled (diff score 680; per-slot field order, loop and helper shapes all searched without closing it). What it does:
- Attribute layouts (m_Attr, D3DVERTEXSHADERINPUT: StreamIndex, Offset, Format, TessellationType, TessellationSource) are set for layouts 0-4 and 6-8; each block clears all 16 slots and sets Format NONE before filling them in. Layout 0 is the bit-combination path: stream 0 = position@0, NORMPACKED3 normal@12 and uv@16; stream 1 = colour. Layout 1 is the normal-mapped path, which adds stream 2 = colour@0 and FLOAT4@4. Layout 2 is the skinned path; its streams 3-10 are left NONE because RewireVertexShaderInputs supplies them. Layouts 3 and 7 are sprite/swoosh: position, normal@12, colour@16, uv@20. Layout 4 is lines (position, colour@12), layout 6 is sprite2D (position, colour@12, uv@16) and layout 8 is shadow (position, stream 2 colour).
- There are 10 declarations. The 512 bit-combination shaders pick one by index & 3: 0 takes an empty declaration (END only, so the inputs come from m_Attr), 1 and 3 take the skinned declaration (25 tokens: streams 0-10) and 2 takes the normal declaration.
- 17 fixed programs (m_FixedShaders, creation order): sprite, line3d, line2d, sprite2D, swoosh, shadowskin twice (shadow declaration, then skinned), shadownormal, heatskin, sprite2D again (3 uv sets), sprite2Dw (FLOAT4 position), refraction, refraction_deformer, refraction_normal, heat_normal, heat_deformer, occluder.
- SetVertexShader(n) table: m_FixedHandles[3..19] = sprite, line3d, line2d, sprite2D, swoosh, shadow, shadowskin, shadownormal, sprite2D-3uv, sprite2Dw, refraction, refraction_normal, refraction_deformer, heatskin, heat_normal, heat_deformer, occluder. m_FixedLoad is 1 (LoadVertexShader instead of SetVertexShader) for 8-10 and 13-19 (shadow, heat, refraction, occluder) and 0 for 3-7, 11 and 12. m_FixedCurrent = -1.
- Constants set once: c8 = (0, 1, -96, 256), c30 = (0.5, 0.25, 2, 12) (XBGeoEntity overwrites c30 with the refraction parameters per entity), c74 = (1, 1, 1, 1).

**XBGeoEntity draw path** (commit 9b90d70, background agent; unit 47/53 -> 187/206; enginex 4409/6994 functions). DrawDirectX is NEAR (7485/7520), Draw 1958/1984, DoShadowCast 1812/1872, CalculateVSSkin 843/864. Not done: GetFaceNormalsBuffer, DoRayCast, ApplySkinMorph, ApplySkin, GetRandomVertexArray.
- **Draw** snapshots the transforms and render environment into an EXCachedGeoEntity and queues it on the active window. The queue it goes to: the shadow-map pass (shadow and light lists), refraction, heat, or render order (0-2 opaque/alpha, 3 before and 4 after the refraction pass, 5 unsorted without Z).
- **DrawDirectX**, per entity:
  - Z, cull and blend defaults, then fog.
  - Vertex and index buffers, or the subdivision LoD's.
  - Lighting: the item's three directional lights from the extended render info, then the dynamic point and directional-point lights. Refraction and heat entities get the camera vector in object space at c40 and c30 = (0.5, strength, 2, 1/world scale) instead.
  - c0-c3 = object to clip.
  - Shadow-map sampling: receivers sample the map on stage 1 through the shadow UV matrix (shader bit 0x80).
  - Vertex streams: 0 positions/normals, or the dynamic buffer when the CPU rewrote them. 1 vertex colours: two light sets blended by c68, the second on its own stream. 2 and 3 the morph sets (weights in c44). The skinning streams after that.
- **DrawDirectX**, per strip:
  - Texture with U/V clamp, fade, environment map, lighting bits (dropped for unlit or transparent strips).
  - Blend mode: 0 normal (blend only when something needs alpha, else alpha test at 0x96), 1 additive, 2 subtractive (both unfogged).
  - Refraction, heat and shadow passes blend with source alpha and pick their fixed shader.
  - The strip is drawn as an indexed triangle strip from the running index offset.
- **Skinning**: stream 2 holds the bone weights. Each bone's matrix, moved to turn about the bone's rest position, goes in as 3 rows at register RemapB2VS[bone]. CPU-skinned or deformed vertices skip this.
- **Trade**: the standalone EXRenderInfo::AlphaMultEnabled/RGBMultEnabled copies went from MATCH to DIFF so the inlined uses could match; net +125 matches.

**PC run 2026-10-22 11:16 (NVIDIA, build 06:06):** the weapon list's focus blur is the same in every burst frame (bookmarks 2-5), where before it flickered. No GL errors, steady 30 fps.
