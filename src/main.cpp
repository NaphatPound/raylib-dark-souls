// Soulslike â€” Boss Arena, ported from Godot to raylib/C++.
// Top-level loop: lighting, pause, restart, and the player-vs-boss fight.
#include "raylib.h"
#include "rlgl.h"
#include "mathx.h"
#include "game.h"
#include "anim.h"
#include "assets.h"
#include "sword.h"
#include "events.h"
#include "audio_sys.h"
#include "fx.h"
#include "juice.h"
#include "render.h"
#include "arena.h"
#include "assets.h"
#include "player.h"
#include "boss.h"
#include "hud.h"
#include "screens.h"
#include "pause_menu.h"
#include "save.h"
#include <vector>
#include <cstdio>

// ---- start-menu + bonfire drawing helpers (defined below main) ----
static void draw_start_menu(int sel);
static void draw_bonfire(Vector3 pos, float t);

int main(int argc, char** argv) {
    bool auto_demo = false, diag = false, scenic = false, level_arg = false;
    for (int i = 1; i < argc; i++) {
        if (TextIsEqual(argv[i], "auto")) auto_demo = true;
        if (TextIsEqual(argv[i], "diag")) diag = true;
        if (TextIsEqual(argv[i], "scenic")) scenic = true;
        if (TextIsEqual(argv[i], "ice") || TextIsEqual(argv[i], "frozen")) { g_level = LEVEL_FROZEN; level_arg = true; }
        if (TextIsEqual(argv[i], "forge") || TextIsEqual(argv[i], "lava")) { g_level = LEVEL_FORGE; level_arg = true; }
        if (TextIsEqual(argv[i], "colosseum") || TextIsEqual(argv[i], "pit")) { g_level = LEVEL_COLOSSEUM; level_arg = true; }
    }

    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(1280, 720, "Soulslike - Boss Arena (raylib)");
    SetTargetFPS(60);
    InitAudioDevice();
    SetExitKey(KEY_NULL);             // Esc is pause, not quit

    // ---- model diagnostic: render both characters from a clear camera ----
    if (diag) {
        g_lit.load();
        g_fx.init();
        Model pm = LoadModel(assets::path("characters/player.glb").c_str());
        Model em = LoadModel(assets::path("characters/enemy.glb").c_str());
        AnimController pac, eac;
        pac.load(&pm, assets::path("characters/player.glb").c_str()); pac.set_anim("standing_idle");
        eac.load(&em, assets::path("characters/enemy.glb").c_str()); eac.set_anim("zombie_idle");
        for (int i = 0; i < pm.materialCount; i++) {
            Color c = pm.materials[i].maps[MATERIAL_MAP_DIFFUSE].color;
            TraceLog(LOG_WARNING, "PLAYER mat %d  color=%d,%d,%d,%d  tex=%u", i, c.r, c.g, c.b, c.a,
                     pm.materials[i].maps[MATERIAL_MAP_DIFFUSE].texture.id);
        }
        for (int i = 0; i < em.materialCount; i++) {
            Color c = em.materials[i].maps[MATERIAL_MAP_DIFFUSE].color;
            TraceLog(LOG_WARNING, "ENEMY mat %d  color=%d,%d,%d,%d  tex=%u", i, c.r, c.g, c.b, c.a,
                     em.materials[i].maps[MATERIAL_MAP_DIFFUSE].texture.id);
        }
        Camera3D cam{}; cam.position = { 0, 1.2f, 5.0f }; cam.target = { 0, 1.0f, 0 };
        cam.up = { 0, 1, 0 }; cam.fovy = 55; cam.projection = CAMERA_PERSPECTIVE;
        int f = 0;
        while (!WindowShouldClose() && f < 240) {
            float dt = GetFrameTime();
            pac.update(dt); eac.update(dt);
            if (f % 18 == 0) g_fx.ripple({ -1.1f, 0, 0 }, 4.5f);   // ripple test at the player's feet
            if (f % 30 == 0) g_fx.hit({ 1.1f, 1.1f, 0 }, 26.0f);   // blood test on the enemy
            g_fx.update(dt);
            g_lit.set_frame(cam.position);
            bool lit = (f / 80) % 2 == 0;            // alternate lit / default shader
            Shader def{ rlGetShaderIdDefault(), rlGetShaderLocsDefault() };
            for (int i = 0; i < pm.materialCount; i++) pm.materials[i].shader = lit ? g_lit.shader : def;
            for (int i = 0; i < em.materialCount; i++) em.materials[i].shader = lit ? g_lit.shader : def;
            BeginDrawing();
            ClearBackground(Color{ 40, 30, 40, 255 });
            BeginMode3D(cam);
            DrawModelEx(pm, { -1.1f, 0, 0 }, { 0, 1, 0 }, 0, { 1, 1, 1 }, WHITE);
            draw_hand_sword(pm, pac, { -1.1f, 0, 0 }, 0.0f);
            DrawModelEx(em, { 1.1f, 0, 0 }, { 0, 1, 0 }, 0, { 1, 1, 1 }, WHITE);
            DrawGrid(10, 1.0f);
            g_fx.draw(cam);
            EndMode3D();
            DrawText(TextFormat("DIAG  player(L) enemy(R)  shader=%s  frame=%d", lit ? "lit" : "default", f), 10, 10, 20, RAYWHITE);
            EndDrawing();
            f++;
            if (f == 40)  TakeScreenshot("diag_lit.png");
            if (f == 120) TakeScreenshot("diag_default.png");
        }
        pac.unload(); eac.unload(); UnloadModel(pm); UnloadModel(em);
        CloseAudioDevice(); CloseWindow();
        return 0;
    }

    // ---- scenic: look across the water toward the moon to check reflection ----
    if (scenic) {
        g_lit.load();
        arena::load(g_lit.shader);
        RenderTexture2D rfx = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
        Camera3D cam{}; cam.position = { 0, 9.0f, -8 }; cam.target = { 0, 7.0f, -120 };
        cam.up = { 0, 1, 0 }; cam.fovy = 62; cam.projection = CAMERA_PERSPECTIVE;
        int f = 0;
        while (!WindowShouldClose() && f < 120) {
            arena::update(GetFrameTime());
            // mirror the camera across the water plane (same FOV) so the reflection
            // texture can be sampled at each water fragment's own screen position.
            Camera3D mc = cam; float wy = arena::water_level;
            mc.position.y = 2 * wy - mc.position.y; mc.target.y = 2 * wy - mc.target.y; mc.up.y = -mc.up.y;
            BeginTextureMode(rfx);
                ClearBackground(Color{ 13, 6, 13, 255 });
                g_lit.set_frame(mc.position);
                g_lit.set_clip(arena::water_level, true);
                BeginMode3D(mc);
                    arena::draw_sky(mc);
                    rlDisableBackfaceCulling();
                    arena::draw_world(mc);
                    rlEnableBackfaceCulling();
                EndMode3D();
            EndTextureMode();
            g_lit.set_clip(0.0f, false);
            Vector2 scr{ (float)GetScreenWidth(), (float)GetScreenHeight() };
            BeginDrawing();
            ClearBackground(Color{ 13, 6, 13, 255 });
            g_lit.set_frame(cam.position);
            BeginMode3D(cam);
                arena::draw_sky(cam);
                arena::draw_world(cam);
                arena::draw_water(cam, rfx.texture, scr);
            EndMode3D();
            // preview the reflection render-texture (bottom-right corner)
            int pw = GetScreenWidth() / 4, ph = GetScreenHeight() / 4;
            DrawTexturePro(rfx.texture, { 0, 0, (float)rfx.texture.width, -(float)rfx.texture.height },
                           { (float)(GetScreenWidth() - pw - 8), (float)(GetScreenHeight() - ph - 8), (float)pw, (float)ph },
                           { 0, 0 }, 0, WHITE);
            DrawText("SCENIC: water reflection (inset = reflection texture)", 10, 10, 20, RAYWHITE);
            EndDrawing();
            f++;
            if (f == 60) TakeScreenshot("scenic.png");
        }
        UnloadRenderTexture(rfx); arena::unload(); g_lit.unload();
        CloseAudioDevice(); CloseWindow();
        return 0;
    }

    // ---- per-session systems; the level itself is loaded/unloaded on demand ----
    g_fx.init();
    g_audio.init();
    g_juice.init();
    g_juice.enabled = true;

    HUD hud; hud.init();
    Screens screens; screens.init();
    save_load();

    Player player;
    Boss boss;
    PauseMenu pause; pause.player = &player;

    const Vector3 PLAYER_SPAWN{ 0, 0, 16 }, BOSS_SPAWN{ 0, 0, -10 }, BONFIRE_POS{ 0, 0, 7 };

    g_game.restart_requested.connect([&]() {
        player.pos = PLAYER_SPAWN; player.reset_state();
        boss.pos = BOSS_SPAWN;     boss.reset_state();
    });

    RenderTexture2D reflect = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
    std::vector<Hurtbox*> boss_targets, player_targets;
    bool level_loaded = false;

    auto load_level = [&](int lvl) {
        g_level = lvl;
        g_lit.load();
        arena::load(g_lit.shader);
        player.init(PLAYER_SPAWN); player.apply_shader(g_lit.shader);
        boss.init(BOSS_SPAWN);     boss.apply_shader(g_lit.shader);
        boss_targets   = { &boss.hurtbox };
        player_targets = { &player.hurtbox };
        player.autopilot = auto_demo;
        g_game.set_state(Game::TITLE);
        level_loaded = true;
    };
    auto unload_level = [&]() {
        if (!level_loaded) return;
        player.unload(); boss.unload();
        arena::unload(); g_lit.unload();
        level_loaded = false;
    };

    enum AppState { APP_MENU, APP_PLAY };
    int app = APP_MENU;
    int menu_sel = g_save.last_level;
    bool paused = false, bonfire_lit = false;
    long frame = 0;

    // CLI shortcut: `auto` or an explicit level arg skip the menu and play directly
    if (auto_demo || level_arg) { load_level(g_level); app = APP_PLAY; DisableCursor(); }

    while (!WindowShouldClose()) {
        float real_dt = GetFrameTime();
        if (real_dt > 0.05f) real_dt = 0.05f;
        g_audio.update(real_dt);

        // -------------------------------------------------------- START MENU
        if (app == APP_MENU) {
            if (IsCursorHidden()) EnableCursor();
            if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) menu_sel = (menu_sel + 1) % NUM_LEVELS;
            if (IsKeyPressed(KEY_UP)   || IsKeyPressed(KEY_W)) menu_sel = (menu_sel + NUM_LEVELS - 1) % NUM_LEVELS;
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_E)) {
                load_level(menu_sel); app = APP_PLAY; bonfire_lit = false; paused = false; DisableCursor();
            }
            BeginDrawing();
            ClearBackground(Color{ 8, 6, 10, 255 });
            draw_start_menu(menu_sel);
            if (IsKeyPressed(KEY_F2)) TakeScreenshot("shot.png");
            EndDrawing();
            continue;
        }

        // -------------------------------------------------------- PLAY
        g_juice.update(real_dt);

        if (IsKeyPressed(KEY_ESCAPE)) {
            if (!(g_game.state == Game::DEAD || g_game.state == Game::VICTORY)) {
                paused = !paused;
                g_game.paused = paused;
                if (paused) { EnableCursor(); pause.open(); }
                else DisableCursor();
            }
        }

        bool near_bonfire = false;
        if (!paused) {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !IsCursorHidden()) DisableCursor();

            player.handle_input();
            float dt = real_dt * g_juice.time_scale();
            player.update(dt, boss_targets);
            boss.update(dt, player_targets);

            // body-vs-body separation (player can't overlap the boss)
            Vector3 d = player.pos - boss.pos; d.y = 0;
            float dist = Vector3Length(d);
            float mind = 0.4f + 0.45f;
            if (dist < mind && dist > 0.0001f) {
                Vector3 push = Vector3Scale(Vector3Normalize(d), (mind - dist) * 0.5f);
                player.pos = player.pos + push;
                boss.pos = boss.pos - push;
                arena::resolve(player.pos, 0.4f);
                arena::resolve(boss.pos, 0.45f);
            }

            arena::update(dt);
            g_fx.update(dt);
            hud.update(real_dt);

            if (g_game.state == Game::DEAD) {                 // die -> retry the level
                if (IsKeyPressed(KEY_E) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER))
                    g_game.request_restart();
            } else if (g_game.state == Game::VICTORY) {        // win -> a bonfire ignites
                bonfire_lit = true;
                Vector3 bd = player.pos - BONFIRE_POS; bd.y = 0;
                near_bonfire = Vector3Length(bd) < 2.6f;
                if (near_bonfire && (IsKeyPressed(KEY_E) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER))) {
                    g_save.beaten[g_level] = true;             // rest = save progress, return to menu
                    g_save.last_level = g_level;
                    save_write();
                    unload_level();
                    menu_sel = g_level; app = APP_MENU; bonfire_lit = false;
                }
            }
        }
        if (app == APP_MENU) continue;   // rested at the bonfire -> show the menu next frame

        // --- planar reflection pass: render scenery from a water-mirrored camera ---
        if (reflect.texture.width != GetScreenWidth() || reflect.texture.height != GetScreenHeight()) {
            UnloadRenderTexture(reflect);
            reflect = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
        }
        // mirror the camera across the water plane (same FOV) so the reflection
        // texture can be sampled at each water fragment's own screen position.
        Camera3D mcam = player.camera;
        float wy = arena::water_level;
        mcam.position.y = 2.0f * wy - mcam.position.y;
        mcam.target.y = 2.0f * wy - mcam.target.y;
        mcam.up.y = -mcam.up.y;
        BeginTextureMode(reflect);
            ClearBackground(Color{ 13, 6, 13, 255 });
            g_lit.set_frame(mcam.position);
            g_lit.set_clip(arena::water_level, true);   // only above-water geometry reflects
            BeginMode3D(mcam);
                arena::draw_sky(mcam);
                rlDisableBackfaceCulling();   // mirror flips winding -> draw reflected geo unculled
                arena::draw_world(mcam);
                player.draw();                // reflect the fighters in the water too
                boss.draw();
                g_fx.draw(mcam);              // and the blood/spark VFX
                rlEnableBackfaceCulling();
            EndMode3D();
        EndTextureMode();
        g_lit.set_clip(0.0f, false);          // main pass: no clip

        Vector2 screen{ (float)GetScreenWidth(), (float)GetScreenHeight() };
        BeginDrawing();
        ClearBackground(Color{ 13, 6, 13, 255 });
        g_lit.set_frame(player.camera.position);
        BeginMode3D(player.camera);
            arena::draw_sky(player.camera);
            arena::draw_world(player.camera);
            player.draw();
            boss.draw();
            arena::draw_water(player.camera, reflect.texture, screen);
            g_fx.draw(player.camera);
            if (bonfire_lit) draw_bonfire(BONFIRE_POS, (float)GetTime());
        EndMode3D();

        hud.draw();
        screens.draw();
        if (bonfire_lit && !paused) {                        // bonfire rest hint
            int sw = GetScreenWidth(), sh = GetScreenHeight();
            const char* msg = near_bonfire ? "Press  E  to rest at the bonfire"
                                           : "Walk to the bonfire to rest";
            Color mc = near_bonfire ? Color{ 255, 200, 110, 255 } : Color{ 200, 170, 120, 200 };
            DrawText(msg, sw / 2 - MeasureText(msg, 22) / 2, sh - 96, 22, mc);
        }
        if (paused) {
            pause.draw_and_update();
            if (pause.resume_requested) { paused = false; g_game.paused = false; DisableCursor(); }
            if (pause.quit_requested) {                      // quit to the start menu
                unload_level(); menu_sel = g_level; app = APP_MENU;
                paused = false; g_game.paused = false; bonfire_lit = false;
            }
        }
        if (IsKeyPressed(KEY_F2)) TakeScreenshot("shot.png");
        EndDrawing();
        if (app == APP_MENU) continue;   // quit-to-menu from pause

        frame++;
        if (auto_demo) {
            player.hurtbox.invulnerable = true;   // god-mode so the demo reaches phase-2 + victory
            static int p2_shot = 0, vic_shot = 0;
            if (frame % 120 == 0)
                TraceLog(LOG_INFO, "DBG f=%ld player_hp=%.0f boss_hp=%.0f boss_phase=%d boss_state=%d",
                         frame, player.health.current, boss.health.current, boss.phase, boss.state);
            if (frame == 90)  TakeScreenshot("demo_title.png");
            if (frame == 600) TakeScreenshot("demo_fight.png");
            if (boss.phase == 2 && !p2_shot) { TakeScreenshot("demo_phase2.png"); p2_shot = 1; }
            if (g_game.state == Game::VICTORY && !vic_shot) { TakeScreenshot("demo_victory.png"); vic_shot = 1; }
            if (vic_shot && frame % 120 == 0) break;   // a moment after victory
            if (frame >= 4000) break;
        }
    }

    unload_level();
    UnloadRenderTexture(reflect);
    assets::unload_all();
    g_audio.shutdown();
    CloseAudioDevice();
    CloseWindow();
    return 0;
}

