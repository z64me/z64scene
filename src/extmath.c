//
// extmath.h
//
// amalgamated extlib vector/matrix
// and other useful math functions

#include "extmath.h"
#include "n64.h" // n64_graph_alloc

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#if 1 // region: misc

static s16 sintable[0x400] = {
	0x0000, 0x0032, 0x0064, 0x0096, 0x00C9, 0x00FB, 0x012D, 0x0160, 0x0192, 0x01C4, 0x01F7, 0x0229, 0x025B, 0x028E,
	0x02C0, 0x02F2, 0x0324, 0x0357, 0x0389, 0x03BB, 0x03EE, 0x0420, 0x0452, 0x0484, 0x04B7, 0x04E9, 0x051B, 0x054E,
	0x0580, 0x05B2, 0x05E4, 0x0617, 0x0649, 0x067B, 0x06AD, 0x06E0, 0x0712, 0x0744, 0x0776, 0x07A9, 0x07DB, 0x080D,
	0x083F, 0x0871, 0x08A4, 0x08D6, 0x0908, 0x093A, 0x096C, 0x099F, 0x09D1, 0x0A03, 0x0A35, 0x0A67, 0x0A99, 0x0ACB,
	0x0AFE, 0x0B30, 0x0B62, 0x0B94, 0x0BC6, 0x0BF8, 0x0C2A, 0x0C5C, 0x0C8E, 0x0CC0, 0x0CF2, 0x0D25, 0x0D57, 0x0D89,
	0x0DBB, 0x0DED, 0x0E1F, 0x0E51, 0x0E83, 0x0EB5, 0x0EE7, 0x0F19, 0x0F4B, 0x0F7C, 0x0FAE, 0x0FE0, 0x1012, 0x1044,
	0x1076, 0x10A8, 0x10DA, 0x110C, 0x113E, 0x116F, 0x11A1, 0x11D3, 0x1205, 0x1237, 0x1269, 0x129A, 0x12CC, 0x12FE,
	0x1330, 0x1361, 0x1393, 0x13C5, 0x13F6, 0x1428, 0x145A, 0x148C, 0x14BD, 0x14EF, 0x1520, 0x1552, 0x1584, 0x15B5,
	0x15E7, 0x1618, 0x164A, 0x167B, 0x16AD, 0x16DF, 0x1710, 0x1741, 0x1773, 0x17A4, 0x17D6, 0x1807, 0x1839, 0x186A,
	0x189B, 0x18CD, 0x18FE, 0x1930, 0x1961, 0x1992, 0x19C3, 0x19F5, 0x1A26, 0x1A57, 0x1A88, 0x1ABA, 0x1AEB, 0x1B1C,
	0x1B4D, 0x1B7E, 0x1BAF, 0x1BE1, 0x1C12, 0x1C43, 0x1C74, 0x1CA5, 0x1CD6, 0x1D07, 0x1D38, 0x1D69, 0x1D9A, 0x1DCB,
	0x1DFC, 0x1E2D, 0x1E5D, 0x1E8E, 0x1EBF, 0x1EF0, 0x1F21, 0x1F52, 0x1F82, 0x1FB3, 0x1FE4, 0x2015, 0x2045, 0x2076,
	0x20A7, 0x20D7, 0x2108, 0x2139, 0x2169, 0x219A, 0x21CA, 0x21FB, 0x222B, 0x225C, 0x228C, 0x22BD, 0x22ED, 0x231D,
	0x234E, 0x237E, 0x23AE, 0x23DF, 0x240F, 0x243F, 0x2470, 0x24A0, 0x24D0, 0x2500, 0x2530, 0x2560, 0x2591, 0x25C1,
	0x25F1, 0x2621, 0x2651, 0x2681, 0x26B1, 0x26E1, 0x2711, 0x2740, 0x2770, 0x27A0, 0x27D0, 0x2800, 0x2830, 0x285F,
	0x288F, 0x28BF, 0x28EE, 0x291E, 0x294E, 0x297D, 0x29AD, 0x29DD, 0x2A0C, 0x2A3C, 0x2A6B, 0x2A9B, 0x2ACA, 0x2AF9,
	0x2B29, 0x2B58, 0x2B87, 0x2BB7, 0x2BE6, 0x2C15, 0x2C44, 0x2C74, 0x2CA3, 0x2CD2, 0x2D01, 0x2D30, 0x2D5F, 0x2D8E,
	0x2DBD, 0x2DEC, 0x2E1B, 0x2E4A, 0x2E79, 0x2EA8, 0x2ED7, 0x2F06, 0x2F34, 0x2F63, 0x2F92, 0x2FC0, 0x2FEF, 0x301E,
	0x304C, 0x307B, 0x30A9, 0x30D8, 0x3107, 0x3135, 0x3163, 0x3192, 0x31C0, 0x31EF, 0x321D, 0x324B, 0x3279, 0x32A8,
	0x32D6, 0x3304, 0x3332, 0x3360, 0x338E, 0x33BC, 0x33EA, 0x3418, 0x3446, 0x3474, 0x34A2, 0x34D0, 0x34FE, 0x352B,
	0x3559, 0x3587, 0x35B5, 0x35E2, 0x3610, 0x363D, 0x366B, 0x3698, 0x36C6, 0x36F3, 0x3721, 0x374E, 0x377C, 0x37A9,
	0x37D6, 0x3803, 0x3831, 0x385E, 0x388B, 0x38B8, 0x38E5, 0x3912, 0x393F, 0x396C, 0x3999, 0x39C6, 0x39F3, 0x3A20,
	0x3A4D, 0x3A79, 0x3AA6, 0x3AD3, 0x3B00, 0x3B2C, 0x3B59, 0x3B85, 0x3BB2, 0x3BDE, 0x3C0B, 0x3C37, 0x3C64, 0x3C90,
	0x3CBC, 0x3CE9, 0x3D15, 0x3D41, 0x3D6D, 0x3D99, 0x3DC5, 0x3DF1, 0x3E1D, 0x3E49, 0x3E75, 0x3EA1, 0x3ECD, 0x3EF9,
	0x3F25, 0x3F50, 0x3F7C, 0x3FA8, 0x3FD3, 0x3FFF, 0x402B, 0x4056, 0x4082, 0x40AD, 0x40D8, 0x4104, 0x412F, 0x415A,
	0x4186, 0x41B1, 0x41DC, 0x4207, 0x4232, 0x425D, 0x4288, 0x42B3, 0x42DE, 0x4309, 0x4334, 0x435F, 0x4389, 0x43B4,
	0x43DF, 0x4409, 0x4434, 0x445F, 0x4489, 0x44B4, 0x44DE, 0x4508, 0x4533, 0x455D, 0x4587, 0x45B1, 0x45DC, 0x4606,
	0x4630, 0x465A, 0x4684, 0x46AE, 0x46D8, 0x4702, 0x472C, 0x4755, 0x477F, 0x47A9, 0x47D2, 0x47FC, 0x4826, 0x484F,
	0x4879, 0x48A2, 0x48CC, 0x48F5, 0x491E, 0x4948, 0x4971, 0x499A, 0x49C3, 0x49EC, 0x4A15, 0x4A3E, 0x4A67, 0x4A90,
	0x4AB9, 0x4AE2, 0x4B0B, 0x4B33, 0x4B5C, 0x4B85, 0x4BAD, 0x4BD6, 0x4BFE, 0x4C27, 0x4C4F, 0x4C78, 0x4CA0, 0x4CC8,
	0x4CF0, 0x4D19, 0x4D41, 0x4D69, 0x4D91, 0x4DB9, 0x4DE1, 0x4E09, 0x4E31, 0x4E58, 0x4E80, 0x4EA8, 0x4ED0, 0x4EF7,
	0x4F1F, 0x4F46, 0x4F6E, 0x4F95, 0x4FBD, 0x4FE4, 0x500B, 0x5032, 0x505A, 0x5081, 0x50A8, 0x50CF, 0x50F6, 0x511D,
	0x5144, 0x516B, 0x5191, 0x51B8, 0x51DF, 0x5205, 0x522C, 0x5253, 0x5279, 0x52A0, 0x52C6, 0x52EC, 0x5313, 0x5339,
	0x535F, 0x5385, 0x53AB, 0x53D1, 0x53F7, 0x541D, 0x5443, 0x5469, 0x548F, 0x54B5, 0x54DA, 0x5500, 0x5525, 0x554B,
	0x5571, 0x5596, 0x55BB, 0x55E1, 0x5606, 0x562B, 0x5650, 0x5675, 0x569B, 0x56C0, 0x56E5, 0x5709, 0x572E, 0x5753,
	0x5778, 0x579D, 0x57C1, 0x57E6, 0x580A, 0x582F, 0x5853, 0x5878, 0x589C, 0x58C0, 0x58E5, 0x5909, 0x592D, 0x5951,
	0x5975, 0x5999, 0x59BD, 0x59E1, 0x5A04, 0x5A28, 0x5A4C, 0x5A6F, 0x5A93, 0x5AB7, 0x5ADA, 0x5AFD, 0x5B21, 0x5B44,
	0x5B67, 0x5B8B, 0x5BAE, 0x5BD1, 0x5BF4, 0x5C17, 0x5C3A, 0x5C5D, 0x5C7F, 0x5CA2, 0x5CC5, 0x5CE7, 0x5D0A, 0x5D2D,
	0x5D4F, 0x5D71, 0x5D94, 0x5DB6, 0x5DD8, 0x5DFA, 0x5E1D, 0x5E3F, 0x5E61, 0x5E83, 0x5EA5, 0x5EC6, 0x5EE8, 0x5F0A,
	0x5F2C, 0x5F4D, 0x5F6F, 0x5F90, 0x5FB2, 0x5FD3, 0x5FF4, 0x6016, 0x6037, 0x6058, 0x6079, 0x609A, 0x60BB, 0x60DC,
	0x60FD, 0x611E, 0x613E, 0x615F, 0x6180, 0x61A0, 0x61C1, 0x61E1, 0x6202, 0x6222, 0x6242, 0x6263, 0x6283, 0x62A3,
	0x62C3, 0x62E3, 0x6303, 0x6323, 0x6342, 0x6362, 0x6382, 0x63A1, 0x63C1, 0x63E0, 0x6400, 0x641F, 0x643F, 0x645E,
	0x647D, 0x649C, 0x64BB, 0x64DA, 0x64F9, 0x6518, 0x6537, 0x6556, 0x6574, 0x6593, 0x65B2, 0x65D0, 0x65EF, 0x660D,
	0x662B, 0x664A, 0x6668, 0x6686, 0x66A4, 0x66C2, 0x66E0, 0x66FE, 0x671C, 0x673A, 0x6757, 0x6775, 0x6792, 0x67B0,
	0x67CD, 0x67EB, 0x6808, 0x6825, 0x6843, 0x6860, 0x687D, 0x689A, 0x68B7, 0x68D4, 0x68F1, 0x690D, 0x692A, 0x6947,
	0x6963, 0x6980, 0x699C, 0x69B9, 0x69D5, 0x69F1, 0x6A0E, 0x6A2A, 0x6A46, 0x6A62, 0x6A7E, 0x6A9A, 0x6AB5, 0x6AD1,
	0x6AED, 0x6B08, 0x6B24, 0x6B40, 0x6B5B, 0x6B76, 0x6B92, 0x6BAD, 0x6BC8, 0x6BE3, 0x6BFE, 0x6C19, 0x6C34, 0x6C4F,
	0x6C6A, 0x6C84, 0x6C9F, 0x6CBA, 0x6CD4, 0x6CEF, 0x6D09, 0x6D23, 0x6D3E, 0x6D58, 0x6D72, 0x6D8C, 0x6DA6, 0x6DC0,
	0x6DDA, 0x6DF3, 0x6E0D, 0x6E27, 0x6E40, 0x6E5A, 0x6E73, 0x6E8D, 0x6EA6, 0x6EBF, 0x6ED9, 0x6EF2, 0x6F0B, 0x6F24,
	0x6F3D, 0x6F55, 0x6F6E, 0x6F87, 0x6FA0, 0x6FB8, 0x6FD1, 0x6FE9, 0x7002, 0x701A, 0x7032, 0x704A, 0x7062, 0x707A,
	0x7092, 0x70AA, 0x70C2, 0x70DA, 0x70F2, 0x7109, 0x7121, 0x7138, 0x7150, 0x7167, 0x717E, 0x7196, 0x71AD, 0x71C4,
	0x71DB, 0x71F2, 0x7209, 0x7220, 0x7236, 0x724D, 0x7264, 0x727A, 0x7291, 0x72A7, 0x72BD, 0x72D4, 0x72EA, 0x7300,
	0x7316, 0x732C, 0x7342, 0x7358, 0x736E, 0x7383, 0x7399, 0x73AE, 0x73C4, 0x73D9, 0x73EF, 0x7404, 0x7419, 0x742E,
	0x7443, 0x7458, 0x746D, 0x7482, 0x7497, 0x74AC, 0x74C0, 0x74D5, 0x74EA, 0x74FE, 0x7512, 0x7527, 0x753B, 0x754F,
	0x7563, 0x7577, 0x758B, 0x759F, 0x75B3, 0x75C7, 0x75DA, 0x75EE, 0x7601, 0x7615, 0x7628, 0x763B, 0x764F, 0x7662,
	0x7675, 0x7688, 0x769B, 0x76AE, 0x76C1, 0x76D3, 0x76E6, 0x76F9, 0x770B, 0x771E, 0x7730, 0x7742, 0x7754, 0x7767,
	0x7779, 0x778B, 0x779D, 0x77AF, 0x77C0, 0x77D2, 0x77E4, 0x77F5, 0x7807, 0x7818, 0x782A, 0x783B, 0x784C, 0x785D,
	0x786E, 0x787F, 0x7890, 0x78A1, 0x78B2, 0x78C3, 0x78D3, 0x78E4, 0x78F4, 0x7905, 0x7915, 0x7925, 0x7936, 0x7946,
	0x7956, 0x7966, 0x7976, 0x7985, 0x7995, 0x79A5, 0x79B5, 0x79C4, 0x79D4, 0x79E3, 0x79F2, 0x7A02, 0x7A11, 0x7A20,
	0x7A2F, 0x7A3E, 0x7A4D, 0x7A5B, 0x7A6A, 0x7A79, 0x7A87, 0x7A96, 0x7AA4, 0x7AB3, 0x7AC1, 0x7ACF, 0x7ADD, 0x7AEB,
	0x7AF9, 0x7B07, 0x7B15, 0x7B23, 0x7B31, 0x7B3E, 0x7B4C, 0x7B59, 0x7B67, 0x7B74, 0x7B81, 0x7B8E, 0x7B9B, 0x7BA8,
	0x7BB5, 0x7BC2, 0x7BCF, 0x7BDC, 0x7BE8, 0x7BF5, 0x7C02, 0x7C0E, 0x7C1A, 0x7C27, 0x7C33, 0x7C3F, 0x7C4B, 0x7C57,
	0x7C63, 0x7C6F, 0x7C7A, 0x7C86, 0x7C92, 0x7C9D, 0x7CA9, 0x7CB4, 0x7CBF, 0x7CCB, 0x7CD6, 0x7CE1, 0x7CEC, 0x7CF7,
	0x7D02, 0x7D0C, 0x7D17, 0x7D22, 0x7D2C, 0x7D37, 0x7D41, 0x7D4B, 0x7D56, 0x7D60, 0x7D6A, 0x7D74, 0x7D7E, 0x7D88,
	0x7D91, 0x7D9B, 0x7DA5, 0x7DAE, 0x7DB8, 0x7DC1, 0x7DCB, 0x7DD4, 0x7DDD, 0x7DE6, 0x7DEF, 0x7DF8, 0x7E01, 0x7E0A,
	0x7E13, 0x7E1B, 0x7E24, 0x7E2C, 0x7E35, 0x7E3D, 0x7E45, 0x7E4D, 0x7E56, 0x7E5E, 0x7E66, 0x7E6D, 0x7E75, 0x7E7D,
	0x7E85, 0x7E8C, 0x7E94, 0x7E9B, 0x7EA3, 0x7EAA, 0x7EB1, 0x7EB8, 0x7EBF, 0x7EC6, 0x7ECD, 0x7ED4, 0x7EDB, 0x7EE1,
	0x7EE8, 0x7EEE, 0x7EF5, 0x7EFB, 0x7F01, 0x7F08, 0x7F0E, 0x7F14, 0x7F1A, 0x7F20, 0x7F25, 0x7F2B, 0x7F31, 0x7F36,
	0x7F3C, 0x7F41, 0x7F47, 0x7F4C, 0x7F51, 0x7F56, 0x7F5B, 0x7F60, 0x7F65, 0x7F6A, 0x7F6F, 0x7F74, 0x7F78, 0x7F7D,
	0x7F81, 0x7F85, 0x7F8A, 0x7F8E, 0x7F92, 0x7F96, 0x7F9A, 0x7F9E, 0x7FA2, 0x7FA6, 0x7FA9, 0x7FAD, 0x7FB0, 0x7FB4,
	0x7FB7, 0x7FBA, 0x7FBE, 0x7FC1, 0x7FC4, 0x7FC7, 0x7FCA, 0x7FCC, 0x7FCF, 0x7FD2, 0x7FD4, 0x7FD7, 0x7FD9, 0x7FDC,
	0x7FDE, 0x7FE0, 0x7FE2, 0x7FE4, 0x7FE6, 0x7FE8, 0x7FEA, 0x7FEC, 0x7FED, 0x7FEF, 0x7FF1, 0x7FF2, 0x7FF3, 0x7FF5,
	0x7FF6, 0x7FF7, 0x7FF8, 0x7FF9, 0x7FFA, 0x7FFB, 0x7FFB, 0x7FFC, 0x7FFD, 0x7FFD, 0x7FFE, 0x7FFE, 0x7FFE, 0x7FFE,
	0x7FFE, 0x7FFF,
};

