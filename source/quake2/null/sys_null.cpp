// sys_null.h -- null system driver to aid porting efforts

#include "../qcommon/qcommon.h"
#include "errno.h"

void Sys_Error(const char* error, ...)
{
	va_list argptr;

	printf("Sys_Error: ");
	va_start(argptr,error);
	vprintf(error,argptr);
	va_end(argptr);
	printf("\n");

	exit(1);
}

void Sys_Quit(void)
{
	exit(0);
}

void Sys_AppActivate(void)
{
}

void    Sys_Init(void)
{
}


//=============================================================================

void main(int argc, char** argv)
{
	Qcommon_Init(argc, argv);

	while (1)
	{
		Qcommon_Frame(0.1);
	}
}
