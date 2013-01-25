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

#include "../../common/qcommon.h"
#include "../../common/strings.h"
#include "local.h"

static const fielddef_t* FindField( const fielddef_t* defs, const char* name ) {
	for ( int i = 0; defs[ i ].name; i++ ) {
		if ( !String::Cmp( defs[ i ].name, name ) ) {
			return &defs[ i ];
		}
	}
	return NULL;
}

static bool ReadNumber( source_t* source, const fielddef_t* fd, void* p ) {
	token_t token;
	if ( !PC_ExpectAnyToken( source, &token ) ) {
		return false;
	}

	//check for minus sign
	bool negative = false;
	if ( token.type == TT_PUNCTUATION ) {
		//if not a minus sign
		if ( String::Cmp( token.string, "-" ) ) {
			SourceError( source, "unexpected punctuation %s", token.string );
			return false;
		}
		negative = true;
		//read the number
		if ( !PC_ExpectAnyToken( source, &token ) ) {
			return false;
		}
	}
	//check if it is a number
	if ( token.type != TT_NUMBER ) {
		SourceError( source, "expected number, found %s", token.string );
		return false;
	}
	//check for a float value
	if ( token.subtype & TT_FLOAT ) {
		if ( ( fd->type & FT_TYPE ) != FT_FLOAT ) {
			SourceError( source, "unexpected float" );
			return 0;
		}
		double floatval = token.floatvalue;
		if ( negative ) {
			floatval = -floatval;
		}
		*( float* )p = ( float )floatval;
		return true;
	}

	int intval = token.intvalue;
	if ( negative ) {
		intval = -intval;
	}
	//check bounds
	if ( ( fd->type & FT_TYPE ) == FT_INT ) {
		int intmin = -32768;
		int intmax = 32767;
		if ( intval < intmin || intval > intmax ) {
			SourceError( source, "value %d out of range [%d, %d]", intval, intmin, intmax );
			return false;
		}
	}
	//store the value
	if ( ( fd->type & FT_TYPE ) == FT_INT ) {
		*( int* )p = ( int )intval;
	} else if ( ( fd->type & FT_TYPE ) == FT_FLOAT )       {
		*( float* )p = ( float )intval;
	}
	return true;
}

static bool ReadString( source_t* source, const fielddef_t* fd, void* p ) {
	token_t token;
	if ( !PC_ExpectTokenType( source, TT_STRING, 0, &token ) ) {
		return true;
	}
	//remove the double quotes
	StripDoubleQuotes( token.string );
	//copy the string
	String::NCpyZ( ( char* )p, token.string, MAX_STRINGFIELD );
	return true;
}

bool ReadStructure( source_t* source, const structdef_t* def, char* structure ) {
	if ( !PC_ExpectTokenString( source, "{" ) ) {
		return false;
	}
	while ( 1 ) {
		token_t token;
		if ( !PC_ExpectAnyToken( source, &token ) ) {
			return false;
		}
		//if end of structure
		if ( !String::Cmp( token.string, "}" ) ) {
			break;
		}
		//find the field with the name
		const fielddef_t* fd = FindField( def->fields, token.string );
		if ( !fd ) {
			SourceError( source, "unknown structure field %s", token.string );
			return false;
		}
		int num;
		if ( fd->type & FT_ARRAY ) {
			num = fd->maxarray;
			if ( !PC_ExpectTokenString( source, "{" ) ) {
				return false;
			}
		} else   {
			num = 1;
		}
		void* p = ( void* )( structure + fd->offset );
		while ( num-- > 0 ) {
			if ( fd->type & FT_ARRAY ) {
				if ( PC_CheckTokenString( source, "}" ) ) {
					break;
				}
			}
			switch ( fd->type & FT_TYPE ) {
			case FT_INT:
				if ( !ReadNumber( source, fd, p ) ) {
					return false;
				}
				p = ( char* )p + sizeof ( int );
				break;
			case FT_FLOAT:
				if ( !ReadNumber( source, fd, p ) ) {
					return false;
				}
				p = ( char* )p + sizeof ( float );
				break;
			case FT_STRING:
				if ( !ReadString( source, fd, p ) ) {
					return false;
				}
				p = ( char* )p + MAX_STRINGFIELD;
				break;
			}
			if ( fd->type & FT_ARRAY ) {
				if ( !PC_ExpectAnyToken( source, &token ) ) {
					return false;
				}
				if ( !String::Cmp( token.string, "}" ) ) {
					break;
				}
				if ( String::Cmp( token.string, "," ) ) {
					SourceError( source, "expected a comma, found %s", token.string );
					return false;
				}
			}
		}
	}
	return true;
}