int16_t sins(uint16_t x) {
	int16_t value;

	x >>= 4;

	if (x & 0x400) {
		value = sintable[0x3FF - (x & 0x3FF)];
	} else {
		value = sintable[x & 0x3FF];
	}

	if (x & 0x800) {
		return -value;
	} else {
		return value;
	}
}

int16_t coss(uint16_t angle) {
	return sins(angle + 0x4000);
}

#endif

#if 1 // region: vector

Vec3f Vec3fRGBfromHSV(float h, float s, float v) {
	float r, g, b;
	
	int i = floor(h * 6);
	float f = h * 6 - i;
	float p = v * (1 - s);
	float q = v * (1 - f * s);
	float t = v * (1 - (1 - f) * s);
	
	switch (i % 6) {
		case 0: r = v, g = t, b = p; break;
		case 1: r = q, g = v, b = p; break;
		case 2: r = p, g = v, b = t; break;
		case 3: r = p, g = q, b = v; break;
		case 4: r = t, g = p, b = v; break;
		default: r = v, g = p, b = q; break;
	}
	
	return (Vec3f){r, g, b};
}

uint32_t Vec3fRGBto24bit(Vec3f color)
{
	color = Vec3f_MulVal(color, 255);
	uint32_t result = 0;
	
	result |= ((uint32_t)color.x) << 16;
	result |= ((uint32_t)color.y) <<  8;
	result |= ((uint32_t)color.z) <<  0;
	
	return result;
}

s16 Atan2S(f32 x, f32 y) {
	return RadToBin(atan2f(y, x));
}

void VecSphToVec3f(Vec3f* dest, VecSph* sph) {
	sph->pitch = clamp(sph->pitch, DegToBin(-89.995f), DegToBin(89.995f));
	dest->x = sph->r * SinS(sph->pitch - DegToBin(90)) * SinS(sph->yaw);
	dest->y = sph->r * CosS(sph->pitch - DegToBin(90));
	dest->z = sph->r * SinS(sph->pitch - DegToBin(90)) * CosS(sph->yaw);
}

void Math_AddVecSphToVec3f(Vec3f* dest, VecSph* sph) {
	sph->pitch = clamp(sph->pitch, DegToBin(-89.995f), DegToBin(89.995f));
	dest->x += sph->r * SinS(sph->pitch - DegToBin(90)) * SinS(sph->yaw);
	dest->y += sph->r * CosS(sph->pitch - DegToBin(90));
	dest->z += sph->r * SinS(sph->pitch - DegToBin(90)) * CosS(sph->yaw);
}

VecSph* VecSph_FromVec3f(VecSph* dest, Vec3f* vec) {
	VecSph sph;
	
	f32 distSquared = SQ(vec->x) + SQ(vec->z);
	f32 dist = sqrtf(distSquared);
	
	if ((dist == 0.0f) && (vec->y == 0.0f)) {
		sph.pitch = 0;
	} else {
		sph.pitch = DegToBin(RadToDeg(atan2(dist, vec->y)));
	}
	
	sph.r = sqrtf(SQ(vec->y) + distSquared);
	if ((vec->x == 0.0f) && (vec->z == 0.0f)) {
		sph.yaw = 0;
	} else {
		sph.yaw = DegToBin(RadToDeg(atan2(vec->x, vec->z)));
	}
	
	*dest = sph;
	
	return dest;
}

VecSph* VecSph_GeoFromVec3f(VecSph* dest, Vec3f* vec) {
	VecSph sph;
	
	VecSph_FromVec3f(&sph, vec);
	sph.pitch = 0x3FFF - sph.pitch;
	
	*dest = sph;
	
	return dest;
}

VecSph* VecSph_GeoFromVec3fDiff(VecSph* dest, Vec3f* a, Vec3f* b) {
	Vec3f sph;
	
	sph.x = b->x - a->x;
	sph.y = b->y - a->y;
	sph.z = b->z - a->z;
	
	return VecSph_GeoFromVec3f(dest, &sph);
}

Vec3f* Vec3f_Up(Vec3f* dest, s16 pitch, s16 yaw, s16 roll) {
	f32 sinPitch;
	f32 cosPitch;
	f32 sinYaw;
	f32 cosYaw;
	f32 sinNegRoll;
	f32 cosNegRoll;
	Vec3f spA4;
	f32 sp54;
	f32 sp4C;
	f32 cosPitchCosYawSinRoll;
	f32 negSinPitch;
	f32 temp_f10_2;
	f32 cosPitchcosYaw;
	f32 temp_f14;
	f32 negSinPitchSinYaw;
	f32 negSinPitchCosYaw;
	f32 cosPitchSinYaw;
	f32 temp_f4_2;
	f32 temp_f6;
	f32 temp_f8;
	f32 temp_f8_2;
	f32 temp_f8_3;
	
	sinPitch = SinS(pitch);
	cosPitch = CosS(pitch);
	sinYaw = SinS(yaw);
	cosYaw = CosS(yaw);
	negSinPitch = -sinPitch;
	sinNegRoll = SinS(-roll);
	cosNegRoll = CosS(-roll);
	negSinPitchSinYaw = negSinPitch * sinYaw;
	temp_f14 = 1.0f - cosNegRoll;
	cosPitchSinYaw = cosPitch * sinYaw;
	sp54 = SQ(cosPitchSinYaw);
	sp4C = (cosPitchSinYaw * sinPitch) * temp_f14;
	cosPitchcosYaw = cosPitch * cosYaw;
	temp_f4_2 = ((1.0f - sp54) * cosNegRoll) + sp54;
	cosPitchCosYawSinRoll = cosPitchcosYaw * sinNegRoll;
	negSinPitchCosYaw = negSinPitch * cosYaw;
	temp_f6 = (cosPitchcosYaw * cosPitchSinYaw) * temp_f14;
	temp_f10_2 = sinPitch * sinNegRoll;
	spA4.x = ((negSinPitchSinYaw * temp_f4_2) + (cosPitch * (sp4C - cosPitchCosYawSinRoll))) +
		(negSinPitchCosYaw * (temp_f6 + temp_f10_2));
	sp54 = SQ(sinPitch);
	temp_f4_2 = (sinPitch * cosPitchcosYaw) * temp_f14;
	temp_f8_3 = cosPitchSinYaw * sinNegRoll;
	temp_f8 = sp4C + cosPitchCosYawSinRoll;
	spA4.y = ((negSinPitchSinYaw * temp_f8) + (cosPitch * (((1.0f - sp54) * cosNegRoll) + sp54))) +
		(negSinPitchCosYaw * (temp_f4_2 - temp_f8_3));
	temp_f8_2 = temp_f6 - temp_f10_2;
	spA4.z = ((negSinPitchSinYaw * temp_f8_2) + (cosPitch * (temp_f4_2 + temp_f8_3))) +
		(negSinPitchCosYaw * (((1.0f - SQ(cosPitchcosYaw)) * cosNegRoll) + SQ(cosPitchcosYaw)));
	*dest = spA4;
	
	return dest;
}

// given the three points of a triangle, get the normal vector
// (get the direction the triangle is facing)
Vec3f Vec3f_NormalFromTriangleVertices(Vec3f v1, Vec3f v2, Vec3f v3)
{
	// is Vec3f_Normalize() required here?
	return Vec3f_Normalize(Vec3f_Cross(Vec3_Minus(v2, v1), Vec3_Minus(v3, v1)));
}

// get yaw pitch vec3 for snapping instance to surface
Vec3f Vec3f_FaceNormalToYawPitch64(Vec3f direction)
{
	Vec3f angles = {0};
	
	// yaw
	angles.y = atan2f(direction.x, direction.z);
	
	// pitch
	float horizontalLength = sqrtf(direction.x * direction.x + direction.z * direction.z);
	angles.z = atan2f(direction.y, horizontalLength);
	
	return angles;
}

/*
f32 Math_DelSmoothStepToF(f32* pValue, f32 target, f32 fraction, f32 step, f32 minStep) {
	step *= gDeltaTime;
	minStep *= gDeltaTime;
	
	if (*pValue != target) {
		f32 stepSize = (target - *pValue) * fraction;
		
		if ((stepSize >= minStep) || (stepSize <= -minStep)) {
			if (stepSize > step) {
				stepSize = step;
			}
			
			if (stepSize < -step) {
				stepSize = -step;
			}
			
			*pValue += stepSize;
		} else {
			if (stepSize < minStep) {
				*pValue += minStep;
				stepSize = minStep;
				
				if (target < *pValue) {
					*pValue = target;
				}
				
			}
			
			if (stepSize > -minStep) {
				*pValue += -minStep;
				
				if (*pValue < target) {
					*pValue = target;
				}
				
			}
			
		}
		
	}
	
	return fabsf(target - *pValue);
}

f64 Math_DelSmoothStepToD(f64* pValue, f64 target, f64 fraction, f64 step, f64 minStep) {
	step *= gDeltaTime;
	minStep *= gDeltaTime;
	
	if (*pValue != target) {
		f64 stepSize = (target - *pValue) * fraction;
		
		if ((stepSize >= minStep) || (stepSize <= -minStep))
			*pValue += clamp(stepSize, -step, step);
		
		else {
			if (stepSize < minStep) {
				*pValue += minStep;
				stepSize = minStep;
				
				if (target < *pValue)
					*pValue = target;
			}
			
			if (stepSize > -minStep) {
				*pValue += -minStep;
				
				if (*pValue < target)
					*pValue = target;
			}
			
		}
		
	}
	
	return fabs(target - *pValue);
}
*/

s16 Math_SmoothStepToS(s16* pValue, s16 target, s16 scale, s16 step, s16 minStep) {
	s16 stepSize = 0;
	s16 diff = target - *pValue;
	
	if (*pValue != target) {
		stepSize = diff / scale;
		
		if ((stepSize > minStep) || (stepSize < -minStep))
			*pValue += clamp(stepSize, -step, step);
		
		else {
			if (diff >= 0) {
				*pValue += minStep;
				
				if ((s16)(target - *pValue) <= 0)
					*pValue = target;
			} else {
				*pValue -= minStep;
				
				if ((s16)(target - *pValue) >= 0)
					*pValue = target;
			}
			
		}
		
	}
	
	return diff;
}

int Math_SmoothStepToI(int* pValue, int target, int scale, int step, int minStep) {
	int stepSize = 0;
	int diff = target - *pValue;
	
	if (*pValue != target) {
		stepSize = diff / scale;
		
		if ((stepSize > minStep) || (stepSize < -minStep))
			*pValue += clamp(stepSize, -step, step);
		
		else {
			if (diff >= 0) {
				*pValue += minStep;
				
				if ((s16)(target - *pValue) <= 0)
					*pValue = target;
			} else {
				*pValue -= minStep;
				
				if ((s16)(target - *pValue) >= 0)
					*pValue = target;
			}
			
		}
		
	}
	
	return diff;
}

void Rect_ToCRect(CRect* dst, Rect* src) {
	if (dst) {
		dst->x1 = src->x;
		dst->y1 = src->y;
		dst->x2 = src->x + src->w;
		dst->y2 = src->y + src->h;
	}
	
}

void Rect_ToRect(Rect* dst, CRect* src) {
	if (dst) {
		dst->x = src->x1;
		dst->y = src->y1;
		dst->w = src->x2 + src->x1;
		dst->h = src->y2 + src->y1;
	}
	
}

bool Rect_Check_PosIntersect(Rect* rect, Vec2s* pos) {
	return !(
		pos->y < rect->y ||
		pos->y > rect->h ||
		pos->x < rect->x ||
		pos->x > rect->w
	);
}

Rect Rect_Translate(Rect r, s32 x, s32 y) {
	r.x += x;
	r.w += x;
	r.y += y;
	r.h += y;
	
	return r;
}

Rect Rect_New(s32 x, s32 y, s32 w, s32 h) {
	Rect dest = { x, y, w, h };
	
	return dest;
}

Rect Rect_Add(Rect a, Rect b) {
	return (Rect) {
			   a.x + b.x,
			   a.y + b.y,
			   a.w + b.w,
			   a.h + b.h,
	};
}

Rect Rect_Sub(Rect a, Rect b) {
	return (Rect) {
			   a.x - b.x,
			   a.y - b.y,
			   a.w - b.w,
			   a.h - b.h,
	};
}

Rect Rect_AddPos(Rect a, Rect b) {
	return (Rect) {
			   a.x + b.x,
			   a.y + b.y,
			   a.w,
			   a.h,
	};
}

Rect Rect_SubPos(Rect a, Rect b) {
	return (Rect) {
			   a.x - b.x,
			   a.y - b.y,
			   a.w,
			   a.h,
	};
}

bool Rect_PointIntersect(Rect* rect, s32 x, s32 y) {
	if (x >= rect->x && x < rect->x + rect->w)
		if (y >= rect->y && y < rect->y + rect->h)
			return true;
	
	return false;
}

Vec2s Rect_ClosestPoint(Rect* rect, s32 x, s32 y) {
	Vec2s r;
	
	if (x >= rect->x && x < rect->x + rect->w)
		r.x = x;
	
	else
		r.x = Closest(x, rect->x, rect->x + rect->w);
	
	if (y >= rect->y && y < rect->y + rect->h)
		r.y = y;
	
	else
		r.y = Closest(y, rect->y, rect->y + rect->h);
	
	return r;
}

Vec2s Rect_MidPoint(Rect* rect) {
	return (Vec2s) {
			   rect->x + rect->w * 0.5,
			   rect->y + rect->h * 0.5
	};
}

f32 Rect_PointDistance(Rect* rect, s32 x, s32 y) {
	Vec2s p = { x, y };
	Vec2s r = Rect_ClosestPoint(rect, x, y);
	
	return Vec2s_DistXZ(r, p);
}

int RectW(Rect r) {
	return r.x + r.w;
}

int RectH(Rect r) {
	return r.y + r.h;
}

int Rect_RectIntersect(Rect r, Rect i) {
	if (r.x >= i.x + i.w || i.x >= r.x + r.w)
		return false;
	
	if (r.y >= i.y + i.h || i.y >= r.y + r.h)
		return false;
	
	return true;
}

Rect Rect_Clamp(Rect r, Rect l) {
	int diff;
	
	if ((diff = l.x - r.x) > 0) {
		r.x += diff;
		r.w -= diff;
	}
	
	if ((diff = l.y - r.y) > 0) {
		r.y += diff;
		r.h -= diff;
	}
	
	if ((diff = RectW(r) - RectW(l)) > 0) {
		r.w -= diff;
	}
	
	if ((diff = RectH(r) - RectH(l)) > 0) {
		r.h -= diff;
	}
	
	return r;
}

Rect Rect_FlipHori(Rect r, Rect p) {
	r = Rect_SubPos(r, p);
	r.x = remapf(r.x, 0, p.w, p.w, 0) - r.w;
	return Rect_AddPos(r, p);
}

Rect Rect_FlipVerti(Rect r, Rect p) {
	r = Rect_SubPos(r, p);
	r.y = remapf(r.y, 0, p.h, p.h, 0) - r.h;
	return Rect_AddPos(r, p);
}

Rect Rect_ExpandX(Rect r, int amount) {
	if (amount < 0) {
		r.x -= ABS(amount);
		r.w += ABS(amount);
	} else {
		r.w += ABS(amount);
	}
	
	return r;
}

