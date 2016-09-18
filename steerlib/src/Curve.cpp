//
// Copyright (c) 2009-2015 Glen Berseth, Mubbasir Kapadia, Shawn Singh, Petros Faloutsos, Glenn Reinman
// See license.txt for complete license.
// Copyright (c) 2015 Mahyar Khayatkhoei
//

#include <algorithm>
#include <vector>
#include <util/Geometry.h>
#include <util/Curve.h>
#include <util/Color.h>
#include <util/DrawLib.h>
#include "Globals.h"

using namespace Util;


// Function declarations 
bool sortControlPointsByTime(const CurvePoint& p1, const CurvePoint& p2);

Curve::Curve(const CurvePoint& startPoint, int curveType) : type(curveType)
{
	controlPoints.push_back(startPoint);
}

Curve::Curve(const std::vector<CurvePoint>& inputPoints, int curveType) : type(curveType)
{
	controlPoints = inputPoints;
	sortControlPoints();
}

// Add one control point to the vector controlPoints
void Curve::addControlPoint(const CurvePoint& inputPoint)
{
	controlPoints.push_back(inputPoint);
	sortControlPoints();
}

// Add a vector of control points to the vector controlPoints
void Curve::addControlPoints(const std::vector<CurvePoint>& inputPoints)
{
	for (int i = 0; i < inputPoints.size(); i++)
		controlPoints.push_back(inputPoints[i]);
	sortControlPoints();
}

// Draw the curve shape on screen, usign window as step size (bigger window: less accurate shape)
void Curve::drawCurve(Color curveColor, float curveThickness, int window)
{
#ifdef ENABLE_GUI
	// Move on the curve from t=0 to t=finalPoint, using window as step size, and linearly interpolate the curve points
	// Note that you must draw the whole curve at each frame, that means connecting line segments between each two points on the curve
	

	// Draw nothing if there are less than two control points
	if (!checkRobust()) { return; }

	Point startPoint, nextPoint;
	for (int i = 0; i < controlPoints.size(); ++i) 
	{		
		if (i == controlPoints.size() - 1) // Draw the from current position to last point -- NOT WORKING PROPERLY
		{
			DrawLib::glColor(curveColor);
			DrawLib::drawLine(startPoint, controlPoints[i].position, curveColor, curveThickness);
			break;
		}

		startPoint = controlPoints[i].position;		

		for (int j = (controlPoints[i]).time; j <= (controlPoints[i + 1]).time; j += window)   // Step through controlPoints by using window as time
		{
			calculatePoint(nextPoint, j);
			DrawLib::glColor(curveColor);
			DrawLib::drawLine(startPoint, nextPoint, curveColor, curveThickness);

			startPoint = nextPoint;
		}
	}
		
#endif
}

// Sort controlPoints vector in ascending order: min-first
void Curve::sortControlPoints()
{
	std::sort(controlPoints.begin(), controlPoints.end(), sortControlPointsByTime);
}

bool sortControlPointsByTime(const CurvePoint& p1, const CurvePoint& p2)
{
	return p1.time < p2.time;
}

// Calculate the position on curve corresponding to the given time, outputPoint is the resulting position
// Note that this function should return false if the end of the curve is reached, or no next point can be found
bool Curve::calculatePoint(Point& outputPoint, float time)
{
	// Robustness: make sure there is at least two control point: start and end points
	if (!checkRobust())
		return false;

	// Define temporary parameters for calculation
	unsigned int nextPoint;

	// Find the current interval in time, supposing that controlPoints is sorted (sorting is done whenever control points are added)
	// Note that nextPoint is an integer containing the index of the next control point
	if (!findTimeInterval(nextPoint, time))
		return false;

	// Calculate position at t = time on curve given the next control point (nextPoint)
	if (type == hermiteCurve)
	{
		outputPoint = useHermiteCurve(nextPoint, time);
	}
	else if (type == catmullCurve)
	{
		outputPoint = useCatmullCurve(nextPoint, time);
	}

	// Return
	return true;
}

// Check Roboustness
bool Curve::checkRobust()
{
	if (controlPoints.size() < 2) { return false; }     // Curve needs at least two points

	else return true;
}

