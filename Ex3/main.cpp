
#include <stdio.h>
#include <stdlib.h>
//#include <cstdlib>
#include <cstring>
#include "ObjLoader.hpp"
#include "glut.h"
#include "Camera.cpp"
#include "Vector3d.hpp"
#include "time.h"
#include "Windows.h"
#include "ball.hpp"

#define ModelSize 6
#define StatusLantency 50
using std::string;
using std::vector;

//模型路径
std::string ObjPath[ModelSize] =
{
	"asset/s1/s1.obj",
	"asset/s2/s2.obj",
	"asset/s3/s3.obj",
	"asset/s4/s4.obj",
	"asset/s5/s5.obj",
	"asset/s6/s6.obj"
};

std::string SpherePath = "sphere_init.obj";

//ObjLoader objModel;
//ObjLoader objSphere;

Camera camera = Camera(Vector3d(0, -6, 2), Vector3d(-13 * PI / 16, - PI / 2, 1));
Vector3d camera_move_offset = Vector3d(0, 0, -3);
Vector3d camera_rotate_offset = Vector3d(0, 0, 0);
Vector3d camera_relative_pos = Vector3d(0, -4, -2);
float moving_speed = 0.01f;
float rotation_sensitity = 0.003f;

float FrictionModule = 0.005;//通用摩擦系数 0.005还行
float FrictionModule_Final = 0;//最后一段摩擦系数
float stime = 1.0 / 1600.0;//时间片长度
Vector3d offset(4.3, 0.5, -11);//小球初始位置
Vector3d g(0, -9.8, 0);
float fSet = 5;//给力大小
float M = 0.5;//小球质量
float second = 0;//游戏时间

int startTime;
int nowTime = 0;
int wHeight = 0;
int wWidth = 0;

//状态机
bool isGameOver = false;
bool isGameStart = false;
bool isGameComplete = false;
bool isGamePause = false;
bool timer = false;

int FrameCount = 0;//帧数计数器


//ui 贴图
GLuint sphere;
GLuint congrats;
GLuint gameOver;
vector<GLuint> obj_tex;
vector<GLuint> sky_tex;

//实现移动鼠标观察模型所需变量
static float c = 3.1415926 / 180.0f;
static float r = 1.0f;
static int degree = 90;
static int oldPosY = -1;
static int oldPosX = -1;

ObjLoader objSphere;

vector<ObjLoader> objModel;

ball HelloBall;

Vector3d Nf(0, 0, 0);

//重新开始函数
void Reset()
{
	HelloBall.SetOffset(offset);
	HelloBall.Init();
}

void _gameStart()
{
	isGameStart = true;
	isGameOver = false;
	isGameComplete = false;
	timer = true;
	startTime = clock();
	Reset();
}

void _gameOver()
{
	startTime = 0;
	isGameOver = true;
	isGameComplete = false;
	timer = false;
}

void _gameComplete()
{
	startTime = 0;
	isGameComplete = true;
	isGameOver = false;
	timer = false;
}

void status(ball HelloBall)
{
	Vector3d position = HelloBall.GetOffset();
	if (!isGameComplete && (position.y <= -1.2||position.y>=5))
	{
		_gameOver();
	}
	if (fabs(position.y) <= 0.05 && (position.x<5.3&&position.x>3.3) && (position.z<-3 && position.z>-5))
		_gameComplete();
}