Rect Rect_ShrinkX(Rect r, int amount) {
	if (amount < 0) {
		r.w -= ABS(amount);
	} else {
		r.x += ABS(amount);
		r.w -= ABS(amount);
	}
	
	return r;
}

Rect Rect_ExpandY(Rect r, int amount) {
	amount = -amount;
	
	if (amount < 0) {
		r.y -= ABS(amount);
		r.h += ABS(amount);
	} else {
		r.h += ABS(amount);
	}
	
	return r;
}

Rect Rect_ShrinkY(Rect r, int amount) {
	amount = -amount;
	
	if (amount < 0) {
		r.h -= ABS(amount);
	} else {
		r.y += ABS(amount);
		r.h -= ABS(amount);
	}
	
	return r;
}

Rect Rect_Scale(Rect r, int x, int y) {
	x = -x;
	y = -y;
	
	r.x += (x);
	r.w -= (x) * 2;
	r.y += (y);
	r.h -= (y) * 2;
	
	return r;
}

Rect Rect_Vec2x2(Vec2s a, Vec2s b) {
	int minx = Min(a.x, b.x);
	int maxx = Max(a.x, b.x);
	int miny = Min(a.y, b.y);
	int maxy = Max(a.y, b.y);
	
	return Rect_New(minx, miny, maxx - minx, maxy - miny);
}

BoundBox BoundBox_New3F(Vec3f point) {
	BoundBox this;
	
	this.xMax = point.x;
	this.xMin = point.x;
	this.yMax = point.y;
	this.yMin = point.y;
	this.zMax = point.z;
	this.zMin = point.z;
	
	return this;
}

BoundBox BoundBox_New2F(Vec2f point) {
	BoundBox this = {};
	
	this.xMax = point.x;
	this.xMin = point.x;
	this.yMax = point.y;
	this.yMin = point.y;
	
	return this;
}

void BoundBox_Adjust3F(BoundBox* this, Vec3f point) {
	this->xMax = Max(this->xMax, point.x);
	this->xMin = Min(this->xMin, point.x);
	this->yMax = Max(this->yMax, point.y);
	this->yMin = Min(this->yMin, point.y);
	this->zMax = Max(this->zMax, point.z);
	this->zMin = Min(this->zMin, point.z);
}

void BoundBox_Adjust2F(BoundBox* this, Vec2f point) {
	this->xMax = Max(this->xMax, point.x);
	this->xMin = Min(this->xMin, point.x);
	this->yMax = Max(this->yMax, point.y);
	this->yMin = Min(this->yMin, point.y);
}

bool Vec2f_PointInShape(Vec2f p, Vec2f* poly, u32 numPoly) {
	bool in = false;
	
	for (u32 i = 0, j = numPoly - 1; i < numPoly; j = i++) {
		if ((poly[i].y > p.y) != (poly[j].y > p.y)
			&& p.x < (poly[j].x - poly[i].x) * (p.y - poly[i].y) / (poly[j].y - poly[i].y) + poly[i].x ) {
			in = !in;
		}
		
	}
	
	return in;
}

Vec3f Vec3f_Cross(Vec3f a, Vec3f b) {
	return (Vec3f) {
			   (a.y * b.z - b.y * a.z),
			   (a.z * b.x - b.z * a.x),
			   (a.x * b.y - b.x * a.y)
	};
}

Vec3s Vec3s_Cross(Vec3s a, Vec3s b) {
	return (Vec3s) {
			   (a.y * b.z - b.y * a.z),
			   (a.z * b.x - b.z * a.x),
			   (a.x * b.y - b.x * a.y)
	};
}

f32 Vec3f_DistXZ(Vec3f a, Vec3f b) {
	f32 dx = b.x - a.x;
	f32 dz = b.z - a.z;
	
	return sqrtf(SQ(dx) + SQ(dz));
}

f32 Vec3f_DistXYZ(Vec3f a, Vec3f b) {
	f32 dx = b.x - a.x;
	f32 dy = b.y - a.y;
	f32 dz = b.z - a.z;
	
	return sqrtf(SQ(dx) + SQ(dy) + SQ(dz));
}

f32 Vec3f_DistXYZ_NoSqrt(Vec3f a, Vec3f b) {
	f32 dx = b.x - a.x;
	f32 dy = b.y - a.y;
	f32 dz = b.z - a.z;
	
	return SQ(dx) + SQ(dy) + SQ(dz);
}

f32 Vec3s_DistXZ(Vec3s a, Vec3s b) {
	f32 dx = b.x - a.x;
	f32 dz = b.z - a.z;
	
	return sqrtf(SQ(dx) + SQ(dz));
}

f32 Vec3s_DistXYZ(Vec3s a, Vec3s b) {
	f32 dx = b.x - a.x;
	f32 dy = b.y - a.y;
	f32 dz = b.z - a.z;
	
	return sqrtf(SQ(dx) + SQ(dy) + SQ(dz));
}

f32 Vec2f_DistXZ(Vec2f a, Vec2f b) {
	f32 dx = b.x - a.x;
	f32 dz = b.y - a.y;
	
	return sqrtf(SQ(dx) + SQ(dz));
}

f32 Vec2s_DistXZ(Vec2s a, Vec2s b) {
	f32 dx = b.x - a.x;
	f32 dz = b.y - a.y;
	
	return sqrtf(SQ(dx) + SQ(dz));
}

s16 Vec3f_Yaw(Vec3f a, Vec3f b) {
	f32 dx = b.x - a.x;
	f32 dz = b.z - a.z;
	
	return Atan2S(dz, dx);
}

s16 Vec2f_Yaw(Vec2f a, Vec2f b) {
	f32 dx = b.x - a.x;
	f32 dz = b.y - a.y;
	
	return Atan2S(dz, dx);
}

s16 Vec3f_Pitch(Vec3f a, Vec3f b) {
	return Atan2S(Vec3f_DistXZ(a, b), a.y - b.y);
}

Vec2f Vec2f_Sub(Vec2f a, Vec2f b) {
	return (Vec2f) { a.x - b.x, a.y - b.y };
}

Vec3f Vec3f_Sub(Vec3f a, Vec3f b) {
	return (Vec3f) { a.x - b.x, a.y - b.y, a.z - b.z };
}

Vec4f Vec4f_Sub(Vec4f a, Vec4f b) {
	return (Vec4f) { a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w };
}

Vec2s Vec2s_Sub(Vec2s a, Vec2s b) {
	return (Vec2s) { a.x - b.x, a.y - b.y };
}

Vec3s Vec3s_Sub(Vec3s a, Vec3s b) {
	return (Vec3s) { a.x - b.x, a.y - b.y, a.z - b.z };
}

Vec4s Vec4s_Sub(Vec4s a, Vec4s b) {
	return (Vec4s) { a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w };
}

Vec2f Vec2f_Add(Vec2f a, Vec2f b) {
	return (Vec2f) { a.x + b.x, a.y + b.y };
}

Vec3f Vec3f_Add(Vec3f a, Vec3f b) {
	return (Vec3f) { a.x + b.x, a.y + b.y, a.z + b.z };
}

Vec4f Vec4f_Add(Vec4f a, Vec4f b) {
	return (Vec4f) { a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w };
}

Vec2s Vec2s_Add(Vec2s a, Vec2s b) {
	return (Vec2s) { a.x + b.x, a.y + b.y };
}

Vec3s Vec3s_Add(Vec3s a, Vec3s b) {
	return (Vec3s) { a.x + b.x, a.y + b.y, a.z + b.z };
}

Vec4s Vec4s_Add(Vec4s a, Vec4s b) {
	return (Vec4s) { a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w };
}

Vec2f Vec2f_Div(Vec2f a, Vec2f b) {
	return (Vec2f) { a.x / b.x, a.y / b.y };
}

Vec3f Vec3f_Div(Vec3f a, Vec3f b) {
	return (Vec3f) { a.x / b.x, a.y / b.y, a.z / b.z };
}

Vec4f Vec4f_Div(Vec4f a, Vec4f b) {
	return (Vec4f) { a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w };
}

Vec2f Vec2f_DivVal(Vec2f a, f32 val) {
	return (Vec2f) { a.x / val, a.y / val };
}

Vec3f Vec3f_DivVal(Vec3f a, f32 val) {
	return (Vec3f) { a.x / val, a.y / val, a.z / val };
}

Vec4f Vec4f_DivVal(Vec4f a, f32 val) {
	return (Vec4f) { a.x / val, a.y / val, a.z / val, a.w / val };
}

Vec2s Vec2s_Div(Vec2s a, Vec2s b) {
	return (Vec2s) { a.x / b.x, a.y / b.y };
}

Vec3s Vec3s_Div(Vec3s a, Vec3s b) {
	return (Vec3s) { a.x / b.x, a.y / b.y, a.z / b.z };
}

Vec4s Vec4s_Div(Vec4s a, Vec4s b) {
	return (Vec4s) { a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w };
}

Vec2s Vec2s_DivVal(Vec2s a, f32 val) {
	return (Vec2s) { a.x / val, a.y / val };
}

Vec3s Vec3s_DivVal(Vec3s a, f32 val) {
	return (Vec3s) { a.x / val, a.y / val, a.z / val };
}

Vec4s Vec4s_DivVal(Vec4s a, f32 val) {
	return (Vec4s) { a.x / val, a.y / val, a.z / val, a.w / val };
}

Vec2f Vec2f_Mul(Vec2f a, Vec2f b) {
	return (Vec2f) { a.x* b.x, a.y* b.y };
}

Vec3f Vec3f_Mul(Vec3f a, Vec3f b) {
	return (Vec3f) { a.x* b.x, a.y* b.y, a.z* b.z };
}

Vec4f Vec4f_Mul(Vec4f a, Vec4f b) {
	return (Vec4f) { a.x* b.x, a.y* b.y, a.z* b.z, a.w* b.w };
}

Vec2f Vec2f_MulVal(Vec2f a, f32 val) {
	return (Vec2f) { a.x* val, a.y* val };
}

Vec3f Vec3f_MulVal(Vec3f a, f32 val) {
	return (Vec3f) { a.x* val, a.y* val, a.z* val };
}

Vec4f Vec4f_MulVal(Vec4f a, f32 val) {
	return (Vec4f) { a.x* val, a.y* val, a.z* val, a.w* val };
}

Vec2s Vec2s_Mul(Vec2s a, Vec2s b) {
	return (Vec2s) { a.x* b.x, a.y* b.y };
}

Vec3s Vec3s_Mul(Vec3s a, Vec3s b) {
	return (Vec3s) { a.x* b.x, a.y* b.y, a.z* b.z };
}

Vec4s Vec4s_Mul(Vec4s a, Vec4s b) {
	return (Vec4s) { a.x* b.x, a.y* b.y, a.z* b.z, a.w* b.w };
}

Vec2s Vec2s_MulVal(Vec2s a, f32 val) {
	return (Vec2s) { a.x* val, a.y* val };
}

Vec3s Vec3s_MulVal(Vec3s a, f32 val) {
	return (Vec3s) { a.x* val, a.y* val, a.z* val };
}

Vec4s Vec4s_MulVal(Vec4s a, f32 val) {
	return (Vec4s) { a.x* val, a.y* val, a.z* val, a.w* val };
}

Vec2f Vec2f_New(f32 x, f32 y) {
	return (Vec2f) { x, y };
}

Vec3f Vec3f_New(f32 x, f32 y, f32 z) {
	return (Vec3f) { x, y, z };
}

Vec4f Vec4f_New(f32 x, f32 y, f32 z, f32 w) {
	return (Vec4f) { x, y, z, w };
}

Vec2s Vec2s_New(s16 x, s16 y) {
	return (Vec2s) { x, y };
}

Vec3s Vec3s_New(s16 x, s16 y, s16 z) {
	return (Vec3s) { x, y, z };
}

Vec4s Vec4s_New(s16 x, s16 y, s16 z, s16 w) {
	return (Vec4s) { x, y, z, w };
}

f32 Vec2f_Dot(Vec2f a, Vec2f b) {
	f32 dot = 0.0f;
	
	for (int i = 0; i < 2; i++)
		dot += a.axis[i] * b.axis[i];
	
	return dot;
}

f32 Vec3f_Dot(Vec3f a, Vec3f b) {
	f32 dot = 0.0f;
	
	for (int i = 0; i < 3; i++)
		dot += a.axis[i] * b.axis[i];
	
	return dot;
}

f32 Vec4f_Dot(Vec4f a, Vec4f b) {
	f32 dot = 0.0f;
	
	for (int i = 0; i < 4; i++)
		dot += a.axis[i] * b.axis[i];
	
	return dot;
}

f32 Vec2s_Dot(Vec2s a, Vec2s b) {
	f32 dot = 0.0f;
	
	for (int i = 0; i < 2; i++)
		dot += a.axis[i] * b.axis[i];
	
	return dot;
}

f32 Vec3s_Dot(Vec3s a, Vec3s b) {
	f32 dot = 0.0f;
	
	for (int i = 0; i < 3; i++)
		dot += a.axis[i] * b.axis[i];
	
	return dot;
}

f32 Vec4s_Dot(Vec4s a, Vec4s b) {
	f32 dot = 0.0f;
	
	for (int i = 0; i < 4; i++)
		dot += a.axis[i] * b.axis[i];
	
	return dot;
}

f32 Vec2f_MagnitudeSQ(Vec2f a) {
	return Vec2f_Dot(a, a);
}

f32 Vec3f_MagnitudeSQ(Vec3f a) {
	return Vec3f_Dot(a, a);
}

f32 Vec4f_MagnitudeSQ(Vec4f a) {
	return Vec4f_Dot(a, a);
}

f32 Vec2s_MagnitudeSQ(Vec2s a) {
	return Vec2s_Dot(a, a);
}

f32 Vec3s_MagnitudeSQ(Vec3s a) {
	return Vec3s_Dot(a, a);
}

f32 Vec4s_MagnitudeSQ(Vec4s a) {
	return Vec4s_Dot(a, a);
}

f32 Vec2f_Magnitude(Vec2f a) {
	return sqrtf(Vec2f_MagnitudeSQ(a));
}

f32 Vec3f_Magnitude(Vec3f a) {
	return sqrtf(Vec3f_MagnitudeSQ(a));
}

f32 Vec4f_Magnitude(Vec4f a) {
	return sqrtf(Vec4f_MagnitudeSQ(a));
}

f32 Vec2s_Magnitude(Vec2s a) {
	return sqrtf(Vec2s_MagnitudeSQ(a));
}

f32 Vec3s_Magnitude(Vec3s a) {
	return sqrtf(Vec3s_MagnitudeSQ(a));
}

f32 Vec4s_Magnitude(Vec4s a) {
	return sqrtf(Vec4s_MagnitudeSQ(a));
}

Vec2f Vec2f_Median(Vec2f a, Vec2f b) {
	Vec2f vec;
	
	for (int i = 0; i < 2; i++)
		vec.axis[i] = (a.axis[i] + b.axis[i]) * 0.5f;
	
	return vec;
}

Vec3f Vec3f_Median(Vec3f a, Vec3f b) {
	Vec3f vec;
	
	for (int i = 0; i < 3; i++)
		vec.axis[i] = (a.axis[i] + b.axis[i]) * 0.5f;
	
	return vec;
}

Vec4f Vec4f_Median(Vec4f a, Vec4f b) {
	Vec4f vec;
	
	for (int i = 0; i < 4; i++)
		vec.axis[i] = (a.axis[i] + b.axis[i]) * 0.5f;
	
	return vec;
}

Vec2s Vec2s_Median(Vec2s a, Vec2s b) {
	Vec2s vec;
	
	for (int i = 0; i < 2; i++)
		vec.axis[i] = (a.axis[i] + b.axis[i]) * 0.5f;
	
	return vec;
}

Vec3s Vec3s_Median(Vec3s a, Vec3s b) {
	Vec3s vec;
	
	for (int i = 0; i < 3; i++)
		vec.axis[i] = (a.axis[i] + b.axis[i]) * 0.5f;
	
	return vec;
}

Vec4s Vec4s_Median(Vec4s a, Vec4s b) {
	Vec4s vec;
	
	for (int i = 0; i < 4; i++)
		vec.axis[i] = (a.axis[i] + b.axis[i]) * 0.5f;
	
	return vec;
}

Vec2f Vec2f_Normalize(Vec2f a) {
	f32 mgn = Vec2f_Magnitude(a);
	
	if (mgn == 0.0f)
		return Vec2f_MulVal(a, 0.0f);
	else
		return Vec2f_DivVal(a, mgn);
}

