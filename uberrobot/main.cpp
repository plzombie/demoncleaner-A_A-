/*
	������� ��������:

		���� 3 ������:
		����� Robot �������� �� ������������ ������
		����� AI ��������� �������. �� ����� �������� ���, ������� ��������� �������(�����/map) �� ����� 6�6 �������� � ��������� ������������ ����� ����� �������
		����� ThePathfinderGeneral �������� �� ���������� ����

		� ������� newmain ���������� ����, ������� �������� ������� update ������ AI.
		������� update ���������� true, ���� ������ ���������, ����� false
		���� update ���������� true, �� ���������� ����� �� ����� � ����� ��������� ������

*/

#include "robotapi.h"

#include <stdio.h>

#include <windows.h>

#define PI 3.1415926535

#include <math.h>

// ����� ���������� �������
class Robot
{
	private:
		bool cleaning;

		float robx, roby; double robangle;
	public:
		float move(float lenght, float speed);
		void rotate(float angle);

		float getposition(float &x, float &y)
		{
			x = robx; y = roby;
		}
		double getangle(void)
		{
			return robangle;
		}

		void startcleaning(void)
		{
			robot_startcleaning();
			cleaning = true;
		}
		void endcleaning(void)
		{
			robot_endcleaning();
			cleaning = false;
		}
		bool iscleaning(void)
		{
			return cleaning;
		}

		unsigned int getdirtsensor(void)
		{
			return robot_getdirtsensor();
		}
		unsigned int getpitssensor(void)
		{
			return robot_getpitssensor();
		}
		unsigned int getecholocator(void)
		{
			return robot_getecholocator();
		}

		Robot()
		{
			cleaning = false;

			robx = 0; roby = 0; robangle = 0;
		}
};

float Robot::move(float lenght, float speed)
{
	float result;

	result = robot_move(lenght, speed);

	// ���������� � ������� ������ ���������� �� ����
	// ��������, ��� ����, �� ������� ������� �����, ��������� ������ ������� �������
	robx += cos(robangle)*result;
	roby += sin(robangle)*result;

	return result;
}

void Robot::rotate(float angle)
{
	robot_rotate(angle);

	robangle += angle;
}

typedef struct {
	int type; // 0 - �� ��������, 1 - �������� � �������, 2 - ����������
} TILE;

typedef struct {
	int com; // ������� (0 - ����� ������, 1 - �������, 2 - ��������)
	int param; // �������� (����� ���� ��� ����)
	int param2;
} COMMAND;

typedef struct { // ��� ��������� ������ ���� (A star)
	int inlist; // 0 - �� � �������, 1 - � ��������, 2 - � ��������
	int motherx, mothery; // ���������� ����������� ������ (�� ������� ������ � �������)
	int nextx, nexty; // ���������� ��������� ������ (����������� ������ ���� ���������� � ���� �� ��������� ����� � ��������)
	int len; // ����� ���� �� �������� ������
} VISITEDTILE;

// ����� �������� ����
class ThePathfinderGeneral {
	public:
		COMMAND *commas;

		bool MakeMePathPls(TILE *map, unsigned int sx, unsigned int sy, unsigned int ex, unsigned int ey, unsigned int angle);

		ThePathfinderGeneral()
		{
			commas = new COMMAND[800*800];
			(*commas).com = 0;
		}
		~ThePathfinderGeneral()
		{
			delete [] commas;
		}
};


