//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 3
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

#include "splines.h"
#include "public.h"
#include "q_parse.h"
#include "../../common/Common.h"
#include "../../common/command_buffer.h"
#include "../../common/strings.h"
#include "../../common/files.h"

// TTimo
// handy stuff when tracking isnan problems
#ifndef NDEBUG
#define CHECK_NAN_VEC(v) assert(!IS_NAN(v[0]) && !IS_NAN(v[1]) && !IS_NAN(v[2]))
#else
#define CHECK_NAN_VEC
#endif

// (SA) making a list of cameras so I can use
//		the splines as targets for other things.
//		Certainly better ways to do this, but this lets
//		me get underway quickly with ents that need spline
//		targets.
#define MAX_CAMERAS 64

idCameraDef camera[MAX_CAMERAS];

bool loadCamera(int camNum, const char* name)
{
	if (camNum < 0 || camNum >= MAX_CAMERAS)
	{
		return false;
	}
	camera[camNum].clear();
	// TTimo static_cast confused gcc, went for C-style casting
	return camera[camNum].load(name);
}

bool getCameraInfo(int camNum, int time, float* origin, float* angles, float* fov)
{
	idVec3Splines dir, org;
	if (camNum < 0 || camNum >= MAX_CAMERAS)
	{
		return false;
	}
	org[0] = origin[0];
	org[1] = origin[1];
	org[2] = origin[2];
	if (camera[camNum].getCameraInfo(time, org, dir, fov))
	{
		origin[0] = org[0];
		origin[1] = org[1];
		origin[2] = org[2];
		angles[1] = atan2(dir[1], dir[0]) * 180 / 3.14159;
		angles[0] = asin(dir[2]) * 180 / 3.14159;
		return true;
	}
	return false;
}

void startCamera(int camNum, int time)
{
	if (camNum < 0 || camNum >= MAX_CAMERAS)
	{
		return;
	}
	camera[camNum].startCamera(time);
}

idVec3Splines idSplineList::zero(0,0,0);

void idSplineList::buildSpline()
{
	//int start = Sys_Milliseconds();
	clearSpline();
	for (int i = 3; i < controlPoints.Num(); i++)
	{
		for (float tension = 0.0f; tension < 1.001f; tension += granularity)
		{
			float x = 0;
			float y = 0;
			float z = 0;
			for (int j = 0; j < 4; j++)
			{
				x += controlPoints[i - (3 - j)]->x * calcSpline(j, tension);
				y += controlPoints[i - (3 - j)]->y * calcSpline(j, tension);
				z += controlPoints[i - (3 - j)]->z * calcSpline(j, tension);
			}
			splinePoints.Append(new idVec3Splines(x, y, z));
		}
	}
	dirty = false;
}


float idSplineList::totalDistance()
{

	// FIXME: save dist and return
	//
	if (controlPoints.Num() == 0)
	{
		return 0.0;
	}

	if (dirty)
	{
		buildSpline();
	}

	float dist = 0.0;
	idVec3Splines temp;
	int count = splinePoints.Num();
	for (int i = 1; i < count; i++)
	{
		temp = *splinePoints[i - 1];
		temp -= *splinePoints[i];
		dist += temp.Length();
	}
	return dist;
}

void idSplineList::initPosition(long bt, long totalTime)
{

	if (dirty)
	{
		buildSpline();
	}

	if (splinePoints.Num() == 0)
	{
		return;
	}

	baseTime = bt;
	time = totalTime;

	// calc distance to travel ( this will soon be broken into time segments )
	splineTime.Clear();
	splineTime.Append(bt);
	double dist = totalDistance();
	double distSoFar = 0.0;
	idVec3Splines temp;
	int count = splinePoints.Num();
	//for(int i = 2; i < count - 1; i++) {
	for (int i = 1; i < count; i++)
	{
		temp = *splinePoints[i - 1];
		temp -= *splinePoints[i];
		distSoFar += temp.Length();
		double percent = distSoFar / dist;
		percent *= totalTime;
		splineTime.Append(percent + bt);
	}
	assert(splineTime.Num() == splinePoints.Num());
	activeSegment = 0;
}