Vec3f Vec3f_Normalize(Vec3f a) {
	f32 mgn = Vec3f_Magnitude(a);
	
	if (mgn == 0.0f)
		return Vec3f_MulVal(a, 0.0f);
	else
		return Vec3f_DivVal(a, mgn);
}

Vec4f Vec4f_Normalize(Vec4f a) {
	f32 mgn = Vec4f_Magnitude(a);
	
	if (mgn == 0.0f)
		return Vec4f_MulVal(a, 0.0f);
	else
		return Vec4f_DivVal(a, mgn);
}

Vec2s Vec2s_Normalize(Vec2s a) {
	f32 mgn = Vec2s_Magnitude(a);
	
	if (mgn == 0.0f)
		return Vec2s_MulVal(a, 0.0f);
	else
		return Vec2s_DivVal(a, mgn);
}

Vec3s Vec3s_Normalize(Vec3s a) {
	f32 mgn = Vec3s_Magnitude(a);
	
	if (mgn == 0.0f)
		return Vec3s_MulVal(a, 0.0f);
	else
		return Vec3s_DivVal(a, mgn);
}

Vec4s Vec4s_Normalize(Vec4s a) {
	f32 mgn = Vec4s_Magnitude(a);
	
	if (mgn == 0.0f)
		return Vec4s_MulVal(a, 0.0f);
	else
		return Vec4s_DivVal(a, mgn);
}

Vec2f Vec2f_LineSegDir(Vec2f a, Vec2f b) {
	return Vec2f_Normalize(Vec2f_Sub(b, a));
}

Vec3f Vec3f_LineSegDir(Vec3f a, Vec3f b) {
	return Vec3f_Normalize(Vec3f_Sub(b, a));
}

Vec4f Vec4f_LineSegDir(Vec4f a, Vec4f b) {
	return Vec4f_Normalize(Vec4f_Sub(b, a));
}

Vec2s Vec2s_LineSegDir(Vec2s a, Vec2s b) {
	return Vec2s_Normalize(Vec2s_Sub(b, a));
}

Vec3s Vec3s_LineSegDir(Vec3s a, Vec3s b) {
	return Vec3s_Normalize(Vec3s_Sub(b, a));
}

Vec4s Vec4s_LineSegDir(Vec4s a, Vec4s b) {
	return Vec4s_Normalize(Vec4s_Sub(b, a));
}

Vec2f Vec2f_Project(Vec2f a, Vec2f b) {
	f32 ls = Vec2f_MagnitudeSQ(b);
	
	return Vec2f_MulVal(b, Vec2f_Dot(b, a) / ls);
}

Vec3f Vec3f_Project(Vec3f a, Vec3f b) {
	f32 ls = Vec3f_MagnitudeSQ(b);
	
	return Vec3f_MulVal(b, Vec3f_Dot(b, a) / ls);
}

Vec4f Vec4f_Project(Vec4f a, Vec4f b) {
	f32 ls = Vec4f_MagnitudeSQ(b);
	
	return Vec4f_MulVal(b, Vec4f_Dot(b, a) / ls);
}

Vec2s Vec2s_Project(Vec2s a, Vec2s b) {
	f32 ls = Vec2s_MagnitudeSQ(b);
	
	return Vec2s_MulVal(b, Vec2s_Dot(b, a) / ls);
}

Vec3s Vec3s_Project(Vec3s a, Vec3s b) {
	f32 ls = Vec3s_MagnitudeSQ(b);
	
	return Vec3s_MulVal(b, Vec3s_Dot(b, a) / ls);
}

Vec4s Vec4s_Project(Vec4s a, Vec4s b) {
	f32 ls = Vec4s_MagnitudeSQ(b);
	
	return Vec4s_MulVal(b, Vec4s_Dot(b, a) / ls);
}

Vec2f Vec2f_Invert(Vec2f a) {
	return Vec2f_MulVal(a, -1);
}

Vec3f Vec3f_Invert(Vec3f a) {
	return Vec3f_MulVal(a, -1);
}

Vec4f Vec4f_Invert(Vec4f a) {
	return Vec4f_MulVal(a, -1);
}

Vec2s Vec2s_Invert(Vec2s a) {
	return Vec2s_MulVal(a, -1);
}

Vec3s Vec3s_Invert(Vec3s a) {
	return Vec3s_MulVal(a, -1);
}

Vec4s Vec4s_Invert(Vec4s a) {
	return Vec4s_MulVal(a, -1);
}

Vec2f Vec2f_InvMod(Vec2f a) {
	Vec2f r;
	
	for (int i = 0; i < ArrCount(a.axis); i++)
		r.axis[i] = 1.0f - fabsf(a.axis[i]);
	
	return r;
}

Vec3f Vec3f_InvMod(Vec3f a) {
	Vec3f r;
	
	for (int i = 0; i < ArrCount(a.axis); i++)
		r.axis[i] = 1.0f - fabsf(a.axis[i]);
	
	return r;
}

Vec4f Vec4f_InvMod(Vec4f a) {
	Vec4f r;
	
	for (int i = 0; i < ArrCount(a.axis); i++)
		r.axis[i] = 1.0f - fabsf(a.axis[i]);
	
	return r;
}

bool Vec2f_IsNaN(Vec2f a) {
	for (int i = 0; i < ArrCount(a.axis); i++)
		if (isnan(a.axis[i])) true;
	
	return false;
}

bool Vec3f_IsNaN(Vec3f a) {
	for (int i = 0; i < ArrCount(a.axis); i++)
		if (isnan(a.axis[i])) true;
	
	return false;
}

bool Vec3f_IsNonZero(Vec3f a) {
	return a.x || a.y || a.z;
}

bool Vec4f_IsNaN(Vec4f a) {
	for (int i = 0; i < ArrCount(a.axis); i++)
		if (isnan(a.axis[i])) true;
	
	return false;
}

f32 Vec2f_Cos(Vec2f a, Vec2f b) {
	f32 mp = Vec2f_Magnitude(a) * Vec2f_Magnitude(b);
	
	if (IsZero(mp)) return 0.0f;
	
	return Vec2f_Dot(a, b) / mp;
}

f32 Vec3f_Cos(Vec3f a, Vec3f b) {
	f32 mp = Vec3f_Magnitude(a) * Vec3f_Magnitude(b);
	
	if (IsZero(mp)) return 0.0f;
	
	return Vec3f_Dot(a, b) / mp;
}

f32 Vec4f_Cos(Vec4f a, Vec4f b) {
	f32 mp = Vec4f_Magnitude(a) * Vec4f_Magnitude(b);
	
	if (IsZero(mp)) return 0.0f;
	
	return Vec4f_Dot(a, b) / mp;
}

f32 Vec2s_Cos(Vec2s a, Vec2s b) {
	f32 mp = Vec2s_Magnitude(a) * Vec2s_Magnitude(b);
	
	if (IsZero(mp)) return 0.0f;
	
	return Vec2s_Dot(a, b) / mp;
}

f32 Vec3s_Cos(Vec3s a, Vec3s b) {
	f32 mp = Vec3s_Magnitude(a) * Vec3s_Magnitude(b);
	
	if (IsZero(mp)) return 0.0f;
	
	return Vec3s_Dot(a, b) / mp;
}

f32 Vec4s_Cos(Vec4s a, Vec4s b) {
	f32 mp = Vec4s_Magnitude(a) * Vec4s_Magnitude(b);
	
	if (IsZero(mp)) return 0.0f;
	
	return Vec4s_Dot(a, b) / mp;
}

Vec2f Vec2f_Reflect(Vec2f vec, Vec2f normal) {
	Vec2f negVec = Vec2f_Invert(vec);
	f32 vecDotNormal = Vec2f_Cos(negVec, normal);
	Vec2f normalScale = Vec2f_MulVal(normal, vecDotNormal);
	Vec2f nsVec = Vec2f_Add(normalScale, vec);
	
	return Vec2f_Add(negVec, Vec2f_MulVal(nsVec, 2.0f));
}

Vec3f Vec3f_Reflect(Vec3f vec, Vec3f normal) {
	Vec3f negVec = Vec3f_Invert(vec);
	f32 vecDotNormal = Vec3f_Cos(negVec, normal);
	Vec3f normalScale = Vec3f_MulVal(normal, vecDotNormal);
	Vec3f nsVec = Vec3f_Add(normalScale, vec);
	
	return Vec3f_Add(negVec, Vec3f_MulVal(nsVec, 2.0f));
}

Vec4f Vec4f_Reflect(Vec4f vec, Vec4f normal) {
	Vec4f negVec = Vec4f_Invert(vec);
	f32 vecDotNormal = Vec4f_Cos(negVec, normal);
	Vec4f normalScale = Vec4f_MulVal(normal, vecDotNormal);
	Vec4f nsVec = Vec4f_Add(normalScale, vec);
	
	return Vec4f_Add(negVec, Vec4f_MulVal(nsVec, 2.0f));
}

Vec2s Vec2s_Reflect(Vec2s vec, Vec2s normal) {
	Vec2s negVec = Vec2s_Invert(vec);
	f32 vecDotNormal = Vec2s_Cos(negVec, normal);
	Vec2s normalScale = Vec2s_MulVal(normal, vecDotNormal);
	Vec2s nsVec = Vec2s_Add(normalScale, vec);
	
	return Vec2s_Add(negVec, Vec2s_MulVal(nsVec, 2.0f));
}

Vec3s Vec3s_Reflect(Vec3s vec, Vec3s normal) {
	Vec3s negVec = Vec3s_Invert(vec);
	f32 vecDotNormal = Vec3s_Cos(negVec, normal);
	Vec3s normalScale = Vec3s_MulVal(normal, vecDotNormal);
	Vec3s nsVec = Vec3s_Add(normalScale, vec);
	
	return Vec3s_Add(negVec, Vec3s_MulVal(nsVec, 2.0f));
}

Vec4s Vec4s_Reflect(Vec4s vec, Vec4s normal) {
	Vec4s negVec = Vec4s_Invert(vec);
	f32 vecDotNormal = Vec4s_Cos(negVec, normal);
	Vec4s normalScale = Vec4s_MulVal(normal, vecDotNormal);
	Vec4s nsVec = Vec4s_Add(normalScale, vec);
	
	return Vec4s_Add(negVec, Vec4s_MulVal(nsVec, 2.0f));
}

Vec3f Vec3f_ClosestPointOnRay(Vec3f rayStart, Vec3f rayEnd, Vec3f lineStart, Vec3f lineEnd) {
	Vec3f rayNorm = Vec3f_LineSegDir(rayStart, rayEnd);
	Vec3f lineNorm = Vec3f_LineSegDir(lineStart, lineEnd);
	f32 ndot = Vec3f_Dot(rayNorm, lineNorm);
	
	if (fabsf(ndot) >= 1.0f - FLT_EPSILON) {
		// parallel
		
		return lineStart;
	} else if (IsZero(ndot)) {
		// perpendicular
		Vec3f mod = Vec3f_InvMod(rayNorm);
		Vec3f ls = Vec3f_Mul(lineStart, mod);
		Vec3f rs = Vec3f_Mul(rayStart, mod);
		Vec3f side = Vec3f_Project(Vec3f_Sub(rs, ls), lineNorm);
		
		return Vec3f_Add(lineStart, side);
	} else {
		Vec3f startDiff = Vec3f_Sub(lineStart, rayStart);
		Vec3f crossNorm = Vec3f_Normalize(Vec3f_Cross(lineNorm, rayNorm));
		Vec3f rejection = Vec3f_Sub(
			Vec3f_Sub(startDiff, Vec3f_Project(startDiff, rayNorm)),
			Vec3f_Project(startDiff, crossNorm)
		);
		f32 rejDot = Vec3f_Dot(lineNorm, Vec3f_Normalize(rejection));
		f32 distToLinePos;
		
		if (rejDot == 0.0f)
			return lineStart;
		
		distToLinePos = Vec3f_Magnitude(rejection) / rejDot;
		
		return Vec3f_Sub(lineStart, Vec3f_MulVal(lineNorm, distToLinePos));
	}
	
}

Vec3f Vec3f_ProjectAlong(Vec3f point, Vec3f lineA, Vec3f lineB) {
	Vec3f seg = Vec3f_LineSegDir(lineA, lineB);
	Vec3f proj = Vec3f_Project(Vec3f_Sub(point, lineA), seg);
	
	return Vec3f_Add(lineA, proj);
}

Quat Quat_New(f32 x, f32 y, f32 z, f32 w) {
	return (Quat) { x, y, z, w };
}

Quat Quat_Identity() {
	return (Quat) { .w = 1.0f };
}

Quat Quat_Sub(Quat a, Quat b) {
	return (Quat) {
			   a.x - b.x,
			   a.y - b.y,
			   a.z - b.z,
			   a.w - b.w,
	};
}

Quat Quat_Add(Quat a, Quat b) {
	return (Quat) {
			   a.x + b.x,
			   a.y + b.y,
			   a.z + b.z,
			   a.w + b.w,
	};
}

Quat Quat_Div(Quat a, Quat b) {
	return (Quat) {
			   a.x / b.x,
			   a.y / b.y,
			   a.z / b.z,
			   a.w / b.w,
	};
}

Quat Quat_Mul(Quat a, Quat b) {
	return (Quat) {
			   a.x* b.x,
				   a.y* b.y,
				   a.z* b.z,
				   a.w* b.w,
	};
}

Quat Quat_QMul(Quat a, Quat b) {
	return (Quat) {
			   (a.x * b.w) + (b.x * a.w) + (a.y * b.z) - (a.z * b.y),
			   (a.y * b.w) + (b.y * a.w) + (a.z * b.x) - (a.x * b.z),
			   (a.z * b.w) + (b.z * a.w) + (a.x * b.y) - (a.y * b.x),
			   (a.w * b.w) + (b.x * a.x) + (a.y * b.y) - (a.z * b.z),
	};
}

Quat Quat_SubVal(Quat q, float val) {
	return (Quat) {
			   q.x - val,
			   q.y - val,
			   q.z - val,
			   q.w - val,
	};
}

Quat Quat_AddVal(Quat q, float val) {
	return (Quat) {
			   q.x + val,
			   q.y + val,
			   q.z + val,
			   q.w + val,
	};
}

Quat Quat_DivVal(Quat q, float val) {
	return (Quat) {
			   q.x / val,
			   q.y / val,
			   q.z / val,
			   q.w / val,
	};
}

Quat Quat_MulVal(Quat q, float val) {
	return (Quat) {
			   q.x* val,
				   q.y* val,
				   q.z* val,
				   q.w* val,
	};
}

f32 Quat_MagnitudeSQ(Quat q) {
	return SQ(q.x) + SQ(q.y) + SQ(q.z) + SQ(q.w);
}

f32 Quat_Magnitude(Quat q) {
	return sqrtf(SQ(q.x) + SQ(q.y) + SQ(q.z) + SQ(q.w));
}

Quat Quat_Normalize(Quat q) {
	f32 mgn = Quat_Magnitude(q);
	
	if (mgn)
		return Quat_DivVal(q, mgn);
	return Quat_New(0, 0, 0, 0);
}

Quat Quat_LineSegDir(Vec3f a, Vec3f b) {
	Vec3f c = Vec3f_Cross(a, b);
	f32 d = Vec3f_Dot(a, b);
	Quat q = Quat_New(c.x, c.y, c.z, 1.0f + d);
	
	return Quat_Normalize(q);
}

Quat Quat_Invert(Quat q) {
	f32 mgnSq = Quat_MagnitudeSQ(q);
	
	if (mgnSq)
		return (Quat) {
				   q.x / -mgnSq,
				   q.y / -mgnSq,
				   q.z / -mgnSq,
				   q.w / mgnSq,
		};
	
	return Quat_New(0, 0, 0, 0);
}

bool Quat_Equals(Quat* qa, Quat* qb) {
	f32* a = &qa->x;
	f32* b = &qb->x;
	
	for (int i = 0; i < 4; i++)
		if (fabsf(*a - *b) > FLT_EPSILON)
			return false;
	return true;
}

