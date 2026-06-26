#pragma once

/** Limits for light/aging gate (GateSpec.ini): sensor fps, current mA, sensor temp */
struct GateChannelLimits
{
	double minSsrFps;
	double maxSsrFps;
	double minCurrent_mA;
	double maxCurrent_mA;
	double minSensorTemp_C;
	double maxSensorTemp_C;
};