float idSplineList::calcSpline(int step, float tension)
{
	switch (step)
	{
	case 0: return (pow(1 - tension, 3)) / 6;
	case 1: return (3 * pow(tension, 3) - 6 * pow(tension, 2) + 4) / 6;
	case 2: return (-3 * pow(tension, 3) + 3 * pow(tension, 2) + 3 * tension + 1) / 6;
	case 3: return pow(tension, 3) / 6;
	}
	return 0.0;
}

const idVec3Splines* idSplineList::getPosition(long t)
{
	static idVec3Splines interpolatedPos;

	int count = splineTime.Num();
	if (count == 0)
	{
		return &zero;
	}

	assert(splineTime.Num() == splinePoints.Num());

	while (activeSegment < count)
	{
		if (splineTime[activeSegment] >= t)
		{
			if (activeSegment > 0 && activeSegment < count - 1)
			{
				double timeHi = splineTime[activeSegment + 1];
				double timeLo = splineTime[activeSegment - 1];
				double percent = (timeHi - t) / (timeHi - timeLo);
				// pick two bounding points
				idVec3Splines v1 = *splinePoints[activeSegment - 1];
				idVec3Splines v2 = *splinePoints[activeSegment + 1];
				v2 *= (1.0 - percent);
				v1 *= percent;
				v2 += v1;
				interpolatedPos = v2;
				return &interpolatedPos;
			}
			return splinePoints[activeSegment];
		}
		else
		{
			activeSegment++;
		}
	}
	return splinePoints[count - 1];
}

void idSplineList::parse(const char** text)
{
	const char* token;
	//Com_MatchToken( text, "{" );
	do
	{
		token = Com_Parse(text);

		if (!token[0])
		{
			break;
		}
		if (!String::ICmp(token, "}"))
		{
			break;
		}

		do
		{
			// if token is not a brace, it is a key for a key/value pair
			if (!token[0] || !String::ICmp(token, "(") || !String::ICmp(token, "}"))
			{
				break;
			}

			Com_UngetToken();
			idStr key = Com_ParseOnLine(text);
			const char* token = Com_Parse(text);
			if (String::ICmp(key.CStr(), "granularity") == 0)
			{
				granularity = String::Atof(token);
			}
			else if (String::ICmp(key.CStr(), "name") == 0)
			{
				name = token;
			}
			token = Com_Parse(text);

		}
		while (1);

		if (!String::ICmp(token, "}"))
		{
			break;
		}

		Com_UngetToken();
		// read the control point
		idVec3Splines point;
		Com_Parse1DMatrix(text, 3, point);
		addPoint(point.x, point.y, point.z);
	}
	while (1);

	//Com_UngetToken();
	//Com_MatchToken( text, "}" );
	dirty = true;
}

