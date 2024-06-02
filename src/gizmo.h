
#ifndef Z64SCENE_GIZMO_H_INCLUDED
#define Z64SCENE_GIZMO_H_INCLUDED

#include "window.h"

// opaque structure
struct Gizmo;

// functions
struct Gizmo *GizmoNew(void);
void GizmoFree(struct Gizmo *gizmo);
void GizmoAddChild(struct Gizmo *gizmo, Vec3f *childPos);
void GizmoRemoveChildren(struct Gizmo *gizmo);
bool GizmoHasFocus(struct Gizmo *gizmo);
bool GizmoIsHovered(struct Gizmo *gizmo);
void GizmoSetPosition(struct Gizmo *gizmo, float x, float y, float z);
void GizmoUpdate(struct Gizmo *gizmo, Vec3f *rayPos);
void GizmoDraw(struct Gizmo *gizmo, struct CameraFly *camera);

#endif