BOOL WriteBitmapFile(char * filename, int width, int height, unsigned char * bitmapData)
{
	//填充BITMAPFILEHEADER
	BITMAPFILEHEADER bitmapFileHeader;
	memset(&bitmapFileHeader, 0, sizeof(BITMAPFILEHEADER));
	bitmapFileHeader.bfSize = sizeof(BITMAPFILEHEADER);
	bitmapFileHeader.bfType = 0x4d42;	//BM
	bitmapFileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

	//填充BITMAPINFOHEADER
	BITMAPINFOHEADER bitmapInfoHeader;
	memset(&bitmapInfoHeader, 0, sizeof(BITMAPINFOHEADER));
	bitmapInfoHeader.biSize = sizeof(BITMAPINFOHEADER);
	bitmapInfoHeader.biWidth = width;
	bitmapInfoHeader.biHeight = height;
	bitmapInfoHeader.biPlanes = 1;
	bitmapInfoHeader.biBitCount = 24;
	bitmapInfoHeader.biCompression = BI_RGB;
	bitmapInfoHeader.biSizeImage = width * abs(height) * 3;

	//////////////////////////////////////////////////////////////////////////
	FILE * filePtr;			//连接要保存的bitmap文件用
	unsigned char tempRGB;	//临时色素
	int imageIdx;

	//交换R、B的像素位置,bitmap的文件放置的是BGR,内存的是RGB
	for (imageIdx = 0; imageIdx < bitmapInfoHeader.biSizeImage; imageIdx += 3)
	{
		tempRGB = bitmapData[imageIdx];
		bitmapData[imageIdx] = bitmapData[imageIdx + 2];
		bitmapData[imageIdx + 2] = tempRGB;
	}
	errno_t err;
	err = fopen_s(&filePtr, filename, "wb");
	if (NULL == filePtr)
	{
		return FALSE;
	}

	fwrite(&bitmapFileHeader, sizeof(BITMAPFILEHEADER), 1, filePtr);

	fwrite(&bitmapInfoHeader, sizeof(BITMAPINFOHEADER), 1, filePtr);

	fwrite(bitmapData, bitmapInfoHeader.biSizeImage, 1, filePtr);

	fclose(filePtr);
	return TRUE;
}

//截图按序号保存
void grab(int width, int height)
{
	static void* screenData;
	RECT rc;
	int len = width * height * 3;
	screenData = malloc(len);
	memset(screenData, 0, len);
	glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, screenData);

	char fileName;
	int i = 1;
	FILE * filePtr;
	char a[] = ".bmp";
	char b[10];
	_itoa_s(i, b, 10);
	strcat_s(b, a);
	errno_t err;
	err = fopen_s(&filePtr, b, "rb");
	while (filePtr != NULL)
	{
		i += 1;
		_itoa_s(i, b, 10);
		strcat_s(b, a);

		err = fopen_s(&filePtr, b, "rb");
	}
	WriteBitmapFile(b, width, height, (unsigned char *)screenData);

	free(screenData);
}

int load_texture(string filename)
{
	int t_width, t_height, t_bytes;
	GLint last_texture_id = 0;
	GLuint texture_id = 0;
	GLubyte* pixels = 0;

	FILE *tf = fopen(filename.c_str(), "rb");
	if (tf == 0) return 0;

	//读取BMP文件头部信息
	fseek(tf, 0x0012, SEEK_SET);
	fread(&t_width, 4, 1, tf);
	fread(&t_height, 4, 1, tf);
	fseek(tf, 54, SEEK_SET);//54是BMP文件头部长度

	//计算总大小
	int line_bytes = t_width * 3;//rgb
	while (line_bytes % 4)
		++line_bytes;
	t_bytes = line_bytes * t_height;

	//存储像素信息，即图片内容
	pixels = (GLubyte*)malloc(t_bytes);
	if (pixels == 0) {
		fclose(tf);
		return 0;
	}
	if (fread(pixels, t_bytes, 1, tf) <= 0) {
		free(pixels);
		fclose(tf);
		return 0;
	}

	//分配一个纹理编号给该纹理
	glGenTextures(1, &texture_id);

	if (texture_id == 0) {
		free(pixels);
		fclose(tf);
		return 0;
	}

	//绑定纹理
	GLint lastTextureID = last_texture_id;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture_id);
	glBindTexture(GL_TEXTURE_2D, texture_id);
	//材质模型不匹配策略
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	//载入纹理
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, t_width, t_height, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, pixels);

	free(pixels);
	return texture_id;
}

