#if defined(_WIN32) || defined(_WIN64) || defined(_WINDOWS_) || defined(WIN32)
	#ifndef USEFORWIN
		#define USEFORWIN 
	#endif	
#else
	#define USEFORLINUX 
#endif

#ifdef USEFORWIN 
	#include "MyWinMethod.h"
	#define strsep(param1, param2) strSplit(param1, param2)
	#define strcasecmp(param1, param2) strcmp(param1, param2)
	#define srandom(param1) srand(param1)
	#define random() rand()
	#define sleep(param1) Sleep(1000*param1)

	#define snprintf _snprintf  //may be err
	#define socketpair win32_socketpair
#endif
