
//**************************************************************************
//**
//** sbar.h
//**
//** $Header: /HexenWorld/Client/sbar.h 2     2/04/98 11:08p Rjohnson $
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

void SB_Init(void);
void SB_Draw(void);
void SB_IntermissionOverlay(void);
void SB_FinaleOverlay(void);
void SB_InvChanged(void);
void SB_InvReset(void);
void SB_ViewSizeChanged(void);
void Sbar_Changed(void);
void Sbar_Draw(void);
void Sbar_IntermissionOverlay(void);
void Sbar_FinaleOverlay(void);
void Sbar_DeathmatchOverlay(void);
void Sbar_Init(void);

// PUBLIC DATA DECLARATIONS ------------------------------------------------

extern int sbqh_lines;// scan lines to draw
