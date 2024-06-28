//
// extmath.h
//
// amalgamated extlib vector/matrix
// and other useful math functions

#ifndef EXTMATH_H_INCLUDED
#define EXTMATH_H_INCLUDED

#pragma GCC diagnostic ignored "-Wmissing-braces"

#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <float.h>

#ifndef M_PI
	#define M_PI 3.14159265359
#endif

typedef uint8_t u8;
typedef int8_t s8;
typedef uint16_t u16;
typedef int16_t s16;
typedef uint32_t u32;
typedef int32_t s32;
typedef float f32;
typedef double f64;

#if 1 // region: misc

int16_t sins(uint16_t x);
int16_t coss(uint16_t angle);

#define SQ(x)   ((x) * (x))
#define SinS(x) sinf(BinToRad((s16)(x)))
#define CosS(x) cosf(BinToRad((s16)(x)))
#define SinF(x) sinf(DegToRad(x))
#define CosF(x) cosf(DegToRad(x))
#define SinR(x) sinf(x)
#define CosR(x) cosf(x)

// trigonometry macros
#define DegToBin(degreesf) (int16_t)(degreesf * 182.04167f + .5f)
#define RadToBin(radf)     (int16_t)(radf * (32768.0f / M_PI))
#define RadToDeg(radf)     (radf * (180.0f / M_PI))
#define DegToRad(degf)     (degf * (M_PI / 180.0f))
#define BinFlip(angle)     ((int16_t)(angle - 0x7FFF))
#define BinSub(a, b)       ((int16_t)(a - b))
#define BinToDeg(binang)   ((float)binang * (360.0001525f / 65535.0f))
#define BinToRad(binang)   (((float)binang / 32768.0f) * M_PI)

// misc
#define absmax(a, b)         (ABS(a) > ABS(b) ? (a) : (b))
#define absmin(a, b)         (ABS(a) <= ABS(b) ? (a) : (b))
#define ABS(val)             ((val) < 0 ? -(val) : (val))
#define clamp(val, min, max) ((val) < (min) ? (min) : (val) > (max) ? (max) : (val))
#define clamp_min(val, min)  ((val) < (min) ? (min) : (val))
#define clamp_max(val, max)  ((val) > (max) ? (max) : (val))

#define Max(a, b) ((a) > (b) ? (a) : (b))
#define Min(a, b) ((a) < (b) ? (a) : (b))

#define clamp_s8(val)  (s8)clamp(((f32)val), (-__INT8_MAX__ - 1), __INT8_MAX__)
#define clamp_s16(val) (s16)clamp(((f32)val), (-__INT16_MAX__ - 1), __INT16_MAX__)
#define clamp_s32(val) (s32)clamp(((f32)val), (-__INT32_MAX__ - 1), (f32)__INT32_MAX__)

#define ArrCount(arr) (u32)(sizeof(arr) / sizeof(arr[0]))

#endif // endregion: misc



#if 1 // region: misc inlines

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

static inline f32 lerpf(f32 x, f32 min, f32 max) {
	return min + (max - min) * x;
}

static inline f32 normf(f32 v, f32 min, f32 max) {
	return (v - min) / (max - min);
}

static inline f64 lerpd(f64 x, f64 min, f64 max) {
	return min + (max - min) * x;
}

static inline f64 normd(f64 v, f64 min, f64 max) {
	return (v - min) / (max - min);
}

static inline int wrap(int x, int min, int max) {
	int range = max - min;
	
	if (x < min)
		x += range * ((min - x) / range + 1);
	
	return min + (x - min) % range;
}

static inline f32 wrapf(f32 x, f32 min, f32 max) {
	f64 range = max - min;
	
	if (x < min)
		x += range * roundf((min - x) / range + 1);
	
	return min + fmodf((x - min), range);
}

static inline s32 pingpongi(s32 v, s32 min, s32 max) {
	min = wrap(v, min, max * 2);
	if (min < max)
		return min;
	else
		return 2 * max - min;
}

static inline f32 pingpongf(f32 v, f32 min, f32 max) {
	min = wrapf(v, min, max * 2);
	if (min < max)
		return min;
	else
		return 2 * max - min;
}

static inline f32 Closest(f32 sample, f32 x, f32 y) {
	if ((fminf(sample, x) - fmaxf(sample, x) > fminf(sample, y) - fmaxf(sample, y)))
		return x;
	
	return y;
}

