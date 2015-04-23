#define _CRT_SECURE_NO_WARNINGS

#define DEFAULT_SPHERE_PATH "C:\\sphere.8"
/************************************************************
* Handout: rotate-sphere-new.cpp (A Sample Code for Shader-Based OpenGL ---
for OpenGL version 3.1 and later)
* Originally from Ed Angel's textbook "Interactive Computer Graphics" 6th Ed
sample code "example3.cpp" of Chapter 4.
* Moodified by Yi-Jen Chiang to include the use of a general rotation function
Rotate(angle, x, y, z), where the vector (x, y, z) can have length != 1.0,
and also to include the use of the function NormalMatrix(mv) to return the
normal matrix (mat3) of a given model-view matrix mv (mat4).

(The functions Rotate() and NormalMatrix() are added to the file "mat-yjc-new.h"
by Yi-Jen Chiang, where a new and correct transpose function "transpose1()" and
other related functions such as inverse(m) for the inverse of 3x3 matrix m are
also added; see the file "mat-yjc-new.h".)

* Extensively modified by Yi-Jen Chiang for the program structure and user
interactions. See the function keyboard() for the keyboard actions.
Also extensively re-structured by Yi-Jen Chiang to create and use the new
function drawObj() so that it is easier to draw multiple objects. Now a floor
and a rotating sphere are drawn.

** Perspective view of a color sphere using LookAt() and Perspective()

** Colors are assigned to each vertex and then the rasterizer interpolates
those colors across the triangles.
**************************************************************/

#include "Angel-yjc.h"
#include <stdio.h>
#include <math.h>
#include <iostream>
#include <fstream.h>

#define PI 3.1415926535897

#define sqrt3f(x,y,z) sqrt(x*x+y*y+z*z)

using namespace std;


typedef Angel::vec3  color3;
typedef Angel::vec3  point3;

GLuint Angel::InitShader(const char* vShaderFile, const char* fShaderFile);

GLuint program;       /* shader program object id */
GLuint sphere_buffer;   /* vertex buffer object id for sphere */
GLuint floor_buffer;  /* vertex buffer object id for floor */

// Projection transformation parameters
GLfloat  fovy = 45.0;  // Field-of-view in Y direction angle (in degrees)
GLfloat  aspect;       // Viewport aspect ratio
GLfloat  zNear = 0.5, zFar = 3.0;

GLfloat angle = 0.2; // rotation angle in degrees
GLfloat theta = 0.0;
point3 init_eye(7.0, 3.0, -10.0); // initial viewer position
point3 eye = init_eye;               // current viewer position

point3 track[] = { point3(3, 1, 5), point3(-2, 1, -2.5), point3(2, 1, -4) };
point3 ground[] = { point3(5, 0, 8), point3(5, 0, -4), point3(-5, 0, -4), point3(-5, 0, 8) };
int cSegment = 0;
int tSegment = 3;
point3 centerPosition = track[cSegment];

GLfloat radius = 1.0;

bool begin = false, rolling = false;

GLfloat** sphereData;

point3* dirvector;
point3* rotationAxis;

int count = -1, num, polygon_n;

mat4 M = { vec4(1, 0, 0, 0), vec4(0, 1, 0, 0), vec4(0, 0, 1, 0), vec4(0, 0, 0, 1) };

int animationFlag = 1; // 1: animation; 0: non-animation. Toggled by key 'a' or 'A'


int sphereFlag = 1;   // 1: solid sphere; 0: wireframe sphere. Toggled by key 'c' or 'C'
int floorFlag = 1;  // 1: solid floor; 0: wireframe floor. Toggled by key 'f' or 'F'

const int sphere_NumVertices = 24; //(8 faces)*(1 triangles/face)*(3 vertices/triangle)
point3 sphere_points[sphere_NumVertices];
const int floor_NumVertices = 6; //(1 face)*(2 triangles/face)*(3 vertices/triangle)
point3 floor_points[floor_NumVertices]; // positions for all vertices

// RGBA colors
color3 vertex_colors[3] = {
	color3(1.0, 0.84, 0),
	color3(0.0, 1.0, 0.0),
	color3(0.529, 0.807, 0.92)// black
};
//----------------------------------------------------------------------------
int Index = 0; // YJC: This must be a global variable since quad() is called
//      multiple times and Index should then go up to 36 for
//      the 36 vertices and colors

/* Compute distance between two points */
float distanceAt(point3 p1, point3 p2);

/* Reset viewer to (7,3,-10) */
void setDefaultView();

bool beg = false;

void display(void);

