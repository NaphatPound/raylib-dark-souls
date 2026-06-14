#include "render.h"
#include "assets.h"
#include "game.h"
#include "raymath.h"

LitShader g_lit;

void LitShader::load() {
    shader = LoadShader(assets::path("shaders/lit.vs").c_str(),
                        assets::path("shaders/lit.fs").c_str());
    ok = (shader.id != 0);
    if (!ok) return;
    shader.locs[SHADER_LOC_MATRIX_MODEL]  = GetShaderLocation(shader, "matModel");
    shader.locs[SHADER_LOC_MATRIX_NORMAL] = GetShaderLocation(shader, "matNormal");
    loc_lightDir   = GetShaderLocation(shader, "uLightDir");
    loc_lightColor = GetShaderLocation(shader, "uLightColor");
    loc_ambient    = GetShaderLocation(shader, "uAmbient");
    loc_viewPos    = GetShaderLocation(shader, "uViewPos");
    loc_fogColor   = GetShaderLocation(shader, "uFogColor");
    loc_fogDensity = GetShaderLocation(shader, "uFogDensity");
    loc_emissive   = GetShaderLocation(shader, "uEmissive");
    loc_plightN    = GetShaderLocation(shader, "uPLightN");
    loc_plights    = GetShaderLocation(shader, "uPLights");
    loc_plightColor= GetShaderLocation(shader, "uPLightColor");
    loc_clipY      = GetShaderLocation(shader, "uClipY");
    loc_clipBelow  = GetShaderLocation(shader, "uClipBelow");

    Vector3 lightDir = Vector3Normalize({ 0.12f, 0.82f, -0.58f });  // toward the moon
    Vector3 lightCol = { 1.45f, 0.92f, 0.84f };                      // rose moonlight
    Vector3 ambient  = { 1.15f, 0.78f, 0.74f };                      // soft rose ambient (lifted)
    Vector3 fogCol   = { 0.24f, 0.09f, 0.10f };
    float   fogDen   = 0.0030f;
    if (g_level == LEVEL_FROZEN) {                                   // cold frozen-cathedral lighting (per design spec)
        lightDir = Vector3Normalize({ 0.35f, 0.82f, 0.45f });        // high pale overcast light
        lightCol = { 0.90f, 1.05f, 1.22f };                          // cool blue-white
        ambient  = { 0.70f, 0.84f, 1.05f };                          // pale icy ambient (lifted)
        fogCol   = { 0.72f, 0.83f, 0.92f };                          // milky cold haze (#D8EEF3-ish)
        fogDen   = 0.0120f;                                          // low-hanging cold mist
    } else if (g_level == LEVEL_FORGE) {                             // hot volcanic-forge lighting
        lightDir = Vector3Normalize({ 0.10f, 0.55f, 0.40f });        // low warm light
        lightCol = { 1.35f, 0.70f, 0.34f };                          // hot orange
        ambient  = { 0.80f, 0.42f, 0.28f };                          // warm dim ambient (lava bounce)
        fogCol   = { 0.34f, 0.11f, 0.06f };                          // smoky orange-black
        fogDen   = 0.0052f;
    } else if (g_level == LEVEL_COLOSSEUM) {                         // stormy sunken-colosseum lighting
        lightDir = Vector3Normalize({ -0.30f, 0.80f, 0.30f });       // flat overcast light
        lightCol = { 0.86f, 0.92f, 1.00f };                          // cool desaturated daylight
        ambient  = { 0.60f, 0.68f, 0.78f };                          // moody grey-blue ambient
        fogCol   = { 0.46f, 0.54f, 0.60f };                          // grey rain haze
        fogDen   = 0.0140f;                                          // thick wet mist
    } else if (g_level == LEVEL_GRAVEYARD) {                         // misty necropolis lighting
        lightDir = Vector3Normalize({ 0.20f, 0.78f, -0.40f });       // high pale moonlight
        lightCol = { 0.82f, 0.92f, 0.84f };                          // wan green-white moon
        ambient  = { 0.50f, 0.62f, 0.52f };                          // sickly green-grey ambient
        fogCol   = { 0.36f, 0.43f, 0.35f };                          // graveyard green-grey haze
        fogDen   = 0.0130f;                                          // thick low ground mist
    } else if (g_level == LEVEL_FUNGAL) {                            // bioluminescent grotto lighting
        lightDir = Vector3Normalize({ -0.15f, 0.70f, 0.30f });       // dim diffuse cave glow
        lightCol = { 0.42f, 0.76f, 0.80f };                          // faint cool teal light
        ambient  = { 0.30f, 0.50f, 0.52f };                          // teal-cyan ambient (lit by spores)
        fogCol   = { 0.09f, 0.21f, 0.23f };                          // deep teal grotto haze
        fogDen   = 0.0150f;                                          // thick, enclosed underground
    } else if (g_level == LEVEL_DESERT) {                           // warm dusk desert lighting
        lightDir = Vector3Normalize({ 0.45f, 0.42f, -0.30f });       // low raking sun
        lightCol = { 1.35f, 1.02f, 0.62f };                          // warm amber sunlight
        ambient  = { 0.82f, 0.66f, 0.46f };                          // warm sand-bounce ambient
        fogCol   = { 0.60f, 0.49f, 0.34f };                          // sandy dusk haze
        fogDen   = 0.0062f;                                          // clear-ish desert air
    } else if (g_level == LEVEL_WRECK) {                            // foggy moonlit cove lighting
        lightDir = Vector3Normalize({ 0.10f, 0.80f, -0.55f });       // pale moon overhead
        lightCol = { 0.82f, 0.92f, 1.12f };                          // cold blue-white moonlight
        ambient  = { 0.52f, 0.64f, 0.78f };                          // cool sea-mist ambient
        fogCol   = { 0.30f, 0.38f, 0.46f };                          // cold blue-grey sea fog
        fogDen   = 0.0110f;                                          // thick cove mist
    } else if (g_level == LEVEL_SANCTUM) {                          // ethereal high-altitude sky lighting
        lightDir = Vector3Normalize({ 0.30f, 0.86f, 0.20f });        // bright open sky
        lightCol = { 1.10f, 1.14f, 1.28f };                          // clean cool daylight
        ambient  = { 0.78f, 0.84f, 0.98f };                          // bright sky-blue ambient
        fogCol   = { 0.66f, 0.74f, 0.86f };                          // pale cloud haze (distant islands fade out)
        fogDen   = 0.0090f;                                          // airy, distance falls into sky
    } else if (g_level == LEVEL_CLOCK) {                            // dim warm brass-vault lighting
        lightDir = Vector3Normalize({ 0.20f, 0.65f, 0.35f });        // low industrial key light
        lightCol = { 1.10f, 0.84f, 0.52f };                          // warm furnace-lit brass
        ambient  = { 0.46f, 0.40f, 0.34f };                          // dim warm metal ambient
        fogCol   = { 0.16f, 0.14f, 0.13f };                          // sooty brown-black haze
        fogDen   = 0.0095f;                                          // enclosed, smoky
    } else if (g_level == LEVEL_SHRINE) {                           // misty dusk shrine lighting
        lightDir = Vector3Normalize({ 0.25f, 0.72f, -0.40f });       // soft low dusk light
        lightCol = { 1.05f, 0.92f, 0.86f };                          // warm-neutral evening
        ambient  = { 0.60f, 0.62f, 0.68f };                          // cool misty ambient
        fogCol   = { 0.50f, 0.52f, 0.58f };                          // soft grey-blue mountain mist
        fogDen   = 0.0110f;                                          // thick valley haze
    } else if (g_level == LEVEL_BONES) {                            // desolate deathly boneyard lighting
        lightDir = Vector3Normalize({ -0.25f, 0.70f, 0.25f });       // flat pale overcast
        lightCol = { 0.92f, 0.96f, 0.90f };                          // washed-out bone-white
        ambient  = { 0.58f, 0.62f, 0.56f };                          // sickly grey-green ambient
        fogCol   = { 0.50f, 0.54f, 0.48f };                          // pale deathly ash haze
        fogDen   = 0.0120f;                                          // thick still air over the basin
    } else if (g_level == LEVEL_GEODE) {                            // dark crystal-cavern lighting
        lightDir = Vector3Normalize({ 0.15f, 0.62f, 0.30f });        // faint cave key light
        lightCol = { 0.52f, 0.50f, 0.74f };                          // dim cool violet light
        ambient  = { 0.34f, 0.32f, 0.52f };                          // deep amethyst ambient (lit by crystals)
        fogCol   = { 0.09f, 0.07f, 0.15f };                          // near-black violet cave haze
        fogDen   = 0.0130f;                                          // thick, enclosed underground
    } else if (g_level == LEVEL_TEMPLE) {                           // humid green jungle lighting
        lightDir = Vector3Normalize({ 0.30f, 0.78f, 0.20f });        // dappled canopy light
        lightCol = { 0.84f, 1.02f, 0.78f };                          // green-filtered daylight
        ambient  = { 0.48f, 0.60f, 0.44f };                          // lush green ambient
        fogCol   = { 0.32f, 0.44f, 0.32f };                          // humid green haze
        fogDen   = 0.0120f;                                          // thick jungle humidity
    } else if (g_level == LEVEL_MINE) {                             // dark flooded-mine lighting
        lightDir = Vector3Normalize({ 0.20f, 0.60f, 0.35f });        // dim lantern key light
        lightCol = { 0.92f, 0.78f, 0.56f };                          // warm lantern-lit timber
        ambient  = { 0.40f, 0.36f, 0.32f };                          // dim warm earth ambient
        fogCol   = { 0.11f, 0.10f, 0.09f };                          // near-black dust haze
        fogDen   = 0.0110f;                                          // enclosed underground
    } else if (g_level == LEVEL_OBS) {                              // cool moonlit observatory lighting
        lightDir = Vector3Normalize({ 0.10f, 0.82f, -0.50f });       // pale moon overhead
        lightCol = { 0.70f, 0.80f, 1.02f };                          // cool moonlight
        ambient  = { 0.44f, 0.50f, 0.66f };                          // cool night-blue ambient
        fogCol   = { 0.09f, 0.11f, 0.20f };                          // deep night-blue haze
        fogDen   = 0.0090f;                                          // clear night air (stars visible)
    } else if (g_level == LEVEL_LIB) {                              // dusty warm archive lighting
        lightDir = Vector3Normalize({ 0.30f, 0.74f, 0.30f });        // soft hall light
        lightCol = { 1.02f, 0.90f, 0.70f };                          // warm candle-lit parchment
        ambient  = { 0.50f, 0.46f, 0.40f };                          // dim warm dusty ambient
        fogCol   = { 0.22f, 0.20f, 0.17f };                          // dusty brown haze
        fogDen   = 0.0100f;                                          // motes hanging in the air
    } else if (g_level == LEVEL_CAMP) {                             // grim overcast battlefield-dusk lighting
        lightDir = Vector3Normalize({ -0.30f, 0.74f, 0.30f });       // flat grey daylight
        lightCol = { 0.96f, 0.90f, 0.80f };                          // pale cold overcast
        ambient  = { 0.50f, 0.50f, 0.50f };                          // neutral grey ambient
        fogCol   = { 0.40f, 0.40f, 0.42f };                          // grey battlefield haze
        fogDen   = 0.0100f;                                          // smoky field
    } else if (g_level == LEVEL_AQUA) {                             // dank flooded-cistern lighting
        lightDir = Vector3Normalize({ 0.20f, 0.72f, 0.30f });        // dim shaft light from above
        lightCol = { 0.70f, 0.82f, 0.78f };                          // cool damp green-grey
        ambient  = { 0.42f, 0.50f, 0.46f };                          // green-grey cistern ambient
        fogCol   = { 0.15f, 0.20f, 0.18f };                          // dark green-grey damp haze
        fogDen   = 0.0120f;                                          // thick underground mist
    } else if (g_level == LEVEL_COURT) {                            // solemn moonlit statue-court lighting
        lightDir = Vector3Normalize({ 0.10f, 0.82f, -0.50f });       // pale moon overhead
        lightCol = { 0.78f, 0.86f, 1.02f };                          // cool moonlight
        ambient  = { 0.50f, 0.54f, 0.64f };                          // cool grey-blue ambient
        fogCol   = { 0.24f, 0.28f, 0.36f };                          // cool grey-blue haze
        fogDen   = 0.0100f;                                          // still night air
    } else if (g_level == LEVEL_GARDEN) {                           // warm sunlit garden lighting
        lightDir = Vector3Normalize({ 0.40f, 0.70f, 0.25f });        // golden-hour sun
        lightCol = { 1.22f, 1.08f, 0.82f };                          // warm sunlight
        ambient  = { 0.72f, 0.72f, 0.62f };                          // soft bright ambient
        fogCol   = { 0.62f, 0.66f, 0.60f };                          // pale airy haze
        fogDen   = 0.0055f;                                          // clear, sunny
    } else if (g_level == LEVEL_BRIDGE) {                           // windy high-chasm dusk lighting
        lightDir = Vector3Normalize({ -0.25f, 0.74f, 0.35f });       // flat stormy daylight
        lightCol = { 0.88f, 0.92f, 1.02f };                          // pale cold sky
        ambient  = { 0.54f, 0.58f, 0.68f };                          // cool windy ambient
        fogCol   = { 0.46f, 0.50f, 0.58f };                          // grey-blue chasm haze
        fogDen   = 0.0085f;                                          // distance falls into mist
    } else if (g_level == LEVEL_BEACON) {                           // stormy night-coast lighting
        lightDir = Vector3Normalize({ -0.20f, 0.66f, 0.40f });       // dim stormy light
        lightCol = { 0.66f, 0.74f, 0.90f };                          // cold sea-storm
        ambient  = { 0.42f, 0.48f, 0.60f };                          // dim blue-grey ambient
        fogCol   = { 0.30f, 0.35f, 0.44f };                          // wet sea haze
        fogDen   = 0.0115f;                                          // thick coastal mist
    } else if (g_level == LEVEL_MILL) {                             // overcast windy farmland lighting
        lightDir = Vector3Normalize({ 0.35f, 0.74f, 0.25f });        // soft daylight
        lightCol = { 1.06f, 1.02f, 0.86f };                          // warm-neutral overcast
        ambient  = { 0.62f, 0.64f, 0.56f };                          // soft outdoor ambient
        fogCol   = { 0.60f, 0.62f, 0.56f };                          // pale windy haze
        fogDen   = 0.0060f;                                          // clear-ish open field
    } else if (g_level == LEVEL_OSSUARY) {                          // candlelit crypt lighting
        lightDir = Vector3Normalize({ 0.20f, 0.62f, 0.30f });        // dim crypt key
        lightCol = { 0.86f, 0.72f, 0.54f };                          // warm candle-lit bone
        ambient  = { 0.40f, 0.36f, 0.32f };                          // dim warm ambient
        fogCol   = { 0.14f, 0.12f, 0.11f };                          // near-black dusty haze
        fogDen   = 0.0120f;                                          // close, enclosed
    } else if (g_level == LEVEL_FAIR) {                             // murky abandoned-carnival dusk
        lightDir = Vector3Normalize({ -0.25f, 0.66f, 0.35f });       // dim dusk
        lightCol = { 0.84f, 0.78f, 0.84f };                          // cool faded light
        ambient  = { 0.50f, 0.46f, 0.52f };                          // murky violet-grey ambient
        fogCol   = { 0.32f, 0.30f, 0.36f };                          // murky haze
        fogDen   = 0.0090f;                                          // moody, lights pop
    } else if (g_level == LEVEL_THRONE) {                           // dim regal throne-hall lighting
        lightDir = Vector3Normalize({ 0.15f, 0.66f, 0.40f });        // dim hall key
        lightCol = { 1.00f, 0.86f, 0.62f };                          // warm gold sconce-light
        ambient  = { 0.52f, 0.46f, 0.40f };                          // dim warm regal ambient
        fogCol   = { 0.17f, 0.15f, 0.13f };                          // dark warm haze
        fogDen   = 0.0090f;                                          // grand, shadowed
    } else if (g_level == LEVEL_SPRINGS) {                          // steamy geothermal-basin lighting
        lightDir = Vector3Normalize({ 0.25f, 0.80f, 0.15f });        // pale misty daylight
        lightCol = { 1.02f, 0.98f, 0.90f };                          // warm-pale light through steam
        ambient  = { 0.62f, 0.62f, 0.60f };                          // bright steamy ambient
        fogCol   = { 0.62f, 0.64f, 0.62f };                          // warm-white steam haze
        fogDen   = 0.0110f;                                          // thick, steaming
    } else if (g_level == LEVEL_PFOREST) {                          // dead petrified-forest lighting
        lightDir = Vector3Normalize({ -0.30f, 0.72f, 0.20f });       // flat overcast ash-light
        lightCol = { 0.94f, 0.90f, 0.82f };                          // pale, drained sun
        ambient  = { 0.50f, 0.49f, 0.46f };                          // dim grey ambient
        fogCol   = { 0.56f, 0.54f, 0.50f };                          // hanging ash dust
        fogDen   = 0.0125f;                                          // dusty, choking
    } else if (g_level == LEVEL_HAMLET) {                           // sickly plague-village lighting
        lightDir = Vector3Normalize({ 0.20f, 0.70f, -0.30f });       // low overcast sun
        lightCol = { 0.86f, 0.88f, 0.74f };                          // sallow greenish daylight
        ambient  = { 0.46f, 0.48f, 0.42f };                          // murky ambient
        fogCol   = { 0.50f, 0.53f, 0.46f };                          // sickly green-grey miasma
        fogDen   = 0.0130f;                                          // close, miasmic
    } else if (g_level == LEVEL_HENGE) {                            // druid stone-circle twilight
        lightDir = Vector3Normalize({ -0.35f, 0.55f, -0.40f });      // low cold dusk sun
        lightCol = { 0.74f, 0.80f, 0.92f };                          // pale blue twilight
        ambient  = { 0.42f, 0.46f, 0.54f };                          // cool blue ambient
        fogCol   = { 0.42f, 0.48f, 0.56f };                          // moorland mist
        fogDen   = 0.0115f;                                          // misty heath
    } else if (g_level == LEVEL_BOG) {                              // fetid swamp lighting
        lightDir = Vector3Normalize({ 0.18f, 0.62f, 0.30f });        // weak filtered daylight
        lightCol = { 0.78f, 0.84f, 0.66f };                          // sickly green-tinged light
        ambient  = { 0.40f, 0.46f, 0.38f };                          // dim murky ambient
        fogCol   = { 0.34f, 0.42f, 0.32f };                          // thick green miasma
        fogDen   = 0.0150f;                                          // dense, fetid
    } else if (g_level == LEVEL_SAWMILL) {                          // timber-yard overcast lighting
        lightDir = Vector3Normalize({ 0.30f, 0.74f, -0.20f });       // soft daylight
        lightCol = { 1.00f, 0.92f, 0.78f };                          // warm sawdust-lit day
        ambient  = { 0.54f, 0.52f, 0.46f };                          // neutral working ambient
        fogCol   = { 0.55f, 0.52f, 0.46f };                          // pale sawdust haze
        fogDen   = 0.0095f;                                          // light, dusty
    } else if (g_level == LEVEL_GAOL) {                             // grim torch-lit gaol
        lightDir = Vector3Normalize({ -0.20f, 0.66f, 0.30f });       // weak cold skylight
        lightCol = { 0.66f, 0.66f, 0.72f };                          // pale cold stone light
        ambient  = { 0.34f, 0.33f, 0.36f };                          // dim, shadowed
        fogCol   = { 0.16f, 0.15f, 0.16f };                          // dark dungeon murk
        fogDen   = 0.0115f;                                          // close, gloomy
    } else if (g_level == LEVEL_TAR) {                              // hazy sulfurous tar-pit lighting
        lightDir = Vector3Normalize({ 0.28f, 0.60f, 0.18f });        // low hot haze-filtered sun
        lightCol = { 0.96f, 0.80f, 0.52f };                          // warm brown-amber light
        ambient  = { 0.46f, 0.40f, 0.32f };                          // dim warm ambient
        fogCol   = { 0.40f, 0.34f, 0.24f };                          // brown-yellow sulfurous haze
        fogDen   = 0.0135f;                                          // thick, acrid
    } else if (g_level == LEVEL_VINE) {                             // golden-hour vineyard lighting
        lightDir = Vector3Normalize({ 0.45f, 0.50f, 0.20f });        // low warm sun
        lightCol = { 1.10f, 0.92f, 0.66f };                          // rich golden light
        ambient  = { 0.52f, 0.48f, 0.40f };                          // warm soft ambient
        fogCol   = { 0.60f, 0.52f, 0.40f };                          // hazy golden dusk
        fogDen   = 0.0080f;                                          // light, clear
    } else if (g_level == LEVEL_FLOE) {                             // glacial frozen-sea lighting
        lightDir = Vector3Normalize({ -0.25f, 0.62f, 0.30f });       // pale low arctic sun
        lightCol = { 0.82f, 0.90f, 1.00f };                          // cold blue-white light
        ambient  = { 0.52f, 0.58f, 0.66f };                          // bright cold ambient
        fogCol   = { 0.66f, 0.74f, 0.84f };                          // icy blue-grey haze
        fogDen   = 0.0090f;                                          // crisp arctic air
    } else if (g_level == LEVEL_QUARRY) {                           // dusty stone-quarry lighting
        lightDir = Vector3Normalize({ 0.35f, 0.72f, 0.10f });        // high hazy sun
        lightCol = { 1.02f, 0.96f, 0.82f };                          // warm dusty daylight
        ambient  = { 0.52f, 0.50f, 0.45f };                          // neutral stone ambient
        fogCol   = { 0.58f, 0.55f, 0.48f };                          // pale rock-dust haze
        fogDen   = 0.0090f;                                          // dusty, open
    } else if (g_level == LEVEL_DOCK) {                             // high-altitude skydock lighting
        lightDir = Vector3Normalize({ 0.30f, 0.78f, 0.25f });        // bright high sun
        lightCol = { 1.06f, 1.02f, 0.96f };                          // clean bright daylight
        ambient  = { 0.58f, 0.62f, 0.70f };                          // bright cool sky ambient
        fogCol   = { 0.74f, 0.82f, 0.92f };                          // pale blue altitude haze
        fogDen   = 0.0070f;                                          // thin, clear air
    } else if (g_level == LEVEL_GLASS) {                            // bright glasshouse lighting
        lightDir = Vector3Normalize({ 0.20f, 0.86f, 0.25f });        // high overhead sun through glass
        lightCol = { 1.08f, 1.06f, 0.92f };                          // bright warm-white daylight
        ambient  = { 0.60f, 0.64f, 0.56f };                          // bright humid green ambient
        fogCol   = { 0.72f, 0.80f, 0.70f };                          // soft warm-green humidity
        fogDen   = 0.0075f;                                          // light, humid
    } else if (g_level == LEVEL_APIARY) {                           // sunny meadow apiary lighting
        lightDir = Vector3Normalize({ 0.40f, 0.74f, 0.20f });        // warm midday sun
        lightCol = { 1.10f, 1.02f, 0.82f };                          // golden meadow light
        ambient  = { 0.58f, 0.60f, 0.50f };                          // bright warm ambient
        fogCol   = { 0.74f, 0.78f, 0.62f };                          // soft golden-green haze
        fogDen   = 0.0070f;                                          // light, clear
    } else if (g_level == LEVEL_KILN) {                            // hot dusty kiln-yard lighting
        lightDir = Vector3Normalize({ 0.28f, 0.70f, 0.22f });        // hazy overcast sun
        lightCol = { 1.02f, 0.88f, 0.70f };                          // warm firelit-dust light
        ambient  = { 0.50f, 0.44f, 0.38f };                          // warm working ambient
        fogCol   = { 0.52f, 0.46f, 0.40f };                          // brick-dust + kiln-smoke haze
        fogDen   = 0.0110f;                                          // smoky, close
    } else if (g_level == LEVEL_REEF) {                             // deep underwater reef lighting
        lightDir = Vector3Normalize({ 0.10f, 0.90f, 0.15f });        // dim light filtering from above
        lightCol = { 0.62f, 0.82f, 0.92f };                          // cool blue-green light
        ambient  = { 0.40f, 0.54f, 0.58f };                          // submerged teal ambient
        fogCol   = { 0.16f, 0.40f, 0.50f };                          // deep blue-green water
        fogDen   = 0.0170f;                                          // thick, deep
    } else if (g_level == LEVEL_FOUNDRY) {                          // dark molten-lit ironworks
        lightDir = Vector3Normalize({ -0.20f, 0.66f, 0.25f });       // dim high vents
        lightCol = { 0.70f, 0.60f, 0.52f };                          // weak warm light
        ambient  = { 0.34f, 0.30f, 0.30f };                          // dim, smoky
        fogCol   = { 0.20f, 0.16f, 0.16f };                          // dark soot haze
        fogDen   = 0.0125f;                                          // smoky, close
    } else if (g_level == LEVEL_NAVE) {                             // dim candlelit cathedral
        lightDir = Vector3Normalize({ -0.25f, 0.74f, 0.20f });       // soft clerestory light
        lightCol = { 0.80f, 0.76f, 0.70f };                          // pale stone light
        ambient  = { 0.40f, 0.38f, 0.40f };                          // dim, reverent
        fogCol   = { 0.22f, 0.20f, 0.24f };                          // shadowed nave haze
        fogDen   = 0.0090f;                                          // soft interior gloom
    } else if (g_level == LEVEL_WATERMILL) {                        // pastoral watermill daylight
        lightDir = Vector3Normalize({ 0.32f, 0.72f, 0.22f });        // soft overcast sun
        lightCol = { 1.00f, 0.96f, 0.84f };                          // warm pastoral light
        ambient  = { 0.54f, 0.56f, 0.50f };                          // gentle ambient
        fogCol   = { 0.62f, 0.66f, 0.60f };                          // soft river haze
        fogDen   = 0.0085f;                                          // light, fresh
    } else if (g_level == LEVEL_SALT) {                             // blinding salt-pan glare
        lightDir = Vector3Normalize({ 0.30f, 0.84f, 0.18f });        // harsh high sun
        lightCol = { 1.16f, 1.12f, 1.04f };                          // blinding white light
        ambient  = { 0.66f, 0.66f, 0.64f };                          // bright bounced glare
        fogCol   = { 0.80f, 0.82f, 0.82f };                          // pale heat haze
        fogDen   = 0.0070f;                                          // shimmering, open
    } else if (g_level == LEVEL_STILT) {                            // hazy coastal lagoon lighting
        lightDir = Vector3Normalize({ 0.30f, 0.66f, 0.28f });        // soft overcast coastal sun
        lightCol = { 0.96f, 0.92f, 0.84f };                          // warm-grey daylight
        ambient  = { 0.52f, 0.54f, 0.54f };                          // soft sea ambient
        fogCol   = { 0.58f, 0.62f, 0.62f };                          // pale brackish sea haze
        fogDen   = 0.0100f;                                          // humid, hazy
    } else if (g_level == LEVEL_HIVE) {                             // dark amber-wax hive lighting
        lightDir = Vector3Normalize({ -0.20f, 0.70f, 0.25f });       // dim filtered glow
        lightCol = { 0.90f, 0.66f, 0.36f };                          // warm amber light
        ambient  = { 0.40f, 0.30f, 0.22f };                          // dim warm-brown ambient
        fogCol   = { 0.26f, 0.18f, 0.12f };                          // dark waxy haze
        fogDen   = 0.0120f;                                          // close, resinous
    } else if (g_level == LEVEL_BREW) {                             // dim warm brewery lighting
        lightDir = Vector3Normalize({ 0.24f, 0.70f, 0.20f });        // dim hall light
        lightCol = { 0.92f, 0.78f, 0.60f };                          // warm coppery light
        ambient  = { 0.44f, 0.40f, 0.34f };                          // warm working ambient
        fogCol   = { 0.30f, 0.26f, 0.22f };                          // malty steam haze
        fogDen   = 0.0105f;                                          // close, steamy
    } else if (g_level == LEVEL_BAMBOO) {                           // green dappled bamboo-grove light
        lightDir = Vector3Normalize({ 0.18f, 0.82f, 0.20f });        // dappled overhead sun
        lightCol = { 0.84f, 1.00f, 0.78f };                          // cool green-filtered light
        ambient  = { 0.48f, 0.56f, 0.46f };                          // soft green ambient
        fogCol   = { 0.46f, 0.58f, 0.46f };                          // green forest haze
        fogDen   = 0.0095f;                                          // soft, leafy
    } else if (g_level == LEVEL_COLLIER) {                          // smoky charcoal-camp lighting
        lightDir = Vector3Normalize({ 0.26f, 0.68f, 0.22f });        // hazy filtered sun
        lightCol = { 0.92f, 0.82f, 0.66f };                          // warm smoke-filtered light
        ambient  = { 0.46f, 0.42f, 0.36f };                          // dim warm ambient
        fogCol   = { 0.46f, 0.42f, 0.36f };                          // grey woodsmoke haze
        fogDen   = 0.0130f;                                          // thick, smoky
    } else if (g_level == LEVEL_PLAZA) {                            // bright sunny civic-plaza light
        lightDir = Vector3Normalize({ 0.35f, 0.80f, 0.25f });        // clear midday sun
        lightCol = { 1.06f, 1.00f, 0.88f };                          // warm bright daylight
        ambient  = { 0.58f, 0.58f, 0.54f };                          // open bright ambient
        fogCol   = { 0.66f, 0.68f, 0.66f };                          // light city haze
        fogDen   = 0.0070f;                                          // clear, open
    } else if (g_level == LEVEL_PUMPKIN) {                          // warm autumn-dusk harvest light
        lightDir = Vector3Normalize({ 0.40f, 0.52f, 0.30f });        // low golden sun
        lightCol = { 1.08f, 0.78f, 0.46f };                          // rich amber dusk
        ambient  = { 0.50f, 0.42f, 0.34f };                          // warm dusky ambient
        fogCol   = { 0.46f, 0.36f, 0.30f };                          // dim autumn haze
        fogDen   = 0.0100f;                                          // soft, hazy
    } else if (g_level == LEVEL_BELL) {                             // overcast bronze bell-yard light
        lightDir = Vector3Normalize({ 0.28f, 0.72f, 0.24f });        // soft overcast sun
        lightCol = { 0.98f, 0.90f, 0.76f };                          // warm bronze-tinged light
        ambient  = { 0.48f, 0.46f, 0.42f };                          // neutral working ambient
        fogCol   = { 0.50f, 0.48f, 0.44f };                          // pale overcast haze
        fogDen   = 0.0090f;                                          // soft, open
    } else if (g_level == LEVEL_AVIARY) {                           // dim dusty aviary lighting
        lightDir = Vector3Normalize({ 0.22f, 0.78f, 0.20f });        // soft light through bars
        lightCol = { 0.92f, 0.86f, 0.72f };                          // warm dusty light
        ambient  = { 0.46f, 0.44f, 0.40f };                          // dim shaded ambient
        fogCol   = { 0.42f, 0.40f, 0.36f };                          // dusty cage haze
        fogDen   = 0.0105f;                                          // close, feathered
    } else if (g_level == LEVEL_WEB) {                              // dark webbed-cavern lighting
        lightDir = Vector3Normalize({ -0.20f, 0.66f, 0.25f });       // dim cave light
        lightCol = { 0.62f, 0.70f, 0.62f };                          // sickly pale-green light
        ambient  = { 0.34f, 0.38f, 0.34f };                          // dim, dank
        fogCol   = { 0.16f, 0.20f, 0.17f };                          // dark cavern murk
        fogDen   = 0.0125f;                                          // close, gloomy
    } else if (g_level == LEVEL_LOTUS) {                            // serene lotus-pond lighting
        lightDir = Vector3Normalize({ 0.30f, 0.74f, 0.22f });        // soft warm sun
        lightCol = { 1.02f, 0.96f, 0.84f };                          // gentle warm daylight
        ambient  = { 0.54f, 0.56f, 0.52f };                          // soft bright ambient
        fogCol   = { 0.62f, 0.68f, 0.64f };                          // soft misty haze
        fogDen   = 0.0080f;                                          // light, calm
    } else if (g_level == LEVEL_ARCHTREE) {                         // sacred-grove lighting
        lightDir = Vector3Normalize({ 0.26f, 0.80f, 0.34f });        // soft sun filtering through the canopy
        lightCol = { 0.96f, 0.98f, 0.78f };                         // warm green-gold daylight
        ambient  = { 0.46f, 0.54f, 0.42f };                         // mossy green ambient
        fogCol   = { 0.50f, 0.62f, 0.50f };                         // soft green forest haze
        fogDen   = 0.0090f;                                          // gentle dappled depth
    } else if (g_level == LEVEL_CHESS) {                            // marble-hall gambit lighting
        lightDir = Vector3Normalize({ 0.32f, 0.86f, 0.30f });        // clean overhead hall light
        lightCol = { 0.96f, 0.97f, 1.02f };                         // cool neutral marble white
        ambient  = { 0.50f, 0.52f, 0.58f };                         // soft cool fill
        fogCol   = { 0.58f, 0.62f, 0.72f };                         // pale cool haze
        fogDen   = 0.0075f;                                          // crisp, airy
    } else if (g_level == LEVEL_MAZE) {                             // hedge-maze garden lighting
        lightDir = Vector3Normalize({ 0.30f, 0.78f, 0.40f });        // soft late-afternoon sun
        lightCol = { 1.00f, 0.97f, 0.82f };                         // warm golden daylight
        ambient  = { 0.46f, 0.52f, 0.44f };                         // soft green-shadow fill
        fogCol   = { 0.56f, 0.64f, 0.54f };                         // gentle green garden haze
        fogDen   = 0.0085f;                                          // soft enclosed depth
    } else if (g_level == LEVEL_FALLS) {                            // misty waterfall lighting
        lightDir = Vector3Normalize({ 0.28f, 0.82f, 0.36f });        // bright fresh overhead
        lightCol = { 0.92f, 0.98f, 1.02f };                         // cool fresh daylight
        ambient  = { 0.50f, 0.56f, 0.60f };                         // cool misty fill
        fogCol   = { 0.66f, 0.74f, 0.78f };                         // pale spray haze
        fogDen   = 0.0095f;                                          // humid, hazy
    } else if (g_level == LEVEL_CRATER) {                           // fallen-star impact lighting
        lightDir = Vector3Normalize({ 0.20f, 0.78f, 0.30f });        // faint cold overhead
        lightCol = { 0.62f, 0.58f, 0.74f };                         // dim violet-tinged moonlight
        ambient  = { 0.30f, 0.28f, 0.40f };                         // dark violet ambient
        fogCol   = { 0.20f, 0.18f, 0.28f };                         // deep dark haze
        fogDen   = 0.0110f;                                          // thick, ominous
    } else if (g_level == LEVEL_TITAN) {                            // fallen-titan desolate-dusk lighting
        lightDir = Vector3Normalize({ 0.42f, 0.58f, 0.30f });        // low warm raking dusk sun
        lightCol = { 1.04f, 0.84f, 0.58f };                         // warm amber light
        ambient  = { 0.46f, 0.42f, 0.38f };                         // dusty warm-grey fill
        fogCol   = { 0.60f, 0.52f, 0.42f };                         // dusty haze on the lone sands
        fogDen   = 0.0090f;                                          // hazy, melancholic
    } else if (g_level == LEVEL_HOODOO) {                           // red-rock badlands lighting
        lightDir = Vector3Normalize({ 0.40f, 0.74f, 0.30f });        // warm sandstone sun
        lightCol = { 1.10f, 0.82f, 0.56f };                         // warm orange daylight
        ambient  = { 0.50f, 0.46f, 0.42f };                         // warm rock fill
        fogCol   = { 0.78f, 0.66f, 0.52f };                         // warm dusty haze
        fogDen   = 0.0070f;                                          // clear bright desert air
    } else if (g_level == LEVEL_MOAI) {                             // windswept coastal-headland lighting
        lightDir = Vector3Normalize({ 0.34f, 0.70f, 0.36f });        // soft diffuse overcast
        lightCol = { 0.86f, 0.88f, 0.90f };                         // cool overcast daylight
        ambient  = { 0.48f, 0.50f, 0.52f };                         // cool even fill
        fogCol   = { 0.66f, 0.70f, 0.72f };                         // cool coastal haze
        fogDen   = 0.0090f;                                          // misty sea air
    } else if (g_level == LEVEL_CAVERN) {                           // dripstone-cavern lighting
        lightDir = Vector3Normalize({ 0.20f, 0.84f, 0.30f });        // faint light from above
        lightCol = { 0.70f, 0.74f, 0.78f };                         // dim cool cave light
        ambient  = { 0.34f, 0.38f, 0.42f };                         // dark cool fill
        fogCol   = { 0.26f, 0.32f, 0.36f };                         // dark teal-grey murk
        fogDen   = 0.0120f;                                          // close, enclosed depth
    } else if (g_level == LEVEL_PINES) {                            // snowy pine-forest lighting
        lightDir = Vector3Normalize({ 0.32f, 0.80f, 0.30f });        // bright winter sun
        lightCol = { 0.92f, 0.96f, 1.06f };                         // cold bright snow-light
        ambient  = { 0.56f, 0.60f, 0.68f };                         // bright cool snow fill
        fogCol   = { 0.74f, 0.80f, 0.90f };                         // cold pale haze
        fogDen   = 0.0085f;                                          // crisp winter air
    } else if (g_level == LEVEL_GALLEON) {                          // storm-tossed ship-deck lighting
        lightDir = Vector3Normalize({ 0.30f, 0.66f, 0.40f });        // low stormy light
        lightCol = { 0.70f, 0.74f, 0.82f };                         // cold grey sea-storm light
        ambient  = { 0.38f, 0.42f, 0.50f };                         // dim cool fill
        fogCol   = { 0.40f, 0.46f, 0.54f };                         // grey sea spray
        fogDen   = 0.0105f;                                          // heavy storm haze
    } else if (g_level == LEVEL_SUNFLOWER) {                        // sunny sunflower-field lighting
        lightDir = Vector3Normalize({ 0.36f, 0.78f, 0.34f });        // warm high sun
        lightCol = { 1.12f, 1.00f, 0.70f };                         // warm golden daylight
        ambient  = { 0.52f, 0.52f, 0.42f };                         // warm fill
        fogCol   = { 0.74f, 0.74f, 0.54f };                         // warm hazy gold
        fogDen   = 0.0072f;                                          // bright clear air
    } else if (g_level == LEVEL_SPHINX) {                          // dawn-lit Giza necropolis lighting
        lightDir = Vector3Normalize({ 0.52f, 0.50f, 0.30f });       // low raking dawn sun
        lightCol = { 1.14f, 0.92f, 0.62f };                        // warm gold light
        ambient  = { 0.50f, 0.50f, 0.52f };                        // cool dawn fill
        fogCol   = { 0.72f, 0.72f, 0.66f };                        // pale warm desert haze
        fogDen   = 0.0072f;                                         // clear dawn air
    } else if (g_level == LEVEL_DAM) {                             // great-dam hydro lighting
        lightDir = Vector3Normalize({ 0.34f, 0.80f, 0.32f });       // bright overcast daylight
        lightCol = { 0.94f, 0.97f, 1.00f };                        // cool neutral concrete light
        ambient  = { 0.50f, 0.54f, 0.58f };                        // cool fill
        fogCol   = { 0.64f, 0.70f, 0.74f };                        // cool spray haze
        fogDen   = 0.0088f;                                         // damp, hazy
    } else if (g_level == LEVEL_THEATRE) {                         // sunlit classical-theatre lighting
        lightDir = Vector3Normalize({ 0.42f, 0.72f, 0.34f });       // warm afternoon sun
        lightCol = { 1.10f, 1.00f, 0.80f };                        // warm marble daylight
        ambient  = { 0.52f, 0.52f, 0.48f };                        // soft warm fill
        fogCol   = { 0.76f, 0.74f, 0.64f };                        // warm golden haze
        fogDen   = 0.0066f;                                         // bright clear air
    } else if (g_level == LEVEL_WHEEL) {                           // dusk fairground-wheel lighting
        lightDir = Vector3Normalize({ 0.40f, 0.66f, 0.42f });       // low golden-dusk light
        lightCol = { 1.00f, 0.86f, 0.74f };                        // warm sunset glow
        ambient  = { 0.50f, 0.48f, 0.56f };                        // soft violet-blue evening fill
        fogCol   = { 0.46f, 0.44f, 0.54f };                        // dusky haze
        fogDen   = 0.0080f;                                        // soft evening depth
    } else if (g_level == LEVEL_REDWOOD) {                         // ancient-grove filtered lighting
        lightDir = Vector3Normalize({ 0.34f, 0.84f, 0.30f });       // soft light from high above
        lightCol = { 0.96f, 0.98f, 0.78f };                        // green-gold filtered daylight
        ambient  = { 0.44f, 0.52f, 0.42f };                        // cool green shade
        fogCol   = { 0.56f, 0.64f, 0.54f };                        // misty green-grey
        fogDen   = 0.0092f;                                        // soft cathedral haze
    } else if (g_level == LEVEL_BALLOON) {                         // dawn balloon-festival lighting
        lightDir = Vector3Normalize({ 0.42f, 0.66f, 0.40f });       // low warm dawn sun
        lightCol = { 1.06f, 0.94f, 0.74f };                        // warm dawn glow
        ambient  = { 0.52f, 0.54f, 0.56f };                        // cool morning fill
        fogCol   = { 0.74f, 0.76f, 0.78f };                        // pale morning mist
        fogDen   = 0.0078f;                                        // clear bright dawn
    } else if (g_level == LEVEL_CANAL) {                           // warm Venetian-canal lighting
        lightDir = Vector3Normalize({ 0.40f, 0.74f, 0.32f });       // warm afternoon sun
        lightCol = { 1.08f, 0.96f, 0.76f };                        // warm golden daylight
        ambient  = { 0.50f, 0.52f, 0.50f };                        // soft warm fill
        fogCol   = { 0.70f, 0.72f, 0.66f };                        // warm hazy
        fogDen   = 0.0078f;                                        // bright clear air
    } else if (g_level == LEVEL_SILO) {                            // dusty prairie grain-works lighting
        lightDir = Vector3Normalize({ 0.44f, 0.72f, 0.34f });       // warm prairie sun
        lightCol = { 1.10f, 1.00f, 0.78f };                        // warm golden daylight
        ambient  = { 0.50f, 0.50f, 0.46f };                        // warm fill
        fogCol   = { 0.78f, 0.74f, 0.62f };                        // warm grain-dust haze
        fogDen   = 0.0082f;                                        // dusty
    } else if (g_level == LEVEL_ORGAN) {                           // dim warm organ-hall lighting
        lightDir = Vector3Normalize({ 0.30f, 0.80f, 0.34f });       // soft overhead
        lightCol = { 0.92f, 0.82f, 0.64f };                        // warm candle/interior light
        ambient  = { 0.42f, 0.40f, 0.42f };                        // dim interior fill
        fogCol   = { 0.30f, 0.28f, 0.30f };                        // dark interior haze
        fogDen   = 0.0100f;                                        // close, hushed
    } else if (g_level == LEVEL_HYPO) {                            // hypostyle-hall sandstone lighting
        lightDir = Vector3Normalize({ 0.30f, 0.84f, 0.30f });       // light from the clerestory above
        lightCol = { 1.00f, 0.86f, 0.60f };                        // warm sandstone light
        ambient  = { 0.40f, 0.38f, 0.34f };                        // dim warm fill
        fogCol   = { 0.36f, 0.32f, 0.26f };                        // dark warm interior haze
        fogDen   = 0.0098f;                                        // deep, vast
    } else if (g_level == LEVEL_FORT) {                            // overcast star-fort lighting
        lightDir = Vector3Normalize({ 0.36f, 0.72f, 0.38f });       // flat overcast light
        lightCol = { 0.90f, 0.88f, 0.84f };                        // cool grey daylight
        ambient  = { 0.48f, 0.48f, 0.50f };                        // even fill
        fogCol   = { 0.58f, 0.58f, 0.58f };                        // grey siege haze
        fogDen   = 0.0090f;                                        // smoky, overcast
    } else if (g_level == LEVEL_TRIUMPH) {                         // warm imperial-monument lighting
        lightDir = Vector3Normalize({ 0.40f, 0.74f, 0.34f });       // warm late-afternoon sun
        lightCol = { 1.10f, 0.98f, 0.74f };                        // warm imperial gold
        ambient  = { 0.50f, 0.50f, 0.46f };                        // soft warm fill
        fogCol   = { 0.72f, 0.70f, 0.60f };                        // warm golden haze
        fogDen   = 0.0072f;                                        // bright clear air
    } else if (g_level == LEVEL_ORCHARD) {                         // spring-blossom orchard lighting
        lightDir = Vector3Normalize({ 0.34f, 0.78f, 0.36f });       // soft warm spring sun
        lightCol = { 1.08f, 1.00f, 0.86f };                        // gentle warm daylight
        ambient  = { 0.54f, 0.54f, 0.52f };                        // soft bright fill
        fogCol   = { 0.80f, 0.74f, 0.76f };                        // soft pink-tinged haze
        fogDen   = 0.0072f;                                        // light, airy
    } else if (g_level == LEVEL_LOOM) {                            // warm dim weaver's-hall lighting
        lightDir = Vector3Normalize({ 0.30f, 0.80f, 0.36f });       // soft workshop light
        lightCol = { 0.96f, 0.86f, 0.68f };                        // warm lamp glow
        ambient  = { 0.44f, 0.42f, 0.40f };                        // dim warm fill
        fogCol   = { 0.34f, 0.30f, 0.28f };                        // dark warm interior haze
        fogDen   = 0.0098f;                                        // close, hushed
    } else if (g_level == LEVEL_SAVANNA) {                         // warm golden savanna lighting
        lightDir = Vector3Normalize({ 0.42f, 0.70f, 0.32f });       // warm low golden sun
        lightCol = { 1.14f, 0.96f, 0.66f };                        // golden daylight
        ambient  = { 0.52f, 0.50f, 0.42f };                        // warm fill
        fogCol   = { 0.82f, 0.74f, 0.56f };                        // golden dusty haze
        fogDen   = 0.0078f;                                        // hazy heat
    } else if (g_level == LEVEL_MOSQUE) {                          // warm sunlit-mosque lighting
        lightDir = Vector3Normalize({ 0.38f, 0.74f, 0.34f });       // warm afternoon sun
        lightCol = { 1.10f, 0.98f, 0.78f };                        // warm golden daylight
        ambient  = { 0.52f, 0.52f, 0.50f };                        // soft warm fill
        fogCol   = { 0.74f, 0.74f, 0.70f };                        // soft warm haze
        fogDen   = 0.0070f;                                        // bright clear air
    } else if (g_level == LEVEL_TOTEM) {                           // misty coastal totem-village lighting
        lightDir = Vector3Normalize({ 0.32f, 0.74f, 0.36f });       // soft overcast forest light
        lightCol = { 0.86f, 0.90f, 0.88f };                        // cool grey-green daylight
        ambient  = { 0.46f, 0.50f, 0.48f };                        // cool even fill
        fogCol   = { 0.60f, 0.66f, 0.64f };                        // cool coastal mist
        fogDen   = 0.0098f;                                        // damp, foggy
    } else if (g_level == LEVEL_OASIS) {                           // bright desert-oasis lighting
        lightDir = Vector3Normalize({ 0.36f, 0.82f, 0.30f });       // high desert sun
        lightCol = { 1.18f, 1.06f, 0.82f };                        // brilliant warm sunlight
        ambient  = { 0.54f, 0.52f, 0.44f };                        // warm sand-bounce fill
        fogCol   = { 0.86f, 0.80f, 0.66f };                        // pale heat haze
        fogDen   = 0.0060f;                                        // clear dry air, far dunes hazed
    } else if (g_level == LEVEL_PAGODA) {                         // serene golden-afternoon temple light
        lightDir = Vector3Normalize({ 0.40f, 0.66f, 0.42f });       // warm low afternoon sun
        lightCol = { 1.12f, 0.98f, 0.80f };                        // soft warm gold
        ambient  = { 0.50f, 0.48f, 0.46f };                        // gentle warm fill
        fogCol   = { 0.86f, 0.78f, 0.78f };                        // soft cherry-blossom haze
        fogDen   = 0.0072f;                                        // calm, slightly hazy
    } else if (g_level == LEVEL_STEPWELL) {                       // warm sandstone stepwell light
        lightDir = Vector3Normalize({ 0.42f, 0.78f, 0.30f });       // bright high desert-India sun
        lightCol = { 1.16f, 1.02f, 0.80f };                        // warm sandstone glow
        ambient  = { 0.52f, 0.48f, 0.42f };                        // warm bounced fill (deep court)
        fogCol   = { 0.80f, 0.72f, 0.58f };                        // dusty golden haze
        fogDen   = 0.0062f;                                        // clear, dry
    } else if (g_level == LEVEL_JANTAR) {                         // bright clear-midday observatory light
        lightDir = Vector3Normalize({ 0.35f, 0.86f, 0.30f });       // high midday sun (sharp shadows)
        lightCol = { 1.18f, 1.08f, 0.86f };                        // bright warm daylight
        ambient  = { 0.50f, 0.48f, 0.44f };                        // even warm fill
        fogCol   = { 0.82f, 0.78f, 0.66f };                        // faint dusty haze
        fogDen   = 0.0050f;                                        // very clear air
    } else if (g_level == LEVEL_TERRACOTTA) {                     // warm dusty excavation-pit light
        lightDir = Vector3Normalize({ 0.40f, 0.70f, 0.40f });       // soft raking light into the trench
        lightCol = { 1.08f, 0.94f, 0.74f };                        // warm earthy daylight
        ambient  = { 0.46f, 0.42f, 0.36f };                        // dim earthy fill
        fogCol   = { 0.66f, 0.56f, 0.42f };                        // thick ochre dust haze
        fogDen   = 0.0110f;                                        // dusty, enclosed dig
    } else if (g_level == LEVEL_BALLCOURT) {                      // warm tropical Maya-ruin light
        lightDir = Vector3Normalize({ 0.40f, 0.78f, 0.34f });       // warm jungle sun
        lightCol = { 1.14f, 1.04f, 0.84f };                        // warm limestone glow
        ambient  = { 0.50f, 0.50f, 0.44f };                        // soft warm fill
        fogCol   = { 0.74f, 0.76f, 0.62f };                        // humid green-gold haze
        fogDen   = 0.0078f;                                        // tropical humidity
    } else if (g_level == LEVEL_PETRA) {                          // warm rose-red rock-cut canyon light
        lightDir = Vector3Normalize({ 0.45f, 0.62f, 0.40f });       // low sun raking the facade
        lightCol = { 1.18f, 0.96f, 0.74f };                        // warm golden light on red stone
        ambient  = { 0.48f, 0.40f, 0.34f };                        // deep warm shadow in the canyon
        fogCol   = { 0.74f, 0.56f, 0.44f };                        // rose-red dust haze
        fogDen   = 0.0072f;                                        // dry, dusty
    } else if (g_level == LEVEL_BAZAAR) {                         // warm covered-souk lantern light
        lightDir = Vector3Normalize({ 0.35f, 0.72f, 0.40f });       // soft light through the awnings
        lightCol = { 1.10f, 0.96f, 0.74f };                        // warm filtered daylight
        ambient  = { 0.48f, 0.44f, 0.38f };                        // dim warm interior fill
        fogCol   = { 0.66f, 0.58f, 0.46f };                        // warm spice-dust haze
        fogDen   = 0.0090f;                                        // hazy, enclosed market
    } else if (g_level == LEVEL_SIEGE) {                          // grim smoky overcast battlefield light
        lightDir = Vector3Normalize({ 0.40f, 0.60f, 0.42f });       // flat overcast light
        lightCol = { 0.92f, 0.90f, 0.86f };                        // cold grey daylight
        ambient  = { 0.44f, 0.44f, 0.46f };                        // dull cool fill
        fogCol   = { 0.56f, 0.54f, 0.50f };                        // grey-brown battle smoke
        fogDen   = 0.0110f;                                        // thick smoke
    } else if (g_level == LEVEL_WHALING) {                        // cold grey foggy-harbour light
        lightDir = Vector3Normalize({ 0.40f, 0.62f, 0.40f });       // flat cold daylight
        lightCol = { 0.86f, 0.90f, 0.94f };                        // pale cold light
        ambient  = { 0.44f, 0.46f, 0.50f };                        // cool damp fill
        fogCol   = { 0.62f, 0.66f, 0.70f };                        // thick cold sea fog
        fogDen   = 0.0120f;                                        // dense fog off the water
    } else if (g_level == LEVEL_CACTUS) {                         // blazing hot Sonoran-desert light
        lightDir = Vector3Normalize({ 0.38f, 0.84f, 0.30f });       // high blasting sun
        lightCol = { 1.20f, 1.10f, 0.86f };                        // brilliant warm daylight
        ambient  = { 0.54f, 0.50f, 0.42f };                        // warm sand-bounce fill
        fogCol   = { 0.84f, 0.78f, 0.64f };                        // pale heat haze
        fogDen   = 0.0055f;                                        // very clear, far heat shimmer
    } else if (g_level == LEVEL_ZIMBABWE) {                       // warm African-afternoon granite light
        lightDir = Vector3Normalize({ 0.42f, 0.74f, 0.36f });       // warm low afternoon sun
        lightCol = { 1.16f, 1.04f, 0.80f };                        // golden light on grey granite
        ambient  = { 0.50f, 0.48f, 0.42f };                        // warm fill
        fogCol   = { 0.78f, 0.72f, 0.58f };                        // dry golden haze
        fogDen   = 0.0065f;                                        // clear, dusty
    } else if (g_level == LEVEL_BASIL) {                          // crisp cold bright-day light
        lightDir = Vector3Normalize({ 0.36f, 0.80f, 0.34f });       // clear winter sun
        lightCol = { 1.12f, 1.08f, 1.02f };                        // bright cool daylight
        ambient  = { 0.52f, 0.54f, 0.58f };                        // cool blue-sky fill
        fogCol   = { 0.80f, 0.82f, 0.86f };                        // pale cold haze
        fogDen   = 0.0058f;                                        // crisp, clear air
    } else if (g_level == LEVEL_PRINT) {                          // warm dim candlelit-workshop light
        lightDir = Vector3Normalize({ 0.36f, 0.70f, 0.42f });       // soft slanted window light
        lightCol = { 1.04f, 0.92f, 0.74f };                        // warm lamp daylight
        ambient  = { 0.42f, 0.40f, 0.36f };                        // dim warm fill
        fogCol   = { 0.58f, 0.54f, 0.46f };                        // warm ink-and-dust haze
        fogDen   = 0.0095f;                                        // dim, hazy interior
    } else if (g_level == LEVEL_GER) {                            // bright open-steppe light
        lightDir = Vector3Normalize({ 0.40f, 0.80f, 0.34f });       // high steppe sun
        lightCol = { 1.16f, 1.10f, 0.88f };                        // warm clear daylight
        ambient  = { 0.54f, 0.54f, 0.48f };                        // open sky fill
        fogCol   = { 0.80f, 0.82f, 0.70f };                        // pale grassland haze
        fogDen   = 0.0055f;                                        // very clear, wide horizon
    } else if (g_level == LEVEL_ALCHEMY) {                        // dim eerie alchemical-glow light
        lightDir = Vector3Normalize({ 0.30f, 0.66f, 0.42f });       // weak slanted lamplight
        lightCol = { 0.84f, 0.86f, 0.74f };                        // dim sickly daylight
        ambient  = { 0.34f, 0.38f, 0.36f };                        // murky green-grey fill
        fogCol   = { 0.36f, 0.42f, 0.40f };                        // dark vaporous haze
        fogDen   = 0.0115f;                                        // thick fumes
    } else if (g_level == LEVEL_BATHS) {                          // warm soft steamy bath-house light
        lightDir = Vector3Normalize({ 0.36f, 0.74f, 0.40f });       // soft light through the dome
        lightCol = { 1.12f, 1.04f, 0.90f };                        // warm marble glow
        ambient  = { 0.52f, 0.52f, 0.50f };                        // soft even fill
        fogCol   = { 0.82f, 0.82f, 0.80f };                        // pale warm steam
        fogDen   = 0.0095f;                                        // thick rising steam
    } else if (g_level == LEVEL_FLOAT) {                          // warm humid tropical-river light
        lightDir = Vector3Normalize({ 0.40f, 0.78f, 0.34f });       // bright tropical sun
        lightCol = { 1.16f, 1.06f, 0.84f };                        // warm golden daylight
        ambient  = { 0.52f, 0.52f, 0.46f };                        // warm humid fill
        fogCol   = { 0.76f, 0.80f, 0.70f };                        // green-gold river haze
        fogDen   = 0.0072f;                                        // humid, slightly hazy
    } else if (g_level == LEVEL_ISHTAR) {                         // bright warm Babylonian-monument light
        lightDir = Vector3Normalize({ 0.40f, 0.78f, 0.32f });       // high desert sun
        lightCol = { 1.18f, 1.08f, 0.86f };                        // brilliant warm daylight
        ambient  = { 0.52f, 0.52f, 0.50f };                        // soft sky fill (lifts the blue)
        fogCol   = { 0.82f, 0.78f, 0.66f };                        // pale dusty haze
        fogDen   = 0.0058f;                                        // clear, dry
    } else if (g_level == LEVEL_TANNERY) {                        // hot bright North-African tannery light
        lightDir = Vector3Normalize({ 0.40f, 0.82f, 0.30f });       // high blazing sun
        lightCol = { 1.18f, 1.08f, 0.84f };                        // brilliant warm daylight
        ambient  = { 0.54f, 0.52f, 0.44f };                        // warm earthy fill
        fogCol   = { 0.82f, 0.78f, 0.64f };                        // pungent dusty haze
        fogDen   = 0.0058f;                                        // clear, dry, hot
    } else if (g_level == LEVEL_MUSEUM) {                         // soft dusty museum-hall light
        lightDir = Vector3Normalize({ 0.34f, 0.80f, 0.40f });       // soft light from the skylight
        lightCol = { 1.06f, 1.02f, 0.92f };                        // cool neutral daylight
        ambient  = { 0.46f, 0.46f, 0.46f };                        // even soft fill
        fogCol   = { 0.66f, 0.66f, 0.66f };                        // dim dusty haze
        fogDen   = 0.0085f;                                        // hushed, dusty interior
    } else if (g_level == LEVEL_GOMPA) {                          // bright cold high-altitude light
        lightDir = Vector3Normalize({ 0.36f, 0.80f, 0.36f });       // crisp mountain sun
        lightCol = { 1.14f, 1.10f, 1.02f };                        // clear bright daylight
        ambient  = { 0.52f, 0.54f, 0.58f };                        // cool blue-sky fill
        fogCol   = { 0.82f, 0.85f, 0.90f };                        // pale cold mountain haze
        fogDen   = 0.0062f;                                        // thin, clear high air
    } else if (g_level == LEVEL_BUDDHA) {                         // warm serene golden grotto light
        lightDir = Vector3Normalize({ 0.36f, 0.74f, 0.42f });       // soft warm afternoon sun
        lightCol = { 1.14f, 1.04f, 0.84f };                        // warm gilded light
        ambient  = { 0.50f, 0.50f, 0.46f };                        // soft warm fill
        fogCol   = { 0.78f, 0.74f, 0.62f };                        // warm incense haze
        fogDen   = 0.0068f;                                        // soft, serene
    } else if (g_level == LEVEL_ANGKOR) {                         // warm humid tropical-temple light
        lightDir = Vector3Normalize({ 0.40f, 0.74f, 0.36f });       // warm jungle sun
        lightCol = { 1.14f, 1.06f, 0.84f };                        // warm sandstone glow
        ambient  = { 0.52f, 0.52f, 0.46f };                        // soft warm fill
        fogCol   = { 0.78f, 0.80f, 0.68f };                        // humid green-gold haze
        fogDen   = 0.0072f;                                        // tropical humidity
    } else if (g_level == LEVEL_BADGIR) {                         // hot bright Persian-desert-town light
        lightDir = Vector3Normalize({ 0.40f, 0.82f, 0.30f });       // high blazing sun
        lightCol = { 1.20f, 1.10f, 0.86f };                        // brilliant warm daylight
        ambient  = { 0.54f, 0.52f, 0.44f };                        // warm earthy fill
        fogCol   = { 0.84f, 0.78f, 0.64f };                        // pale dusty heat haze
        fogDen   = 0.0055f;                                        // very clear, hot, dry
    } else if (g_level == LEVEL_TOPIARY) {                        // bright sunny formal-garden light
        lightDir = Vector3Normalize({ 0.36f, 0.80f, 0.40f });       // clear afternoon sun
        lightCol = { 1.16f, 1.12f, 0.96f };                        // bright fresh daylight
        ambient  = { 0.54f, 0.56f, 0.52f };                        // soft sky fill
        fogCol   = { 0.82f, 0.86f, 0.80f };                        // soft green-tinged air
        fogDen   = 0.0055f;                                        // clear, pleasant
    } else if (g_level == LEVEL_GLASSWORKS) {                     // hot dim furnace-lit hot-shop light
        lightDir = Vector3Normalize({ 0.34f, 0.68f, 0.42f });       // dim slanted work light
        lightCol = { 0.92f, 0.84f, 0.72f };                        // warm dim daylight
        ambient  = { 0.40f, 0.38f, 0.36f };                        // dim warm fill
        fogCol   = { 0.50f, 0.42f, 0.36f };                        // hot smoky haze
        fogDen   = 0.0095f;                                        // dim, smoky interior
    } else if (g_level == LEVEL_SHIPYARD) {                       // cool grey dawn over a coastal slipway
        lightDir = Vector3Normalize({ 0.40f, 0.58f, 0.46f });       // low, raking sea-dawn sun
        lightCol = { 0.86f, 0.86f, 0.82f };                        // overcast, slightly cool
        ambient  = { 0.36f, 0.38f, 0.40f };                        // soft blue-grey sea fill
        fogCol   = { 0.62f, 0.66f, 0.70f };                        // salt-air sea mist
        fogDen   = 0.0072f;                                        // hazy harbour morning
    } else if (g_level == LEVEL_GOTHIC) {                         // cold moonlit night over a Gothic close
        lightDir = Vector3Normalize({ 0.30f, 0.72f, -0.42f });      // high moon behind the spires
        lightCol = { 0.60f, 0.66f, 0.86f };                        // pale cold moonlight
        ambient  = { 0.24f, 0.27f, 0.36f };                        // deep blue night fill
        fogCol   = { 0.28f, 0.32f, 0.44f };                        // cold blue night haze
        fogDen   = 0.0090f;                                        // moody, dim
    } else if (g_level == LEVEL_PUEBLO) {                         // warm low desert dusk in a cliff alcove
        lightDir = Vector3Normalize({ 0.52f, 0.46f, 0.30f });      // low raking golden sun
        lightCol = { 1.16f, 0.92f, 0.64f };                        // warm sandstone gold
        ambient  = { 0.40f, 0.34f, 0.28f };                        // warm alcove fill
        fogCol   = { 0.66f, 0.50f, 0.36f };                        // dusty warm haze
        fogDen   = 0.0060f;                                        // clear, dry desert air
    }
    float   emissive = 0.0f;
    SetShaderValue(shader, loc_lightDir, &lightDir, SHADER_UNIFORM_VEC3);
    SetShaderValue(shader, loc_lightColor, &lightCol, SHADER_UNIFORM_VEC3);
    SetShaderValue(shader, loc_ambient, &ambient, SHADER_UNIFORM_VEC3);
    SetShaderValue(shader, loc_fogColor, &fogCol, SHADER_UNIFORM_VEC3);
    SetShaderValue(shader, loc_fogDensity, &fogDen, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shader, loc_emissive, &emissive, SHADER_UNIFORM_FLOAT);
    set_clip(0.0f, false);   // reflection clip off by default (main pass)
}

