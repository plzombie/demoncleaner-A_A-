
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <wchar.h>
#include <time.h>

#include <windows.h>

#include <GL/gl.h>
#include <GL/glu.h>
#include "glext.h"
#include "wglext.h"

#define bool int
#define true 1
#define false 0

// Апи робота
typedef struct {
	float (_cdecl *robot_move)(float pathlen, float speed);
	void (_cdecl *robot_rotate)(float angle);
	void (_cdecl *robot_startcleaning)(void);
	void (_cdecl *robot_endcleaning)(void);
	unsigned int (_cdecl *robot_getdirtsensor)(void);
	unsigned int (_cdecl *robot_getpitssensor)(void);
	unsigned int (_cdecl *robot_getecholocator)(void);
} VISUAL_API;

float _cdecl robot_move(float pathlen, float speed);
void _cdecl robot_rotate(float angle);
void _cdecl robot_startcleaning(void);
void _cdecl robot_endcleaning(void);
unsigned int _cdecl robot_getdirtsensor(void);
unsigned int _cdecl robot_getpitssensor(void);
unsigned int _cdecl robot_getecholocator(void);

VISUAL_API visual_api = {robot_move, robot_rotate, robot_startcleaning, robot_endcleaning, robot_getdirtsensor, robot_getpitssensor, robot_getecholocator};
// Апи робота
void (_cdecl *robotfunc)(void);

typedef struct {
	unsigned int mapx, mapy; // Ширина/высота карты

	float robot_x, robot_y; double robot_angle;

	float robot_speed;

	float robot_wastedenergy;
	
	float robot_pathlen, robot_resultpathlen;

	int currdirt, startdirt; // Текущее количество грязи, изначальное количество грязи

	unsigned int robot_dirtsensor, robot_pitssensor, robot_echolocator;

	bool robot_fall, robot_cleaning;

	unsigned char *collmap, *dirtmap, *pitsmap; // Карта столкновений, карта грязи, карта дырок в полу
} ROOM;

// Внутреннее API робота
void robDraw(void);
void robMove(float movelen);
void robEatDust(void);
DWORD WINAPI robStart(LPVOID);
void robUpdatePitsSensor(void);
void robEcholocation(void);
unsigned int robLoadTexture(unsigned int sizex, unsigned int sizey, unsigned int bpp, void *buf);
void robUpdateTexture(unsigned int glid, unsigned int sizex, unsigned int sizey, unsigned int bpp, void *buf);
int robLoadFont(void);
void robDestroyFont(void);
void robPrintText(char *text, unsigned int x, unsigned int y);
int robInitWindow(void);
void robCloseWindow(void);
void robUpdateWindow(void);
float robGetspf(void);
// Внутреннее API робота

unsigned int fps_deltaclocks = 1;
unsigned int fps_lastclocks = 0;

unsigned int fonttexid;

HGLRC hRC = NULL; // Permanent Rendering Context 
HDC hDC = NULL; // Private GDI Device Context 
HWND hWnd = NULL; // Holds Our Window Handle 
HINSTANCE hInstance = NULL; // Holds The Instance Of The Application
unsigned int exitmsg = 0;

int npot_textures; // Поддержка текстур не в степени двойки

// WGL_EXT_swap_control
PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT;

ROOM room;

bool bmpLoad(char *filename, unsigned int wid, unsigned int hei, unsigned int bpp, unsigned char **data);
bool tgaLoad(char *filename, unsigned int wid, unsigned int hei, unsigned int bpp, unsigned char **data);

unsigned int colltexid, dirttexid, pitstexid, robottexid;

HANDLE ghMutex;
//CRITICAL_SECTION cs;