static inline f32 remapf(f32 v, f32 iMin, f32 iMax, f32 oMin, f32 oMax) {
	return lerpf(normf(v, iMin, iMax), oMin, oMax);
}

static inline f64 remapd(f64 v, f64 iMin, f64 iMax, f64 oMin, f64 oMax) {
	return lerpd(normd(v, iMin, iMax), oMin, oMax);
}

static inline f32 AccuracyF(f32 v, f32 mod) {
	return rint(rint(v * mod) / mod);
}

static inline f32 roundstepf(f32 v, f32 step) {
	return rintf(v * (1.0f / step)) * step;
}

static inline f32 invertf(f32 v) {
	return remapf(v, 0.0f, 1.0f, 1.0f, 0.0f);
}

#pragma GCC diagnostic pop

#endif // endregion: misc inlines

#if 1 // region: vector

#define veccmp(a, b) ({ \
		bool r = false; \
		for (int i = 0; i < sizeof((*(a))) / sizeof((*(a)).x); i++) \
		if ((*(a)).axis[i] != (*(b)).axis[i]) r = true; \
		r; \
	})

#define veccpy(a, b) do { \
	for (int i = 0; i < sizeof((*(a))) / sizeof((*(a)).x); i++) \
	(*(a)).axis[i] = (*(b)).axis[i]; \
} while (0)

#define UnfoldRect(rect) (rect).x, (rect).y, (rect).w, (rect).h
#define UnfoldVec2(vec)  (vec).x, (vec).y
#define UnfoldVec3(vec)  (vec).x, (vec).y, (vec).z
#define UnfoldVec4(vec)  (vec).x, (vec).y, (vec).z, (vec).w
#define IsZero(f32)      ((fabsf(f32) < FLT_EPSILON))

#define VEC_TYPE(type, suffix) \
		typedef union { \
			struct { \
				type x; \
				type y; \
				type z; \
				type w; \
			}; \
			type axis[4]; \
		} Vec4 ## suffix; \
		typedef union { \
			struct { \
				type x; \
				type y; \
				type z; \
			}; \
			type axis[3]; \
		} Vec3 ## suffix; \
		typedef union { \
			struct { \
				type x; \
				type y; \
			}; \
			type axis[2]; \
		} Vec2 ## suffix;

VEC_TYPE(f32, f)
VEC_TYPE(s16, s)

typedef struct {
	f32 r;
	s16 pitch;
	s16 yaw;
} VecSph;

typedef struct {
	f32 x;
	f32 y;
	f32 w;
	f32 h;
} Rectf32;

typedef struct {
	s32 x;
	s32 y;
	s32 w;
	s32 h;
} Rect;

typedef struct {
	s32 x1;
	s32 y1;
	s32 x2;
	s32 y2;
} CRect;

typedef struct {
	f32 xMin, xMax;
	f32 zMin, zMax;
	f32 yMin, yMax;
} BoundBox;

typedef struct {
	f32 x; f32 y; f32 z; f32 w;
} Quat;

#define Vec3_Substract(dest, a, b) \
	dest.x = a.x - b.x; \
	dest.y = a.y - b.y; \
	dest.z = a.z - b.z

//extern const f32 EPSILON;
//extern f32 gDeltaTime;

