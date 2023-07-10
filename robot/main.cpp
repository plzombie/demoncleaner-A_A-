#include "robotapi.h"

#include <windows.h>

#define PI 3.1415926535

void _cdecl newmain(void)
{
	while(1) {
		if(GetKeyState(27) & 0x80) break;

		robot_rotate((PI/2)*(rand()%4));
		robot_move(6+6*(rand()%8), 10+rand()%20);
		if(rand()%4 > 0)
			robot_startcleaning();
		else
			robot_endcleaning();
	}
}
