
#include <windows.h>

typedef struct {
	float (_cdecl *robot_move)(float pathlen, float speed);
	void (_cdecl *robot_rotate)(float angle);
	void (_cdecl *robot_startcleaning)(void);
	void (_cdecl *robot_endcleaning)(void);
	unsigned int (_cdecl *robot_getdirtsensor)(void);
	unsigned int (_cdecl *robot_getpitssensor)(void);
	unsigned int (_cdecl *robot_getecholocator)(void);
} VISUAL_API;

float (_cdecl *robot_move)(float pathlen, float speed);
void (_cdecl *robot_rotate)(float angle);
void (_cdecl *robot_startcleaning)(void);
void (_cdecl *robot_endcleaning)(void);
unsigned int (_cdecl *robot_getdirtsensor)(void);
unsigned int (_cdecl *robot_getpitssensor)(void);
unsigned int (_cdecl *robot_getecholocator)(void);

extern void _cdecl newmain(void);

extern "C" __declspec(dllexport) void setvisualapi(VISUAL_API visual_api)
{	
	robot_move = visual_api.robot_move;
	robot_rotate = visual_api.robot_rotate;
	robot_startcleaning = visual_api.robot_startcleaning;
	robot_endcleaning = visual_api.robot_endcleaning;
	robot_getdirtsensor = visual_api.robot_getdirtsensor;
	robot_getpitssensor = visual_api.robot_getpitssensor;
	robot_getecholocator = visual_api.robot_getecholocator;
}

extern "C" __declspec(dllexport) void _cdecl robotfunc(void)
{
	newmain();
}

BOOL APIENTRY LibMain(HANDLE hinstDLL, DWORD  fdwReason, LPVOID lpvReserved ) 
{ 
	return( 1 );
}