// ---------------------------------------------------------------- menu + bonfire
static void draw_start_menu(int sel) {
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    const char* title = "DARK SOULS";
    DrawText(title, sw / 2 - MeasureText(title, 70) / 2, sh / 6, 70, Color{ 196, 38, 30, 255 });
    const char* sub = "raylib port  -  select your world";
    DrawText(sub, sw / 2 - MeasureText(sub, 20) / 2, sh / 6 + 84, 20, Color{ 150, 140, 132, 255 });

    int y = sh / 2 - 40;
    for (int i = 0; i < NUM_LEVELS; i++) {
        bool on = (i == sel);
        Color c = on ? Color{ 255, 222, 130, 255 } : Color{ 150, 142, 132, 255 };
        int fs = 32, x = sw / 2 - 230;
        if (on) DrawText(">", x - 34, y, fs, c);
        DrawText(level_name(i), x, y, fs, c);
        if (g_save.beaten[i])
            DrawText("- cleared", x + 300, y + 6, 22, Color{ 120, 190, 120, 255 });
        y += 56;
    }
    const char* hint = "W / S  or  Up / Down to choose      Enter  to enter";
    DrawText(hint, sw / 2 - MeasureText(hint, 18) / 2, sh - 70, 18, Color{ 120, 115, 110, 255 });
    const char* hint2 = "Beat the boss, then rest at the bonfire to save.";
    DrawText(hint2, sw / 2 - MeasureText(hint2, 16) / 2, sh - 44, 16, Color{ 95, 90, 86, 255 });
}

