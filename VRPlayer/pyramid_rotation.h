#pragma once
#ifndef PYRAMID_ROT
#define PYRAMID_ROT
#include <iostream>
#define _USE_MATH_DEFINES
#include <math.h>
#include <stdint.h>
#include "ViewPointTable.h"
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif // !M_PI
//#define DISABLE_ROTATE
typedef float M3DMatrix44f[16];

struct polarCoord
{
	float phi;
	float theta;
};
class pyramid_rotation
{
public:

	polarCoord userViewpoint;
	polarCoord frameViewpoint;
	uint32_t viewPointIndex;

public:
	pyramid_rotation();
	void updateViewpoint(float x, float y, float z);
	void updateViewPointIndex(uint32_t index);
	void getRotationMatrix(M3DMatrix44f m);
	uint32_t getFrameIndex();
};
#endif