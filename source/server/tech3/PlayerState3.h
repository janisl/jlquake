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

//	This is initial portion of shared entity struct that is common in all
// Tech3 games.
struct sharedPlayerState_t
{
	int commandTime;	// cmd->serverTime of last executed command
	int pm_type;
	int bobCycle;		// for view bobbing and footstep generation
	int pm_flags;		// ducked, jump_held, etc
	int pm_time;
	vec3_t origin;
	vec3_t velocity;
	int weaponTime;
};

class idPlayerState3 : public Interface
{
public:
	idPlayerState3();

	void SetGEntity(void* newPS);

	/*
	int commandTime;	// cmd->serverTime of last executed command
	int pm_type;
	int bobCycle;		// for view bobbing and footstep generation
	int pm_flags;		// ducked, jump_held, etc
	int pm_time;
	*/
	const float* GetOrigin() const;
	void SetOrigin(const vec3_t value);
	/*
	vec3_t velocity;
	int weaponTime;
	 */

	virtual float GetLeanf() const = 0;
	virtual void SetLeanf(float value) = 0;
	virtual int GetClientNum() const = 0;
	virtual void SetClientNum(int value) = 0;
	virtual const float* GetViewAngles() const = 0;
	virtual void SetViewAngles(const vec3_t value) = 0;
	virtual int GetViewHeight() const = 0;
	virtual void SetViewHeight(int value) = 0;

protected:
	sharedPlayerState_t* ps;
};

inline idPlayerState3::idPlayerState3()
: ps(NULL)
{
}

inline void idPlayerState3::SetGEntity(void* newPS)
{
	ps = reinterpret_cast<sharedPlayerState_t*>(newPS);
}

inline const float* idPlayerState3::GetOrigin() const
{
	return ps->origin;
}

inline void idPlayerState3::SetOrigin(const vec3_t value)
{
	VectorCopy(value, ps->origin);
}