void LitShader::unload() { if (ok) UnloadShader(shader); }

void LitShader::set_frame(Vector3 viewPos) {
    if (!ok) return;
    SetShaderValue(shader, loc_viewPos, &viewPos, SHADER_UNIFORM_VEC3);
}

void LitShader::set_emissive(float e) {
    if (!ok) return;
    SetShaderValue(shader, loc_emissive, &e, SHADER_UNIFORM_FLOAT);
}

void LitShader::set_point_lights(const Vector4* lights, int count, Vector3 color) {
    if (!ok) return;
    if (count > 32) count = 32;
    SetShaderValue(shader, loc_plightN, &count, SHADER_UNIFORM_INT);
    SetShaderValue(shader, loc_plightColor, &color, SHADER_UNIFORM_VEC3);
    if (count > 0)
        SetShaderValueV(shader, loc_plights, lights, SHADER_UNIFORM_VEC4, count);
}

void LitShader::set_clip(float water_y, bool below) {
    if (!ok) return;
    int b = below ? 1 : 0;
    SetShaderValue(shader, loc_clipBelow, &b, SHADER_UNIFORM_INT);
    SetShaderValue(shader, loc_clipY, &water_y, SHADER_UNIFORM_FLOAT);
}

void LitShader::apply_to_model(Model& m) {
    if (!ok) return;
    for (int i = 0; i < m.materialCount; i++) m.materials[i].shader = shader;
}
