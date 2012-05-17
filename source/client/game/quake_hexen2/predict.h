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

extern Cvar* cl_predict_players;
extern Cvar* cl_predict_players2;

void CLQW_SetSolidEntities();
void CLHW_SetSolidEntities();
void CLQW_PredictUsercmd(qwplayer_state_t* from, qwplayer_state_t* to, qwusercmd_t* u, bool spectator);
void CLHW_PredictUsercmd(hwplayer_state_t* from, hwplayer_state_t* to, hwusercmd_t* u, bool spectator);
void CLQW_SetUpPlayerPrediction(bool dopred);
void CLHW_SetUpPlayerPrediction(bool dopred);
void CLQHW_SetSolidPlayers(int playernum);
void CLQW_PredictMove();
void CLHW_PredictMove();
void CLQHW_InitPrediction();
