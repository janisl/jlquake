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

#ifndef __idWorldSurface__
#define __idWorldSurface__

#include "Surface.h"
#include "shader.h"
#include "../../common/math/Vec2.h"
#include "../../common/math/Vec3.h"
#include "../../common/bv/Bounds.h"

// everything that is needed by the backend needs
// to be double buffered to allow it to run in
// parallel on a dual cpu machine
#define SMP_FRAMES      2

struct idWorldVertex {
	idVec3 xyz;
	idVec3 normal;
	idVec2 st;
	idVec2 lightmap;
	quint8 color[ 4 ];
};

class idWorldSurface : public idSurface {
public:
	// Q1, Q2 - should be drawn when node is crossed
	// Q3 - if == tr.viewCount, already added
	int viewCount;
	shader_t* shader;
	int fogIndex;

	// dynamic lighting information
	int dlightBits[ SMP_FRAMES ];

	// culling information
	idBounds bounds;

	int numVertexes;
	idWorldVertex* vertexes;

	idWorldSurface();
	virtual ~idWorldSurface();

	virtual bool IsGrid() const;
	virtual void ProjectDecal( struct decalProjector_t* dp, struct mbrush46_model_t* bmodel ) const;
	virtual bool CheckAddMarks( const vec3_t mins, const vec3_t maxs, const vec3_t dir ) const;
	virtual void MarkFragments( const vec3_t projectionDir,
		int numPlanes, const vec3_t* normals, const float* dists,
		int maxPoints, vec3_t pointBuffer,
		int maxFragments, markFragment_t* fragmentBuffer,
		int* returnedPoints, int* returnedFragments,
		const vec3_t mins, const vec3_t maxs ) const;
	virtual void MarkFragmentsWolf( const vec3_t projectionDir,
		int numPlanes, const vec3_t* normals, const float* dists,
		int maxPoints, vec3_t pointBuffer,
		int maxFragments, markFragment_t* fragmentBuffer,
		int* returnedPoints, int* returnedFragments,
		const vec3_t mins, const vec3_t maxs,
		bool oldMapping, const vec3_t center, float radius, const vec3_t bestnormal, int orientation, int numPoints ) const;
	virtual bool AddToNodeBounds() const;
	bool Cull( shader_t* shader, int* frontFace );
	int MarkDynamicLights( int dlightBits );

	void* GetBrush46Data() const;
	void SetBrush46Data( void* data );

protected:
	void* data;		// any of srf*_t

	virtual bool DoCull( shader_t* shader ) const;
	virtual bool DoCullET( shader_t* shader, int* frontFace ) const;
	virtual int DoMarkDynamicLights( int dlightBits );
	virtual int DoMarkDynamicLightsET( int dlightBits );

	bool ClipDecal( struct decalProjector_t* dp ) const;
};

inline idWorldSurface::idWorldSurface() {
	viewCount = 0;
	fogIndex = 0;
	shader = NULL;
	dlightBits[ 0 ] = 0;
	dlightBits[ 1 ] = 0;
	numVertexes = 0;
	vertexes = NULL;
	data = NULL;
}

inline void* idWorldSurface::GetBrush46Data() const {
	return data;
}

inline void idWorldSurface::SetBrush46Data( void* data ) {
	this->data = data;
}

#endif