int main(int argc,char *argv[])
{
	unsigned int i;
	FILE *f;
	char tempstr[512], collfn[256], dirtfn[256], pitsfn[256];
	unsigned char *crap;
	void (_cdecl *setvisualapi)(VISUAL_API);
	HMODULE dllhandle;

	HANDLE robothread;
	DWORD ThreadID;

	if(argc < 3)
		{ printf("%s","err: need 2 args\n"); return 60; }

	f = fopen(argv[1], "r");
	if(!f) {
		printf("\nerror!!!21\n");
		return 21;
	}
	if(fscanf(f, "%d %d", &room.mapx, &room.mapy) != 2) {
		fclose(f);
		printf("\nerror!!!22\n");
		return 22;
	}

	if(fscanf(f, "%f %f %lf", &room.robot_x, &room.robot_y, &room.robot_angle) != 3) {
		fclose(f);
		printf("\nerror!!!23\n");
		return 23;
	}

	if(fscanf(f, "%s %s %s", collfn, dirtfn, pitsfn) != 3) {
		fclose(f);
		printf("\nerror!!!24\n");
		return 24;
	}

	fclose(f);

	dllhandle = LoadLibraryA(argv[2]);
	if(!dllhandle) { printf("\nerror!!!11\n"); return 11; }
	setvisualapi = (void(_cdecl *)(VISUAL_API))GetProcAddress(dllhandle, "setvisualapi");
	if(!setvisualapi) {
		setvisualapi = (void(_cdecl *)(VISUAL_API))GetProcAddress(dllhandle, "_setvisualapi");

		if(!setvisualapi) {
			FreeLibrary(dllhandle);
			printf("\nerror!!!12\n");
			return 12;
		}
	}

	robotfunc = (void(_cdecl *)(void))GetProcAddress(dllhandle, "robotfunc");
	if(!robotfunc) {
		robotfunc = (void(_cdecl *)(void))GetProcAddress(dllhandle, "_robotfunc");

		if(!robotfunc) {
			FreeLibrary(dllhandle);
			printf("\nerror!!!13\n");
			return 13;
		}
	}

	setvisualapi(visual_api);

	room.robot_speed = 0;
	room.robot_pathlen = 0;
	room.robot_fall = false;
	room.robot_cleaning = false;
	room.robot_dirtsensor = 0;
	room.robot_echolocator = 0;

	if(!bmpLoad(collfn, room.mapx, room.mapy, 3, &room.collmap)) {
		FreeLibrary(dllhandle);
		printf("\nerror!!!1\n");
		return 1;
	}

	if(!bmpLoad(dirtfn, room.mapx, room.mapy, 3, &room.dirtmap)) {
		FreeLibrary(dllhandle);
		free(room.collmap);
		printf("\nerror!!!2\n");
		return 2;
	}

	room.startdirt = 0;
	for(i = 0; i < (room.mapx*room.mapy)*3; i+= 3)
		room.startdirt += room.dirtmap[i];
	room.currdirt = room.startdirt;

	if(!bmpLoad(pitsfn, room.mapx, room.mapy, 3, &room.pitsmap)) {
		FreeLibrary(dllhandle);
		free(room.collmap);
		free(room.dirtmap);
		printf("\nerror!!!3\n");
		return 3;
	}
	
	if(!robInitWindow()) {
		FreeLibrary(dllhandle);
		free(room.collmap);
		free(room.dirtmap);
		free(room.pitsmap);
		return 0;
	}

	// Загрузка текстур
	crap = (unsigned char *)malloc(4*24*24);
	if(!tgaLoad("media/robot.tga", 24, 24, 4, &crap)) {
		FreeLibrary(dllhandle);
		free(room.collmap);
		free(room.dirtmap);
		free(room.pitsmap);
		robCloseWindow();
		printf("\nerror!!!4\n");
		return 4;
	}
	robottexid = robLoadTexture(24, 24, 4, crap);
	free(crap);

	colltexid = robLoadTexture(room.mapx, room.mapy, 3, room.collmap);
	dirttexid = robLoadTexture(room.mapx, room.mapy, 3, room.dirtmap);
	pitstexid = robLoadTexture(room.mapx, room.mapy, 3, room.pitsmap);

	if(!robLoadFont()) {
		glDeleteTextures(1, (const GLuint *)&colltexid);
		glDeleteTextures(1, (const GLuint *)&dirttexid);
		glDeleteTextures(1, (const GLuint *)&pitstexid);
		glDeleteTextures(1, (const GLuint *)&robottexid);
		free(room.collmap);
		free(room.dirtmap);
		free(room.pitsmap);
		robCloseWindow();
		printf("\nerror!!!5\n");
		return 5;
	}

	// Создание мьютекса и второго потока
	ghMutex = CreateMutex(NULL, FALSE, NULL);
	if (!ghMutex) {
		FreeLibrary(dllhandle);
		glDeleteTextures(1, (const GLuint *)&colltexid);
		glDeleteTextures(1, (const GLuint *)&dirttexid);
		glDeleteTextures(1, (const GLuint *)&pitstexid);
		glDeleteTextures(1, (const GLuint *)&robottexid);
		free(room.collmap);
		free(room.dirtmap);
		free(room.pitsmap);
		robDestroyFont();
		robCloseWindow();
		return 0;
	}
	
	printf("mutex created\n"); 
	
	printf("thread func ptr %d\n", (int)robotfunc);

	robothread = CreateThread( 
                     NULL,       // default security attributes
                     0,          // default stack size
                     (LPTHREAD_START_ROUTINE) robStart, 
                     NULL,       // no thread function arguments
                     0,          // default creation flags
                     &ThreadID);

	printf("thread created, id %d\n", ThreadID);

	if(!robothread) {
		CloseHandle(ghMutex);
		FreeLibrary(dllhandle);
		glDeleteTextures(1, (const GLuint *)&colltexid);
		glDeleteTextures(1, (const GLuint *)&dirttexid);
		glDeleteTextures(1, (const GLuint *)&pitstexid);
		glDeleteTextures(1, (const GLuint *)&robottexid);
		free(room.collmap);
		free(room.dirtmap);
		free(room.pitsmap);
		robDestroyFont();
		robCloseWindow();
		return 0;
	}
	
	printf("start main cycle\n");

	while(!exitmsg) {
		float pathlen;

		if(WaitForSingleObject(ghMutex, INFINITE) != WAIT_OBJECT_0)
			printf("mutex error\n");
		else { 
			pathlen = room.robot_speed*robGetspf();
			if(fabs(pathlen) > room.robot_pathlen) {
				if(room.robot_speed > 0)
					pathlen = room.robot_pathlen;
				else
					pathlen = -room.robot_pathlen;
			}
			//nlPrint(L"Robspeed %f %d %f", room.robot_speed*nGetspf(), (int)(room.robot_speed*nGetspf()), nGetspf() );
			robEatDust();
			robUpdatePitsSensor();
			robMove(pathlen);
			robEcholocation();

			glLoadIdentity();

			glBindTexture(GL_TEXTURE_2D, colltexid);
			glBegin(GL_QUADS);
				glColor4f(1.0,1.0,1.0,1.0);
				glTexCoord2f(0, 1); glVertex3f(0, 0, 0);
				glTexCoord2f(1, 1); glVertex3f(room.mapx, 0, 0);
				glTexCoord2f(1, 0); glVertex3f(room.mapx, room.mapy, 0);
				glTexCoord2f(0, 0); glVertex3f(0, room.mapy, 0);
			glEnd();

			
			if(room.robot_dirtsensor)
				robUpdateTexture(dirttexid, room.mapx, room.mapy, 3, room.dirtmap);
				//nvUpdateTextureFromMemory(dirttexid, room.dirtmap);
			glBindTexture(GL_TEXTURE_2D, dirttexid);
			glBegin(GL_QUADS);
				glColor4f(0,1.0,0,0.25);
				glTexCoord2f(0, 1); glVertex3f(0, 0, 0);
				glTexCoord2f(1, 1); glVertex3f(room.mapx, 0, 0);
				glTexCoord2f(1, 0); glVertex3f(room.mapx, room.mapy, 0);
				glTexCoord2f(0, 0); glVertex3f(0, room.mapy, 0);
			glEnd();

			glBindTexture(GL_TEXTURE_2D, pitstexid);
			glBegin(GL_QUADS);
				glColor4f(1.0,0,0,0.25);
				glTexCoord2f(0, 1); glVertex3f(0, 0, 0);
				glTexCoord2f(1, 1); glVertex3f(room.mapx, 0, 0);
				glTexCoord2f(1, 0); glVertex3f(room.mapx, room.mapy, 0);
				glTexCoord2f(0, 0); glVertex3f(0, room.mapy, 0);
			glEnd();

			robDraw();

			// Вывод информации о роботе
			if(WaitForSingleObject(robothread, 0) == WAIT_OBJECT_0)
				robPrintText("Robot finished", 0, room.mapy);
			else if(room.robot_cleaning)
				robPrintText("Robot cleaning", 0, room.mapy);
			else
				robPrintText("Robot", 0, room.mapy);
			sprintf(tempstr, "Fall into pit: %s, pits sensor %d, Wasted energy: %f, ", room.robot_fall?"yes":"no", robot_getpitssensor(), room.robot_wastedenergy);
			robPrintText(tempstr, 0, room.mapy+16);
			sprintf(tempstr, "Dirt: %d/%d, Dirt sensor %d", room.currdirt, room.startdirt, room.robot_dirtsensor);
			robPrintText(tempstr, 0, room.mapy+32);
			sprintf(tempstr, "Echolocator %d", room.robot_echolocator);
			robPrintText(tempstr, 0, room.mapy+48);

			robUpdateWindow();

			ReleaseMutex(ghMutex);
		} 
	}

	if(WaitForSingleObject(robothread, 0) != WAIT_OBJECT_0)
		TerminateThread(robothread, 0);

	CloseHandle(robothread);

	CloseHandle(ghMutex);
	
	FreeLibrary(dllhandle);

	glDeleteTextures(1, (const GLuint *)&colltexid);
	glDeleteTextures(1, (const GLuint *)&dirttexid);
	glDeleteTextures(1, (const GLuint *)&pitstexid);
	glDeleteTextures(1, (const GLuint *)&robottexid);

	free(room.collmap);
	free(room.dirtmap);
	free(room.pitsmap);

	robDestroyFont();

	robCloseWindow();

	return 0;
}