bool idCameraDef::getCameraInfo(long time, idVec3Splines&origin, idVec3Splines&direction, float* fv)
{

	char buff[1024];

	if ((time - startTime) / 1000 > totalTime)
	{
		return false;
	}


	for (int i = 0; i < events.Num(); i++)
	{
		if (time >= startTime + events[i]->getTime() && !events[i]->getTriggered())
		{
			events[i]->setTriggered(true);
			if (events[i]->getType() == idCameraEvent::EVENT_TARGET)
			{
				setActiveTargetByName(events[i]->getParam());
				getActiveTarget()->start(startTime + events[i]->getTime());
			}
			else if (events[i]->getType() == idCameraEvent::EVENT_TRIGGER)
			{
				// empty!
			}
			else if (events[i]->getType() == idCameraEvent::EVENT_FOV)
			{
				memset(buff, 0, sizeof(buff));
				String::Cpy(buff, events[i]->getParam());
				const char* param1 = strtok(buff, " \t,\0");
				const char* param2 = strtok(NULL, " \t,\0");
				float len = (param2) ? String::Atof(param2) : 0;
				float newfov = (param1) ? String::Atof(param1) : 90;
				fov.reset(fov.getFOV(time), newfov, time, len);
				//*fv = fov = String::Atof(events[i]->getParam());
			}
			else if (events[i]->getType() == idCameraEvent::EVENT_FADEIN)
			{
				float time = String::Atof(events[i]->getParam());
				Cbuf_AddText(va("fade 0 0 0 0 %f", time));
				Cbuf_Execute();
			}
			else if (events[i]->getType() == idCameraEvent::EVENT_FADEOUT)
			{
				float time = String::Atof(events[i]->getParam());
				Cbuf_AddText(va("fade 0 0 0 255 %f", time));
				Cbuf_Execute();
			}
			else if (events[i]->getType() == idCameraEvent::EVENT_CAMERA)
			{
				memset(buff, 0, sizeof(buff));
				String::Cpy(buff, events[i]->getParam());
				const char* param1 = strtok(buff, " \t,\0");
				const char* param2 = strtok(NULL, " \t,\0");

				if (param2)
				{
					loadCamera(String::Atoi(param1), va("cameras/%s.camera", param2));
					startCamera(time);
				}
				else
				{
					loadCamera(0, va("cameras/%s.camera", events[i]->getParam()));
					startCamera(time);
				}
				return true;
			}
			else if (events[i]->getType() == idCameraEvent::EVENT_STOP)
			{
				return false;
			}
		}
	}

	origin = *cameraPosition->getPosition(time);

	CHECK_NAN_VEC(origin);

	*fv = fov.getFOV(time);

	idVec3Splines temp = origin;

	int numTargets = targetPositions.Num();
	if (numTargets == 0)
	{
		// empty!
	}
	else
	{
		temp = *getActiveTarget()->getPosition(time);
	}

	temp -= origin;
	temp.Normalize();
	direction = temp;

	return true;
}

#define NUM_CCELERATION_SEGS 10
#define CELL_AMT 5

void idCameraDef::buildCamera()
{
	int i;
	idList<float> waits;
	idList<int> targets;

	totalTime = baseTime;
	cameraPosition->setTime(totalTime * 1000);
	// we have a base time layout for the path and the target path
	// now we need to layer on any wait or speed changes
	for (i = 0; i < events.Num(); i++)
	{
		events[i]->setTriggered(false);
		switch (events[i]->getType())
		{
		case idCameraEvent::EVENT_TARGET: {
			targets.Append(i);
			break;
		}
		case idCameraEvent::EVENT_FEATHER: {
			long startTime = 0;
			float speed = 0;
			long loopTime = 10;
			float stepGoal = cameraPosition->getBaseVelocity() / (1000 / loopTime);
			while (startTime <= 1000)
			{
				cameraPosition->addVelocity(startTime, loopTime, speed);
				speed += stepGoal;
				if (speed > cameraPosition->getBaseVelocity())
				{
					speed = cameraPosition->getBaseVelocity();
				}
				startTime += loopTime;
			}

			// TTimo gcc warns: assignment to `long int' from `float'
			// more efficient to do (long int)(totalTime) * 1000 - 1000
			// safer to (long int)(totalTime * 1000 - 1000)
			startTime = (long int)(totalTime * 1000 - 1000);
			long endTime = startTime + 1000;
			speed = cameraPosition->getBaseVelocity();
			while (startTime < endTime)
			{
				speed -= stepGoal;
				if (speed < 0)
				{
					speed = 0;
				}
				cameraPosition->addVelocity(startTime, loopTime, speed);
				startTime += loopTime;
			}
			break;

		}
		case idCameraEvent::EVENT_WAIT: {
			waits.Append(String::Atof(events[i]->getParam()));

			//FIXME: this is quite hacky for Wolf E3, accel and decel needs
			// do be parameter based etc..
			long startTime = events[i]->getTime() - 1000;
			if (startTime < 0)
			{
				startTime = 0;
			}
			float speed = cameraPosition->getBaseVelocity();
			long loopTime = 10;
			float steps = speed / ((events[i]->getTime() - startTime) / loopTime);
			while (startTime <= events[i]->getTime() - loopTime)
			{
				cameraPosition->addVelocity(startTime, loopTime, speed);
				speed -= steps;
				startTime += loopTime;
			}
			cameraPosition->addVelocity(events[i]->getTime(), String::Atof(events[i]->getParam()) * 1000, 0);

			startTime = (long int)(events[i]->getTime() + String::Atof(events[i]->getParam()) * 1000);
			long endTime = startTime + 1000;
			speed = 0;
			while (startTime <= endTime)
			{
				cameraPosition->addVelocity(startTime, loopTime, speed);
				speed += steps;
				startTime += loopTime;
			}
			break;
		}
		case idCameraEvent::EVENT_TARGETWAIT: {
			//targetWaits.Append(i);
			break;
		}
		case idCameraEvent::EVENT_SPEED: {
/*
                // take the average delay between up to the next five segments
                float adjust = String::Atof(events[i]->getParam());
                int index = events[i]->getSegment();
                total = 0;
                count = 0;

                // get total amount of time over the remainder of the segment
                for (j = index; j < cameraSpline.numSegments() - 1; j++) {
                    total += cameraSpline.getSegmentTime(j + 1) - cameraSpline.getSegmentTime(j);
                    count++;
                }

                // multiply that by the adjustment
                double newTotal = total * adjust;
                // what is the difference..
                newTotal -= total;
                totalTime += newTotal / 1000;

                // per segment difference
                newTotal /= count;
                int additive = newTotal;

                // now propogate that difference out to each segment
                for (j = index; j < cameraSpline.numSegments(); j++) {
                    cameraSpline.addSegmentTime(j, additive);
                    additive += newTotal;
                }
                break;
*/
		default:
			break;
		}
		}
	}


	for (i = 0; i < waits.Num(); i++)
	{
		totalTime += waits[i];
	}

	// on a new target switch, we need to take time to this point ( since last target switch )
	// and allocate it across the active target, then reset time to this point
	long timeSoFar = 0;
	long total = (long int)(totalTime * 1000);
	for (i = 0; i < targets.Num(); i++)
	{
		long t;
		if (i < targets.Num() - 1)
		{
			t = events[targets[i + 1]]->getTime();
		}
		else
		{
			t = total - timeSoFar;
		}
		// t is how much time to use for this target
		setActiveTargetByName(events[targets[i]]->getParam());
		getActiveTarget()->setTime(t);
		timeSoFar += t;
	}


}

