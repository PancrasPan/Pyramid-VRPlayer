#include "pyramid_rotation.h"


pyramid_rotation::pyramid_rotation()
{
	userViewpoint.phi = 0;
	userViewpoint.theta = 0;
	frameViewpoint.phi = 90;
	frameViewpoint.theta = 0;
	viewPointIndex = 9;
}

void pyramid_rotation::updateViewpoint(float x, float y, float z)
{
	//calculate user viewpoint
	userViewpoint.theta = atan2(x, z) + M_PI;//look at -z at first
	userViewpoint.phi = atan2(sqrt(x*x + z*z), y);
	//printf("%f %f \n", userViewpoint.theta*180/M_PI, userViewpoint.phi*180/M_PI);
	//search frame central viewpoint
	uint32_t latitude = userViewpoint.phi * 180 / M_PI;
	uint32_t longtitude = userViewpoint.theta * 180 / M_PI;
	//printf("%d %d \n", latitude, longtitude);

	uint32_t index_y = ((latitude / 15) + 1) >> 1;
	uint32_t index_x = longtitude % 360 / 15;

	//update viewpoint index
	viewPointIndex = frameViewpointIndex[index_y][index_x];
#ifdef DISABLE_ROTATE
	viewPointIndex = 19;
#endif
	frameViewpoint.phi = (90 - frameViewPointTable[viewPointIndex][0]) * M_PI / 180;
	frameViewpoint.theta = frameViewPointTable[viewPointIndex][1] * M_PI / 180;
	//printf("%d %d %d %f %f\n", viewPointIndex, frameViewPointTable[viewPointIndex][0], frameViewPointTable[viewPointIndex][1],frameViewpoint.phi, frameViewpoint.theta);

}

void pyramid_rotation::updateViewPointIndex(uint32_t index)
{
	viewPointIndex = index;
	frameViewpoint.phi = (90 - frameViewPointTable[viewPointIndex][0]) * M_PI / 180;
	frameViewpoint.theta = frameViewPointTable[viewPointIndex][1] * M_PI / 180;
}

void pyramid_rotation::getRotationMatrix(M3DMatrix44f m)
{
	float sin_phi = sin(frameViewpoint.phi);
	float cos_phi = cos(frameViewpoint.phi);
	float sin_theta = sin(frameViewpoint.theta);
	float cos_theta = cos(frameViewpoint.theta);

#define M(row,col)  m[col*4+row]
	M(0, 0) = cos_theta;
	M(0, 1) = sin_theta*sin_phi;
	M(0, 2) = sin_theta*cos_phi;
	M(0, 3) = 0.0;
	M(1, 0) = 0.0;
	M(1, 1) = cos_phi;
	M(1, 2) = -sin_phi;
	M(1, 3) = 0.0;
	M(2, 0) = -sin_theta;
	M(2, 1) = cos_theta*sin_phi;
	M(2, 2) = cos_theta*cos_phi;
	M(2, 3) = 0.0;
	M(3, 0) = 0.0;
	M(3, 1) = 0.0;
	M(3, 2) = 0.0;
	M(3, 3) = 1.0;
#undef M

}

uint32_t pyramid_rotation::getFrameIndex()
{
	return uint32_t(this->viewPointIndex);
}