// API робота

float _cdecl robot_move(float pathlen, float speed)
{
	float resultpathlen = 0;

	if(WaitForSingleObject(ghMutex, INFINITE) == WAIT_OBJECT_0) {
		if(speed > 30) speed = 30;
		if(speed <-30) speed = -30;
		room.robot_speed = speed;
		room.robot_pathlen = pathlen;
		if(!ReleaseMutex(ghMutex)) printf("mutex error\n");
		while(1) {
			WaitForSingleObject(ghMutex, INFINITE);
			if(room.robot_fall) {
				ReleaseMutex(ghMutex);
				break;
			}
			if(room.robot_pathlen == 0.0) {
				resultpathlen = room.robot_resultpathlen;
				room.robot_resultpathlen = 0;
				ReleaseMutex(ghMutex);
				break;
			}
			ReleaseMutex(ghMutex);
		}
	}

	return resultpathlen;
}

void _cdecl robot_rotate(float angle)
{
	if(WaitForSingleObject(ghMutex, INFINITE) == WAIT_OBJECT_0) { 
		room.robot_angle += angle;
		robEcholocation(); // Пользователь может вызвать robot_getecholocator до того, как вызовется функция robEcholocation();, поэтому нужно вызвать robEcholocation(); сейчас
		if(!ReleaseMutex(ghMutex)) printf("mutex error\n");
	} 
}