/* file_in(): file input function. Modify here. */
void fileReadIn(void)
{
	const int MAX_FILE_LEN = 1000;
	char* filePath = new char[MAX_FILE_LEN];
	cout << "Please enter sphere data file path, or press ENTER to use the sample file("
		<< DEFAULT_SPHERE_PATH << ") " << endl;
	cin.sync();
	cin.getline(filePath, MAX_FILE_LEN);
	if (strlen(filePath) == 0){
		cout << "No input file '" << filePath << "'. Use sample data." << endl;
		filePath = DEFAULT_SPHERE_PATH;
	}

	ifstream file(filePath);
	if (file.fail()){
		cout << "Cannot open file " << filePath << endl;
		exit(0);
	}

	//total number of triangles
	file >> num;
	cout << "Total number of triangles: " << num << endl;

	sphereData = new GLfloat*[num];
	for (int i = 0; i < num; i++)
	{
		int numOfPoints = 0;
		file >> numOfPoints;
		sphereData[i] = new GLfloat[3 * numOfPoints];

		for (int j = 0; j<3 * numOfPoints; j++)
		{
			file >> sphereData[i][j];
		}
	}
	file.close();
}

void setDefaultView(){
	init_eye.x = 7.0;
	init_eye.y = 3.0;
	init_eye.z = -10.0;
}

void quit(){
	delete[] dirvector;
	delete[] rotationAxis;
	delete[] sphereData;

	exit(1);
}

void main_menu(int index)
{
	switch (index)
	{
	case(0) :
	{
		setDefaultView();
		break;
	}
	case(1) :
	{
		quit();
		break;
	}
	}
	display();
}


void addMenu(){
	glutCreateMenu(main_menu);

	glutAddMenuEntry("Default View Point", 0);
	glutAddMenuEntry("Quit", 1);

	glutAttachMenu(GLUT_LEFT_BUTTON);
}

//----------------------------------------------------------------------------
// generate 12 triangles: 36 vertices and 36 colors
void spherecolor()
{
	int z = 0;
	for (int i = 0; i < 8; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			sphere_points[z] = sphereData[i][j];
			z++;
		}
	}
}
//-------------------------------
// generate 2 triangles: 6 vertices and 6 colors
void floor()
{
	floor_points[0] = ground[1];
	floor_points[1] = ground[0];
	floor_points[2] = ground[2];
	floor_points[3] = ground[2];
	floor_points[4] = ground[3];
	floor_points[5] = ground[1];
}
//----------------------------------------------------------------------------
// OpenGL initialization
void init()
{
	spherecolor();
#if 0 //YJC: The following is not needed
	// Create a vertex array object
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
#endif

	// Create and initialize a vertex buffer object for sphere, to be used in display()
	glGenBuffers(1, &sphere_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, sphere_buffer);

#if 0
	glBufferData(GL_ARRAY_BUFFER, sizeof(sphere_points) + sizeof(sphere_colors),
		NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(sphere_points), sphere_points);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(sphere_points), sizeof(sphere_colors),
		sphere_colors);
#endif
#if 1
	glBufferData(GL_ARRAY_BUFFER, sizeof(point3) * sphere_NumVertices + 1,
		NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(point3) * sphere_NumVertices, sphereData);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(point3) * sphere_NumVertices, 1,
		vertex_colors[2]);
