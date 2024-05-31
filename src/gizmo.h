
#ifndef Z64SCENE_GIZMO_H_INCLUDED
#define Z64SCENE_GIZMO_H_INCLUDED

// opaque structure
struct Gizmo;

// functions
struct Gizmo *GizmoNew(void);
void GizmoSetPosition(struct Gizmo *gizmo, float x, float y, float z);
void GizmoDraw(struct Gizmo *gizmo);

#endif