// randomized angle version; requires significantly fewer iterations
// than the other brute force method, at the cost of a little accuracy
// (is way faster: 46,656,000 iterations -> 1000 iterations)
Vec3f Vec3f_BruteforceEulerAnglesTowardsDirection(Vec3f angles, Vec3f dir, Vec3f up) {
	float mag = 100;
	float nearest = FLT_MAX;
	Vec3f distal = Vec3f_MulVal(Vec3f_Normalize(up), mag);
	Vec3f idealDistal = Vec3f_MulVal(Vec3f_Normalize(dir), mag);
	Vec3f result = angles;
	float deviation;
		
	bool TryVariation(Vec3f variation) {
		Vec3f thisDistal;
		Matrix_Push(); {
			Matrix_Translate(0, 0, 0, MTXMODE_NEW);
			Matrix_RotateY(variation.y, MTXMODE_APPLY);
			Matrix_RotateX(variation.x, MTXMODE_APPLY);
			Matrix_RotateZ(variation.z, MTXMODE_APPLY);
			Matrix_MultVec3f(&distal, &thisDistal);
		} Matrix_Pop();
		
		deviation = Vec3f_DistXYZ_NoSqrt(thisDistal, idealDistal);
		if (deviation < nearest) {
			nearest = deviation;
			result = variation;
			return true;
		}
		
		return false;
	}
	
	// first pass: if input angle is already good, preserve it
	if (TryVariation(angles)
		&& sqrt(deviation) < mag * 0.01
	) return angles;
	
	// brute-force 45 degree angles first so
	// perpendicular meshes don't suffer precision
	// (shouldn't be slow enough to matter)
	int bruteStep = 45;
	for (int i = 0; i < 360; i += bruteStep) {
		for (int j = 0; j < 360; j += bruteStep) {
			for (int k = 0; k < 360; k += bruteStep) {
				Vec3f variation = {
					DegToRad(i), DegToRad(j), DegToRad(k)
				};
				if (TryVariation(variation)
					&& sqrt(deviation) < mag * 0.01
				) return result;
			}
		}
	}
	
	// try a bunch of random rotations, use whichever one is closest
	for (int l = 0; l < 1000; ++l) {
		// testing axis alignment, revert to % 360 it causes problems
		int i = rand() % 360;
		int j = rand() % 180 + 180;
		int k = 0;//rand() % 360;
		Vec3f variation = {
			DegToRad(i), DegToRad(j), DegToRad(k)
		};
		
		TryVariation(variation);
	}
	
	return result;
}

#if 0
// brute force version, incredibly slow b/c it iterates too many
// times (see above function), but keeping it here for reference
// since it seems to reliably produce the correct Euler angles
Vec3f Vec3f_BruteforceEulerAnglesTowardsDirection(Vec3f angles, Vec3f dir, Vec3f up) {
	float mag = 100;
	float nearest = FLT_MAX;
	Vec3f distal = Vec3f_MulVal(Vec3f_Normalize(up), mag);
	Vec3f idealDistal = Vec3f_MulVal(Vec3f_Normalize(dir), mag);
	Vec3f result = angles;
	
	for (int i = 0; i < 360; ++i) {
		for (int j = 0; j < 360; ++j) {
			for (int k = 0; k < 360; ++k) {
				Vec3f thisDistal;
				float deviation;
				Vec3f variation = {
					DegToRad(i), DegToRad(j), DegToRad(k)
				};
				
				Matrix_Push(); {
					Matrix_Translate(0, 0, 0, MTXMODE_NEW);
					Matrix_RotateY(variation.y, MTXMODE_APPLY);
					Matrix_RotateX(variation.x, MTXMODE_APPLY);
					Matrix_RotateZ(variation.z, MTXMODE_APPLY);
					Matrix_MultVec3f(&distal, &thisDistal);
				} Matrix_Pop();
				
				deviation = Vec3f_DistXYZ_NoSqrt(thisDistal, idealDistal);
				if (deviation < nearest) {
					nearest = deviation;
					result = variation;
				}
			}
		}
	}
	
	return result;
}
#endif

// XXX limited by Vec3f_FaceNormalToYawPitch64() output
//     not being accurate enough, led to above methods
#if 0
// it's unideal that this must exist, but it's used very
// infrequently and doesn't involve many iterations; this
// takes an Euler rotation that is 'mostly' correct, and
// tries every combination of +-xyz to determine how to
// correct it so anything rotated using it points in the
// same direction as the given vector (converting vector
// directions to Euler angles is nontrivial to begin with,
// due to gimbal lock and different game instances wanting
// to face different directions, so this is acceptable)
Vec3f Vec3f_BruteforceEulerAnglesTowardsDirection(Vec3f angles, Vec3f dir, Vec3f up) {
	float mag = 100;
	float nearest = FLT_MAX;
	Vec3f distal = Vec3f_MulVal(Vec3f_Normalize(up), mag);
	Vec3f idealDistal = Vec3f_MulVal(Vec3f_Normalize(dir), mag);
	Vec3f result = angles;
	float components[3] = { angles.x, angles.y, angles.z };
	int signs[8][3] = {
		{1, 1, 1},
		{1, 1, -1},
		{1, -1, 1},
		{1, -1, -1},
		{-1, 1, 1},
		{-1, 1, -1},
		{-1, -1, 1},
		{-1, -1, -1}
	};
	
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			if (j == i) continue;
			for (int k = 0; k < 3; k++) {
				if (k == i || k == j) continue;
				for (int s = 0; s < 8; s++) {
					Vec3f variation = {
						components[i] * signs[s][0],
						components[j] * signs[s][1],
						components[k] * signs[s][2]
					};
					Vec3f thisDistal;
					float deviation;
					
					Matrix_Push(); {
						Matrix_Translate(0, 0, 0, MTXMODE_NEW);
						Matrix_RotateY(variation.y, MTXMODE_APPLY);
						Matrix_RotateX(variation.x, MTXMODE_APPLY);
						Matrix_RotateZ(variation.z, MTXMODE_APPLY);
						Matrix_MultVec3f(&distal, &thisDistal);
					} Matrix_Pop();
					
					deviation = Vec3f_DistXYZ_NoSqrt(thisDistal, idealDistal);
					if (deviation < nearest) {
						nearest = deviation;
						result = variation;
					}
				}
			}
		}
	}
	
	return result;
}
#endif

#endif // endregion: vector

#if 1 // region: matrix

#define    FTOFIX32(x) (long)((x) * (float)0x00010000)
#define    FIX32TOF(x) ((float)(x) * (1.0f / (float)0x00010000))

Matrix* gMatrixStack;
Matrix* gCurrentMatrix;
const Matrix gMtxFClear = {
	1.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 1.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 1.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 1.0f,
};

void Matrix_Init() {
	if (gCurrentMatrix)
		return;
	gCurrentMatrix = calloc(20, sizeof(Matrix));
	gMatrixStack = gCurrentMatrix;
}

void Matrix_Clear(Matrix* mf) {
	mf->xx = 1.0f;
	mf->yy = 1.0f;
	mf->zz = 1.0f;
	mf->ww = 1.0f;
	mf->yx = 0.0f;
	mf->zx = 0.0f;
	mf->wx = 0.0f;
	mf->xy = 0.0f;
	mf->zy = 0.0f;
	mf->wy = 0.0f;
	mf->xz = 0.0f;
	mf->yz = 0.0f;
	mf->wz = 0.0f;
	mf->xw = 0.0f;
	mf->yw = 0.0f;
	mf->zw = 0.0f;
}

void Matrix_Push(void) {
	Matrix_MtxFCopy(gCurrentMatrix + 1, gCurrentMatrix);
	gCurrentMatrix++;
}

void Matrix_Pop(void) {
	gCurrentMatrix--;
}

void Matrix_Get(Matrix* dest) {
	assert(dest != NULL);
	Matrix_MtxFCopy(dest, gCurrentMatrix);
}

void Matrix_Put(Matrix* src) {
	assert(src != NULL);
	Matrix_MtxFCopy(gCurrentMatrix, src);
}

void Matrix_Mult(Matrix* mf, MtxMode mode) {
	Matrix* cmf = gCurrentMatrix;
	
	if (mode == MTXMODE_APPLY) {
		Matrix_MtxFMtxFMult(cmf, mf, cmf);
	} else {
		Matrix_MtxFCopy(gCurrentMatrix, mf);
	}
}

void Matrix_MultVec3f_Ext(Vec3f* src, Vec3f* dest, Matrix* mf) {
	dest->x = mf->xw + (mf->xx * src->x + mf->xy * src->y + mf->xz * src->z);
	dest->y = mf->yw + (mf->yx * src->x + mf->yy * src->y + mf->yz * src->z);
	dest->z = mf->zw + (mf->zx * src->x + mf->zy * src->y + mf->zz * src->z);
}

void Matrix_OrientVec3f_Ext(Vec3f* src, Vec3f* dest, Matrix* mf) {
	dest->x = mf->xx * src->x + mf->xy * src->y + mf->xz * src->z;
	dest->y = mf->yx * src->x + mf->yy * src->y + mf->yz * src->z;
	dest->z = mf->zx * src->x + mf->zy * src->y + mf->zz * src->z;
}

void Matrix_MultVec3fToVec4f_Ext(Vec3f* src, Vec4f* dest, Matrix* mf) {
	dest->x = mf->xw + (mf->xx * src->x + mf->xy * src->y + mf->xz * src->z);
	dest->y = mf->yw + (mf->yx * src->x + mf->yy * src->y + mf->yz * src->z);
	dest->z = mf->zw + (mf->zx * src->x + mf->zy * src->y + mf->zz * src->z);
	dest->w = mf->ww + (mf->wx * src->x + mf->wy * src->y + mf->wz * src->z);
}

void Matrix_MultVec3f(Vec3f* src, Vec3f* dest) {
	Matrix* cmf = gCurrentMatrix;
	
	dest->x = cmf->xw + (cmf->xx * src->x + cmf->xy * src->y + cmf->xz * src->z);
	dest->y = cmf->yw + (cmf->yx * src->x + cmf->yy * src->y + cmf->yz * src->z);
	dest->z = cmf->zw + (cmf->zx * src->x + cmf->zy * src->y + cmf->zz * src->z);
}

void Matrix_Transpose(Matrix* mf) {
	f32 temp;
	
	temp = mf->yx;
	mf->yx = mf->xy;
	mf->xy = temp;
	
	temp = mf->zx;
	mf->zx = mf->xz;
	mf->xz = temp;
	
	temp = mf->zy;
	mf->zy = mf->yz;
	mf->yz = temp;
}

void Matrix_MtxFTranslate(Matrix* cmf, f32 x, f32 y, f32 z, MtxMode mode) {
	f32 tx;
	f32 ty;
	
	if (mode == MTXMODE_APPLY) {
		tx = cmf->xx;
		ty = cmf->xy;
		cmf->xw += tx * x + ty * y + cmf->xz * z;
		tx = cmf->yx;
		ty = cmf->yy;
		cmf->yw += tx * x + ty * y + cmf->yz * z;
		tx = cmf->zx;
		ty = cmf->zy;
		cmf->zw += tx * x + ty * y + cmf->zz * z;
		tx = cmf->wx;
		ty = cmf->wy;
		cmf->ww += tx * x + ty * y + cmf->wz * z;
	} else {
		cmf->yx = 0.0f;
		cmf->zx = 0.0f;
		cmf->wx = 0.0f;
		cmf->xy = 0.0f;
		cmf->zy = 0.0f;
		cmf->wy = 0.0f;
		cmf->xz = 0.0f;
		cmf->yz = 0.0f;
		cmf->wz = 0.0f;
		cmf->xx = 1.0f;
		cmf->yy = 1.0f;
		cmf->zz = 1.0f;
		cmf->ww = 1.0f;
		cmf->xw = x;
		cmf->yw = y;
		cmf->zw = z;
	}
}

void Matrix_Translate(f32 x, f32 y, f32 z, MtxMode mode) {
	Matrix_MtxFTranslate(gCurrentMatrix, x, y, z, mode);
}

void Matrix_Scale(f32 x, f32 y, f32 z, MtxMode mode) {
	Matrix* cmf = gCurrentMatrix;
	
	if (mode == MTXMODE_APPLY) {
		cmf->xx *= x;
		cmf->yx *= x;
		cmf->zx *= x;
		cmf->xy *= y;
		cmf->yy *= y;
		cmf->zy *= y;
		cmf->xz *= z;
		cmf->yz *= z;
		cmf->zz *= z;
		cmf->wx *= x;
		cmf->wy *= y;
		cmf->wz *= z;
	} else {
		cmf->yx = 0.0f;
		cmf->zx = 0.0f;
		cmf->wx = 0.0f;
		cmf->xy = 0.0f;
		cmf->zy = 0.0f;
		cmf->wy = 0.0f;
		cmf->xz = 0.0f;
		cmf->yz = 0.0f;
		cmf->wz = 0.0f;
		cmf->xw = 0.0f;
		cmf->yw = 0.0f;
		cmf->zw = 0.0f;
		cmf->ww = 1.0f;
		cmf->xx = x;
		cmf->yy = y;
		cmf->zz = z;
	}
}

void Matrix_RotateX(f32 x, MtxMode mode) {
	Matrix* cmf;
	f32 sin;
	f32 cos;
	f32 temp1;
	f32 temp2;
	
	if (mode == MTXMODE_APPLY) {
		if (x != 0) {
			cmf = gCurrentMatrix;
			
			sin = sinf(x);
			cos = cosf(x);
			
			temp1 = cmf->xy;
			temp2 = cmf->xz;
			cmf->xy = temp1 * cos + temp2 * sin;
			cmf->xz = temp2 * cos - temp1 * sin;
			
			temp1 = cmf->yy;
			temp2 = cmf->yz;
			cmf->yy = temp1 * cos + temp2 * sin;
			cmf->yz = temp2 * cos - temp1 * sin;
			
			temp1 = cmf->zy;
			temp2 = cmf->zz;
			cmf->zy = temp1 * cos + temp2 * sin;
			cmf->zz = temp2 * cos - temp1 * sin;
			
			temp1 = cmf->wy;
			temp2 = cmf->wz;
			cmf->wy = temp1 * cos + temp2 * sin;
			cmf->wz = temp2 * cos - temp1 * sin;
		}
	} else {
		cmf = gCurrentMatrix;
		
		if (x != 0) {
			sin = sinf(x);
			cos = cosf(x);
		} else {
			sin = 0.0f;
			cos = 1.0f;
		}
		
		cmf->yx = 0.0f;
		cmf->zx = 0.0f;
		cmf->wx = 0.0f;
		cmf->xy = 0.0f;
		cmf->wy = 0.0f;
		cmf->xz = 0.0f;
		cmf->wz = 0.0f;
		cmf->xw = 0.0f;
		cmf->yw = 0.0f;
		cmf->zw = 0.0f;
		cmf->xx = 1.0f;
		cmf->ww = 1.0f;
		cmf->yy = cos;
		cmf->zz = cos;
		cmf->zy = sin;
		cmf->yz = -sin;
	}
}

void Matrix_RotateY(f32 y, MtxMode mode) {
	Matrix* cmf;
	f32 sin;
	f32 cos;
	f32 temp1;
	f32 temp2;
	
	if (mode == MTXMODE_APPLY) {
		if (y != 0) {
			cmf = gCurrentMatrix;
			
			sin = sinf(y);
			cos = cosf(y);
			
			temp1 = cmf->xx;
			temp2 = cmf->xz;
			cmf->xx = temp1 * cos - temp2 * sin;
			cmf->xz = temp1 * sin + temp2 * cos;
			
			temp1 = cmf->yx;
			temp2 = cmf->yz;
			cmf->yx = temp1 * cos - temp2 * sin;
			cmf->yz = temp1 * sin + temp2 * cos;
			
			temp1 = cmf->zx;
			temp2 = cmf->zz;
			cmf->zx = temp1 * cos - temp2 * sin;
			cmf->zz = temp1 * sin + temp2 * cos;
			
			temp1 = cmf->wx;
			temp2 = cmf->wz;
			cmf->wx = temp1 * cos - temp2 * sin;
			cmf->wz = temp1 * sin + temp2 * cos;
		}
	} else {
		cmf = gCurrentMatrix;
		
		if (y != 0) {
			sin = sinf(y);
			cos = cosf(y);
		} else {
			sin = 0.0f;
			cos = 1.0f;
		}
		
		cmf->yx = 0.0f;
		cmf->wx = 0.0f;
		cmf->xy = 0.0f;
		cmf->zy = 0.0f;
		cmf->wy = 0.0f;
		cmf->yz = 0.0f;
		cmf->wz = 0.0f;
		cmf->xw = 0.0f;
		cmf->yw = 0.0f;
		cmf->zw = 0.0f;
		cmf->yy = 1.0f;
		cmf->ww = 1.0f;
		cmf->xx = cos;
		cmf->zz = cos;
		cmf->zx = -sin;
		cmf->xz = sin;
	}
}