void idCameraDef::startCamera(long t)
{
	cameraPosition->clearVelocities();
	cameraPosition->start(t);
	buildCamera();
	fov.reset(90, 90, t, 0);
	//for (int i = 0; i < targetPositions.Num(); i++) {
	//	targetPositions[i]->
	//}
	startTime = t;
	cameraRunning = true;
}


void idCameraDef::parse(const char** text)
{

	const char* token;
	do
	{
		token = Com_Parse(text);

		if (!token[0])
		{
			break;
		}
		if (!String::ICmp(token, "}"))
		{
			break;
		}

		if (String::ICmp(token, "time") == 0)
		{
			baseTime = Com_ParseFloat(text);
		}
		else if (String::ICmp(token, "camera_fixed") == 0)
		{
			cameraPosition = new idFixedPosition();
			cameraPosition->parse(text);
		}
		else if (String::ICmp(token, "camera_interpolated") == 0)
		{
			cameraPosition = new idInterpolatedPosition();
			cameraPosition->parse(text);
		}
		else if (String::ICmp(token, "camera_spline") == 0)
		{
			cameraPosition = new idSplinePosition();
			cameraPosition->parse(text);
		}
		else if (String::ICmp(token, "target_fixed") == 0)
		{
			idFixedPosition* pos = new idFixedPosition();
			pos->parse(text);
			targetPositions.Append(pos);
		}
		else if (String::ICmp(token, "target_interpolated") == 0)
		{
			idInterpolatedPosition* pos = new idInterpolatedPosition();
			pos->parse(text);
			targetPositions.Append(pos);
		}
		else if (String::ICmp(token, "target_spline") == 0)
		{
			idSplinePosition* pos = new idSplinePosition();
			pos->parse(text);
			targetPositions.Append(pos);
		}
		else if (String::ICmp(token, "fov") == 0)
		{
			fov.parse(text);
		}
		else if (String::ICmp(token, "event") == 0)
		{
			idCameraEvent* event = new idCameraEvent();
			event->parse(text);
			addEvent(event);
		}


	}
	while (1);

	if (!cameraPosition)
	{
		common->Printf("no camera position specified\n");
		// prevent a crash later on
		cameraPosition = new idFixedPosition();
	}

	Com_UngetToken();
	Com_MatchToken(text, "}");

}