static void draw_bonfire(Vector3 pos, float t) {
    // ash mound + a coiled sword stuck in it
    DrawCylinderEx(pos, pos + Vector3{ 0, 0.16f, 0 }, 0.95f, 1.05f, 12, Color{ 32, 27, 25, 255 });
    DrawCylinderEx(pos + Vector3{ 0, 0.12f, 0 }, pos + Vector3{ 0, 0.45f, 0 }, 0.72f, 0.42f, 10, Color{ 22, 17, 15, 255 });
    DrawCylinderEx(pos + Vector3{ 0, 0.30f, 0 }, pos + Vector3{ 0.05f, 1.75f, 0 }, 0.045f, 0.018f, 6, Color{ 120, 122, 132, 255 });
    DrawCube(pos + Vector3{ 0.02f, 0.62f, 0 }, 0.24f, 0.05f, 0.05f, Color{ 96, 98, 108, 255 });   // guard
    // flames (additive, flickering)
    BeginBlendMode(BLEND_ADDITIVE);
    for (int i = 0; i < 7; i++) {
        float a = i * 0.92f;
        float fl = 0.5f + 0.5f * sinf(t * 7.0f + a * 2.3f);
        Vector3 fp = pos + Vector3{ 0.20f * sinf(a + t * 1.4f), 0.42f + (0.35f + 0.55f * fl) * 0.5f, 0.20f * cosf(a * 1.3f + t) };
        DrawSphereEx(fp, 0.17f + 0.11f * fl, 6, 6, Color{ 255, (unsigned char)(135 + 90 * fl), 28, 120 });
    }
    DrawSphereEx(pos + Vector3{ 0, 0.7f, 0 }, 1.15f, 8, 8, Color{ 255, 120, 30, 42 });   // glow
    EndBlendMode();
}