void _cdecl robot_startcleaning(void)
{
	if(WaitForSingleObject(ghMutex, INFINITE) == WAIT_OBJECT_0) { 
		room.robot_cleaning = true;
		robEatDust(); // Пользователь может вызвать robot_getdirtsensor или robot_endcleaning до того, как будет вызвана robEatDust();, лучше вызвать сразу
		if(!ReleaseMutex(ghMutex)) printf("mutex error\n");
	} 
}

void _cdecl robot_endcleaning(void)
{
	if(WaitForSingleObject(ghMutex, INFINITE) == WAIT_OBJECT_0) { 
		room.robot_cleaning = false;
		if(!ReleaseMutex(ghMutex)) printf("mutex error\n");
	} 
}

unsigned int _cdecl robot_getdirtsensor(void)
{
	unsigned int dirtsensor = 0;

	if(WaitForSingleObject(ghMutex, INFINITE) == WAIT_OBJECT_0) { 
		dirtsensor = room.robot_dirtsensor;
		if(!ReleaseMutex(ghMutex)) printf("mutex error\n");
	} 

	return dirtsensor;
}

unsigned int _cdecl robot_getpitssensor(void)
{
	unsigned int pitssensor = 0;

	if(WaitForSingleObject(ghMutex, INFINITE) == WAIT_OBJECT_0) { 
		pitssensor = room.robot_pitssensor;
		if(!ReleaseMutex(ghMutex)) printf("mutex error\n");
	} 

	return pitssensor;
}

unsigned int _cdecl robot_getecholocator(void)
{
	unsigned int echolocator = 0;

	if(WaitForSingleObject(ghMutex, INFINITE) == WAIT_OBJECT_0) { 
		echolocator = room.robot_echolocator;
		if(!ReleaseMutex(ghMutex)) printf("mutex error\n");
	} 

	return echolocator;
}

// Внутреннее API робота

DWORD WINAPI robStart(LPVOID lpParam)
{
	robotfunc();

	return TRUE; 
}

void robUpdatePitsSensor(void)
{
	int minx, miny, maxx, maxy;
	unsigned int i, j, pits = 0;

	minx = room.robot_x-12;
	if(minx < 0) minx = 0;

	miny = room.robot_y-12;
	if(miny < 0) miny = 0;
	
	maxx = room.robot_x+12;
	if(maxx >= room.mapx) maxx = room.mapx-1;

	maxy = room.robot_y+12;
	if(maxy >= room.mapy) maxy = room.mapy-1;

	for(i = miny; i < maxy; i++)
		for(j = minx; j < maxx; j++) {
			if(room.pitsmap[(room.mapy-i-1)*3*room.mapx+j*3])
				pits++;
		}

	if(pits > 24*24/2)
		room.robot_fall = true;

	if(pits)
		room.robot_pitssensor = 1;
	else
		room.robot_pitssensor = 0;
}

void robEcholocation(void)
{
	unsigned int len = 0xFFFFFFFF;
	float sina, cosa, i, j;

	cosa = cos(room.robot_angle);
	sina = sin(room.robot_angle);

	i = room.robot_y;
	j = room.robot_x;

	while(1) {
		if( (i < 0) || (i > (room.mapy-1)) || (j < 0) || (j > (room.mapx-1)) )
			break;
		if(!room.collmap[(room.mapy-(int)i-1)*room.mapx*3+(int)j*3]) {
			len = sqrt((i-room.robot_y)*(i-room.robot_y)+(j-room.robot_x)*(j-room.robot_x))+0.5;
			break;
		}
		i += sina;
		j += cosa;
	}
	room.robot_echolocator = len;
}

void robEatDust(void)
{
	int i, j, minx, maxx, miny, maxy;
	unsigned int dirtsensor;

	if(room.robot_dirtsensor > (int)(255*robGetspf()))
		room.robot_dirtsensor -= (int)(255*robGetspf());
	else
		room.robot_dirtsensor = 0;

	if(!room.robot_cleaning) return;

	room.robot_wastedenergy += 12*robGetspf();

	minx = room.robot_x-12;
	if(minx < 0) minx = 0;

	miny = room.robot_y-12;
	if(miny < 0) miny = 0;
	
	maxx = room.robot_x+12;
	if(maxx >= room.mapx) maxx = room.mapx-1;

	maxy = room.robot_y+12;
	if(maxy >= room.mapy) maxy = room.mapy-1;
	
	dirtsensor = 0;

	for(i = miny; i < maxy; i++)
		for(j = minx; j < maxx; j++) {
			if(	sqrt((i-room.robot_y)*(i-room.robot_y)+(j-room.robot_x)*(j-room.robot_x)) <= 12) {
				dirtsensor += room.dirtmap[(room.mapy-i-1)*room.mapx*3+j*3];
				room.dirtmap[(room.mapy-i-1)*room.mapx*3+j*3] = 0;
				room.dirtmap[(room.mapy-i-1)*room.mapx*3+j*3+1] = 0;
				room.dirtmap[(room.mapy-i-1)*room.mapx*3+j*3+2] = 0;
			}
		}

	if(dirtsensor) room.robot_dirtsensor = 255;

	room.currdirt -= dirtsensor;
}