// ����� ����
// sx, sy - ��������� ����������
// ex, ey - �������� ����������
// angle - ��������� ���� (�������� 0,1,2,3)
bool ThePathfinderGeneral::MakeMePathPls(TILE *map, unsigned int sx, unsigned int sy, unsigned int ex, unsigned int ey, unsigned int angle)
{
	COMMAND *comma;
	VISITEDTILE *tiles;
	int curx, cury, curl; // ��������� ��������� ������
	int searchminx, searchminy, searchmaxx, searchmaxy; // ������� ������

	// ���� ����

	// �������� �������� ������ �� ��������� ������ ������
	for(int i = ey-2; i < ey+2; i++) {
		for(int j = ex-2; j < ex+2; j++) {
			if(map[j+i*800].type == 2) {
				commas->com = 0;

				return false;
			}
		}
	}

	// ���������� ��������� ������ ���� � �������� ������ (���� ����� ����� �� ��������� ������ � ��������)

	tiles = new VISITEDTILE[800*800];
	memset(tiles, 0, 800*800*sizeof(VISITEDTILE));

	tiles[sx+sy*800].inlist = 1;
	tiles[sx+sy*800].len = 0;

	searchminx = sx;
	searchmaxx = sx;
	searchminy = sy;
	searchmaxy = sy;

	while(1) {
		int t1 = 0, t2 = 0;
		bool haveleft = true, haveright = true, haveup = true, havedown = true; // ������� ������, � ������� ����� ���������, � ������������ �������

		curx = -1;
		cury = -1;
		curl = 0xFFFFFF;

		// ����� � �������� ������ ������ � ���������� ����������
		for(int i = searchminy; i <= searchmaxy; i++) {//for(int i = 2; i < 800-1; i++) {
			for(int j = searchminx; j <= searchmaxx; j++) {//for(int j = 2; j < 800-1; j++) {
				t1++;
				if(tiles[j+i*800].inlist == 1) {
					t2++;
					if( (tiles[j+i*800].len+abs((int)ex-j)+abs((int)ey-i)) < (curl+abs((int)ex-curx)+abs((int)ey-cury)) ) {
						curl = tiles[j+i*800].len;
						curx = j;
						cury = i;
					}
				}
			}
		}

		if(curx == -1) { // ������ ��� ������ � �������� ������
			commas->com = 0;

			delete [] tiles;

			return false;
		}

		if(curx == ex && cury == ey) break; // ����� �������� ������ � �������� ������. ���� ������, ������� �� �����

		// ���������� ��������� ������ � �������� ������
		tiles[curx+cury*800].inlist = 2;

		if(curl >= 400) continue; // ������������� ����������� ����� ���� �� 400 ������
		
		// �������� ������ �� ����������� ������������ ������ � ���
		if( curx-1 < 2 || tiles[curx-1+cury*800].inlist == 2 || (tiles[curx-1+cury*800].inlist == 1 && tiles[curx-1+cury*800].len <= curl) ) haveleft = false;
		if( curx+1 >= 799 || tiles[curx+1+cury*800].inlist == 2 || (tiles[curx+1+cury*800].inlist == 1 && tiles[curx+1+cury*800].len <= curl) ) haveright = false;
		if( cury-1 < 2 || tiles[curx+(cury-1)*800].inlist == 2 || (tiles[curx+(cury-1)*800].inlist == 1 && tiles[curx+(cury-1)*800].len <= curl) ) haveup = false;
		if( cury+1 >= 799 || tiles[curx+(cury+1)*800].inlist == 2 || (tiles[curx+(cury+1)*800].inlist == 1 && tiles[curx+(cury+1)*800].len <= curl) ) havedown = false;

		if(haveleft || haveright) {
			for(int i = cury-2; i < cury+2; i++) {
				if(map[curx-3+i*800].type == 2) haveleft = false;
				if(map[curx+2+i*800].type == 2) haveright = false;
			}
		}

		if(haveup || havedown) {
			for(int j = curx-2; j < curx+2; j++) {
				if(map[j+(cury-3)*800].type == 2) haveup = false;
				if(map[j+(cury+2)*800].type == 2) havedown = false;
			}
		}

		// ���������� �������� ������ � �������� ������
		if(haveleft) {
			if(searchminx > curx-1) searchminx = curx-1;
			tiles[curx-1+cury*800].inlist = 1;
			tiles[curx-1+cury*800].len = curl+1;
			tiles[curx-1+cury*800].motherx = curx;
			tiles[curx-1+cury*800].mothery = cury;
		}

		if(haveright) {
			if(searchmaxx < curx+1) searchmaxx = curx+1;
			tiles[curx+1+cury*800].inlist = 1;
			tiles[curx+1+cury*800].len = curl+1;
			tiles[curx+1+cury*800].motherx = curx;
			tiles[curx+1+cury*800].mothery = cury;
		}

		if(haveup) {
			if(searchminy > cury-1) searchminy = cury-1;
			tiles[curx+(cury-1)*800].inlist = 1;
			tiles[curx+(cury-1)*800].len = curl+1;
			tiles[curx+(cury-1)*800].motherx = curx;
			tiles[curx+(cury-1)*800].mothery = cury;
		}

		if(havedown) {
			if(searchmaxy < cury+1) searchmaxy = cury+1;
			tiles[curx+(cury+1)*800].inlist = 1;
			tiles[curx+(cury+1)*800].len = curl+1;
			tiles[curx+(cury+1)*800].motherx = curx;
			tiles[curx+(cury+1)*800].mothery = cury;
		}
	}

	// ��������� ������ ������ ��� ������
	comma = commas;
	while(curx != sx || cury != sy) {
		int tempx, tempy;
		tiles[tiles[curx+cury*800].motherx+tiles[curx+cury*800].mothery*800].nextx = curx;
		tiles[tiles[curx+cury*800].motherx+tiles[curx+cury*800].mothery*800].nexty = cury;

		tempx = curx; tempy = cury;
		curx = tiles[tempx+tempy*800].motherx;
		cury = tiles[tempx+tempy*800].mothery;
	}


	while(curx != ex || cury != ey) {
		int nextx, nexty;

		nextx = tiles[curx+cury*800].nextx;
		nexty = tiles[curx+cury*800].nexty;

		if(nextx < curx) {
			if(angle != 2) {
				comma->com = 1;
				comma->param = 2-angle;
				angle = 2;
				comma++;
			}
		} else if (nextx > curx) {
			if(angle != 0) {
				comma->com = 1;
				comma->param = -(int)angle;
				angle = 0;
				comma++;
			}
		} else if(nexty < cury) {
			if(angle != 3) {
				comma->com = 1;
				comma->param = 3-angle;
				angle = 3;
				comma++;
			}
		} else if (nexty > cury) {
			if(angle != 1) {
				comma->com = 1;
				comma->param = 1-angle;
				angle = 1;
				comma++;
			}
		}

		comma->com = 2;
		comma->param = 1;
		comma->param2 = 30;
		comma++;

		curx = nextx;
		cury = nexty;
	}

	comma->com = 0;

	delete [] tiles;

	return true;
}

