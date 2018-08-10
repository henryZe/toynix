#ifndef INC_DEBUG_H
#define INC_DEBUG_H

enum {
	CPU_INFO,
	MEM_INFO,

	MAXDEBUGOPT,
};

static const char * const debug_option[MAXDEBUGOPT] = {
	[CPU_INFO]	= "cpu",
	[MEM_INFO]	= "mem",
};

#endif