#endif

	floor();
	// Create and initialize a vertex buffer object for floor, to be used in display()
	glGenBuffers(1, &floor_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, floor_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(floor_points) + 1,
		NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(floor_points), floor_points);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(floor_points), 1,
		vertex_colors[1]);

	// Load shaders and create a shader program (to be used in display())
	program = InitShader("vshader42.glsl", "fshader42.glsl");

	glEnable(GL_DEPTH_TEST);
	glClearColor(0.0, 0.0, 0.0, 1.0);
	glLineWidth(2.0);
}
//----------------------------------------------------------------------------
// drawObj(buffer, num_vertices):
//   draw the object that is associated with the vertex buffer object "buffer"
//   and has "num_vertices" vertices.
//
void drawObj(GLuint buffer, int num_vertices)
{
	//--- Activate the vertex buffer object to be drawn ---//
	glBindBuffer(GL_ARRAY_BUFFER, buffer);

	/*----- Set up vertex attribute arrays for each vertex attribute -----*/
	GLuint vPosition = glGetAttribLocation(program, "vPosition");
	glEnableVertexAttribArray(vPosition);
	glVertexAttribPointer(vPosition, 3, GL_FLOAT, GL_FALSE, 0,
		BUFFER_OFFSET(0));

	GLuint vColor = glGetAttribLocation(program, "vColor");
	glEnableVertexAttribArray(vColor);
	glVertexAttribPointer(vColor, 3, GL_FLOAT, GL_FALSE, 0,
		BUFFER_OFFSET(sizeof(point3) * num_vertices));
	// the offset is the (total) size of the previous vertex attribute array(s)

	/* Draw a sequence of geometric objs (triangles) from the vertex buffer
	(using the attributes specified in each enabled vertex attribute array) */
	glDrawArrays(GL_LINE_LOOP, 0, num_vertices);

	/*--- Disable each vertex attribute array being enabled ---*/
	glDisableVertexAttribArray(vPosition);
	glDisableVertexAttribArray(vColor);
}
//----------------------------------------------------------------------------
void display(void)
{
	glClear(GL_COLOR_BUFFER_BIT);  // clear frame buffer (also called the color buffer)
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(viewer.x,viewer.y,viewer.z,  0,0,0,  0,1,0);

	//draw the ground
	glColor3f(0.0, 1.0, 0.0);      // draw in green.
	glBegin(GL_POLYGON);
	glVertex3f(ground[0].x,ground[0].y,ground[0].z);
	glVertex3f(ground[1].x,ground[1].y,ground[1].z);
	glVertex3f(ground[2].x,ground[2].y,ground[2].z);
	glVertex3f(ground[3].x,ground[3].y,ground[3].z);
	glEnd();


	//glLineWidth(2.0);
	glColor3f(1.0,0.0,0.0);//draw in red
	glBegin(GL_LINES);
	glVertex3f(0,0,0);
	glVertex3f(100,0,0);
	glEnd();

	glColor3f(1.0,0.0,1.0);//draw in magenta
	glBegin(GL_LINES);
	glVertex3f(0,0,0);
	glVertex3f(0,100,0);
	glEnd();

	glColor3f(0.0,0.0,1.0);//draw in blue
	glBegin(GL_LINES);
	glVertex3f(0,0,0);
	glVertex3f(0,0,100);
	glEnd();

	glTranslatef(centerPos.x,centerPos.y,centerPos.z);
	//add rotation here
	//Rotate the sphere around vector 8i+7k
	glMultMatrixf(acc_matrix);
	//glRotatef(theta,rotationAxis[currentSegment].x,rotationAxis[currentSegment].y,rotationAxis[currentSegment].z);

	glColor3f(1.0, 0.84, 0.0);      // draw in golden yellow.
	for (int a = 0; a < num; a++)
	{
		glBegin(GL_LINE_LOOP);
		glColor3f(1,0.84,0);
		glVertex3f(sphereData[a][0], sphereData[a][1], sphereData[a][2]);
		glVertex3f(sphereData[a][3], sphereData[a][4], sphereData[a][5]);
		glVertex3f(sphereData[a][6], sphereData[a][7], sphereData[a][8]);
		glEnd();
	}

	glFlush();         // Render (draw) the object
	glutSwapBuffers(); // Swap buffers in double buffering.
}
point3 calculateDirection(point3 from, point3 to){
	point3 v;
	v.x = to.x - from.x;
	v.y = to.y - from.y;
	v.z = to.z - from.z;

	//convert v to unit-length
	float d = sqrt3f(v.x, v.y, v.z);
	v.x = v.x / d;
	v.y = v.y / d;
	v.z = v.z / d;

	return v;
}

point3 crossProduct(point3 u, point3 v){
	point3 n;
	n.x = u.y*v.z - u.z*v.y;
	n.y = u.z*v.x - u.x*v.z;
	n.z = u.x*v.y - u.y*v.x;
	return n;
}

float calculateRadius(){
	float y_max = -10000, y_min = 10000;
	for (int i = 0; i < num; i++)
	{
		{
			GLfloat y = sphereData[i][1];
			y_max = (y > y_max) ? y : y_max;
			y_min = (y < y_min) ? y : y_min;
		}
		{
			GLfloat y = sphereData[i][4];
			y_max = (y > y_max) ? y : y_max;
			y_min = (y < y_min) ? y : y_min;
		}
		{
			GLfloat y = sphereData[i][7];
			y_max = (y > y_max) ? y : y_max;
			y_min = (y < y_min) ? y : y_min;
		}
	}
	return (y_max - y_min) / 2;
}