void Matrix_RotateZ(f32 z, MtxMode mode) {
	Matrix* cmf;
	f32 sin;
	f32 cos;
	f32 temp1;
	f32 temp2;
	
	if (mode == MTXMODE_APPLY) {
		if (z != 0) {
			cmf = gCurrentMatrix;
			
			sin = sinf(z);
			cos = cosf(z);
			
			temp1 = cmf->xx;
			temp2 = cmf->xy;
			cmf->xx = temp1 * cos + temp2 * sin;
			cmf->xy = temp2 * cos - temp1 * sin;
			
			temp1 = cmf->yx;
			temp2 = cmf->yy;
			cmf->yx = temp1 * cos + temp2 * sin;
			cmf->yy = temp2 * cos - temp1 * sin;
			
			temp1 = cmf->zx;
			temp2 = cmf->zy;
			cmf->zx = temp1 * cos + temp2 * sin;
			cmf->zy = temp2 * cos - temp1 * sin;
			
			temp1 = cmf->wx;
			temp2 = cmf->wy;
			cmf->wx = temp1 * cos + temp2 * sin;
			cmf->wy = temp2 * cos - temp1 * sin;
		}
	} else {
		cmf = gCurrentMatrix;
		
		if (z != 0) {
			sin = sinf(z);
			cos = cosf(z);
		} else {
			sin = 0.0f;
			cos = 1.0f;
		}
		
		cmf->zx = 0.0f;
		cmf->wx = 0.0f;
		cmf->zy = 0.0f;
		cmf->wy = 0.0f;
		cmf->xz = 0.0f;
		cmf->yz = 0.0f;
		cmf->wz = 0.0f;
		cmf->xw = 0.0f;
		cmf->yw = 0.0f;
		cmf->zw = 0.0f;
		cmf->zz = 1.0f;
		cmf->ww = 1.0f;
		cmf->xx = cos;
		cmf->yy = cos;
		cmf->yx = sin;
		cmf->xy = -sin;
	}
}

void Matrix_MtxFCopy(Matrix* dest, Matrix* src) {
	dest->xx = src->xx;
	dest->yx = src->yx;
	dest->zx = src->zx;
	dest->wx = src->wx;
	dest->xy = src->xy;
	dest->yy = src->yy;
	dest->zy = src->zy;
	dest->wy = src->wy;
	dest->xx = src->xx;
	dest->yx = src->yx;
	dest->zx = src->zx;
	dest->wx = src->wx;
	dest->xy = src->xy;
	dest->yy = src->yy;
	dest->zy = src->zy;
	dest->wy = src->wy;
	dest->xz = src->xz;
	dest->yz = src->yz;
	dest->zz = src->zz;
	dest->wz = src->wz;
	dest->xw = src->xw;
	dest->yw = src->yw;
	dest->zw = src->zw;
	dest->ww = src->ww;
	dest->xz = src->xz;
	dest->yz = src->yz;
	dest->zz = src->zz;
	dest->wz = src->wz;
	dest->xw = src->xw;
	dest->yw = src->yw;
	dest->zw = src->zw;
	dest->ww = src->ww;
}

void Matrix_ToMtxF(Matrix* mtx) {
	Matrix_MtxFCopy(mtx, gCurrentMatrix);
}

void Matrix_MtxToMtxF(MtxN64* src, Matrix* dest) {
	u16* m1 = (void*)((u8*)src);
	u16* m2 = (void*)((u8*)src + 0x20);
	
	dest->xx = ((m1[0] << 0x10) | m2[0]) * (1 / 65536.0f);
	dest->yx = ((m1[1] << 0x10) | m2[1]) * (1 / 65536.0f);
	dest->zx = ((m1[2] << 0x10) | m2[2]) * (1 / 65536.0f);
	dest->wx = ((m1[3] << 0x10) | m2[3]) * (1 / 65536.0f);
	dest->xy = ((m1[4] << 0x10) | m2[4]) * (1 / 65536.0f);
	dest->yy = ((m1[5] << 0x10) | m2[5]) * (1 / 65536.0f);
	dest->zy = ((m1[6] << 0x10) | m2[6]) * (1 / 65536.0f);
	dest->wy = ((m1[7] << 0x10) | m2[7]) * (1 / 65536.0f);
	dest->xz = ((m1[8] << 0x10) | m2[8]) * (1 / 65536.0f);
	dest->yz = ((m1[9] << 0x10) | m2[9]) * (1 / 65536.0f);
	dest->zz = ((m1[10] << 0x10) | m2[10]) * (1 / 65536.0f);
	dest->wz = ((m1[11] << 0x10) | m2[11]) * (1 / 65536.0f);
	dest->xw = ((m1[12] << 0x10) | m2[12]) * (1 / 65536.0f);
	dest->yw = ((m1[13] << 0x10) | m2[13]) * (1 / 65536.0f);
	dest->zw = ((m1[14] << 0x10) | m2[14]) * (1 / 65536.0f);
	dest->ww = ((m1[15] << 0x10) | m2[15]) * (1 / 65536.0f);
}

MtxN64* Matrix_MtxFToMtx(Matrix* src, MtxN64* dest) {
	mat44_to_matn64((void*)dest, (void*)src);
	return dest;
	s32 temp;
	u16* m1 = (void*)((u8*)dest);
	u16* m2 = (void*)((u8*)dest + 0x20);
	
	temp = src->xx * 0x10000;
	m1[0] = (temp >> 0x10);
	m1[16 + 0] = temp & 0xFFFF;
	
	temp = src->yx * 0x10000;
	m1[1] = (temp >> 0x10);
	m1[16 + 1] = temp & 0xFFFF;
	
	temp = src->zx * 0x10000;
	m1[2] = (temp >> 0x10);
	m1[16 + 2] = temp & 0xFFFF;
	
	temp = src->wx * 0x10000;
	m1[3] = (temp >> 0x10);
	m1[16 + 3] = temp & 0xFFFF;
	
	temp = src->xy * 0x10000;
	m1[4] = (temp >> 0x10);
	m1[16 + 4] = temp & 0xFFFF;
	
	temp = src->yy * 0x10000;
	m1[5] = (temp >> 0x10);
	m1[16 + 5] = temp & 0xFFFF;
	
	temp = src->zy * 0x10000;
	m1[6] = (temp >> 0x10);
	m1[16 + 6] = temp & 0xFFFF;
	
	temp = src->wy * 0x10000;
	m1[7] = (temp >> 0x10);
	m1[16 + 7] = temp & 0xFFFF;
	
	temp = src->xz * 0x10000;
	m1[8] = (temp >> 0x10);
	m1[16 + 8] = temp & 0xFFFF;
	
	temp = src->yz * 0x10000;
	m1[9] = (temp >> 0x10);
	m2[9] = temp & 0xFFFF;
	
	temp = src->zz * 0x10000;
	m1[10] = (temp >> 0x10);
	m2[10] = temp & 0xFFFF;
	
	temp = src->wz * 0x10000;
	m1[11] = (temp >> 0x10);
	m2[11] = temp & 0xFFFF;
	
	temp = src->xw * 0x10000;
	m1[12] = (temp >> 0x10);
	m2[12] = temp & 0xFFFF;
	
	temp = src->yw * 0x10000;
	m1[13] = (temp >> 0x10);
	m2[13] = temp & 0xFFFF;
	
	temp = src->zw * 0x10000;
	m1[14] = (temp >> 0x10);
	m2[14] = temp & 0xFFFF;
	
	temp = src->ww * 0x10000;
	m1[15] = (temp >> 0x10);
	m2[15] = temp & 0xFFFF;
	
	return dest;
}

MtxN64* Matrix_ToMtx(MtxN64* dest) {
	mat44_to_matn64((void*)dest, (void*)gCurrentMatrix);
	return dest;
	
	// old code
	return Matrix_MtxFToMtx(gCurrentMatrix, dest);
}

MtxN64* Matrix_NewMtxN64() {
	return Matrix_ToMtx(n64_graph_alloc(sizeof(MtxN64))); // previously x_alloc
}

/**
 * Changes the 3x3 part of the current matrix to `mf` * S, where S is the scale in the current matrix.
 *
 * In details, S is a diagonal where each coefficient is the norm of the column in the 3x3 current matrix.
 * The 3x3 part can then be written as R * S where R has its columns normalized.
 * Since R is typically a rotation matrix, and the 3x3 part is changed from R * S to `mf` * S, this operation can be
 * seen as replacing the R rotation with `mf`, hence the function name.
 */
void Matrix_ReplaceRotation(Matrix* mf) {
	Matrix* cmf = gCurrentMatrix;
	f32 acc;
	f32 temp;
	f32 curColNorm;
	
	// compute the Euclidean norm of the first column of the current matrix
	acc = cmf->xx;
	acc *= acc;
	temp = cmf->yx;
	acc += SQ(temp);
	temp = cmf->zx;
	acc += SQ(temp);
	curColNorm = sqrtf(acc);
	
	cmf->xx = mf->xx * curColNorm;
	cmf->yx = mf->yx * curColNorm;
	cmf->zx = mf->zx * curColNorm;
	
	// second column
	acc = cmf->xy;
	acc *= acc;
	temp = cmf->yy;
	acc += SQ(temp);
	temp = cmf->zy;
	acc += SQ(temp);
	curColNorm = sqrtf(acc);
	
	cmf->xy = mf->xy * curColNorm;
	cmf->yy = mf->yy * curColNorm;
	cmf->zy = mf->zy * curColNorm;
	
	// third column
	acc = cmf->xz;
	acc *= acc;
	temp = cmf->yz;
	acc += SQ(temp);
	temp = cmf->zz;
	acc += SQ(temp);
	curColNorm = sqrtf(acc);
	
	cmf->xz = mf->xz * curColNorm;
	cmf->yz = mf->yz * curColNorm;
	cmf->zz = mf->zz * curColNorm;
}

void Matrix_MtxFMtxFMult(Matrix* mfA, Matrix* mfB, Matrix* dest) {
	f32 cx;
	f32 cy;
	f32 cz;
	f32 cw;
	//---ROW1---
	f32 rx = mfA->xx;
	f32 ry = mfA->xy;
	f32 rz = mfA->xz;
	f32 rw = mfA->xw;
	
	//--------
	
	cx = mfB->xx;
	cy = mfB->yx;
	cz = mfB->zx;
	cw = mfB->wx;
	dest->xx = (rx * cx) + (ry * cy) + (rz * cz) + (rw * cw);
	
	cx = mfB->xy;
	cy = mfB->yy;
	cz = mfB->zy;
	cw = mfB->wy;
	dest->xy = (rx * cx) + (ry * cy) + (rz * cz) + (rw * cw);
	
	cx = mfB->xz;
	cy = mfB->yz;
	cz = mfB->zz;
	cw = mfB->wz;
	dest->xz = (rx * cx) + (ry * cy) + (rz * cz) + (rw * cw);
	
	cx = mfB->xw;
	cy = mfB->yw;
	cz = mfB->zw;
	cw = mfB->ww;
	dest->xw = (rx * cx) + (ry * cy) + (rz * cz) + (rw * cw);
	
	//---ROW2---
	rx = mfA->yx;
	ry = mfA->yy;
	rz = mfA->yz;
	rw = mfA->yw;
	//--------
	cx = mfB->xx;
	cy = mfB->yx;
	cz = mfB->zx;
	cw = mfB->wx;
	dest->yx = (rx * cx) + (ry * cy) + (rz * cz) + (rw * cw);
	
	cx = mfB->xy;
	cy = mfB->yy;
	cz = mfB->zy;
	cw = mfB->wy;
	dest->yy = (rx * cx) + (ry * cy) + (rz * cz) + (rw * cw);
	
	cx = mfB->xz;
	cy = mfB->yz;
	cz = mfB->zz;
	cw = mfB->wz;
	dest->yz = (rx * cx) + (ry * cy) + (rz * cz) + (rw * cw);
	
	cx = mfB->xw;
	cy = mfB->yw;
	cz = mfB->zw;
	cw = mfB->ww;
	dest->yw = (rx * cx) + (ry * cy) + (rz * cz) + (rw * cw);
	
	//---ROW3---
	rx = mfA->zx;
	ry = mfA->zy;
	rz = mfA->zz;
	rw = mfA->zw;
	//--------
	cx = mfB->xx;
	cy = mfB->yx;
	cz = mfB->zx;
	cw = mfB->wx;
	dest->zx = (rx * cx) + (ry * cy) + (rz * cz) + (rw * cw);
	
	cx = mfB->xy;
	cy = mfB->yy;
	cz = mfB->zy;
	cw = mfB->wy;
	dest->zy = (rx * cx) + (ry * cy) + (rz * cz) + (rw * cw);
	
	cx = mfB->xz;
	cy = mfB->yz;
	cz = mfB->zz;
	cw = mfB->wz;
	dest->zz = (rx * cx) + (ry * cy) + (rz * cz) + (rw * cw);
	
	cx = mfB->xw;
	cy = mfB->yw;
	cz = mfB->zw;
	cw = mfB->ww;
	dest->zw = (rx * cx) + (ry * cy) + (rz * cz) + (rw * cw);
	
	//---ROW4---
	rx = mfA->wx;
	ry = mfA->wy;
	rz = mfA->wz;
	rw = mfA->ww;
	//--------
	cx = mfB->xx;
	cy = mfB->yx;
	cz = mfB->zx;
	cw = mfB->wx;
	dest->wx = (rx * cx) + (ry * cy) + (rz * cz) + (rw * cw);
	
	cx = mfB->xy;
	cy = mfB->yy;
	cz = mfB->zy;
	cw = mfB->wy;
	dest->wy = (rx * cx) + (ry * cy) + (rz * cz) + (rw * cw);
	
	cx = mfB->xz;
	cy = mfB->yz;
	cz = mfB->zz;
	cw = mfB->wz;
	dest->wz = (rx * cx) + (ry * cy) + (rz * cz) + (rw * cw);
	
	cx = mfB->xw;
	cy = mfB->yw;
	cz = mfB->zw;
	cw = mfB->ww;
	dest->ww = (rx * cx) + (ry * cy) + (rz * cz) + (rw * cw);
}

void Matrix_Projection(Matrix* mtx, f32 fovy, f32 aspect, f32 near, f32 far, f32 scale) {
	f32 yscale;
	s32 row;
	s32 col;
	
	Matrix_Clear(mtx);
	
	fovy *= M_PI / 180.0;
	yscale = cosf(fovy / 2) / sinf(fovy / 2);
	mtx->mf[0][0] = yscale;
	mtx->mf[1][1] = yscale * aspect;
	mtx->mf[2][2] = (near + far) / (near - far);
	mtx->mf[2][3] = -1;
	mtx->mf[3][2] = 2 * near * far / (near - far);
	mtx->mf[3][3] = 0.0f;
	
	for (row = 0; row < 4; row++) {
		for (col = 0; col < 4; col++) {
			mtx->mf[row][col] *= scale;
		}
	}
}

static void Matrix_OrthoImpl(Matrix* mtx, f32 l, f32 r, f32 t, f32 b, f32 near, f32 far) {
	f32 rl = r - l;
	f32 tb = t - b;
	f32 fn = far - near;
	
	Matrix_Clear(mtx);
	
	mtx->xx = 2.0f / rl;
	mtx->yy = 2.0f / tb;
	mtx->zz = -2.0f / fn;
	
	mtx->xw = -(l + r) / rl;
	mtx->yw = -(t + b) / tb;
	mtx->zw = -(far + near) / fn;
	mtx->ww = 1.0f;
}

void Matrix_Ortho(Matrix* mtx, f32 fovy, f32 aspect, f32 near, f32 far) {
	f32 t = fovy / 2.0f;
	f32 r = t * aspect;
	
	Matrix_OrthoImpl(mtx, -r, r, t, -t, near, far);
}

void Matrix_LookAt(Matrix* mf, Vec3f eye, Vec3f at, Vec3f up) {
	f32 length;
	Vec3f look;
	Vec3f right;
	
	Matrix_Clear(mf);
	
	look.x = at.x - eye.x;
	look.y = at.y - eye.y;
	look.z = at.z - eye.z;
	length = -1.0 / sqrtf(SQ(look.x) + SQ(look.y) + SQ(look.z));
	look.x *= length;
	look.y *= length;
	look.z *= length;
	
	right.x = up.y * look.z - up.z * look.y;
	right.y = up.z * look.x - up.x * look.z;
	right.z = up.x * look.y - up.y * look.x;
	length = 1.0 / sqrtf(SQ(right.x) + SQ(right.y) + SQ(right.z));
	right.x *= length;
	right.y *= length;
	right.z *= length;
	
	up.x = look.y * right.z - look.z * right.y;
	up.y = look.z * right.x - look.x * right.z;
	up.z = look.x * right.y - look.y * right.x;
	length = 1.0 / sqrtf(SQ(up.x) + SQ(up.y) + SQ(up.z));
	up.x *= length;
	up.y *= length;
	up.z *= length;
	
	mf->mf[0][0] = right.x;
	mf->mf[1][0] = right.y;
	mf->mf[2][0] = right.z;
	mf->mf[3][0] = -(eye.x * right.x + eye.y * right.y + eye.z * right.z);
	
	mf->mf[0][1] = up.x;
	mf->mf[1][1] = up.y;
	mf->mf[2][1] = up.z;
	mf->mf[3][1] = -(eye.x * up.x + eye.y * up.y + eye.z * up.z);
	
	mf->mf[0][2] = look.x;
	mf->mf[1][2] = look.y;
	mf->mf[2][2] = look.z;
	mf->mf[3][2] = -(eye.x * look.x + eye.y * look.y + eye.z * look.z);
	
	mf->mf[0][3] = 0;
	mf->mf[1][3] = 0;
	mf->mf[2][3] = 0;
	mf->mf[3][3] = 1;
}