s16 Atan2S(f32 x, f32 y);
void VecSphToVec3f(Vec3f* dest, VecSph* sph);
void Math_AddVecSphToVec3f(Vec3f* dest, VecSph* sph);
VecSph* VecSph_FromVec3f(VecSph* dest, Vec3f* vec);
VecSph* VecSph_GeoFromVec3f(VecSph* dest, Vec3f* vec);
VecSph* VecSph_GeoFromVec3fDiff(VecSph* dest, Vec3f* a, Vec3f* b);
Vec3f* Vec3f_Up(Vec3f* dest, s16 pitch, s16 yaw, s16 roll);
f32 Math_DelSmoothStepToF(f32* pValue, f32 target, f32 fraction, f32 step, f32 minStep);
f64 Math_DelSmoothStepToD(f64* pValue, f64 target, f64 fraction, f64 step, f64 minStep);
s16 Math_SmoothStepToS(s16* pValue, s16 target, s16 scale, s16 step, s16 minStep);
int Math_SmoothStepToI(int* pValue, int target, int scale, int step, int minStep);
void Rect_ToCRect(CRect* dst, Rect* src);
void Rect_ToRect(Rect* dst, CRect* src);
bool Rect_Check_PosIntersect(Rect* rect, Vec2s* pos);
Rect Rect_Translate(Rect r, s32 x, s32 y);
Rect Rect_New(s32 x, s32 y, s32 w, s32 h);
Rect Rect_Add(Rect a, Rect b);
Rect Rect_Sub(Rect a, Rect b);
Rect Rect_AddPos(Rect a, Rect b);
Rect Rect_SubPos(Rect a, Rect b);
bool Rect_PointIntersect(Rect* rect, s32 x, s32 y);
Vec2s Rect_ClosestPoint(Rect* rect, s32 x, s32 y);
Vec2s Rect_MidPoint(Rect* rect);
f32 Rect_PointDistance(Rect* rect, s32 x, s32 y);
int RectW(Rect r);
int RectH(Rect r);
int Rect_RectIntersect(Rect r, Rect i);
Rect Rect_Clamp(Rect r, Rect l);
Rect Rect_FlipHori(Rect r, Rect p);
Rect Rect_FlipVerti(Rect r, Rect p);
Rect Rect_ExpandX(Rect r, int amount);
Rect Rect_ShrinkX(Rect r, int amount);
Rect Rect_ExpandY(Rect r, int amount);
Rect Rect_ShrinkY(Rect r, int amount);
Rect Rect_Scale(Rect r, int x, int y);
Rect Rect_Vec2x2(Vec2s a, Vec2s b);
BoundBox BoundBox_New3F(Vec3f point);
BoundBox BoundBox_New2F(Vec2f point);
void BoundBox_Adjust3F(BoundBox* bbox, Vec3f point);
void BoundBox_Adjust2F(BoundBox* bbox, Vec2f point);

bool Vec2f_PointInShape(Vec2f p, Vec2f* poly, u32 numPoly);

Vec3f Vec3f_Cross(Vec3f a, Vec3f b);
Vec3s Vec3s_Cross(Vec3s a, Vec3s b);

f32 Vec3f_DistXZ(Vec3f a, Vec3f b);
f32 Vec3f_DistXYZ(Vec3f a, Vec3f b);
f32 Vec3s_DistXZ(Vec3s a, Vec3s b);
f32 Vec3s_DistXYZ(Vec3s a, Vec3s b);
f32 Vec2f_DistXZ(Vec2f a, Vec2f b);
f32 Vec2s_DistXZ(Vec2s a, Vec2s b);

s16 Vec3f_Yaw(Vec3f a, Vec3f b);
s16 Vec2f_Yaw(Vec2f a, Vec2f b);

s16 Vec3f_Pitch(Vec3f a, Vec3f b);

Vec2f Vec2f_Sub(Vec2f a, Vec2f b);
Vec3f Vec3f_Sub(Vec3f a, Vec3f b);
Vec4f Vec4f_Sub(Vec4f a, Vec4f b);

Vec2s Vec2s_Sub(Vec2s a, Vec2s b);
Vec3s Vec3s_Sub(Vec3s a, Vec3s b);
Vec4s Vec4s_Sub(Vec4s a, Vec4s b);

Vec2f Vec2f_Add(Vec2f a, Vec2f b);
Vec3f Vec3f_Add(Vec3f a, Vec3f b);
Vec4f Vec4f_Add(Vec4f a, Vec4f b);

Vec2s Vec2s_Add(Vec2s a, Vec2s b);
Vec3s Vec3s_Add(Vec3s a, Vec3s b);
Vec4s Vec4s_Add(Vec4s a, Vec4s b);

Vec2f Vec2f_Div(Vec2f a, Vec2f b);
Vec3f Vec3f_Div(Vec3f a, Vec3f b);
Vec4f Vec4f_Div(Vec4f a, Vec4f b);

Vec2f Vec2f_DivVal(Vec2f a, f32 val);
Vec3f Vec3f_DivVal(Vec3f a, f32 val);
Vec4f Vec4f_DivVal(Vec4f a, f32 val);

Vec2s Vec2s_Div(Vec2s a, Vec2s b);
Vec3s Vec3s_Div(Vec3s a, Vec3s b);
Vec4s Vec4s_Div(Vec4s a, Vec4s b);

Vec2s Vec2s_DivVal(Vec2s a, f32 val);
Vec3s Vec3s_DivVal(Vec3s a, f32 val);
Vec4s Vec4s_DivVal(Vec4s a, f32 val);