void my_init()
{
	//calculate the radius of the sphere
	radius = calculateRadius();
	//calculate the rolling directions
	tSegment = sizeof(track) / sizeof(point3);
	dirvector = new point3[tSegment];
	rotationAxis = new point3[tSegment];
	for (int i = 0; i<tSegment - 1; i++){
		dirvector[i] = calculateDirection(track[i], track[i + 1]);
	}
	//and the last point to the first one
	dirvector[tSegment - 1] = calculateDirection(track[tSegment - 1], track[0]);

	//calculate the rotating axis vectors
	point3 y_axis = { 0, 1, 0 };
	for (int i = 0; i<tSegment; i++){
		rotationAxis[i] = crossProduct(y_axis, dirvector[i]);
	}

	glClearColor(0.529, 0.807, 0.92, 0);
}

double calculateFovy(){
	//distance between camera and the center of object
	point3 obj_center = { 0, radius, 2 };
	double distance = distanceAt(init_eye, obj_center);
	//compute size of the whole scene
	double size = sqrt3f(5.0, radius, 6.0);

	double radtheta, degtheta;

	radtheta = 2.0 * atan2(size, distance);
	degtheta = (180.0 * radtheta) / PI;

	return degtheta;
}

void reshape(int w, int h)
{
	int size = (w>h) ? h : w;
	glViewport((w - size) / 2, (h - size) / 2, size, size);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	//calculate fovy
	double fovy = calculateFovy();
	gluPerspective(fovy, w / h, 0, 1000);
}

int nextModel(){
	int next = cSegment + 1;
	return (next == tSegment) ? 0 : next;
}

float distanceAt(point3 p1, point3 p2){
	float dx = p1.x - p2.x;
	float dy = p1.y - p2.y;
	float dz = p1.z - p2.z;
	return sqrt3f(dx, dy, dz);
}

/* distance between current position and next point is greater than one between current track point and the next point*/
bool isTrespass(){
	int next = nextModel();
	point3 from = track[cSegment];
	point3 to = track[next];
	float d1 = distanceAt(centerPosition, from);
	float d2 = distanceAt(to, from);

	return d1 > d2;
}

void rotationMatrix(){
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glRotatef(angle, rotationAxis[cSegment].x, rotationAxis[cSegment].y, rotationAxis[cSegment].z);
	glMultMatrixf(M);
	glGetFloatv(GL_MODELVIEW_MATRIX, M);
	glPopMatrix();
}

void idle(void)
{
	//rotate by constant speed
	theta += angle;
	if (theta > 360.0)
		theta -= 360.0;

	//translate on direction 
	float offset = (radius * angle* PI) / 180;
	centerPosition.x = centerPosition.x + dirvector[cSegment].x*offset;
	centerPosition.y = centerPosition.y + dirvector[cSegment].y*offset;
	centerPosition.z = centerPosition.z + dirvector[cSegment].z*offset;

	if (isTrespass()){
		cSegment = nextModel();
		centerPosition = track[cSegment];
	}

	//compute accumulated rotation matrix
	rotationMatrix();

	/* display()*/
	glutPostRedisplay();
}

void key(unsigned char key, int x, int y)
{
	if (key == 'b' || key == 'B'){
		beg = true;
		// Start rolling
		glutIdleFunc(idle);
	}
	if (key == 'x') init_eye.x -= 1.0;
	if (key == 'X') init_eye.x += 1.0;
	if (key == 'y') init_eye.y -= 1.0;
	if (key == 'Y') init_eye.y += 1.0;
	if (key == 'z') init_eye.z -= 1.0;
	if (key == 'Z') init_eye.z += 1.0;
}

void mouse(int button, int state, int x, int y){
	if (button == GLUT_RIGHT_BUTTON && state == GLUT_UP && beg){
		rolling = !rolling;
	}
	if (rolling){
		// Stop rolling
		glutIdleFunc(idle);
	}
	else{
		glutIdleFunc(NULL);
	}
}

void main(int argc, char **argv)
{
	/*---- Initialize & Open Window ---*/
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB); // double-buffering and RGB color
	// mode.
	glutInitWindowSize(500, 500);
	glutInitWindowPosition(30, 30);  // Graphics window position
	glutCreateWindow("Assignment 2"); // Window title is "Rectangle"

	fileReadIn();
	addMenu();

	glutDisplayFunc(display); // Register our display() function as the display 
	// call-back function
	glutReshapeFunc(reshape); // Register our reshape() function as the reshape 
	// call-back function
	glutMouseFunc(mouse);  // for mouse 
	glutKeyboardFunc(key); // for keyboard

	my_init();                // initialize variables

	glutMainLoop();          // Enter the event loop
}