class AI {
	private:
		Robot robot; // ����� ���������� �������
		ThePathfinderGeneral path; // ����� �������� ����
		TILE *map;

		unsigned int posx, posy; // posx = (int)robot.robx/6, posy = (int)robot.roby/6
		// ������ �� �����, ������� �������� �����
		// (posx-2,posy-2) (posx-1,posy-2) (posx  ,posy-2) (posx+1,posy-2)
		// (posx-2,posy-1) (posx-1,posy-1) (posx  ,posy-1) (posx+1,posy-1)
		// (posx-2,posy  ) (posx-1,posy  ) (posx  ,posy  ) (posx+1,posy  )
		// (posx-2,posy+1) (posx-1,posy+1) (posx  ,posy+1) (posx+1,posy+1)

		// angle = 0 => robot.robangle = 0
		// angle = 1 => robot.robangle = PI/2
		// angle = 2 => robot.robangle = PI
		// angle = 3 => robot.robangle = PI*3/2
		unsigned int angle;

		bool move(unsigned int tiles, float speed); // ��������� ����� �� tiles ������

		void echolocator(void); // �������� �� ����� ��������� �����������

		void rotate(int a); // ����������� �� ���� a*PI/2

	public:
		// ��������� �������� ������
		// ���������� true, ���� ����� �������� ������
		bool update(void);

		AI()
		{
			map = new TILE[((2400*2)/6)*((2400*2)/6)];
			memset(map, 0, ((2400*2)/6)*((2400*2)/6)*sizeof(TILE));

			// ������� �������� (� ������, � �� ��������)
			posx = 400; posy = 400; // 400 = 2400/6
			for(int j = posy-2; j < posy+2; j++) {
				for(int i = posx-2; i < posx+2; i++) {
					map[i+j*800].type = 1;
				}
			}

			// ���� ����������� �������� = PI/2*angle
			// angle = {0,1,2,3}
			angle = 0;
		}

		~AI()
		{
			delete [] map;
		}
};

void AI::rotate(int a)
{
	int iangle;

	robot.rotate(a*PI/2);

	iangle = angle+a;

	while(iangle < 0) iangle += 4;

	angle = iangle%4;
}

