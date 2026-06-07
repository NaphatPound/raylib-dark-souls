#pragma once
// Skeletal animation driver over a raylib Model + its ModelAnimation array.
// Ports the GDScript AnimationPlayer helpers used by player.gd / boss.gd:
//   _set_anim(clip)        -> set_anim: loop at natural speed, skip if unchanged
//   _play_fitted(clip,dur) -> play_fitted: one-shot, time-scaled to finish in `dur`
#include "raylib.h"
#include <string>
#include <unordered_map>

class AnimController {
public:
    // raylib bakes glTF animations at 1000/GLTF_ANIMDELAY ms => ~58.82 fps.
    static constexpr float ANIM_FPS = 1000.0f / 17.0f;

    void load(Model* m, const char* path);
    void unload();

    bool has(const std::string& name) const { return find(name) >= 0; }
    void set_anim(const std::string& name);                 // looping, natural speed
    void play_fitted(const std::string& name, float dur);   // one-shot, fitted to dur
    void play_once(const std::string& name);                // one-shot at the clip's natural speed
    void set_speed_scale(float s) { speed_scale_ = s; }
    float speed_scale() const { return speed_scale_; }
    bool finished() const { return finished_; }
    bool playing() const { return cur_ >= 0 && !(finished_ && !looping_); }
    const std::string& current() const { return current_name_; }

    void update(float dt);
    // Model-space transform of a bone at the current frame (for bone attachments).
    Matrix bone_local_matrix(int boneIndex) const;

private:
    int find(const std::string& name) const;
    float natural_dur(int idx) const;

    Model* model_ = nullptr;
    ModelAnimation* anims_ = nullptr;
    int count_ = 0;
    std::unordered_map<std::string, int> by_name_;

    int cur_ = -1;
    std::string current_name_;
    float phase_ = 0.0f;       // 0..1 through the current clip
    float clip_dur_ = 1.0f;    // seconds the clip should take
    bool looping_ = false;
    bool finished_ = false;
    float speed_scale_ = 1.0f;
    int last_frame_ = 0;
};