// Find the current time interval (i.e. index of the next control point to follow according to current time)
bool Curve::findTimeInterval(unsigned int& nextPoint, float time)
{
	// if time is greater than the last control point's time then we've reached past the end
	if (time > controlPoints.back().time) 
	{
		return false;
	}

	// Set nextPoint to index of next control point
	for (unsigned int i = 0; i < controlPoints.size(); ++i)
	{
		if (controlPoints[i].time > time) 
		{
			nextPoint = i;
			break;
		}
	}

	return true; // when nextPoint is found
}


// current
// Implement Hermite curve
Point Curve::useHermiteCurve(const unsigned int nextPoint, const float time)
{
	unsigned int prevPoint = nextPoint - 1;

	Point prevPointPos = controlPoints[prevPoint].position;      // Position of previous control point
	Point nextPointPos = controlPoints[nextPoint].position;      // Position of next control point
	Vector prevPointTangent = controlPoints[prevPoint].tangent;  // Tangent of previous control point
	Vector nextPointTangent = controlPoints[nextPoint].tangent;  // Tangent of next control point

	// Time variables
	float prevPointTime = controlPoints[prevPoint].time;  // t0
	float nextPointTime = controlPoints[nextPoint].time;  // t1
	float elapsedTime = time - prevPointTime;             // t
	float intervalTime = (time - prevPointTime) / (nextPointTime - prevPointTime); 	// Normalize time from prevPoint to nextPoint (e.g (t - t0) / (t1 - t0) )

	/*
    //generic hermite function: does not interpolate
	Vector a = -2.0f*(nextPointPos - prevPointPos) + prevPointTangent + nextPointTangent;
	Vector b = 3.0f*(nextPointPos - prevPointPos) - 2.0f*prevPointTangent - nextPointTangent;
	Vector c = prevPointTangent;
	Point  d = prevPointPos;

	Point newPosition = d + a*pow(intervalTime, 3) + b*pow(intervalTime, 2) + c*intervalTime;  
	*/

	/*
	// Non-expanded for of interpolation function
	Vector a = (-2.0f*(nextPointPos - prevPointPos) / pow(nextPointTime - prevPointTime, 3)) + ((prevPointTangent + nextPointTangent) / pow(nextPointTime - prevPointTime, 2));
	Vector b = (3.0f*(nextPointPos - prevPointPos) / pow(nextPointTime - prevPointTime, 2)) - ((2.0f*prevPointTangent + nextPointTangent) / (nextPointTime - prevPointTime));
	Vector c = prevPointTangent;
	Point  d = prevPointPos;
	
	Point newPosition = d + a*pow(elapsedTime, 3) + b*pow(elapsedTime, 2) + c*elapsedTime;
	*/

	// Hermite Blending function
	Point a = prevPointPos * (2 * pow(intervalTime, 3) - 3 * pow(intervalTime, 2) + 1);
	Point b = nextPointPos * (-2 * pow(intervalTime, 3) + 3 * pow(intervalTime, 2));
	Vector c = prevPointTangent * ((pow(elapsedTime, 3) / pow(nextPointTime - prevPointTime, 2) + -2 * pow(elapsedTime, 2) / (nextPointTime - prevPointTime) + elapsedTime));
	Vector d = nextPointTangent * ((pow(elapsedTime, 3) / pow(nextPointTime - prevPointTime, 2)) - (pow(elapsedTime, 2) / (nextPointTime - prevPointTime)));

	Point newPosition = a + b + c + d;

	// Return result
	return newPosition;
}

// Implement Catmull-Rom curve
Point Curve::useCatmullCurve(const unsigned int nextPoint, const float time)
{
	Point newPosition;

	//================DELETE THIS PART AND THEN START CODING===================
	static bool flag = false;
	if (!flag)
	{
		std::cerr << "ERROR>>>>Member function useCatmullCurve is not implemented!" << std::endl;
		flag = true;
	}
	//=========================================================================

	// Calculate position at t = time on Catmull-Rom curve
	
	// Return result
	return newPosition;
}