void robMove(float movelen)
{
	int i, j, minx, maxx, miny, maxy;
	float newrobot_x, newrobot_y;

	if(room.robot_fall) return;

	newrobot_x = room.robot_x + movelen*cos(room.robot_angle);
	newrobot_y = room.robot_y + movelen*sin(room.robot_angle);

	minx = newrobot_x-12;
	if(minx < 0) minx = 0;

	miny = newrobot_y-12;
	if(miny < 0) miny = 0;
	
	maxx = newrobot_x+12;
	if(maxx >= room.mapx) maxx = room.mapx-1;

	maxy = newrobot_y+12;
	if(maxy >= room.mapy) maxy = room.mapy-1;

	for(i = miny; i < maxy; i++)
		for(j = minx; j < maxx; j++) {
			if(	sqrt((i-newrobot_y)*(i-newrobot_y)+(j-newrobot_x)*(j-newrobot_x)) <= 12) {
				if(!room.collmap[(room.mapy-i-1)*room.mapx*3+j*3]) {
					if(movelen > 0.5)
						robMove(movelen/2);
					else
						room.robot_pathlen = 0;
					return;
				}
			}
		}

	room.robot_pathlen -= fabs(movelen);
	room.robot_wastedenergy += fabs(movelen);
	//printf("pathlen %f\n", room.robot_pathlen);
	room.robot_resultpathlen += fabs(movelen);
	room.robot_x = newrobot_x;
	room.robot_y = newrobot_y;
}

void robDraw(void)
{
	float x1, y1, x2, y2, x3, y3, x4, y4;
	/*
		Порядок вывода вершин (v[])

		x2y2-----x3y3
		|			|
		|			|
		|			|
		|			|
		x1y1-----x4y4


		робот - круг, диаметром 24 пикселя
		для наглядности рисуется как квадрат

		один метр кв. - 64*64 пикселя

		angle == 0 - Направление влево
		angle == PI/2 - Направление вниз
		и т.д.
	*/

	x1 = room.robot_x+ -12*cos(room.robot_angle)+12*sin(room.robot_angle); // -16
	y1 = room.robot_y+ -12*sin(room.robot_angle)-12*cos(room.robot_angle); // -8

	x2 = room.robot_x+ -12*cos(room.robot_angle)-12*sin(room.robot_angle); // -16
	y2 = room.robot_y+ -12*sin(room.robot_angle)+12*cos(room.robot_angle); // 8

	x3 = room.robot_x+ 12*cos(room.robot_angle)-12*sin(room.robot_angle); // 16
	y3 = room.robot_y+ 12*sin(room.robot_angle)+12*cos(room.robot_angle); // 8

	x4 = room.robot_x+ 12*cos(room.robot_angle)+12*sin(room.robot_angle); // 16
	y4 = room.robot_y+ 12*sin(room.robot_angle)-12*cos(room.robot_angle); // -8

	glBindTexture(GL_TEXTURE_2D, robottexid);
	glBegin(GL_QUADS);
		glColor4f(1.0,1.0,1.0,1.0);
		glTexCoord2f(0, 1); glVertex3f(x1, y1, 0);
		glTexCoord2f(0, 0); glVertex3f(x2, y2, 0);
		glTexCoord2f(1, 0); glVertex3f(x3, y3, 0);
		glTexCoord2f(1, 1); glVertex3f(x4, y4, 0);
	glEnd();
}

unsigned int robLoadTexture(unsigned int sizex, unsigned int sizey, unsigned int bpp, void *buf)
{
	unsigned int glid, freebuf = 0;

	glGenTextures(1, &glid);
	glBindTexture(GL_TEXTURE_2D, glid);

	if((sizex&(sizex-1)) || (sizey&(sizey-1))) {
		if(!strstr((char*)glGetString(GL_EXTENSIONS),"GL_ARB_texture_non_power_of_two" )) {
			int oldsizex, oldsizey;
			void *oldbuf;

			oldbuf = buf;
			oldsizex = sizex;
			oldsizey = sizey;

			if((sizex&(sizex-1))) sizex = 1 << ((int)(log((float)sizex)/log(2.0))+1);
			if((sizey&(sizey-1))) sizey = 1 << ((int)(log((float)sizey)/log(2.0))+1);

			buf = malloc(sizex*sizey*bpp);

			if(bpp == 3)
				gluScaleImage(GL_RGB, oldsizex, oldsizey, GL_UNSIGNED_BYTE, oldbuf, sizex, sizey, GL_UNSIGNED_BYTE, buf);
			else
				gluScaleImage(GL_RGBA, oldsizex, oldsizey, GL_UNSIGNED_BYTE, oldbuf, sizex, sizey, GL_UNSIGNED_BYTE, buf);

			freebuf = 1;
		}
	}

	if(bpp == 3) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, sizex, sizey, 0, GL_RGB, GL_UNSIGNED_BYTE, buf);
	} else {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, sizex, sizey, 0, GL_BGRA, GL_UNSIGNED_BYTE, buf);
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	if(freebuf) free(buf);

	return glid;
}

// Изменение размера изображения
void PointResize(unsigned bpp, unsigned int osx, unsigned int osy, unsigned int nsx, unsigned int nsy, char *inbuf, char *outbuf)
{
	unsigned int i, j, k;
	float dx, dy;
	float ox = 0, oy = 0;

	dx = (float)osx/nsx;
	dy = (float)osy/nsy;

	for(j = 0; j < nsy; j++) {
		for(i = 0; i < nsx; i++) {
			for(k = 0; k < bpp; k++) {
				outbuf[i*bpp+j*bpp*nsx+k] = inbuf[((int)ox)*bpp+((int)oy)*bpp*osx+k];
			}
			ox += dx;
		}
		ox = 0;
		oy += dy;
	}
}