void getFPS()
{
	static int frame = 0, time, timebase = 0;
	static char buffer[256];

	//char mode[64];
	if (timer) {
		nowTime = clock();
		second = nowTime - startTime;
		second /= 1000;
	}

	frame++;
	time = glutGet(GLUT_ELAPSED_TIME);
	//stime = time - timebase;
	if (time - timebase > 1000) {

		sprintf_s(buffer, "FPS:%4.2f     Time:%4.2f",
			frame*1000.0 / (time - timebase), second);
		timebase = time;
		frame = 0;
	}


	char *c;
	//glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glDisable(GL_TEXTURE_2D);
	glMatrixMode(GL_PROJECTION);  // 选择投影矩阵
	glPushMatrix();               // 保存原矩阵
	glLoadIdentity();             // 装入单位矩阵
	glOrtho(0, 480, 0, 480, -1, 1);    // 位置正投影
	glMatrixMode(GL_MODELVIEW);   // 选择Modelview矩阵
	glPushMatrix();               // 保存原矩阵
	glLoadIdentity();             // 装入单位矩阵
	//glScalef(10, 10, 10);
	glColor3f(1, 0, 0);
	glRasterPos2f(10, 10);
	for (c = buffer; *c != '\0'; c++) {
		glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, *c);
	}
	glMatrixMode(GL_PROJECTION);  // 选择投影矩阵
	glPopMatrix();                // 重置为原保存矩阵
	glMatrixMode(GL_MODELVIEW);   // 选择Modelview矩阵
	glPopMatrix();                // 重置为原保存矩阵
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_LIGHTING);
	//glEnable(GL_DEPTH_TEST);
}

//安置光源
void setLightRes() {
	GLfloat lightPosition[] = { 0.0f, 0.0f, 1.0f, 0.0f };
	glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
	glEnable(GL_LIGHTING); //启用光源
	glEnable(GL_LIGHT0);   //使用指定灯光
}
//初始化
void init() {
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(500, 500);
	glutCreateWindow("ObjLoader");
	//glewInit();
	glEnable(GL_DEPTH_TEST);
	glShadeModel(GL_SMOOTH);
	setLightRes();
	glEnable(GL_DEPTH_TEST);
}

void Draw_Leg()
{
	glScalef(1, 1, 3);
	glutSolidCube(1.0);
}
void Draw_Scene()
{

	//
	//up
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, sky_tex[0]);
	glBegin(GL_QUADS);
	glNormal3f(0, 0, -1);
	glTexCoord2d(0, 0);
	glVertex3f(-300, 300, 300);
	glTexCoord2d(0, 1);
	glVertex3f(-300, -300, 300);
	glTexCoord2d(1, 1);
	glVertex3f(300, -300, 300);
	glTexCoord2d(1, 0);
	glVertex3f(300, 300, 300);
	glEnd();
	//glDisable(GL_TEXTURE_2D);
	//glEnable(GL_TEXTURE_2D);
	//down
	glBindTexture(GL_TEXTURE_2D, sky_tex[1]);
	glBegin(GL_QUADS);
	glNormal3f(0, 0, 1);
	glTexCoord2d(0, 0);
	glVertex3f(-300, -300, -300);
	glTexCoord2d(0, 1);
	glVertex3f(-300, 300, -300);
	glTexCoord2d(1, 1);
	glVertex3f(300, 300, -300);
	glTexCoord2d(1, 0);
	glVertex3f(300, -300, -300);
	glEnd();
	//left
	glBindTexture(GL_TEXTURE_2D, sky_tex[2]);
	glBegin(GL_QUADS);
	glNormal3f(1, 0, 0);
	glTexCoord2d(0, 0);
	glVertex3f(-300, -300, -300);
	glTexCoord2d(0, 1);
	glVertex3f(-300, -300, 300);
	glTexCoord2d(1, 1);
	glVertex3f(-300, 300, 300);
	glTexCoord2d(1, 0);
	glVertex3f(-300, 300, -300);
	glEnd();
	//right
	glBindTexture(GL_TEXTURE_2D, sky_tex[3]);
	glBegin(GL_QUADS);
	glNormal3f(-1, 0, 0);
	glTexCoord2d(0, 0);
	glVertex3f(300, 300, -300);
	glTexCoord2d(0, 1);
	glVertex3f(300, 300, 300);
	glTexCoord2d(1, 1);
	glVertex3f(300, -300, 300);
	glTexCoord2d(1, 0);
	glVertex3f(300, -300, -300);
	glEnd();
	//fornt
	glBindTexture(GL_TEXTURE_2D, sky_tex[4]);
	glBegin(GL_QUADS);
	glNormal3f(0, -1, 0);
	glTexCoord2d(0, 0);
	glVertex3f(-300, 300, -300);
	glTexCoord2d(0, 1);
	glVertex3f(-300, 300, 300);
	glTexCoord2d(1, 1);
	glVertex3f(300, 300, 300);
	glTexCoord2d(1, 0);
	glVertex3f(300, 300, -300);
	glEnd();
	//back
	glBindTexture(GL_TEXTURE_2D, sky_tex[5]);
	glBegin(GL_QUADS);
	glNormal3f(0, 1, 0);
	glTexCoord2d(0, 0);
	glVertex3f(-300, -300, -300);
	glTexCoord2d(0, 1);
	glVertex3f(-300, -300, 300);
	glTexCoord2d(1, 1);
	glVertex3f(300, -300, 300);
	glTexCoord2d(1, 0);
	glVertex3f(300, -300, -300);
	glEnd();

	glDisable(GL_TEXTURE_2D);

	//glColor3f(1.0, 1.0, 1.0);
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//glMatrixMode(GL_MODELVIEW);
	//glLoadIdentity();
	//glTranslatef(0.0f, 0.0f, -20.0f);
	//glScalef(0.02f, 0.02f, 0.02f);

	/*
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, desk_tex);
	glPushMatrix();
	glTranslatef(10, 0, 5);//new
	glScalef(0.05f, 0.05f, 0.05f);
	//glTranslatef(Sx, Sy, Sz);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	objSphere.ElementDraw();
	glPopMatrix();
	glDisable(GL_TEXTURE_2D);
	*/


}