Vec2f Vec2f_Mul(Vec2f a, Vec2f b);
Vec3f Vec3f_Mul(Vec3f a, Vec3f b);
Vec4f Vec4f_Mul(Vec4f a, Vec4f b);

Vec2f Vec2f_MulVal(Vec2f a, f32 val);
Vec3f Vec3f_MulVal(Vec3f a, f32 val);
Vec4f Vec4f_MulVal(Vec4f a, f32 val);

Vec2s Vec2s_Mul(Vec2s a, Vec2s b);
Vec3s Vec3s_Mul(Vec3s a, Vec3s b);
Vec4s Vec4s_Mul(Vec4s a, Vec4s b);

Vec2s Vec2s_MulVal(Vec2s a, f32 val);
Vec3s Vec3s_MulVal(Vec3s a, f32 val);
Vec4s Vec4s_MulVal(Vec4s a, f32 val);

Vec2f Vec2f_New(f32 x, f32 y);
Vec3f Vec3f_New(f32 x, f32 y, f32 z);
Vec4f Vec4f_New(f32 x, f32 y, f32 z, f32 w);
Vec2s Vec2s_New(s16 x, s16 y);
Vec3s Vec3s_New(s16 x, s16 y, s16 z);
Vec4s Vec4s_New(s16 x, s16 y, s16 z, s16 w);

f32 Vec2f_Dot(Vec2f a, Vec2f b);
f32 Vec3f_Dot(Vec3f a, Vec3f b);
f32 Vec4f_Dot(Vec4f a, Vec4f b);
f32 Vec2s_Dot(Vec2s a, Vec2s b);
f32 Vec3s_Dot(Vec3s a, Vec3s b);
f32 Vec4s_Dot(Vec4s a, Vec4s b);

f32 Vec2f_MagnitudeSQ(Vec2f a);
f32 Vec3f_MagnitudeSQ(Vec3f a);
f32 Vec4f_MagnitudeSQ(Vec4f a);
f32 Vec2s_MagnitudeSQ(Vec2s a);
f32 Vec3s_MagnitudeSQ(Vec3s a);
f32 Vec4s_MagnitudeSQ(Vec4s a);

f32 Vec2f_Magnitude(Vec2f a);
f32 Vec3f_Magnitude(Vec3f a);
f32 Vec4f_Magnitude(Vec4f a);
f32 Vec2s_Magnitude(Vec2s a);
f32 Vec3s_Magnitude(Vec3s a);
f32 Vec4s_Magnitude(Vec4s a);

Vec2f Vec2f_Median(Vec2f a, Vec2f b);
Vec3f Vec3f_Median(Vec3f a, Vec3f b);
Vec4f Vec4f_Median(Vec4f a, Vec4f b);
Vec2s Vec2s_Median(Vec2s a, Vec2s b);
Vec3s Vec3s_Median(Vec3s a, Vec3s b);
Vec4s Vec4s_Median(Vec4s a, Vec4s b);

Vec2f Vec2f_Normalize(Vec2f a);
Vec3f Vec3f_Normalize(Vec3f a);
Vec4f Vec4f_Normalize(Vec4f a);
Vec2s Vec2s_Normalize(Vec2s a);
Vec3s Vec3s_Normalize(Vec3s a);
Vec4s Vec4s_Normalize(Vec4s a);

Vec2f Vec2f_LineSegDir(Vec2f a, Vec2f b);
Vec3f Vec3f_LineSegDir(Vec3f a, Vec3f b);
Vec4f Vec4f_LineSegDir(Vec4f a, Vec4f b);
Vec2s Vec2s_LineSegDir(Vec2s a, Vec2s b);
Vec3s Vec3s_LineSegDir(Vec3s a, Vec3s b);
Vec4s Vec4s_LineSegDir(Vec4s a, Vec4s b);

Vec2f Vec2f_Project(Vec2f a, Vec2f b);
Vec3f Vec3f_Project(Vec3f a, Vec3f b);
Vec4f Vec4f_Project(Vec4f a, Vec4f b);
Vec2s Vec2s_Project(Vec2s a, Vec2s b);
Vec3s Vec3s_Project(Vec3s a, Vec3s b);
Vec4s Vec4s_Project(Vec4s a, Vec4s b);

