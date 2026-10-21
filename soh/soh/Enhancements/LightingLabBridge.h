#pragma once
#define YNT_SHADOW_ONLY 0x00000004
#ifdef __cplusplus
extern "C" {
#endif
int LightingLab_ShouldCast(float x,float y,float z,float radius);
void LightingLab_RecordShadowActor(void);
void LightingLab_RecordShadowRoom(void);
int LightingLab_ShouldDrawNativeActorShadow(void);
int LightingLab_ReplacesNativeLight(const void* info);
int LightingLab_ReplacesTempleLightPlane(void);
#ifdef __cplusplus
}
#endif