void updateView(int width, int height)
{
	glViewport(0,0,width,height);						// Reset The Current Viewport
	glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
	glLoadIdentity();									// Reset The Projection Matrix
	float whRatio = (GLfloat)width/(GLfloat)height;
	gluPerspective(60.0f, whRatio, 0.1, 800);
	glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
}

void reshape(int width, int height)
{
	if (height==0)										// Prevent A Divide By Zero By
	{
		height=1;										// Making Height Equal One
	}
	wHeight = height;
	wWidth = width;
	updateView(wWidth, wHeight);
}

void idle()
{
	if (isGamePause || isGameOver ||isGameComplete || !isGameStart);
	else
		HelloBall.GetFrame(Nf, stime);
	FrameCount++;
	if (FrameCount > StatusLantency)
	{
		FrameCount = 0;
		status(HelloBall);
	}
	//camera.setCameraPosition(HelloBall.GetCenter() + camera_relative_pos);
	camera.position.x = HelloBall.GetCenter().x;
	camera.position.y = -HelloBall.GetCenter().z - 1;
	camera.position.z = HelloBall.GetCenter().y + 4;
	glutPostRedisplay();
}

void key(unsigned char k, int x, int y)
{
	switch(k)
	{
	case 27:
	case 'q': {exit(0); break; }

	case 'd': {
		//camera_move_offset += Vector3d(-moving_speed * sinf(camera.rotation.y), moving_speed * cosf(camera.rotation.y), 0);
		Nf = Vector3d(+fSet, 0, 0) + Nf;
		if (!isGameStart) {
			_gameStart();
		}
		break;
	}
	case 'a': {
		//camera_move_offset += Vector3d(moving_speed * sinf(camera.rotation.y), -moving_speed * cosf(camera.rotation.y), 0);
		Nf = Vector3d(-fSet, 0, 0) + Nf;
		if (!isGameStart) {
			_gameStart();
		}
		break;
	}
	case 's': {
		//camera_move_offset += Vector3d(moving_speed * cosf(camera.rotation.y), moving_speed * sinf(camera.rotation.y), 0);
		Nf = Vector3d(0, 0, fSet) + Nf;
		if (!isGameStart) {
			_gameStart();
		}
		break;
	}
	case 'w': {
		//camera_move_offset += Vector3d(-moving_speed * cosf(camera.rotation.y), -moving_speed * sinf(camera.rotation.y), 0);
		Nf = Vector3d(0, 0, -fSet) + Nf;
		if (!isGameStart) {
			_gameStart();
		}
		break;
	}
	case ' ': {
		_gameStart();
	}
	case'p':grab(wWidth, wHeight); break;
	case 'r': {//缺少摄像机位置初始化函数
		startTime = 0;
		isGameStart = false;
		isGameComplete = false;
		isGameOver = false;
		timer = false;
		second = 0;
		Reset();
		break;
	}
	}
}