bool AI::move(unsigned int tiles, float speed)
{
	float result;
	bool mustclean;
	unsigned int i, j;

	while(tiles+1) {
		mustclean = false;

		// �������� ����, ����� �� ������� ������ �����y
		for(j = posy-2; j <= posy+1; j++) {
			for(i = posx-2; i <= posx+1; i++) { 
				if(map[i+j*800].type == 0) {
					map[i+j*800].type = 1;
					mustclean = true;
				}
				if(map[i+j*800].type == 2) map[i+j*800].type = 1;
			}
		}

		if(mustclean) robot.startcleaning(); else robot.endcleaning();

		if(tiles == 0) break; // ����� ��� ������� ���
		

		result = robot.move(6, speed);

		// ���� ���������� �� ����������� ��� ��������
		if((int)(result+0.5) != 6 || robot.getpitssensor() == 1) {
			robot.move(result, -speed);

			// �������� ��������������� ������ �� �����
			switch(angle) {
				case 0:
					if(map[posx+2+(posy-2)*800].type == 0) map[posx+2+(posy-2)*800].type = 2;
					if(map[posx+2+(posy-1)*800].type == 0) map[posx+2+(posy-1)*800].type = 2;
					if(map[posx+2+(posy  )*800].type == 0) map[posx+2+(posy  )*800].type = 2;
					if(map[posx+2+(posy+1)*800].type == 0) map[posx+2+(posy+1)*800].type = 2;
					break;
				case 1:
					if(map[posx-2+(posy+2)*800].type == 0) map[posx-2+(posy+2)*800].type = 2;
					if(map[posx-1+(posy+2)*800].type == 0) map[posx-1+(posy+2)*800].type = 2;
					if(map[posx  +(posy+2)*800].type == 0) map[posx  +(posy+2)*800].type = 2;
					if(map[posx+1+(posy+2)*800].type == 0) map[posx+1+(posy+2)*800].type = 2;
					break;
				case 2:
					if(map[posx-3+(posy-2)*800].type == 0) map[posx-3+(posy-2)*800].type = 2;
					if(map[posx-3+(posy-1)*800].type == 0) map[posx-3+(posy-1)*800].type = 2;
					if(map[posx-3+(posy  )*800].type == 0) map[posx-3+(posy  )*800].type = 2;
					if(map[posx-3+(posy+1)*800].type == 0) map[posx-3+(posy+1)*800].type = 2;
					break;
				case 3:
					if(map[posx-2+(posy-3)*800].type == 0) map[posx-2+(posy-3)*800].type = 2;
					if(map[posx-1+(posy-3)*800].type == 0) map[posx-1+(posy-3)*800].type = 2;
					if(map[posx  +(posy-3)*800].type == 0) map[posx  +(posy-3)*800].type = 2;
					if(map[posx+1+(posy-3)*800].type == 0) map[posx+1+(posy-3)*800].type = 2;
					break;
			}

			return false;
		}

		switch(angle) {
				case 0:
					posx++;
					break;
				case 1:
					posy++;
					break;
				case 2:
					posx--;
					break;
				case 3:
					posy--;
					break;
			}

		tiles--;
	}

	robot.endcleaning();

	return true;
}

void AI::echolocator(void)
{
	// �������� �����������, �� ������� ������� �����
	if(robot.getecholocator() != -1) {
		int lx, ly, echo;

		echo = robot.getecholocator();

		switch(angle) {
			case 0:
				lx = echo/6;
				ly = 0;
				break;
			case 1:
				lx = 0;
				ly = echo/6;
				break;
			case 2:
				lx = -echo/6;
				ly = 0;
				break;
			case 3:
				lx = 0;
				ly = -echo/6;
				break;
		}

		lx += posx;
		ly += posy;

		if(map[lx+ly*800].type == 0) map[lx+ly*800].type = 2;
	}
}

bool AI::update(void) {
	int cellx, celly, celll; // ������� ������, � ������� ����� ����������� ��������, � ���������� �� ���� ������
	COMMAND *comma;

	echolocator(); // �������� �����������, �� ������� ������� �����

	// ����� ����������� ������, ������� �������� � ��� �����������
	celll = 0xFFFFFF;
	for(int i = 2; i < 799; i++) {
		for(int j = 2; j < 799; j++) {
			if(map[j+i*800].type == 0 && (map[(j-1)+i*800].type == 1 || map[(j+1)+i*800].type == 1 || map[j+(i-1)*800].type == 1 || map[j+(i+1)*800].type == 1))
				if( (abs((int)posx-j)+abs((int)posy-i)) < celll) {
					bool freespace = true;

					for(int k = i-2; k < i+2; k++) {
						for(int l = j-2; l < j+2; l++) {
							if(map[l+k*800].type == 2) {
								freespace = false;

								break;
							}
						}

						if(!freespace) break;
					}


					if(freespace) {
						celll = abs((int)posx-j)+abs((int)posy-i);
						cellx = j;
						celly = i;
					}
				}
					
		}
	}

	if(celll == 0xFFFFFF) return true; // ��� ����������� ������ ��������, ������ ���������

	// �������� ����
	if(path.MakeMePathPls(map, posx, posy, cellx, celly, angle)) {

		// ������ ������, ����������� ��������� � ������ (cellx, celly)
		comma = path.commas;
		while(comma->com) {
			if(comma->com == 1) {
				rotate(comma->param);
				echolocator(); // �������� �����������, �� ������� ������� �����
			} else if(comma->com == 2) {
				if(!move(comma->param, comma->param2)) break;
			}

			comma++;
		}
	
	} else {
		map[cellx+celly*800].type = 2;
	}

	return false;
}

void _cdecl newmain(void)
{
	AI ai;

	while(!ai.update());
}