void Matrix_MtxFTranslateRotateZYX(Matrix* cmf, Vec3f* translation, Vec3s* rotation) {
	f32 sin = SinS(rotation->z);
	f32 cos = CosS(rotation->z);
	f32 temp1;
	f32 temp2;
	
	temp1 = cmf->xx;
	temp2 = cmf->xy;
	cmf->xw += temp1 * translation->x + temp2 * translation->y + cmf->xz * translation->z;
	cmf->xx = temp1 * cos + temp2 * sin;
	cmf->xy = temp2 * cos - temp1 * sin;
	
	temp1 = cmf->yx;
	temp2 = cmf->yy;
	cmf->yw += temp1 * translation->x + temp2 * translation->y + cmf->yz * translation->z;
	cmf->yx = temp1 * cos + temp2 * sin;
	cmf->yy = temp2 * cos - temp1 * sin;
	
	temp1 = cmf->zx;
	temp2 = cmf->zy;
	cmf->zw += temp1 * translation->x + temp2 * translation->y + cmf->zz * translation->z;
	cmf->zx = temp1 * cos + temp2 * sin;
	cmf->zy = temp2 * cos - temp1 * sin;
	
	temp1 = cmf->wx;
	temp2 = cmf->wy;
	cmf->ww += temp1 * translation->x + temp2 * translation->y + cmf->wz * translation->z;
	cmf->wx = temp1 * cos + temp2 * sin;
	cmf->wy = temp2 * cos - temp1 * sin;
	
	if (rotation->y != 0) {
		sin = SinS(rotation->y);
		cos = CosS(rotation->y);
		
		temp1 = cmf->xx;
		temp2 = cmf->xz;
		cmf->xx = temp1 * cos - temp2 * sin;
		cmf->xz = temp1 * sin + temp2 * cos;
		
		temp1 = cmf->yx;
		temp2 = cmf->yz;
		cmf->yx = temp1 * cos - temp2 * sin;
		cmf->yz = temp1 * sin + temp2 * cos;
		
		temp1 = cmf->zx;
		temp2 = cmf->zz;
		cmf->zx = temp1 * cos - temp2 * sin;
		cmf->zz = temp1 * sin + temp2 * cos;
		
		temp1 = cmf->wx;
		temp2 = cmf->wz;
		cmf->wx = temp1 * cos - temp2 * sin;
		cmf->wz = temp1 * sin + temp2 * cos;
	}
	
	if (rotation->x != 0) {
		sin = SinS(rotation->x);
		cos = CosS(rotation->x);
		
		temp1 = cmf->xy;
		temp2 = cmf->xz;
		cmf->xy = temp1 * cos + temp2 * sin;
		cmf->xz = temp2 * cos - temp1 * sin;
		
		temp1 = cmf->yy;
		temp2 = cmf->yz;
		cmf->yy = temp1 * cos + temp2 * sin;
		cmf->yz = temp2 * cos - temp1 * sin;
		
		temp1 = cmf->zy;
		temp2 = cmf->zz;
		cmf->zy = temp1 * cos + temp2 * sin;
		cmf->zz = temp2 * cos - temp1 * sin;
		
		temp1 = cmf->wy;
		temp2 = cmf->wz;
		cmf->wy = temp1 * cos + temp2 * sin;
		cmf->wz = temp2 * cos - temp1 * sin;
	}
}

void Matrix_TranslateRotateZYX(Vec3f* translation, Vec3s* rotation) {
	Matrix_MtxFTranslateRotateZYX(gCurrentMatrix, translation, rotation);
}

void Matrix_RotateAToB(Vec3f* a, Vec3f* b, u8 mode) {
	Matrix mtx;
	Vec3f v;
	f32 frac;
	f32 c;
	Vec3f aN;
	Vec3f bN;
	
	// preliminary calculations
	aN = Vec3f_Normalize(*a);
	bN = Vec3f_Normalize(*b);
	c = Vec3f_Dot(aN, bN);
	
	if (fabsf(1.0f + c) < FLT_EPSILON) {
		// The vectors are parallel and opposite. The transformation does not work for
		// this case, but simply inverting scale is sufficient in this situation.
		f32 d = Vec3f_DistXYZ(aN, bN);
		
		if (d > 1.0f)
			Matrix_Scale(1.0f, 1.0f, 1.0f, mode);
		else
			Matrix_Scale(-1.0f, -1.0f, -1.0f, mode);
	} else {
		frac = 1.0f / (1.0f + c);
		v = Vec3f_Cross(aN, bN);
		
		// fill mtx
		mtx.xx = 1.0f - frac * (SQ(v.y) + SQ(v.z));
		mtx.xy = v.z + frac * v.x * v.y;
		mtx.xz = -v.y + frac * v.x * v.z;
		mtx.xw = 0.0f;
		mtx.yx = -v.z + frac * v.x * v.y;
		mtx.yy = 1.0f - frac * (SQ(v.x) + SQ(v.z));
		mtx.yz = v.x + frac * v.y * v.z;
		mtx.yw = 0.0f;
		mtx.zx = v.y + frac * v.x * v.z;
		mtx.zy = -v.x + frac * v.y * v.z;
		mtx.zz = 1.0f - frac * (SQ(v.y) + SQ(v.x));
		mtx.zw = 0.0f;
		mtx.wx = 0.0f;
		mtx.wy = 0.0f;
		mtx.wz = 0.0f;
		mtx.ww = 1.0f;
		
		// apply to stack
		Matrix_Mult(&mtx, mode);
	}
}

void Matrix_MultVec4f_Ext(Vec4f* src, Vec4f* dest, Matrix* mtx) {
	dest->x = mtx->xx * src->x + mtx->xy * src->y + mtx->xz * src->z + mtx->xw * src->w;
	dest->y = mtx->yx * src->x + mtx->yy * src->y + mtx->yz * src->z + mtx->yw * src->w;
	dest->z = mtx->zx * src->x + mtx->zy * src->y + mtx->zz * src->z + mtx->zw * src->w;
	dest->w = mtx->wx * src->x + mtx->wy * src->y + mtx->wz * src->z + mtx->ww * src->w;
}

void Matrix_MtxFToYXZRotS(Vec3s* rotDest, s32 flag) {
	f32 temp;
	f32 temp2;
	f32 temp3;
	Matrix* mf = gCurrentMatrix;
	
	temp = mf->xz;
	temp *= temp;
	temp += SQ(mf->zz);
	
	rotDest->x = RadToBin(atan2f(-mf->yz, sqrtf(temp)));
	
	if ((rotDest->x == 0x4000) || (rotDest->x == -0x4000)) {
		rotDest->z = 0;
		
		rotDest->y = RadToBin(atan2f(-mf->zx, mf->xx));
	} else {
		rotDest->y = RadToBin(atan2f(mf->xz, mf->zz));
		
		if (!flag) {
			rotDest->z = RadToBin(atan2f(mf->yx, mf->yy));
		} else {
			temp = mf->xx;
			temp2 = mf->zx;
			temp3 = mf->zy;
			
			temp *= temp;
			temp += SQ(temp2);
			temp2 = mf->yx;
			temp += SQ(temp2);
			/* temp = xx^2+zx^2+yx^2 == 1 for a rotation matrix */
			temp = sqrtf(temp);
			temp = temp2 / temp;
			
			temp2 = mf->xy;
			temp2 *= temp2;
			temp2 += SQ(temp3);
			temp3 = mf->yy;
			temp2 += SQ(temp3);
			/* temp2 = xy^2+zy^2+yy^2 == 1 for a rotation matrix */
			temp2 = sqrtf(temp2);
			temp2 = temp3 / temp2;
			
			/* for a rotation matrix, temp == yx and temp2 == yy
			 * which is the same as in the !flag branch */
			rotDest->z = RadToBin(atan2f(temp, temp2));
		}
	}
}

static int glhProjectf(float objx, float objy, float objz, float* modelview, float* projection, int* viewport, float* windowCoordinate) {
	// Transformation vectors
	float fTempo[8];
	
	// Modelview transform
	fTempo[0] = modelview[0] * objx + modelview[4] * objy + modelview[8] * objz + modelview[12]; // w is always 1
	fTempo[1] = modelview[1] * objx + modelview[5] * objy + modelview[9] * objz + modelview[13];
	fTempo[2] = modelview[2] * objx + modelview[6] * objy + modelview[10] * objz + modelview[14];
	fTempo[3] = modelview[3] * objx + modelview[7] * objy + modelview[11] * objz + modelview[15];
	// Projection transform, the final row of projection matrix is always [0 0 -1 0]
	// so we optimize for that.
	fTempo[4] = projection[0] * fTempo[0] + projection[4] * fTempo[1] + projection[8] * fTempo[2] + projection[12] * fTempo[3];
	fTempo[5] = projection[1] * fTempo[0] + projection[5] * fTempo[1] + projection[9] * fTempo[2] + projection[13] * fTempo[3];
	fTempo[6] = projection[2] * fTempo[0] + projection[6] * fTempo[1] + projection[10] * fTempo[2] + projection[14] * fTempo[3];
	fTempo[7] = -fTempo[2];
	// The result normalizes between -1 and 1
	if (fTempo[7]==0.0)        // The w value
		return 0;
	fTempo[7] = 1.0 / fTempo[7];
	// Perspective division
	fTempo[4] *= fTempo[7];
	fTempo[5] *= fTempo[7];
	fTempo[6] *= fTempo[7];
	// window coordinates
	// Map x, y to range 0-1
	windowCoordinate[0] = (fTempo[4] * 0.5 + 0.5) * viewport[2] + viewport[0];
	windowCoordinate[1] = (fTempo[5] * 0.5 + 0.5) * viewport[3] + viewport[1];
	// This is only correct when glDepthRange(0.0, 1.0)
	windowCoordinate[2] = (1.0 + fTempo[6]) * 0.5; // Between 0 and 1
	
	return 1;
}

static void MultiplyMatrices4by4OpenGL_FLOAT(float* result, float* matrix1, float* matrix2) {
	result[0] = matrix1[0] * matrix2[0] + matrix1[4] * matrix2[1] + matrix1[8] * matrix2[2] + matrix1[12] * matrix2[3];
	result[1] = matrix1[1] * matrix2[0] + matrix1[5] * matrix2[1] + matrix1[9] * matrix2[2] + matrix1[13] * matrix2[3];
	result[2] = matrix1[2] * matrix2[0] + matrix1[6] * matrix2[1] + matrix1[10] * matrix2[2] + matrix1[14] * matrix2[3];
	result[3] = matrix1[3] * matrix2[0] + matrix1[7] * matrix2[1] + matrix1[11] * matrix2[2] + matrix1[15] * matrix2[3];
	result[4] = matrix1[0] * matrix2[4] + matrix1[4] * matrix2[5] + matrix1[8] * matrix2[6] + matrix1[12] * matrix2[7];
	result[5] = matrix1[1] * matrix2[4] + matrix1[5] * matrix2[5] + matrix1[9] * matrix2[6] + matrix1[13] * matrix2[7];
	result[6] = matrix1[2] * matrix2[4] + matrix1[6] * matrix2[5] + matrix1[10] * matrix2[6] + matrix1[14] * matrix2[7];
	result[7] = matrix1[3] * matrix2[4] + matrix1[7] * matrix2[5] + matrix1[11] * matrix2[6] + matrix1[15] * matrix2[7];
	result[8] = matrix1[0] * matrix2[8] + matrix1[4] * matrix2[9] + matrix1[8] * matrix2[10] + matrix1[12] * matrix2[11];
	result[9] = matrix1[1] * matrix2[8] + matrix1[5] * matrix2[9] + matrix1[9] * matrix2[10] + matrix1[13] * matrix2[11];
	result[10] = matrix1[2] * matrix2[8] + matrix1[6] * matrix2[9] + matrix1[10] * matrix2[10] + matrix1[14] * matrix2[11];
	result[11] = matrix1[3] * matrix2[8] + matrix1[7] * matrix2[9] + matrix1[11] * matrix2[10] + matrix1[15] * matrix2[11];
	result[12] = matrix1[0] * matrix2[12] + matrix1[4] * matrix2[13] + matrix1[8] * matrix2[14] + matrix1[12] * matrix2[15];
	result[13] = matrix1[1] * matrix2[12] + matrix1[5] * matrix2[13] + matrix1[9] * matrix2[14] + matrix1[13] * matrix2[15];
	result[14] = matrix1[2] * matrix2[12] + matrix1[6] * matrix2[13] + matrix1[10] * matrix2[14] + matrix1[14] * matrix2[15];
	result[15] = matrix1[3] * matrix2[12] + matrix1[7] * matrix2[13] + matrix1[11] * matrix2[14] + matrix1[15] * matrix2[15];
}

static void MultiplyMatrixByVector4by4OpenGL_FLOAT(float* resultvector, const float* matrix, const float* pvector) {
	resultvector[0] = matrix[0] * pvector[0] + matrix[4] * pvector[1] + matrix[8] * pvector[2] + matrix[12] * pvector[3];
	resultvector[1] = matrix[1] * pvector[0] + matrix[5] * pvector[1] + matrix[9] * pvector[2] + matrix[13] * pvector[3];
	resultvector[2] = matrix[2] * pvector[0] + matrix[6] * pvector[1] + matrix[10] * pvector[2] + matrix[14] * pvector[3];
	resultvector[3] = matrix[3] * pvector[0] + matrix[7] * pvector[1] + matrix[11] * pvector[2] + matrix[15] * pvector[3];
}

#define SWAP_ROWS_DOUBLE(a, b) do { double* _tmp = a; (a) = (b); (b) = _tmp; } while (0)
#define SWAP_ROWS_FLOAT(a, b)  do { float* _tmp = a; (a) = (b); (b) = _tmp; } while (0)
#define MAT(m, r, c)           (m)[(c) * 4 + (r)]

