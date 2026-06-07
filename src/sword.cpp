#include "sword.h"
#include "anim.h"
#include "raymath.h"
#include "rlgl.h"

// Grip orientation in the hand frame (blade points up out of the fist).
static const Vector3 GRIP_ROT_DEG = { -90.0f, 0.0f, -90.0f };
static const Vector3 GRIP_EXTRA = { 0.0f, 0.0f, 0.0f };   // manual nudge if needed
static const float   FIST_T = 0.6f;                        // wrist(0) -> middle knuckle(1)
static const float   SWORD_SCALE = 1.0f;

static int find_bone(Model& m, const char* name) {
    for (int i = 0; i < m.boneCount; i++)
        if (TextIsEqual(m.bones[i].name, name)) return i;
    return -1;
}

void draw_hand_sword(Model& model, AnimController& anim, Vector3 pos, float yaw_rad) {
    int hand = find_bone(model, "mixamorig_RightHand");
    if (hand < 0) return;
    Matrix mw = MatrixMultiply(MatrixRotateY(yaw_rad), MatrixTranslate(pos.x, pos.y, pos.z));
    Matrix sw = MatrixMultiply(anim.bone_local_matrix(hand), mw);   // hand world frame

    // seat the grip in the fist: 60% from the wrist toward the middle-finger knuckle,
    // expressed in the hand bone's local frame (robust to the bone's arbitrary axes)
    Vector3 localOff = { 0, 0, 0 };
    int mid = find_bone(model, "mixamorig_RightHandMiddle1");
    if (mid >= 0) {
        Matrix midSw = MatrixMultiply(anim.bone_local_matrix(mid), mw);
        Vector3 fingerWorld = { midSw.m12, midSw.m13, midSw.m14 };
        Vector3 fingerLocal = Vector3Transform(fingerWorld, MatrixInvert(sw));
        localOff = Vector3Scale(fingerLocal, FIST_T);
    }

    rlPushMatrix();
    float16 mf = MatrixToFloatV(sw);
    rlMultMatrixf(mf.v);
    rlTranslatef(localOff.x + GRIP_EXTRA.x, localOff.y + GRIP_EXTRA.y, localOff.z + GRIP_EXTRA.z);
    rlRotatef(GRIP_ROT_DEG.x, 1, 0, 0);
    rlRotatef(GRIP_ROT_DEG.y, 0, 1, 0);
    rlRotatef(GRIP_ROT_DEG.z, 0, 0, 1);
    rlScalef(SWORD_SCALE, SWORD_SCALE, SWORD_SCALE);

    Color blade{ 175, 181, 196, 255 }, metal{ 150, 120, 60, 255 }, grip{ 32, 22, 15, 255 };
    DrawCylinderEx({ 0, -0.06f, 0 }, { 0, 0.06f, 0 }, 0.020f, 0.017f, 10, grip);   // grip
    DrawSphere({ 0, -0.075f, 0 }, 0.026f, metal);                                  // pommel
    DrawCube({ 0, 0.075f, 0 }, 0.20f, 0.026f, 0.05f, metal);                       // guard
    DrawCube({ 0, 0.55f, 0 }, 0.05f, 0.92f, 0.013f, blade);                        // blade
    DrawCube({ 0, 1.02f, 0 }, 0.05f, 0.10f, 0.013f, blade);                        // tip
    rlPopMatrix();
}

void sword_blade_segment(Model& model, AnimController& anim, Vector3 pos, float yaw_rad,
                         Vector3& out_base, Vector3& out_tip) {
    int hand = find_bone(model, "mixamorig_RightHand");
    if (hand < 0) { out_base = out_tip = pos; return; }
    Matrix mw = MatrixMultiply(MatrixRotateY(yaw_rad), MatrixTranslate(pos.x, pos.y, pos.z));
    Matrix sw = MatrixMultiply(anim.bone_local_matrix(hand), mw);   // hand world frame
    Vector3 localOff = { 0, 0, 0 };
    int mid = find_bone(model, "mixamorig_RightHandMiddle1");
    if (mid >= 0) {
        Matrix midSw = MatrixMultiply(anim.bone_local_matrix(mid), mw);
        Vector3 fingerWorld = { midSw.m12, midSw.m13, midSw.m14 };
        Vector3 fingerLocal = Vector3Transform(fingerWorld, MatrixInvert(sw));
        localOff = Vector3Scale(fingerLocal, FIST_T);
    }
    // After the grip rotation (GRIP_ROT_DEG) the blade's local +Y runs along the
    // hand frame's +X, so a blade-local height h sits at hand-local (localOff + (h,0,0)).
    out_base = Vector3Transform(Vector3{ localOff.x + 0.13f, localOff.y, localOff.z }, sw);
    out_tip  = Vector3Transform(Vector3{ localOff.x + 1.07f, localOff.y, localOff.z }, sw);
}