bool idCameraDef::load(const char* filename)
{
	char* buf;
	const char* buf_p;
	FS_ReadFile(filename, (void**)&buf);
	if (!buf)
	{
		return false;
	}

	clear();
	Com_BeginParseSession(filename);
	buf_p = buf;
	parse(&buf_p);
	Com_EndParseSession();
	FS_FreeFile(buf);

	return true;
}

void idCameraDef::addEvent(idCameraEvent* event)
{
	events.Append(event);
}

void idCameraDef::addEvent(idCameraEvent::eventType t, const char* param, long time)
{
	addEvent(new idCameraEvent(t, param, time));
	buildCamera();
}


void idCameraEvent::parse(const char** text)
{
	const char* token;
	Com_MatchToken(text, "{");
	do
	{
		token = Com_Parse(text);

		if (!token[0])
		{
			break;
		}
		if (!String::Cmp(token, "}"))
		{
			break;
		}

		// here we may have to jump over brush epairs ( only used in editor )
		do
		{
			// if token is not a brace, it is a key for a key/value pair
			if (!token[0] || !String::Cmp(token, "(") || !String::Cmp(token, "}"))
			{
				break;
			}

			Com_UngetToken();
			idStr key = Com_ParseOnLine(text);
			const char* token = Com_Parse(text);
			if (String::ICmp(key.CStr(), "type") == 0)
			{
				type = static_cast<idCameraEvent::eventType>(String::Atoi(token));
			}
			else if (String::ICmp(key.CStr(), "param") == 0)
			{
				paramStr = token;
			}
			else if (String::ICmp(key.CStr(), "time") == 0)
			{
				time = String::Atoi(token);
			}
			token = Com_Parse(text);

		}
		while (1);

		if (!String::Cmp(token, "}"))
		{
			break;
		}

	}
	while (1);

	Com_UngetToken();
	Com_MatchToken(text, "}");
}

const idVec3Splines* idInterpolatedPosition::getPosition(long t)
{
	static idVec3Splines interpolatedPos;
	float percent = 0.0;
	float velocity = getVelocity(t);
	float timePassed = t - lastTime;
	lastTime = t;

	// convert to seconds
	timePassed /= 1000;

	float distToTravel = timePassed * velocity;

	idVec3Splines temp = startPos;
	temp -= endPos;
	float distance = temp.Length();

	distSoFar += distToTravel;

	// TTimo
	// show_bug.cgi?id=409
	// avoid NaN on fixed cameras
	if (distance != 0.0)		//DAJ added to protect DBZ
	{
		percent = (float)(distSoFar) / distance;
	}

	if (percent > 1.0)
	{
		percent = 1.0;
	}
	else if (percent < 0.0)
	{
		percent = 0.0;
	}

	// the following line does a straigt calc on percentage of time
	// float percent = (float)(startTime + time - t) / time;

	idVec3Splines v1 = startPos;
	idVec3Splines v2 = endPos;
	v1 *= (1.0 - percent);
	v2 *= percent;
	v1 += v2;
	interpolatedPos = v1;
	return &interpolatedPos;
}