Vec2f Vec2f_Invert(Vec2f a);
Vec3f Vec3f_Invert(Vec3f a);
Vec4f Vec4f_Invert(Vec4f a);
Vec2s Vec2s_Invert(Vec2s a);
Vec3s Vec3s_Invert(Vec3s a);
Vec4s Vec4s_Invert(Vec4s a);

Vec2f Vec2f_InvMod(Vec2f a);
Vec3f Vec3f_InvMod(Vec3f a);
Vec4f Vec4f_InvMod(Vec4f a);

bool Vec2f_IsNaN(Vec2f a);
bool Vec3f_IsNaN(Vec3f a);
bool Vec4f_IsNaN(Vec4f a);

f32 Vec2f_Cos(Vec2f a, Vec2f b);
f32 Vec3f_Cos(Vec3f a, Vec3f b);
f32 Vec4f_Cos(Vec4f a, Vec4f b);
f32 Vec2s_Cos(Vec2s a, Vec2s b);
f32 Vec3s_Cos(Vec3s a, Vec3s b);
f32 Vec4s_Cos(Vec4s a, Vec4s b);

Vec2f Vec2f_Reflect(Vec2f vec, Vec2f normal);
Vec3f Vec3f_Reflect(Vec3f vec, Vec3f normal);
Vec4f Vec4f_Reflect(Vec4f vec, Vec4f normal);
Vec2s Vec2s_Reflect(Vec2s vec, Vec2s normal);
Vec3s Vec3s_Reflect(Vec3s vec, Vec3s normal);
Vec4s Vec4s_Reflect(Vec4s vec, Vec4s normal);

Vec3f Vec3f_ClosestPointOnRay(Vec3f rayStart, Vec3f rayEnd, Vec3f lineStart, Vec3f lineEnd);
Vec3f Vec3f_ProjectAlong(Vec3f point, Vec3f lineA, Vec3f lineB);
Vec3f Vec3fRGBfromHSV(float h, float s, float v);
uint32_t Vec3fRGBto24bit(Vec3f color);

#endif // endregion: vector

#if 1 // region: matrix

typedef enum {
	MTXMODE_NEW,
	MTXMODE_APPLY
} MtxMode;

typedef union {
	s32 m[4][4];
	struct {
		u16 intPart[4][4];
		u16 fracPart[4][4];
	};
} MtxN64;

typedef float Matrix4x4[4][4];
typedef union {
	Matrix4x4 mf;
	struct {
		float xx, yx, zx, wx;
		float xy, yy, zy, wy;
		float xz, yz, zz, wz;
		float xw, yw, zw, ww;
	};
} Matrix;

extern Matrix* gMatrixStack;
extern Matrix* gCurrentMatrix;
extern const Matrix gMtxFClear;

void Matrix_Init();
void Matrix_Clear(Matrix* mf);
void Matrix_Push(void);
void Matrix_Pop(void);
void Matrix_Get(Matrix* dest);
void Matrix_Put(Matrix* src);
void Matrix_MultVec3f(Vec3f* src, Vec3f* dest);
void Matrix_Transpose(Matrix* mf);
void Matrix_MtxFTranslate(Matrix* cmf, f32 x, f32 y, f32 z, MtxMode mode);
void Matrix_Translate(f32 x, f32 y, f32 z, MtxMode mode);
void Matrix_Scale(f32 x, f32 y, f32 z, MtxMode mode);
void Matrix_RotateX(f32 x, MtxMode mode);
void Matrix_RotateY(f32 y, MtxMode mode);
void Matrix_RotateZ(f32 z, MtxMode mode);

void Matrix_MultVec3f_Ext(Vec3f* src, Vec3f* dest, Matrix* mf);
void Matrix_MultVec3fToVec4f_Ext(Vec3f* src, Vec4f* dest, Matrix* mf);
void Matrix_OrientVec3f_Ext(Vec3f* src, Vec3f* dest, Matrix* mf);
void Matrix_Mult(Matrix* mf, MtxMode mode);
void Matrix_MtxFCopy(Matrix* dest, Matrix* src);
void Matrix_ToMtxF(Matrix* mtx);
MtxN64* Matrix_ToMtx(MtxN64* dest);
MtxN64* Matrix_NewMtxN64();
void Matrix_MtxToMtxF(MtxN64* src, Matrix* dest);
MtxN64* Matrix_MtxFToMtx(Matrix* src, MtxN64* dest);
void Matrix_MtxFMtxFMult(Matrix* mfA, Matrix* mfB, Matrix* dest);
void Matrix_Projection(Matrix* mtx, f32 fovy, f32 aspect, f32 near, f32 far, f32 scale);
void Matrix_Ortho(Matrix* mtx, f32 fovy, f32 aspect, f32 near, f32 far);
void Matrix_LookAt(Matrix* mf, Vec3f eye, Vec3f at, Vec3f up);

