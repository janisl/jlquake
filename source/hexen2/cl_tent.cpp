
//**************************************************************************
//**
//** cl_tent.c
//**
//** Client side temporary entity effects.
//**
//** $Header: /H2 Mission Pack/CL_TENT.C 6     3/02/98 2:51p Jweier $
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "quakedef.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
// CL_InitTEnts
//
//==========================================================================

void CL_InitTEnts(void)
{
	CLH2_InitTEntsCommon();
}

//==========================================================================
//
// CL_ParseTEnt
//
//==========================================================================

void CL_ParseTEnt()
{
	int type = net_message.ReadByte();
	switch (type)
	{
	case H2TE_WIZSPIKE:	// spike hitting wall
		CLH2_ParseWizSpike(net_message);
		break;
	case H2TE_KNIGHTSPIKE:	// spike hitting wall
	case H2TE_GUNSHOT:		// bullet hitting wall
		CLH2_ParseKnightSpike(net_message);
		break;
	case H2TE_SPIKE:			// spike hitting wall
		CLH2_ParseSpike(net_message);
		break;
	case H2TE_SUPERSPIKE:			// super spike hitting wall
		CLH2_ParseSuperSpike(net_message);
		break;
	case H2TE_EXPLOSION:			// rocket explosion
		CLH2_ParseExplosion(net_message);
		break;
	case H2TE_LIGHTNING1:
	case H2TE_LIGHTNING2:
	case H2TE_LIGHTNING3:
		CLH2_ParseBeam(net_message);
		break;

	case H2TE_STREAM_CHAIN:
	case H2TE_STREAM_SUNSTAFF1:
	case H2TE_STREAM_SUNSTAFF2:
	case H2TE_STREAM_LIGHTNING:
	case H2TE_STREAM_LIGHTNING_SMALL:
	case H2TE_STREAM_COLORBEAM:
	case H2TE_STREAM_ICECHUNKS:
	case H2TE_STREAM_GAZE:
	case H2TE_STREAM_FAMINE:
		CLH2_ParseStream(net_message, type);
		break;

	case H2TE_LAVASPLASH:
		CLH2_ParseLavaSplash(net_message);
		break;
	case H2TE_TELEPORT:
		CLH2_ParseTeleport(net_message);
		break;
	default:
		Sys_Error ("CL_ParseTEnt: bad type");
	}
}

//==========================================================================
//
// CL_UpdateTEnts
//
//==========================================================================

void CL_UpdateTEnts(void)
{
	CLH2_UpdateStreams();
}