void idCameraFOV::parse(const char** text)
{
	const char* token;
	Com_MatchToken(text, "{");
	do
	{
		token = Com_Parse(text);

		if (!token[0])
		{
			break;
		}
		if (!String::Cmp(token, "}"))
		{
			break;
		}

		// here we may have to jump over brush epairs ( only used in editor )
		do
		{
			// if token is not a brace, it is a key for a key/value pair
			if (!token[0] || !String::Cmp(token, "(") || !String::Cmp(token, "}"))
			{
				break;
			}

			Com_UngetToken();
			idStr key = Com_ParseOnLine(text);
			const char* token = Com_Parse(text);
			if (String::ICmp(key.CStr(), "fov") == 0)
			{
				fov = String::Atof(token);
			}
			else if (String::ICmp(key.CStr(), "startFOV") == 0)
			{
				startFOV = String::Atof(token);
			}
			else if (String::ICmp(key.CStr(), "endFOV") == 0)
			{
				endFOV = String::Atof(token);
			}
			else if (String::ICmp(key.CStr(), "time") == 0)
			{
				time = String::Atoi(token);
			}
			token = Com_Parse(text);

		}
		while (1);

		if (!String::Cmp(token, "}"))
		{
			break;
		}

	}
	while (1);

	Com_UngetToken();
	Com_MatchToken(text, "}");
}

bool idCameraPosition::parseToken(const char* key, const char** text)
{
	const char* token = Com_Parse(text);
	if (String::ICmp(key, "time") == 0)
	{
		time = atol(token);
		return true;
	}
	else if (String::ICmp(key, "type") == 0)
	{
		type = static_cast<idCameraPosition::positionType>(String::Atoi(token));
		return true;
	}
	else if (String::ICmp(key, "velocity") == 0)
	{
		long t = atol(token);
		token = Com_Parse(text);
		long d = atol(token);
		token = Com_Parse(text);
		float s = String::Atof(token);
		addVelocity(t, d, s);
		return true;
	}
	else if (String::ICmp(key, "baseVelocity") == 0)
	{
		baseVelocity = String::Atof(token);
		return true;
	}
	else if (String::ICmp(key, "name") == 0)
	{
		name = token;
		return true;
	}
	else if (String::ICmp(key, "time") == 0)
	{
		time = String::Atoi(token);
		return true;
	}
	Com_UngetToken();
	return false;
}



void idFixedPosition::parse(const char** text)
{
	const char* token;
	Com_MatchToken(text, "{");
	do
	{
		token = Com_Parse(text);

		if (!token[0])
		{
			break;
		}
		if (!String::Cmp(token, "}"))
		{
			break;
		}

		// here we may have to jump over brush epairs ( only used in editor )
		do
		{
			// if token is not a brace, it is a key for a key/value pair
			if (!token[0] || !String::Cmp(token, "(") || !String::Cmp(token, "}"))
			{
				break;
			}

			Com_UngetToken();
			idStr key = Com_ParseOnLine(text);

			const char* token = Com_Parse(text);
			if (String::ICmp(key.CStr(), "pos") == 0)
			{
				Com_UngetToken();
				Com_Parse1DMatrix(text, 3, pos);
			}
			else
			{
				Com_UngetToken();
				idCameraPosition::parseToken(key.CStr(), text);
			}
			token = Com_Parse(text);

		}
		while (1);

		if (!String::Cmp(token, "}"))
		{
			break;
		}

	}
	while (1);

	Com_UngetToken();
	Com_MatchToken(text, "}");
}

void idInterpolatedPosition::parse(const char** text)
{
	const char* token;
	Com_MatchToken(text, "{");
	do
	{
		token = Com_Parse(text);

		if (!token[0])
		{
			break;
		}
		if (!String::Cmp(token, "}"))
		{
			break;
		}

		// here we may have to jump over brush epairs ( only used in editor )
		do
		{
			// if token is not a brace, it is a key for a key/value pair
			if (!token[0] || !String::Cmp(token, "(") || !String::Cmp(token, "}"))
			{
				break;
			}

			Com_UngetToken();
			idStr key = Com_ParseOnLine(text);

			const char* token = Com_Parse(text);
			if (String::ICmp(key.CStr(), "startPos") == 0)
			{
				Com_UngetToken();
				Com_Parse1DMatrix(text, 3, startPos);
			}
			else if (String::ICmp(key.CStr(), "endPos") == 0)
			{
				Com_UngetToken();
				Com_Parse1DMatrix(text, 3, endPos);
			}
			else
			{
				Com_UngetToken();
				idCameraPosition::parseToken(key.CStr(), text);
			}
			token = Com_Parse(text);

		}
		while (1);

		if (!String::Cmp(token, "}"))
		{
			break;
		}

	}
	while (1);

	Com_UngetToken();
	Com_MatchToken(text, "}");
}