void robUpdateTexture(unsigned int glid, unsigned int sizex, unsigned int sizey, unsigned int bpp, void *buf)
{
	unsigned int freebuf = 0;

	glBindTexture(GL_TEXTURE_2D, glid);

	if((sizex&(sizex-1)) || (sizey&(sizey-1))) {
		if(!npot_textures) {
			int oldsizex, oldsizey;
			void *oldbuf;

			oldbuf = buf;
			oldsizex = sizex;
			oldsizey = sizey;

			if((sizex&(sizex-1))) sizex = 1 << ((int)(log((float)sizex)/log(2.0))+1);
			if((sizey&(sizey-1))) sizey = 1 << ((int)(log((float)sizey)/log(2.0))+1);

			buf = malloc(sizex*sizey*bpp);

			if(bpp == 3)
				PointResize(3, oldsizex, oldsizey, sizex, sizey, oldbuf, buf);
				//gluScaleImage(GL_RGB, oldsizex, oldsizey, GL_UNSIGNED_BYTE, oldbuf, sizex, sizey, GL_UNSIGNED_BYTE, buf);
			else
				PointResize(4, oldsizex, oldsizey, sizex, sizey, oldbuf, buf);
				//gluScaleImage(GL_RGBA, oldsizex, oldsizey, GL_UNSIGNED_BYTE, oldbuf, sizex, sizey, GL_UNSIGNED_BYTE, buf);

			freebuf = 1;
		}
	}

	if(bpp == 3) {
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, sizex, sizey, GL_BGR, GL_UNSIGNED_BYTE, buf);
	} else {
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, sizex, sizey, GL_BGRA, GL_UNSIGNED_BYTE, buf);
	}

	if(freebuf) free(buf);
}


int robLoadFont(void)
{
	unsigned char *fontdata;
	
	if(!bmpLoad("media/ackfont.bmp", 192, 72, 3, &fontdata))
		return 0;

	fonttexid = robLoadTexture(192, 72, 3, fontdata);

	free(fontdata);

	return 1;
}

void robDestroyFont(void)
{
	glDeleteTextures(1, (const GLuint *)&fonttexid);
}

void robPrintText(char *text, unsigned int x, unsigned int y)
{
	unsigned int i;

	glLoadIdentity(); 

	glTranslatef(x, y, 0.0f);
	
	glBindTexture(GL_TEXTURE_2D, fonttexid);

	for(i = 0; i < strlen(text); i++) {
		glBegin(GL_QUADS);
			glColor4f(1.0,1.0,1.0,1.0);
			glTexCoord2f((text[i]%32)/32.0, (8-text[i]/32)/8.0); glVertex3f(i*6, 0, 0); // Может для х сделать обратно
			glTexCoord2f((text[i]%32+1)/32.0, (8-text[i]/32)/8.0); glVertex3f(i*6+6, 0, 0);
			glTexCoord2f((text[i]%32+1)/32.0, (8-text[i]/32-1)/8.0); glVertex3f(i*6+6, 9, 0);
			glTexCoord2f((text[i]%32)/32.0, (8-text[i]/32-1)/8.0); glVertex3f(i*6, 9, 0);
		glEnd();
	}
}


LRESULT CALLBACK WndProc(	HWND	hWnd,			// Handle For This Window
							UINT	uMsg,			// Message For This Window
							WPARAM	wParam,			// Additional Message Information
							LPARAM	lParam)			// Additional Message Information
{
	switch (uMsg)									// Check For Windows Messages
	{
		case WM_CLOSE:								// Did We Receive A Close Message?
		{
			PostQuitMessage(0);						// Send A Quit Message
			return 0;								// Jump Back
		}
	}

	// Pass All Unhandled Messages To DefWindowProc
	return DefWindowProc(hWnd,uMsg,wParam,lParam);
}


