#pragma once
#include <QDebug>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

inline constexpr float degreesToRadians(float degrees)
{
	return degrees * static_cast<float>(M_PI) / 180.0f;
}

inline constexpr float radiansToDegrees(float radians)
{
	return radians / static_cast<float>(M_PI) * 180.0f;
}

#define check(cmd)							        \
{ 											        \
	bool status = cmd; 						        \
	if (!status) { 							        \
		qDebug() << "ERROR: " << (#cmd) << '\n';    \
	} 										        \
}