void idSplinePosition::parse(const char** text)
{
	const char* token;
	Com_MatchToken(text, "{");
	do
	{
		token = Com_Parse(text);

		if (!token[0])
		{
			break;
		}
		if (!String::Cmp(token, "}"))
		{
			break;
		}

		// here we may have to jump over brush epairs ( only used in editor )
		do
		{
			// if token is not a brace, it is a key for a key/value pair
			if (!token[0] || !String::Cmp(token, "(") || !String::Cmp(token, "}"))
			{
				break;
			}

			Com_UngetToken();
			idStr key = Com_ParseOnLine(text);

			const char* token = Com_Parse(text);
			if (String::ICmp(key.CStr(), "target") == 0)
			{
				target.parse(text);
			}
			else
			{
				Com_UngetToken();
				idCameraPosition::parseToken(key.CStr(), text);
			}
			token = Com_Parse(text);

		}
		while (1);

		if (!String::Cmp(token, "}"))
		{
			break;
		}

	}
	while (1);

	Com_UngetToken();
	Com_MatchToken(text, "}");
}

void idCameraDef::addTarget(const char* name, idCameraPosition::positionType type)
{
	idCameraPosition* pos = newFromType(type);
	if (pos)
	{
		pos->setName(name);
		targetPositions.Append(pos);
		activeTarget = numTargets() - 1;
		if (activeTarget == 0)
		{
			// first one
			addEvent(idCameraEvent::EVENT_TARGET, name, 0);
		}
	}
}

const idVec3Splines* idSplinePosition::getPosition(long t)
{
	static idVec3Splines interpolatedPos;

	float velocity = getVelocity(t);
	float timePassed = t - lastTime;
	lastTime = t;

	// convert to seconds
	timePassed /= 1000;

	float distToTravel = timePassed * velocity;

	distSoFar += distToTravel;
	double tempDistance = target.totalDistance();

	double percent = (double)(distSoFar) / tempDistance;

	double targetDistance = percent * tempDistance;
	tempDistance = 0;

	double lastDistance1,lastDistance2;
	lastDistance1 = lastDistance2 = 0;
	//FIXME: calc distances on spline build
	idVec3Splines temp;
	int count = target.numSegments();
	// TTimo fixed MSVCism
	int i;
	for (i = 1; i < count; i++)
	{
		temp = *target.getSegmentPoint(i - 1);
		temp -= *target.getSegmentPoint(i);
		tempDistance += temp.Length();
		if (i & 1)
		{
			lastDistance1 = tempDistance;
		}
		else
		{
			lastDistance2 = tempDistance;
		}
		if (tempDistance >= targetDistance)
		{
			break;
		}
	}

	if (i >= count - 1)
	{
		interpolatedPos = *target.getSegmentPoint(i - 1);
	}
	else
	{
#if 0
		double timeHi = target.getSegmentTime(i + 1);
		double timeLo = target.getSegmentTime(i - 1);
		double percent = (timeHi - t) / (timeHi - timeLo);
		idVec3Splines v1 = *target.getSegmentPoint(i - 1);
		idVec3Splines v2 = *target.getSegmentPoint(i + 1);
		v2 *= (1.0 - percent);
		v1 *= percent;
		v2 += v1;
		interpolatedPos = v2;
#else
		if (lastDistance1 > lastDistance2)
		{
			double d = lastDistance2;
			lastDistance2 = lastDistance1;
			lastDistance1 = d;
		}

		idVec3Splines v1 = *target.getSegmentPoint(i - 1);
		idVec3Splines v2 = *target.getSegmentPoint(i);
		double percent = (lastDistance2 - targetDistance) / (lastDistance2 - lastDistance1);
		v2 *= (1.0 - percent);
		v1 *= percent;
		v2 += v1;
		interpolatedPos = v2;
#endif
	}
	return &interpolatedPos;

}