int robInitWindow(void)
{
	GLuint		PixelFormat;			// Holds The Results After Searching For A Match
	WNDCLASS	wc;						// Windows Class Structure
	DWORD		dwExStyle;				// Window Extended Style
	DWORD		dwStyle;				// Window Style
	RECT		WindowRect;				// Grabs Rectangle Upper Left / Lower Right Values
	long width, height;

	static	PIXELFORMATDESCRIPTOR pfd=				// pfd Tells Windows How We Want Things To Be
	{
		sizeof(PIXELFORMATDESCRIPTOR),				// Size Of This Pixel Format Descriptor
		1,											// Version Number
		PFD_DRAW_TO_WINDOW |						// Format Must Support Window
		PFD_SUPPORT_OPENGL |						// Format Must Support OpenGL
		PFD_DOUBLEBUFFER,							// Must Support Double Buffering
		PFD_TYPE_RGBA,								// Request An RGBA Format
		32,										// Select Our Color Depth
		0, 0, 0, 0, 0, 0,							// Color Bits Ignored
		0,											// No Alpha Buffer
		0,											// Shift Bit Ignored
		0,											// No Accumulation Buffer
		0, 0, 0, 0,									// Accumulation Bits Ignored
		16,											// 16Bit Z-Buffer (Depth Buffer)  
		0,											// No Stencil Buffer
		0,											// No Auxiliary Buffer
		PFD_MAIN_PLANE,								// Main Drawing Layer
		0,											// Reserved
		0, 0, 0										// Layer Masks Ignored
	};

	width = room.mapx;
	height = room.mapy+64;

	WindowRect.left=(long)0;			// Set Left Value To 0
	WindowRect.right=(long)width;		// Set Right Value To Requested Width
	WindowRect.top=(long)0;				// Set Top Value To 0
	WindowRect.bottom=(long)height;		// Set Bottom Value To Requested Height


	hInstance			= GetModuleHandle(NULL);				// Grab An Instance For Our Window
	wc.style			= CS_HREDRAW | CS_VREDRAW | CS_OWNDC;	// Redraw On Size, And Own DC For Window.
	wc.lpfnWndProc		= (WNDPROC) WndProc;					// WndProc Handles Messages
	wc.cbClsExtra		= 0;									// No Extra Window Data
	wc.cbWndExtra		= 0;									// No Extra Window Data
	wc.hInstance		= hInstance;							// Set The Instance
	wc.hIcon			= LoadIcon(NULL, IDI_WINLOGO);			// Load The Default Icon
	wc.hCursor			= LoadCursor(NULL, IDC_ARROW);			// Load The Arrow Pointer
	wc.hbrBackground	= NULL;									// No Background Required For GL
	wc.lpszMenuName		= NULL;									// We Don't Want A Menu
	wc.lpszClassName	= "OpenGL";								// Set The Class Name

	if (!RegisterClass(&wc))									// Attempt To Register The Window Class
	{
		printf("Failed To Register The Window Class.\n");
		return 0;											// Return FALSE
	}

	//dwExStyle=WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;			// Window Extended Style
	//dwStyle=WS_OVERLAPPEDWINDOW;							// Windows Style
	dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE; // Window Extended Style
	dwStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX; // Windows Style

	AdjustWindowRectEx(&WindowRect, dwStyle, FALSE, dwExStyle);		// Adjust Window To True Requested Size

	// Create The Window
	if (!(hWnd=CreateWindowEx(	dwExStyle,							// Extended Style For The Window
								"OpenGL",							// Class Name
								"Robot",								// Window Title
								dwStyle |							// Defined Window Style
								WS_CLIPSIBLINGS |					// Required Window Style
								WS_CLIPCHILDREN,					// Required Window Style
								0, 0,								// Window Position
								WindowRect.right-WindowRect.left,	// Calculate Window Width
								WindowRect.bottom-WindowRect.top,	// Calculate Window Height
								NULL,								// No Parent Window
								NULL,								// No Menu
								hInstance,							// Instance
								NULL)))								// Dont Pass Anything To WM_CREATE
	{
		robCloseWindow();								// Reset The Display
		printf("Window Creation Error.\n");
		return 0;								// Return FALSE
	}
	
	if (!(hDC=GetDC(hWnd)))							// Did We Get A Device Context?
	{
		robCloseWindow();								// Reset The Display
		printf("Can't Create A GL Device Context.\n");
		return 0;								// Return FALSE
	}

	if (!(PixelFormat=ChoosePixelFormat(hDC,&pfd)))	// Did Windows Find A Matching Pixel Format?
	{
		robCloseWindow();								// Reset The Display
		printf("Can't Find A Suitable PixelFormat.\n");
		return 0;								// Return FALSE
	}

	if(!SetPixelFormat(hDC,PixelFormat,&pfd))		// Are We Able To Set The Pixel Format?
	{
		robCloseWindow();								// Reset The Display
		printf("Can't Set The PixelFormat.\n");
		return 0;								// Return FALSE
	}

	if (!(hRC=wglCreateContext(hDC)))				// Are We Able To Get A Rendering Context?
	{
		robCloseWindow();								// Reset The Display
		printf("Can't Create A GL Rendering Context.\n");
		return 0;								// Return FALSE
	}

	if(!wglMakeCurrent(hDC,hRC))					// Try To Activate The Rendering Context
	{
		robCloseWindow();								// Reset The Display
		printf("Can't Activate The GL Rendering Context.\n");
		return 0;								// Return FALSE
	}

	ShowWindow(hWnd,SW_SHOW);						// Show The Window
	SetForegroundWindow(hWnd);						// Slightly Higher Priority
	SetFocus(hWnd);									// Sets Keyboard Focus To The Window
	
	wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
	if(wglSwapIntervalEXT)
		wglSwapIntervalEXT(1);

	glViewport(0,0,width,height);						// Reset The Current Viewport

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0,width,height,0.0,-1.0,1.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Настройка OpenGL
	glEnable(GL_TEXTURE_2D);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glShadeModel(GL_SMOOTH);
	glDepthFunc(GL_LEQUAL);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_NICEST);
	glPixelStorei(GL_UNPACK_ALIGNMENT,4);
	glPixelStorei(GL_PACK_ALIGNMENT,4);
	glCullFace(GL_FRONT);
	glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
	
	glClearDepth(1.0);
	glClearColor(0.25, 0.25, 0.25, 0.0f);

	npot_textures = (int)strstr((char*)glGetString(GL_EXTENSIONS),"GL_ARB_texture_non_power_of_two" );

	return 1;									// Success
}