void keyup(unsigned char k, int x, int y)
{
	switch (k) {
		case'w':Nf.z = 0; break;
		case's':Nf.z = 0; break;
		case'a':Nf.x = 0; break;
		case'd':Nf.x = 0; break;
	}

}

void specialkey(int k, int x, int y)
{
	switch (k)
	{
		case GLUT_KEY_UP: {
			camera_rotate_offset += Vector3d(rotation_sensitity, 0, 0);
			break;
		}
		case GLUT_KEY_DOWN: {
			camera_rotate_offset += Vector3d(-rotation_sensitity, 0, 0);
			break;
		}
		case GLUT_KEY_RIGHT: {
			camera_rotate_offset += Vector3d(0, -rotation_sensitity, 0);
			break;
		}
		case GLUT_KEY_LEFT: {
			camera_rotate_offset += Vector3d(0, rotation_sensitity, 0);
			break;
		}
	}
}
void specialkeyup(int k, int x, int y)
{
	switch (k)
	{
	case GLUT_KEY_UP: {
		camera_rotate_offset = Vector3d(0, 0, 0);
		break;
	}
	case GLUT_KEY_DOWN: {
		camera_rotate_offset = Vector3d(0, 0, 0);
		break;
	}
	case GLUT_KEY_LEFT: {
		camera_rotate_offset = Vector3d(0, 0, 0);
		break;
	}
	case GLUT_KEY_RIGHT: {
		camera_rotate_offset = Vector3d(0, 0, 0);
		break;
	}
	}
}

void mousemove()
{

}

void redraw()
{

	glColor3f(1.0, 1.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();									// Reset The Current Modelview Matrix

	//glTranslatef(0.0f, -1.0f, -12.0f);
	//glScalef(0.02f, 0.02f, 0.02f);
	setLightRes();
	//glPushMatrix();

	if(camera_move_offset.x || camera_move_offset.y || camera_move_offset.z)
		camera.CameraMove(camera_move_offset);
	if ((camera_rotate_offset.x || camera_rotate_offset.y) && camera.rotation.x >= CAM_LOW_BOUND && camera.rotation.x <= CAM_UP_BOUND) {
		camera.CameraRotate(camera_rotate_offset);
	}
	if (camera.rotation.x < CAM_LOW_BOUND) camera.rotation.x += rotation_sensitity;
	if (camera.rotation.x > CAM_UP_BOUND) camera.rotation.x -= rotation_sensitity;
	camera.CameraLookat();

	glPushMatrix();
	glRotatef(90, 1, 0, 0);

	//
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, sphere);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	HelloBall.Redraw();

	glBindTexture(GL_TEXTURE_2D, obj_tex[0]);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	objModel[0].ElementDraw();

	glBindTexture(GL_TEXTURE_2D, obj_tex[1]);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	objModel[1].ElementDraw();

	glBindTexture(GL_TEXTURE_2D, obj_tex[2]);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	objModel[2].ElementDraw();

	glBindTexture(GL_TEXTURE_2D, obj_tex[3]);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	objModel[3].ElementDraw();

	glBindTexture(GL_TEXTURE_2D, obj_tex[4]);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	objModel[4].ElementDraw();

	glBindTexture(GL_TEXTURE_2D, obj_tex[5]);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	objModel[5].ElementDraw();

	glDisable(GL_TEXTURE_2D);
	
	glPopMatrix();
	//glPopMatrix();
	glColor3f(0, 0, 1);
	getFPS();
	//status(HelloBall);
	
	if (isGameOver)
	{
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, gameOver);

		glBegin(GL_QUADS);
		//glScalef(50.0f, 50.0f, 25.0f);

		//glPushMatrix();
		//glTranslatef(HelloBall.GetCenter().x, HelloBall.GetCenter().z, HelloBall.GetCenter().y);
		glNormal3f(0, 0, 1);
		glTexCoord2f(0.0f, 0.0f);
		glVertex3f(HelloBall.GetCenter().x - 2.4f, - HelloBall.GetCenter().z + 0.0f, HelloBall.GetCenter().y - 1.0f);
		glTexCoord2f(1.0f, 0.0f);
		glVertex3f(HelloBall.GetCenter().x + 2.4f, - HelloBall.GetCenter().z + 0.0f, HelloBall.GetCenter().y - 1.0f);
		glTexCoord2f(1.0f, 1.0f);
		glVertex3f(HelloBall.GetCenter().x + 2.4f, - HelloBall.GetCenter().z + 4.0f, HelloBall.GetCenter().y + 0.5f);
		glTexCoord2f(0.0f, 1.0f);
		glVertex3f(HelloBall.GetCenter().x - 2.4f, - HelloBall.GetCenter().z + 4.0f, HelloBall.GetCenter().y + 0.5f);
		glEnd();
		//glPopMatrix();
		glDisable(GL_TEXTURE_2D);
	}
	if (!isGameOver && isGameComplete) {
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, congrats);
		//glScalef(50.0f, 50.0f, 25.0f);
		glBegin(GL_QUADS);
		glNormal3f(0, 0, 1);
		glTexCoord2f(0.0f, 0.0f);
		glVertex3f(HelloBall.GetCenter().x - 2.4f, -HelloBall.GetCenter().z + 0.0f, HelloBall.GetCenter().y - 1.0f);
		glTexCoord2f(1.0f, 0.0f);
		glVertex3f(HelloBall.GetCenter().x + 2.4f, -HelloBall.GetCenter().z + 0.0f, HelloBall.GetCenter().y - 1.0f);
		glTexCoord2f(1.0f, 1.0f);
		glVertex3f(HelloBall.GetCenter().x + 2.4f, -HelloBall.GetCenter().z + 4.0f, HelloBall.GetCenter().y + 0.5f);
		glTexCoord2f(0.0f, 1.0f);
		glVertex3f(HelloBall.GetCenter().x - 2.4f, -HelloBall.GetCenter().z + 4.0f, HelloBall.GetCenter().y + 0.5f);
		glEnd();
		glDisable(GL_TEXTURE_2D);
	}
	
	glScalef(0.2, 0.2, 0.2);
	Draw_Scene();						// Draw Scene

	glutSwapBuffers();
}



