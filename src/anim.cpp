#include "anim.h"
#include "raymath.h"
#include <cmath>

void AnimController::load(Model* m, const char* path) {
    model_ = m;
    int c = 0;
    anims_ = LoadModelAnimations(path, &c);
    count_ = c;
    by_name_.clear();
    for (int i = 0; i < count_; i++) {
        // raylib stores anim names in a char[32] (<=31 chars); key by that.
        by_name_[std::string(anims_[i].name)] = i;
    }
}

void AnimController::unload() {
    if (anims_) UnloadModelAnimations(anims_, count_);
    anims_ = nullptr;
    count_ = 0;
}

int AnimController::find(const std::string& name) const {
    auto it = by_name_.find(name);
    if (it != by_name_.end()) return it->second;
    if (name.size() > 31) {                       // match raylib's truncation
        it = by_name_.find(name.substr(0, 31));
        if (it != by_name_.end()) return it->second;
    }
    return -1;
}

float AnimController::natural_dur(int idx) const {
    int f = anims_[idx].frameCount;
    if (f <= 1) return 0.2f;
    return (float)(f - 1) / ANIM_FPS;
}

void AnimController::set_anim(const std::string& name) {
    int idx = find(name);
    if (idx < 0) return;
    if (cur_ == idx && looping_ && speed_scale_ == 1.0f) return;  // already looping it
    cur_ = idx;
    current_name_ = name;
    clip_dur_ = natural_dur(idx);
    if (clip_dur_ < 0.001f) clip_dur_ = 0.2f;
    looping_ = true;
    finished_ = false;
    phase_ = 0.0f;
    last_frame_ = 0;       // keep bone-pose lookups valid until the next update()
    speed_scale_ = 1.0f;
}

void AnimController::play_fitted(const std::string& name, float dur) {
    int idx = find(name);
    if (idx < 0) return;
    cur_ = idx;
    current_name_ = name;
    clip_dur_ = (dur > 0.001f) ? dur : 0.2f;
    looping_ = false;
    finished_ = false;
    phase_ = 0.0f;
    last_frame_ = 0;       // keep bone-pose lookups valid until the next update()
    speed_scale_ = 1.0f;
}

void AnimController::play_once(const std::string& name) {
    int idx = find(name);
    if (idx < 0) return;
    play_fitted(name, natural_dur(idx));   // one-shot at natural speed (holds last frame)
}

void AnimController::update(float dt) {
    if (cur_ < 0 || !model_) return;
    phase_ += (dt * speed_scale_) / clip_dur_;
    if (looping_) {
        phase_ -= floorf(phase_);
    } else if (phase_ >= 1.0f) {
        phase_ = 1.0f;
        finished_ = true;
    }
    int fc = anims_[cur_].frameCount;
    int frame = (int)lroundf(phase_ * (float)(fc - 1));
    if (frame < 0) frame = 0;
    if (frame > fc - 1) frame = fc - 1;
    last_frame_ = frame;
    UpdateModelAnimation(*model_, anims_[cur_], frame);
}

Matrix AnimController::bone_local_matrix(int boneIndex) const {
    if (cur_ < 0 || !anims_ || boneIndex < 0 || boneIndex >= anims_[cur_].boneCount)
        return MatrixIdentity();
    int f = last_frame_;
    if (f < 0 || f >= anims_[cur_].frameCount) f = 0;   // clamp: cur_ may have changed since update()
    Transform tr = anims_[cur_].framePoses[f][boneIndex];
    Matrix s = MatrixScale(tr.scale.x, tr.scale.y, tr.scale.z);
    Matrix r = QuaternionToMatrix(tr.rotation);
    Matrix t = MatrixTranslate(tr.translation.x, tr.translation.y, tr.translation.z);
    return MatrixMultiply(MatrixMultiply(s, r), t);
}