void robCloseWindow(void)
{
	if (hRC)											// Do We Have A Rendering Context?
	{
		if (!wglMakeCurrent(NULL,NULL))					// Are We Able To Release The DC And RC Contexts?
		{
			printf("Release Of DC And RC Failed.\n");
		}

		if (!wglDeleteContext(hRC))						// Are We Able To Delete The RC?
		{
			printf("Release Rendering Context Failed.\n");
		}
		hRC=NULL;										// Set RC To NULL
	}

	if (hDC && !ReleaseDC(hWnd,hDC))					// Are We Able To Release The DC
	{
		printf("Release Device Context Failed.\n");
		hDC=NULL;										// Set DC To NULL
	}

	if (hWnd && !DestroyWindow(hWnd))					// Are We Able To Destroy The Window?
	{
		printf("Could Not Release hWnd.\n");
		hWnd=NULL;										// Set hWnd To NULL
	}

	if (!UnregisterClass("OpenGL",hInstance))			// Are We Able To Unregister Class
	{
		printf("Could Not Unregister Class.\n");
		hInstance=NULL;									// Set hInstance To NULL
	}
}

void robUpdateWindow(void)
{
	static MSG m;
	/*Выпили меня в релиз версии*/ int error;

	if(PeekMessage(&m, NULL, 0, 0, PM_REMOVE)) {
		if(m.message == WM_QUIT)
			exitmsg = 1;
		TranslateMessage(&m); // Translate The Message
		DispatchMessage(&m); // Dispatch The Message
	}

	if(!wglSwapLayerBuffers(hDC, WGL_SWAP_MAIN_PLANE))
		SwapBuffers(hDC);

	glClear(GL_COLOR_BUFFER_BIT);

	fps_deltaclocks = clock()-fps_lastclocks;
	fps_lastclocks = clock();

	/*Выпили меня в релиз версии*/ error = glGetError(); if(error) printf("ogl error: %d", error);
}

float robGetspf(void)
{
	return (float)fps_deltaclocks/CLOCKS_PER_SEC;
}

// Загрузка BMP картинок
#define BITMAP_ID 0x4D42

bool bmpLoad(char *filename, unsigned int wid, unsigned int hei, unsigned int bpp, unsigned char **data)
{
	BITMAPFILEHEADER fheader;
	BITMAPINFOHEADER iheader;
	FILE *f;

	f = fopen(filename, "rb");

	if(wid % 4) return false;

	if(!f) return false;

	fread(&fheader, sizeof(fheader), 1, f);
	fread(&iheader, sizeof(iheader), 1, f);

	if(fheader.bfType != BITMAP_ID) return false;

	if(iheader.biBitCount != 8*bpp) return false;

	if(iheader.biWidth != wid || iheader.biHeight != hei) return false;

	*data = (unsigned char *)malloc(bpp*wid*hei);

	fseek(f, fheader.bfOffBits, SEEK_SET);

	fread(*data, 1, bpp*wid*hei, f);

	fclose(f);

	return true;
}

#pragma pack (push, 1)
typedef struct {
	unsigned char IdLeight;		//Длина информации после заголовка 
	unsigned char ColorMap;		//Идентификатор наличия цветовой карты (0 - нет, 1 - есть)
	unsigned char DataType;		//Тип сжатия
								//   0 - No Image Data Included
								//   1 - Uncompressed, Color-mapped Image
								//   2 - Uncompressed, True-color Image
								//   3 - Uncompressed, Black-and-white Image
								//   9 - Run-length encoded, Color-mapped Image
								//   10 - Run-length encoded, True-color Image
								//   11 - Run-length encoded, Black-and-white Image
	unsigned short CmapStart;	//Начало палитры
	unsigned short CmapLength;	//Длина палитры
	unsigned char CmapDepth;	//Глубина элементов палитры (15, 16, 24, 32)
	unsigned short X_Origin;	//Начало изображения по оси X
	unsigned short Y_Origin;	//Начало изображения по оси Y
	unsigned short TGAWidth;	//Ширина изображения
	unsigned short TGAHeight;	//Высота изображения
	unsigned char BitPerPel;	//Кол-во бит на пиксель (8, 16, 24, 32)
	unsigned char Description;	//Описание
} TGAHEADER;
#pragma pack (pop)

bool tgaLoad(char *filename, unsigned int wid, unsigned int hei, unsigned int bpp, unsigned char **data)
{
	TGAHEADER header;
	FILE *f;

	f = fopen(filename, "rb");

	if(wid % 4) return false;

	if(!f) return false;

	fread(&header, sizeof(header), 1, f);

	if(header.DataType != 2) return false;

	if(header.BitPerPel != 8*bpp) return false;

	if(header.TGAWidth != wid || header.TGAHeight != hei) return false;

	*data = (unsigned char *)malloc(bpp*wid*hei);

	fseek(f, header.IdLeight, SEEK_CUR);

	fread(*data, 1, bpp*wid*hei, f);

	fclose(f);

	return true;
}