void Matrix_MtxFTranslateRotateZYX(Matrix* cmf, Vec3f* translation, Vec3s* rotation);
void Matrix_TranslateRotateZYX(Vec3f* translation, Vec3s* rotation);
void Matrix_RotateAToB(Vec3f* a, Vec3f* b, u8 mode);
void Matrix_MultVec4f_Ext(Vec4f* src, Vec4f* dest, Matrix* mtx);
void Matrix_MtxFToYXZRotS(Vec3s* rotDest, s32 flag);
Matrix Matrix_Invert(Matrix* m);
void Matrix_Unproject(Matrix* modelViewMtx, Matrix* projMtx, Vec3f* src, Vec3f* dest, f32 w, f32 h);
void Matrix_Project(Matrix* modelViewMtx, Matrix* projMtx, Vec3f* src, Vec3f* dest, f32 w, f32 h);

static inline void Matrix_RotateX_s(f32 x, MtxMode mode) {
	Matrix_RotateX(BinToRad(x), mode);
}

static inline void Matrix_RotateY_s(f32 y, MtxMode mode) {
	Matrix_RotateY(BinToRad(y), mode);
}

static inline void Matrix_RotateZ_s(f32 z, MtxMode mode) {
	Matrix_RotateZ(BinToRad(z), mode);
}

static inline void Matrix_RotateX_d(f32 x, MtxMode mode) {
	Matrix_RotateX(DegToRad(x), mode);
}

static inline void Matrix_RotateY_d(f32 y, MtxMode mode) {
	Matrix_RotateY(DegToRad(y), mode);
}

static inline void Matrix_RotateZ_d(f32 z, MtxMode mode) {
	Matrix_RotateZ(DegToRad(z), mode);
}

static inline void mat44_to_matn64(unsigned char *dest, float src[16])
{
#define    FTOFIX32(x)    (long)((x) * (float)0x00010000)
	int32_t t;
	unsigned char *intpart = dest;
	unsigned char *fracpart = dest + 0x20;
	for (int i=0; i < 4; ++i)
	{
		for (int k=0; k < 4; ++k)
		{
			t = FTOFIX32(src[4*i+k]);
			short ip = (t >> 16) & 0xFFFF;
			intpart[0]  = (ip >> 8) & 255;
			intpart[1]  = (ip >> 0) & 255;
			fracpart[0] = (t >>  8) & 255;
			fracpart[1] = (t >>  0) & 255;
			intpart  += 2;
			fracpart += 2;
		}
	}
}

#endif // endregion: matrix

#if 1 // region: collision
typedef struct Triangle {
	Vec3f v[3];
	u8    cullBackface;
	u8    cullFrontface;
} Triangle;

typedef struct TriBuffer {
	Triangle* head;
	u32       max;
	u32       num;
} TriBuffer;

typedef struct RayLine {
	Vec3f start;
	Vec3f end;
	f32   nearest;
} RayLine;

typedef struct Sphere {
	Vec3f pos;
	f32   r;
} Sphere;

typedef struct Cylinder {
	Vec3f start;
	Vec3f end;
	f32   r;
} Cylinder;

void TriBuffer_Alloc(TriBuffer* tribuf, u32 num);
void TriBuffer_Realloc(TriBuffer* tribuf);
void TriBuffer_Free(TriBuffer* tribuf);
RayLine RayLine_New(Vec3f start, Vec3f end);
bool Col3D_LineVsTriangle(RayLine* ray, Triangle* tri, Vec3f* outPos, Vec3f* outNor, bool cullBackface, bool cullFrontface);
bool Col3D_LineVsTriBuffer(RayLine* ray, TriBuffer* triBuf, Vec3f* outPos, Vec3f* outNor);
bool Col3D_LineVsCylinder(RayLine* ray, Cylinder* cyl, Vec3f* outPos);
bool Col3D_LineVsSphere(RayLine* ray, Sphere* sph, Vec3f* outPos);
#endif // endregion: collision

#endif
