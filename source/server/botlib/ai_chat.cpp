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

#include "../server.h"
#include "local.h"

bot_chatstate_t* botchatstates[MAX_BOTLIB_CLIENTS_ARRAY + 1];
//console message heap
bot_consolemessage_t* consolemessageheap = NULL;
bot_consolemessage_t* freeconsolemessages = NULL;
//list with match strings
bot_matchtemplate_t* matchtemplates = NULL;
//list with synonyms
bot_synonymlist_t* synonyms = NULL;
//list with random strings
bot_randomlist_t* randomstrings = NULL;
//reply chats
bot_replychat_t* replychats = NULL;
bot_ichatdata_t* ichatdata[MAX_BOTLIB_CLIENTS_ARRAY];