// This code comes directly from GLU except that it is for float
static int glhInvertMatrixf2(float* m, float* out) {
	float wtmp[4][8];
	float m0, m1, m2, m3, s;
	float* r0, * r1, * r2, * r3;
	
	r0 = wtmp[0], r1 = wtmp[1], r2 = wtmp[2], r3 = wtmp[3];
	r0[0] = MAT(m, 0, 0), r0[1] = MAT(m, 0, 1),
	r0[2] = MAT(m, 0, 2), r0[3] = MAT(m, 0, 3),
	r0[4] = 1.0, r0[5] = r0[6] = r0[7] = 0.0,
	r1[0] = MAT(m, 1, 0), r1[1] = MAT(m, 1, 1),
	r1[2] = MAT(m, 1, 2), r1[3] = MAT(m, 1, 3),
	r1[5] = 1.0, r1[4] = r1[6] = r1[7] = 0.0,
	r2[0] = MAT(m, 2, 0), r2[1] = MAT(m, 2, 1),
	r2[2] = MAT(m, 2, 2), r2[3] = MAT(m, 2, 3),
	r2[6] = 1.0, r2[4] = r2[5] = r2[7] = 0.0,
	r3[0] = MAT(m, 3, 0), r3[1] = MAT(m, 3, 1),
	r3[2] = MAT(m, 3, 2), r3[3] = MAT(m, 3, 3),
	r3[7] = 1.0, r3[4] = r3[5] = r3[6] = 0.0;
	/* choose pivot - or die */
	if (fabsf(r3[0]) > fabsf(r2[0]))
		SWAP_ROWS_FLOAT(r3, r2);
	if (fabsf(r2[0]) > fabsf(r1[0]))
		SWAP_ROWS_FLOAT(r2, r1);
	if (fabsf(r1[0]) > fabsf(r0[0]))
		SWAP_ROWS_FLOAT(r1, r0);
	if (0.0 == r0[0])
		return 0;
	/* eliminate first variable */
	m1 = r1[0] / r0[0];
	m2 = r2[0] / r0[0];
	m3 = r3[0] / r0[0];
	s = r0[1];
	r1[1] -= m1 * s;
	r2[1] -= m2 * s;
	r3[1] -= m3 * s;
	s = r0[2];
	r1[2] -= m1 * s;
	r2[2] -= m2 * s;
	r3[2] -= m3 * s;
	s = r0[3];
	r1[3] -= m1 * s;
	r2[3] -= m2 * s;
	r3[3] -= m3 * s;
	s = r0[4];
	if (s != 0.0) {
		r1[4] -= m1 * s;
		r2[4] -= m2 * s;
		r3[4] -= m3 * s;
	}
	s = r0[5];
	if (s != 0.0) {
		r1[5] -= m1 * s;
		r2[5] -= m2 * s;
		r3[5] -= m3 * s;
	}
	s = r0[6];
	if (s != 0.0) {
		r1[6] -= m1 * s;
		r2[6] -= m2 * s;
		r3[6] -= m3 * s;
	}
	s = r0[7];
	if (s != 0.0) {
		r1[7] -= m1 * s;
		r2[7] -= m2 * s;
		r3[7] -= m3 * s;
	}
	/* choose pivot - or die */
	if (fabsf(r3[1]) > fabsf(r2[1]))
		SWAP_ROWS_FLOAT(r3, r2);
	if (fabsf(r2[1]) > fabsf(r1[1]))
		SWAP_ROWS_FLOAT(r2, r1);
	if (0.0 == r1[1])
		return 0;
	/* eliminate second variable */
	m2 = r2[1] / r1[1];
	m3 = r3[1] / r1[1];
	r2[2] -= m2 * r1[2];
	r3[2] -= m3 * r1[2];
	r2[3] -= m2 * r1[3];
	r3[3] -= m3 * r1[3];
	s = r1[4];
	if (0.0 != s) {
		r2[4] -= m2 * s;
		r3[4] -= m3 * s;
	}
	s = r1[5];
	if (0.0 != s) {
		r2[5] -= m2 * s;
		r3[5] -= m3 * s;
	}
	s = r1[6];
	if (0.0 != s) {
		r2[6] -= m2 * s;
		r3[6] -= m3 * s;
	}
	s = r1[7];
	if (0.0 != s) {
		r2[7] -= m2 * s;
		r3[7] -= m3 * s;
	}
	/* choose pivot - or die */
	if (fabsf(r3[2]) > fabsf(r2[2]))
		SWAP_ROWS_FLOAT(r3, r2);
	if (0.0 == r2[2])
		return 0;
	/* eliminate third variable */
	m3 = r3[2] / r2[2];
	r3[3] -= m3 * r2[3], r3[4] -= m3 * r2[4],
	r3[5] -= m3 * r2[5], r3[6] -= m3 * r2[6], r3[7] -= m3 * r2[7];
	/* last check */
	if (0.0 == r3[3])
		return 0;
	s = 1.0 / r3[3];           /* now back substitute row 3 */
	r3[4] *= s;
	r3[5] *= s;
	r3[6] *= s;
	r3[7] *= s;
	m2 = r2[3];                /* now back substitute row 2 */
	s = 1.0 / r2[2];
	r2[4] = s * (r2[4] - r3[4] * m2), r2[5] = s * (r2[5] - r3[5] * m2),
	r2[6] = s * (r2[6] - r3[6] * m2), r2[7] = s * (r2[7] - r3[7] * m2);
	m1 = r1[3];
	r1[4] -= r3[4] * m1, r1[5] -= r3[5] * m1,
	r1[6] -= r3[6] * m1, r1[7] -= r3[7] * m1;
	m0 = r0[3];
	r0[4] -= r3[4] * m0, r0[5] -= r3[5] * m0,
	r0[6] -= r3[6] * m0, r0[7] -= r3[7] * m0;
	m1 = r1[2];                /* now back substitute row 1 */
	s = 1.0 / r1[1];
	r1[4] = s * (r1[4] - r2[4] * m1), r1[5] = s * (r1[5] - r2[5] * m1),
	r1[6] = s * (r1[6] - r2[6] * m1), r1[7] = s * (r1[7] - r2[7] * m1);
	m0 = r0[2];
	r0[4] -= r2[4] * m0, r0[5] -= r2[5] * m0,
	r0[6] -= r2[6] * m0, r0[7] -= r2[7] * m0;
	m0 = r0[1];                /* now back substitute row 0 */
	s = 1.0 / r0[0];
	r0[4] = s * (r0[4] - r1[4] * m0), r0[5] = s * (r0[5] - r1[5] * m0),
	r0[6] = s * (r0[6] - r1[6] * m0), r0[7] = s * (r0[7] - r1[7] * m0);
	MAT(out, 0, 0) = r0[4];
	MAT(out, 0, 1) = r0[5], MAT(out, 0, 2) = r0[6];
	MAT(out, 0, 3) = r0[7], MAT(out, 1, 0) = r1[4];
	MAT(out, 1, 1) = r1[5], MAT(out, 1, 2) = r1[6];
	MAT(out, 1, 3) = r1[7], MAT(out, 2, 0) = r2[4];
	MAT(out, 2, 1) = r2[5], MAT(out, 2, 2) = r2[6];
	MAT(out, 2, 3) = r2[7], MAT(out, 3, 0) = r3[4];
	MAT(out, 3, 1) = r3[5], MAT(out, 3, 2) = r3[6];
	MAT(out, 3, 3) = r3[7];
	
	return 1;
}

static int glhUnProjectf(float winx, float winy, float winz, float* modelview, float* projection, int* viewport, float* objectCoordinate) {
	// Transformation matrices
	float m[16], A[16];
	float in[4], out[4];
	
	// Calculation for inverting a matrix, compute projection x modelview
	// and store in A[16]
	MultiplyMatrices4by4OpenGL_FLOAT(A, projection, modelview);
	// Now compute the inverse of matrix A
	if (glhInvertMatrixf2(A, m)==0)
		return 0;
	// Transformation of normalized coordinates between -1 and 1
	in[0] = (winx - (float)viewport[0]) / (float)viewport[2] * 2.0 - 1.0;
	in[1] = (winy - (float)viewport[1]) / (float)viewport[3] * 2.0 - 1.0;
	in[2] = 2.0 * winz - 1.0;
	in[3] = 1.0;
	// Objects coordinates
	MultiplyMatrixByVector4by4OpenGL_FLOAT(out, m, in);
	if (out[3]==0.0)
		return 0;
	out[3] = 1.0 / out[3];
	objectCoordinate[0] = out[0] * out[3];
	objectCoordinate[1] = out[1] * out[3];
	objectCoordinate[2] = out[2] * out[3];
	
	return 1;
}

Matrix Matrix_Invert(Matrix* m) {
	float det = m->xx * (m->yy * m->zz - m->zy * m->yz) -
		m->yx * (m->xy * m->zz - m->zy * m->xz) +
		m->zx * (m->xy * m->yz - m->yy * m->xz);
	
	if (fabs(det) < 1e-6)
		perror("can't invert");
	assert(fabs(det) >= 1e-6);
	
	Matrix inv;
	inv.xx = (m->yy * m->zz - m->yz * m->zy) / det;
	inv.yx = (m->yx * m->zz - m->yz * m->zx) / det;
	inv.zx = (m->yx * m->zy - m->yy * m->zx) / det;
	inv.wx = 0;
	
	inv.xy = (m->xy * m->zz - m->xz * m->zy) / det;
	inv.yy = (m->xx * m->zz - m->xz * m->zx) / det;
	inv.zy = (m->xx * m->zy - m->xy * m->zx) / det;
	inv.wy = 0;
	
	inv.xz = (m->xy * m->yz - m->xz * m->yy) / det;
	inv.yz = (m->xx * m->yz - m->xz * m->yx) / det;
	inv.zz = (m->xx * m->yy - m->xy * m->yx) / det;
	inv.wz = 0;
	
	inv.xw = 0;
	inv.yw = 0;
	inv.zw = 0;
	inv.ww = 1;
	
	return inv;
}

void Matrix_Unproject(Matrix* modelViewMtx, Matrix* projMtx, Vec3f* src, Vec3f* dest, f32 w, f32 h) {
	s32 vp[] = {
		0, 0, w, h
	};
	
	src->y = h - src->y;
	glhUnProjectf(src->x, src->y, src->z, (float*)modelViewMtx, (float*)projMtx, vp, (float*)dest);
}

void Matrix_Project(Matrix* modelViewMtx, Matrix* projMtx, Vec3f* src, Vec3f* dest, f32 w, f32 h) {
	int vp[] = {
		0, 0, w, h
	};
	
	glhProjectf(src->x, src->y, src->z, (float*)modelViewMtx, (float*)projMtx, vp, (float*)dest);
	dest->y = h - dest->y;
}

#endif // endregion: matrix

#if 1 // region: collision

void TriBuffer_Alloc(TriBuffer* this, u32 num) {
	this->head = calloc(num, sizeof(Triangle));
	this->max = num;
	this->num = 0;
	
	assert(this->head != NULL);
}

void TriBuffer_Realloc(TriBuffer* this) {
	this->max *= 2;
	this->head = realloc(this->head, sizeof(Triangle) * this->max);
}

void TriBuffer_Free(TriBuffer* this) {
	free(this->head);
	memset(this, 0, sizeof(*this));
}

RayLine RayLine_New(Vec3f start, Vec3f end) {
	return (RayLine) { start, end, FLT_MAX };
}

float Col3D_SnapToFloor(Vec3f p, Vec3s *verts, int numVerts, Vec3s *tris, int numTris)
{
	float nearestY = -32000; // Collision.RaycastSnapToFloorFailed
	float nearestDelta = FLT_MAX;
	
	for (int i = 0; i < numTris; ++i, ++tris)
	{
		Vec3f a = { UnfoldVec3(verts[tris->x]) };
		Vec3f b = { UnfoldVec3(verts[tris->y]) };
		Vec3f c = { UnfoldVec3(verts[tris->z]) };
		
		// determine whether point is above triangle
		float d1, d2, d3;
		bool hasNegative, hasPositive;
		
		d1 = (p.x - b.x) * (a.z - b.z) - (a.x - b.x) * (p.z - b.z);
		d2 = (p.x - c.x) * (b.z - c.z) - (b.x - c.x) * (p.z - c.z);
		d3 = (p.x - a.x) * (c.z - a.z) - (c.x - a.x) * (p.z - a.z);
		
		hasNegative = (d1 < 0) || (d2 < 0) || (d3 < 0);
		hasPositive = (d1 > 0) || (d2 > 0) || (d3 > 0);
		
		if (hasNegative && hasPositive)
			continue;
		
		// determine which part of triangle is directly below point
		Vec3f ab = Vec3_Minus(b, a);
		Vec3f ac = Vec3_Minus(c, a);
		Vec3f normal = {
			ab.y * ac.z - ab.z * ac.y,
			ab.z * ac.x - ab.x * ac.z,
			ab.x * ac.y - ab.y * ac.x
		};
		// prevent division by 0, triangle is vertical anyway
		if (normal.y <= 0.00001 && normal.y >= -0.00001)
			continue;
		float D = -(normal.x * a.x + normal.y * a.y + normal.z * a.z);
		float thisY = -(normal.x * p.x + normal.z * p.z + D) / normal.y;
		float thisDelta = p.y - thisY;
		
		// triangle intersection is above point
		if (rintf(thisY) >= rintf(p.y))
			continue;
		
		// choose nearest
		if (thisDelta < nearestDelta)
		{
			nearestY = thisY;
			nearestDelta = thisDelta;
		}
	}
	
	return nearestY;
}

bool Col3D_LineVsTriangle(RayLine* ray, Triangle* tri, Vec3f* outPos, Vec3f* outNor, bool cullBackface, bool cullFrontface) {
	Vec3f vertex0 = tri->v[0];
	Vec3f vertex1 = tri->v[1];
	Vec3f vertex2 = tri->v[2];
	Vec3f edge1, edge2, h, s, q;
	f32 a, f, u, v;
	Vec3f dir = Vec3f_Normalize(Vec3f_Sub(ray->end, ray->start));
	f32 dist = Vec3f_DistXYZ(ray->start, ray->end);
	
	edge1 = Vec3f_Sub(vertex1, vertex0);
	edge2 = Vec3f_Sub(vertex2, vertex0);
	h = Vec3f_Cross(dir, edge2);
	a = Vec3f_Dot(edge1, h);
	if ((cullBackface && a < 0) || (cullFrontface && a > 0))
		return false;
	if (a > -FLT_EPSILON && a < FLT_EPSILON)
		return false;          // This ray is parallel to this triangle.
	f = 1.0 / a;
	s = Vec3f_Sub(ray->start, vertex0);
	u = f * Vec3f_Dot(s, h);
	if (u < 0.0 || u > 1.0)
		return false;
	q = Vec3f_Cross(s, edge1);
	v = f * Vec3f_Dot(dir, q);
	if (v < 0.0 || u + v > 1.0)
		return false;
	// At this stage we can compute t to find out where the intersection point is on the line.
	f32 t = f * Vec3f_Dot(edge2, q);
	
	if (t > FLT_EPSILON && t < dist && t < ray->nearest) { // ray intersection
		if (outPos)
			*outPos = Vec3f_Add(ray->start, Vec3f_MulVal(dir, t));
		if (outNor)
			*outNor = h;
		ray->nearest = t;
		
		return true;
	} else                     // This means that there is a line intersection but not a ray intersection.
		return false;
}

bool Col3D_LineVsTriBuffer(RayLine* ray, TriBuffer* triBuf, Vec3f* outPos, Vec3f* outNor) {
	Triangle* tri = triBuf->head;
	s32 r = 0;
	
	for (int i = 0; i < triBuf->num; i++, tri++) {
		if (Col3D_LineVsTriangle(ray, tri, outPos, outNor, tri->cullBackface, tri->cullFrontface))
			r = true;
	}
	
	return r;
}

bool Col3D_LineVsCylinder(RayLine* ray, Cylinder* cyl, Vec3f* outPos) {
	Vec3f rA, rB;
	Vec3f cA, cB;
	Vec3f cyln = Vec3f_LineSegDir(cyl->start, cyl->end);
	Vec3f up = { 0, 1, 0 };
	Vec3f ip;
	Vec3f o;
	
	Matrix_Push();
	
	Matrix_RotateAToB(&cyln, &up, MTXMODE_NEW);
	Matrix_MultVec3f(&ray->start, &rA);
	Matrix_MultVec3f(&ray->end, &rB);
	Matrix_MultVec3f(&cyl->start, &cA);
	Matrix_MultVec3f(&cyl->end, &cB);
	
	Matrix_Pop();
	
	ip = Vec3f_ClosestPointOnRay(rA, rB, cA, cB);
	if (ip.y < fminf(cA.y, cB.y) || ip.y > fmaxf(cA.y, cB.y))
		return false;
	
	Sphere s = {
		cA, cyl->r
	};
	RayLine r = {
		rA, rB, ray->nearest
	};
	
	r.start.y = 0;
	r.end.y = 0;
	s.pos.y = 0;
	
	if (!Col3D_LineVsSphere(&r, &s, &o))
		return false;
	
	ray->nearest = r.nearest;
	if (outPos) {
		Vec3f out;
		
		out = o;
		out.y = ip.y;
		
		cyln = Vec3f_Invert(cyln);
		Matrix_Push();
		Matrix_RotateAToB(&cyln, &up, MTXMODE_NEW);
		Matrix_MultVec3f(&out, outPos);
		Matrix_Pop();
	}
	
	return true;
}

bool Col3D_LineVsSphere(RayLine* ray, Sphere* sph, Vec3f* outPos) {
	Vec3f dir = Vec3f_LineSegDir(ray->start, ray->end);
	f32 rayCylLen = Vec3f_DistXYZ(ray->start, sph->pos);
	Vec3f npos = Vec3f_Add(ray->start, Vec3f_MulVal(dir, rayCylLen));
	f32 dist = Vec3f_DistXYZ(sph->pos, npos);
	
	if (dist < sph->r && rayCylLen < ray->nearest) {
		ray->nearest = rayCylLen;
		if (outPos) {
			Vec3f nb = Vec3f_LineSegDir(sph->pos, npos);
			
			*outPos = Vec3f_Add(sph->pos, Vec3f_MulVal(nb, dist));
		}
		
		return true;
	}
	
	return false;
}

#endif // endregion: collision
