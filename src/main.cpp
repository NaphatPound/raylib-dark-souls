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
#include <vector>

int main(int argc, char** argv) {
    bool auto_demo = false, diag = false, scenic = false;
    for (int i = 1; i < argc; i++) {
        if (TextIsEqual(argv[i], "auto")) auto_demo = true;
        if (TextIsEqual(argv[i], "diag")) diag = true;
        if (TextIsEqual(argv[i], "scenic")) scenic = true;
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

    g_lit.load();
    g_fx.init();
    g_audio.init();
    g_juice.init();
    g_juice.enabled = true;

    HUD hud; hud.init();
    Screens screens; screens.init();

    arena::load(g_lit.shader);

    Player player; player.init({ 0, 0, 16 }); player.apply_shader(g_lit.shader);
    Boss boss;     boss.init({ 0, 0, -10 }); boss.apply_shader(g_lit.shader);

    PauseMenu pause; pause.player = &player;

    g_game.restart_requested.connect([&]() {
        player.pos = { 0, 0, 16 }; player.reset_state();
        boss.pos = { 0, 0, -10 };  boss.reset_state();
    });

    g_game.set_state(Game::TITLE);
    DisableCursor();
    player.autopilot = auto_demo;

    RenderTexture2D reflect = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
    long frame = 0;
    bool paused = false;
    std::vector<Hurtbox*> boss_targets   = { &boss.hurtbox };   // what the player can hit
    std::vector<Hurtbox*> player_targets = { &player.hurtbox }; // what the boss can hit

    while (!WindowShouldClose()) {
        float real_dt = GetFrameTime();
        if (real_dt > 0.05f) real_dt = 0.05f;
        g_juice.update(real_dt);
        g_audio.update(real_dt);

        if (IsKeyPressed(KEY_ESCAPE)) {
            if (!(g_game.state == Game::DEAD || g_game.state == Game::VICTORY)) {
                paused = !paused;
                g_game.paused = paused;
                if (paused) { EnableCursor(); pause.open(); }
                else DisableCursor();
            }
        }

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

            if (g_game.state == Game::DEAD || g_game.state == Game::VICTORY) {
                if (IsKeyPressed(KEY_E) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER))
                    g_game.request_restart();
            }
        }

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
        EndMode3D();

        hud.draw();
        screens.draw();
        if (paused) {
            pause.draw_and_update();
            if (pause.resume_requested) { paused = false; g_game.paused = false; DisableCursor(); }
            if (pause.quit_requested) break;
        }
        if (IsKeyPressed(KEY_F2)) TakeScreenshot("shot.png");
        EndDrawing();

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

    UnloadRenderTexture(reflect);
    player.unload();
    boss.unload();
    arena::unload();
    g_lit.unload();
    assets::unload_all();
    g_audio.shutdown();
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
