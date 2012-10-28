// sys_null.h -- null system driver to aid porting efforts

#include "quakedef.h"
#include "errno.h"

void main(int argc, char** argv)
{
	quakeparms_t parms;

	parms.memsize = 5861376;
	parms.membase = malloc(parms.memsize);

	COM_InitArgv2(argc, argv);

	parms.argc = argc;
	parms.argv = argv;

	printf("Host_Init\n");
	Host_Init(&parms);
	while (1)
	{
		Host_Frame(0.1);
	}
}