int main (int argc,  char *argv[])
{
	glutInit(&argc, argv);
	init();

	HelloBall = ball(Vector3d(0, 0.37782, 4.7), offset, 0.15, SpherePath, M);
	FaceCloud::MaxFaceCloud->ClearFaceinCloud();

	for (int i = 0; i < ModelSize; i++)
	{
		if (i == 4)
		{
			objModel.push_back(ObjLoader(ObjPath[i], FrictionModule_Final));
			continue;
		}
		objModel.push_back(ObjLoader(ObjPath[i], FrictionModule));
	}

	FaceCloud::MaxFaceCloud->Insort(2);

	sphere = load_texture("wood.bmp");
	gameOver = load_texture("gameover.bmp");
	congrats = load_texture("congrats.bmp");

	obj_tex.push_back(load_texture("asset/s1/wood.bmp"));
	obj_tex.push_back(load_texture("asset/s2/timg.bmp"));
	obj_tex.push_back(load_texture("asset/s3/timg.bmp"));
	obj_tex.push_back(load_texture("asset/s4/color.bmp"));
	obj_tex.push_back(load_texture("asset/s5/timg.bmp"));
	obj_tex.push_back(load_texture("asset/s6/timg.bmp"));

	sky_tex.push_back(load_texture("asset/SkyBox/1.top.bmp"));
	sky_tex.push_back(load_texture("asset/SkyBox/1.bottom.bmp"));
	sky_tex.push_back(load_texture("asset/SkyBox/1.left.bmp"));
	sky_tex.push_back(load_texture("asset/SkyBox/1.right.bmp"));
	sky_tex.push_back(load_texture("asset/SkyBox/1.front.bmp"));
	sky_tex.push_back(load_texture("asset/SkyBox/1.back.bmp"));

	glutIgnoreKeyRepeat(1);
	glutDisplayFunc(redraw);
	glutReshapeFunc(reshape);	
	glutKeyboardFunc(key);
	glutKeyboardUpFunc(keyup);
	glutSpecialFunc(specialkey);
	glutSpecialUpFunc(specialkeyup);

	glutIdleFunc(idle);

	glutMainLoop();
	return 0;
}


