
extern float (_cdecl *robot_move)(float pathlen, float speed);
extern void (_cdecl *robot_rotate)(float angle);
extern void (_cdecl *robot_startcleaning)(void);
extern void (_cdecl *robot_endcleaning)(void);
extern unsigned int (_cdecl *robot_getdirtsensor)(void);
extern unsigned int (_cdecl *robot_getpitssensor)(void);
extern unsigned int (_cdecl *robot_getecholocator)(void);
