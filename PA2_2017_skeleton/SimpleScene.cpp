
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <math.h>
#include <sys/types.h>
#ifdef _MSC_VER
#include <Windows.h>
#endif
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
//#include <GL/glfw.h>
#include <GLFW/glfw3.h>
#include "math/matrix4.h"
#include "math/vector3.h"
#include "math/intersectionTest.h"
#include "WaveFrontOBJ.h"
#include <assert.h>

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif
#if GLFW_VERSION_MAJOR==3
GLFWwindow* g_window=NULL;
#endif
void screenCoordToRay(int x, int y, Ray& ray);

// 'cameras' stores infomation of 5 cameras.
double cameras[5][9] = 
{
	{28,18,28, 0,2,0, 0,1,0},   
	{28,18,-28, 0,2,0, 0,1,0}, 
	{-28,18,28, 0,2,0, 0,1,0}, 
	{-12,12,0, 0,2,0, 0,1,0},  
	{0,100,0,  0,0,0, 1,0,0}
};
int cameraCount = sizeof( cameras ) / sizeof( cameras[0] );

int cameraIndex, camID;
vector<matrix4> wld2cam, cam2wld; 
WaveFrontOBJ* cam;

// Variables for 'cow' object.
matrix4 cow2wld;

int numCow;
matrix4 dupCow[6];
bool clicked = false;
bool ready = false;

WaveFrontOBJ* cow;
int cowID;
bool cursorOnCowBoundingBox;
double animStartTime=0;
struct PickInfo
{
	double cursorRayT;
	vector3 cowPickPosition;
	vector3 cowPickPositionLocal;
	matrix4 cowPickConfiguration; // backs up cow2wld
};

PickInfo pickInfo;

// Variables for 'beethovan' object.
matrix4 bet2wld;
WaveFrontOBJ* bet;
int betID;


unsigned floorTexID;
int width, height;

void drawFrame(float len);

enum { H_DRAG=1, V_DRAG=2};
static int isDrag=0;
//------------------------------------------------------------------------------

Plane makePlane(vector3 const& a, vector3 const& b, vector3 const& n)
{
	vector3 v=a;
	for(int i=0; i<3; i++)
	{
		if(n[i]==1.0)
			v[i]=b[i];
		else if(n[i]==-1.0)
			v[i]=a[i];
		else
			assert(n[i]==0.0);
	}
	//std::cout<<n<<v<<std::endl;
		
	return Plane(cow2wld.rotate(n),cow2wld*v);
}

//------------------------------------------------------------------------------
void drawOtherCamera()
{
	int i;


	// draw other cameras.
	for (i=0; i < (int)wld2cam.size(); i++ )
	{
		if (i != cameraIndex)
		{
			glPushMatrix();												// Push the current matrix on GL to stack. The matrix is wld2cam[cameraIndex].matrix().
			glMultMatrixd(cam2wld[i].GLmatrix());							// Multiply the matrix to draw i-th camera.
			drawFrame(5);											// Draw x, y, and z axis.
			float frontColor[] = {0.2, 0.2, 0.2, 1.0};
			glEnable(GL_LIGHTING);									
			glMaterialfv(GL_FRONT, GL_AMBIENT, frontColor);			// Set ambient property frontColor.
			glMaterialfv(GL_FRONT, GL_DIFFUSE, frontColor);			// Set diffuse property frontColor.
			glScaled(0.5,0.5,0.5);										// Reduce camera size by 1/2.
			glTranslated(1.1,1.1,0.0);									// Translate it (1.1, 1.1, 0.0).
			glCallList(camID);											// Re-draw using display list from camID. 
			glPopMatrix();												// Call the matrix on stack. wld2cam[cameraIndex].matrix() in here.
		}
	}
}


/*********************************************************************************
* Draw x, y, z axis of current frame on screen.
* x, y, and z are corresponded Red, Green, and Blue, resp.
**********************************************************************************/
void drawFrame(float len)
{
	glDisable(GL_LIGHTING);		// Lighting is not needed for drawing axis.
	glBegin(GL_LINES);			// Start drawing lines.
	glColor3d(1,0,0);			// color of x-axis is red.
	glVertex3d(0,0,0);			
	glVertex3d(len,0,0);		// Draw line(x-axis) from (0,0,0) to (len, 0, 0). 
	glColor3d(0,1,0);			// color of y-axis is green.
	glVertex3d(0,0,0);			
	glVertex3d(0,len,0);		// Draw line(y-axis) from (0,0,0) to (0, len, 0).
	glColor3d(0,0,1);			// color of z-axis is  blue.
	glVertex3d(0,0,0);
	glVertex3d(0,0,len);		// Draw line(z-axis) from (0,0,0) - (0, 0, len).
	glEnd();					// End drawing lines.
}

/*********************************************************************************
* Draw 'cow' object.
**********************************************************************************/
void drawCow(matrix4 const& _cow2wld, bool drawBB)
{  

	glPushMatrix();		// Push the current matrix of GL into stack. This is because the matrix of GL will be change while drawing cow.

	// The information about location of cow to be drawn is stored in cow2wld matrix.
	// (Project2 hint) If you change the value of the cow2wld matrix or the current matrix, cow would rotate or move.
	glMultMatrixd(_cow2wld.GLmatrix());

	drawFrame(5);										// Draw x, y, and z axis.
	float frontColor[] = {0.8, 0.2, 0.9, 1.0};
	glEnable(GL_LIGHTING);
	glMaterialfv(GL_FRONT, GL_AMBIENT, frontColor);		// Set ambient property frontColor.
	glMaterialfv(GL_FRONT, GL_DIFFUSE, frontColor);		// Set diffuse property frontColor.
	glCallList(cowID);		// Draw cow. 
	glDisable(GL_LIGHTING);
	if(drawBB){
		glBegin(GL_LINES);
		glColor3d(1,1,1);
		glVertex3d( cow->bbmin.x, cow->bbmin.y, cow->bbmin.z);
		glVertex3d( cow->bbmax.x, cow->bbmin.y, cow->bbmin.z);
		glVertex3d( cow->bbmin.x, cow->bbmax.y, cow->bbmin.z);
		glVertex3d( cow->bbmax.x, cow->bbmax.y, cow->bbmin.z);
		glVertex3d( cow->bbmin.x, cow->bbmin.y, cow->bbmax.z);
		glVertex3d( cow->bbmax.x, cow->bbmin.y, cow->bbmax.z);
		glVertex3d( cow->bbmin.x, cow->bbmax.y, cow->bbmax.z);
		glVertex3d( cow->bbmax.x, cow->bbmax.y, cow->bbmax.z);

		glColor3d(1,1,1);
		glVertex3d( cow->bbmin.x, cow->bbmin.y, cow->bbmin.z);
		glVertex3d( cow->bbmin.x, cow->bbmax.y, cow->bbmin.z);
		glVertex3d( cow->bbmax.x, cow->bbmin.y, cow->bbmin.z);
		glVertex3d( cow->bbmax.x, cow->bbmax.y, cow->bbmin.z);
		glVertex3d( cow->bbmin.x, cow->bbmin.y, cow->bbmax.z);
		glVertex3d( cow->bbmin.x, cow->bbmax.y, cow->bbmax.z);
		glVertex3d( cow->bbmax.x, cow->bbmin.y, cow->bbmax.z);
		glVertex3d( cow->bbmax.x, cow->bbmax.y, cow->bbmax.z);

		glColor3d(1,1,1);
		glVertex3d( cow->bbmin.x, cow->bbmin.y, cow->bbmin.z);
		glVertex3d( cow->bbmin.x, cow->bbmin.y, cow->bbmax.z);
		glVertex3d( cow->bbmax.x, cow->bbmin.y, cow->bbmin.z);
		glVertex3d( cow->bbmax.x, cow->bbmin.y, cow->bbmax.z);
		glVertex3d( cow->bbmin.x, cow->bbmax.y, cow->bbmin.z);
		glVertex3d( cow->bbmin.x, cow->bbmax.y, cow->bbmax.z);
		glVertex3d( cow->bbmax.x, cow->bbmax.y, cow->bbmin.z);
		glVertex3d( cow->bbmax.x, cow->bbmax.y, cow->bbmax.z);


		glColor3d(1,1,1);
		glVertex3d( cow->bbmin.x, cow->bbmin.y, cow->bbmin.z);
		glVertex3d( cow->bbmin.x, cow->bbmax.y, cow->bbmin.z);
		glVertex3d( cow->bbmax.x, cow->bbmin.y, cow->bbmin.z);
		glVertex3d( cow->bbmax.x, cow->bbmax.y, cow->bbmin.z);
		glVertex3d( cow->bbmin.x, cow->bbmin.y, cow->bbmax.z);
		glVertex3d( cow->bbmin.x, cow->bbmax.y, cow->bbmax.z);
		glVertex3d( cow->bbmax.x, cow->bbmin.y, cow->bbmax.z);
		glVertex3d( cow->bbmax.x, cow->bbmax.y, cow->bbmax.z);

		glColor3d(1,1,1);
		glVertex3d( cow->bbmin.x, cow->bbmin.y, cow->bbmin.z);
		glVertex3d( cow->bbmin.x, cow->bbmin.y, cow->bbmax.z);
		glVertex3d( cow->bbmax.x, cow->bbmin.y, cow->bbmin.z);
		glVertex3d( cow->bbmax.x, cow->bbmin.y, cow->bbmax.z);
		glVertex3d( cow->bbmin.x, cow->bbmax.y, cow->bbmin.z);
		glVertex3d( cow->bbmin.x, cow->bbmax.y, cow->bbmax.z);
		glVertex3d( cow->bbmax.x, cow->bbmax.y, cow->bbmin.z);
		glVertex3d( cow->bbmax.x, cow->bbmax.y, cow->bbmax.z);
		glEnd();
	}
	glPopMatrix();			// Pop the matrix in stack to GL. Change it the matrix before drawing cow.
}

/*********************************************************************************
* Draw 'beethovan' object.
**********************************************************************************/
void drawBet()
{  

	glPushMatrix();
	glMultMatrixd(bet2wld.GLmatrix());
	drawFrame(8);
	float frontColor[] = {0.8, 0.3, 0.1, 1.0};
	glEnable(GL_LIGHTING);
	glMaterialfv(GL_FRONT, GL_AMBIENT, frontColor);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, frontColor);
	glCallList(betID);
	glPopMatrix();
}


/*********************************************************************************
* Draw floor on 3D plane.
**********************************************************************************/
void drawFloor()
{  

	glDisable(GL_LIGHTING);

	// Set background color.
	glColor3d(0.35, .2, 0.1);
	// Draw background rectangle. 
	glBegin(GL_POLYGON);
	glVertex3f( 2000,-0.2, 2000);
	glVertex3f( 2000,-0.2,-2000);
	glVertex3f(-2000,-0.2,-2000);
	glVertex3f(-2000,-0.2, 2000);
	glEnd();

	
	// Set color of the floor.
	// Assign checker-patterned texture.
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, floorTexID );

	// Draw the floor. Match the texture's coordinates and the floor's coordinates resp. 
	glBegin(GL_POLYGON);
	glTexCoord2d(0,0);
	glVertex3d(-12,-0.1,-12);		// Texture's (0,0) is bound to (-12,-0.1,-12).
	glTexCoord2d(1,0);
	glVertex3d( 12,-0.1,-12);		// Texture's (1,0) is bound to (12,-0.1,-12).
	glTexCoord2d(1,1);
	glVertex3d( 12,-0.1, 12);		// Texture's (1,1) is bound to (12,-0.1,12).
	glTexCoord2d(0,1);
	glVertex3d(-12,-0.1, 12);		// Texture's (0,1) is bound to (-12,-0.1,12).
	glEnd();

	glDisable(GL_TEXTURE_2D);	
	drawFrame(5);				// Draw x, y, and z axis.
}


/*********************************************************************************
* Call this part whenever display events are needed. 
* Display events are called in case of re-rendering by OS. ex) screen movement, screen maximization, etc.
**********************************************************************************/
void display()
{
	glClearColor(0, 0.6, 0.8, 1);								// Clear color setting
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);				// Clear the screen
	// set viewing transformation.
	glLoadMatrixd(wld2cam[cameraIndex].GLmatrix());

	drawOtherCamera();													// Locate the camera's position, and draw all of them.
	drawFloor();													// Draw floor.

	// TODO: 
	// update cow2wld here to animate the cow.
	//double animTime=glfwGetTime()-animStartTime;
	//you need to modify both the translation and rotation parts of the cow2wld matrix.
		drawCow(cow2wld, cursorOnCowBoundingBox);														// Draw cow.
		for(int i = 0; i < numCow; i++) {
			drawCow(dupCow[i], false);
	}
	
	//drawBet();

	glFlush();

}


/*********************************************************************************
* Call this part whenever size of the window is changed. 
**********************************************************************************/
void reshape( int w, int h)
{
	width = w;
	height = h;
	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);            // Select The Projection Matrix
	glLoadIdentity();                       // Reset The Projection Matrix
	// Define perspective projection frustum
	double aspect = width/double(height);
	gluPerspective(45, aspect, 1, 1024);
	glMatrixMode(GL_MODELVIEW);             // Select The Modelview Matrix
	glLoadIdentity();                       // Reset The Projection Matrix
}

//------------------------------------------------------------------------------
void initialize()
{
	cursorOnCowBoundingBox=false;
	// Set up OpenGL state
	glShadeModel(GL_SMOOTH);         // Set Smooth Shading
	glEnable(GL_DEPTH_TEST);         // Enables Depth Testing
	glDepthFunc(GL_LEQUAL);          // The Type Of Depth Test To Do
	// Use perspective correct interpolation if available
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	// Initialize the matrix stacks
	reshape(width, height);
	// Define lighting for the scene
	float lightDirection[]   = {1.0, 1.0, 1.0, 0};
	float ambientIntensity[] = {0.1, 0.1, 0.1, 1.0};
	float lightIntensity[]   = {0.9, 0.9, 0.9, 1.0};
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambientIntensity);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, lightIntensity);
	glLightfv(GL_LIGHT0, GL_POSITION, lightDirection);
	glEnable(GL_LIGHT0);

	// initialize floor
	{
		// After making checker-patterned texture, use this repetitively.

		// Insert color into checker[] according to checker pattern.
		const int size = 8;
		unsigned char checker[size*size*3];
		for( int i=0; i < size*size; i++ )
		{
			if (((i/size) ^ i) & 1)
			{
				checker[3*i+0] = 200;
				checker[3*i+1] = 32;
				checker[3*i+2] = 32;
			}
			else
			{
				checker[3*i+0] = 200;
				checker[3*i+1] = 200;
				checker[3*i+2] = 32;
			}
		}

		// Make texture which is accessible through floorTexID. 
		glGenTextures( 1, &floorTexID );				
		glBindTexture(GL_TEXTURE_2D, floorTexID);		
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		glTexImage2D(GL_TEXTURE_2D, 0, 3, size, size, 0, GL_RGB, GL_UNSIGNED_BYTE, checker);
	}
	// initialize cow
	{

		// Read information from cow.obj.
		cow = new WaveFrontOBJ( "cow.obj" );

		// Make display list. After this, you can draw cow using 'cowID'.
		cowID = glGenLists(1);				// Create display lists
		glNewList(cowID, GL_COMPILE);		// Begin compiling the display list using cowID
		cow->Draw();						// Draw the cow on display list.
		glEndList();						// Terminate compiling the display list. Now, you can draw cow using 'cowID'.
		glPushMatrix();						// Push the current matrix of GL into stack.
		glLoadIdentity();					// Set the GL matrix Identity matrix.
		glTranslated(0,-cow->bbmin.y,-8);	// Set the location of cow.
		glRotated(-90, 0, 1, 0);			// Set the direction of cow. These information are stored in the matrix of GL.
		// Read the modelview matrix about location and direction set above, and store it in cow2wld matrix.
		cow2wld.getCurrentOpenGLmatrix(	GL_MODELVIEW_MATRIX);

		glPopMatrix();						// Pop the matrix on stack to GL.
	}
	// initialize bethoben
	{

		// Read information from beethovan.obj.
		bet = new WaveFrontOBJ( "beethovan.obj" );

		// Make display list. After this, you can draw beethovan using 'betID'.
		betID = glGenLists(1);
		glNewList(betID, GL_COMPILE);
		bet->Draw();
		glEndList();
		glPushMatrix();
		glLoadIdentity();
		glTranslated(0,-bet->bbmin.y,8);
		glRotated(180, 0, 1, 0);
		// bet2wld will become T * R
		bet2wld.getCurrentOpenGLmatrix(GL_MODELVIEW_MATRIX);
		glPopMatrix();
	}
	// intialize camera model.
	{
		cam = new WaveFrontOBJ("camera.obj");	// Read information of camera from camera.obj.
		camID = glGenLists(1);					// Create display list of the camera.
		glNewList(camID, GL_COMPILE);			// Begin compiling the display list using camID.
		cam->Draw();							// Draw the camera. you can do this job again through camID..
		glEndList();							// Terminate compiling the display list.

		// initialize camera frame transforms.
		for (int i=0; i < cameraCount; i++ )
		{
			double* c = cameras[i];											// 'c' points the coordinate of i-th camera.
			wld2cam.push_back(matrix4());								// Insert {0} matrix to wld2cam vector.
			glPushMatrix();													// Push the current matrix of GL into stack.
			glLoadIdentity();												// Set the GL matrix Identity matrix.
			gluLookAt(c[0],c[1],c[2], c[3],c[4],c[5], c[6],c[7],c[8]);		// Setting the coordinate of camera.
			wld2cam[i].getCurrentOpenGLmatrix(GL_MODELVIEW_MATRIX);
			glPopMatrix();													// Transfer the matrix that was pushed the stack to GL.
			matrix4 invmat;
			invmat.inverse(wld2cam[i]);
			cam2wld.push_back(invmat);
		}
		cameraIndex = 0;
	}
}

/*********************************************************************************
* Call this part whenever mouse button is clicked.
**********************************************************************************/
#if GLFW_VERSION_MAJOR==3
void onMouseButton(GLFWwindow* window, int button, int state, int __mods)
#else
void onMouseButton(int button, int state)
#endif
{
	const int GLFW_DOWN=1;
	const int GLFW_UP=0;
   	int x, y;

#if GLFW_VERSION_MAJOR==3
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, & ypos);
	x=xpos;y=ypos;
#else
	glfwGetMousePos(&x, &y);
#endif

	if (button == GLFW_MOUSE_BUTTON_LEFT)
	{
		if (state == GLFW_DOWN)
		{
			if(!clicked && cursorOnCowBoundingBox)
				clicked = true;
			else if(clicked && !ready) {
				isDrag=V_DRAG;	
				printf( "Left mouse down-click at (%d, %d)\n", x, y );
				// start vertical dragging
				// TODO: you probably need a state variable.
				dupCow[numCow++] = cow2wld;
				if(numCow == 6 ){
					//make catmull-rom spline
				}			
			}	
		}
		else if(state == GLFW_UP)
		{
			if(clicked && !ready) {
			isDrag=H_DRAG;
			printf( "Left mouse up\n");
			// start horizontal dragging using mouse-move events.
			}
		} 
	}
	else if (button == GLFW_MOUSE_BUTTON_RIGHT)
	{
		if (state == GLFW_DOWN)
			printf( "Right mouse click at (%d, %d)\n",x,y );
	}
}



/*********************************************************************************
* Call this part whenever user drags mouse. 
* Input parameters x, y are coordinate of mouse on dragging. 
**********************************************************************************/
#if GLFW_VERSION_MAJOR==3
void onMouseDrag( GLFWwindow* window, double fx, double fy)
{
	int x=fx;
	int y=fy;
#else
void onMouseDrag( int x, int y)
{
#endif
	if (isDrag) 
	{
		printf( "in drag mode %d\n", isDrag);
		if (isDrag==V_DRAG)
		{
			// vertical dragging
			// TODO:
			// create a dragging plane perpendicular to the ray direction, 
			// and test intersection with the screen ray.
			if(cursorOnCowBoundingBox){
				Ray ray;
				screenCoordToRay(x, y, ray);
				PickInfo &pp = pickInfo;

			}
		}
		else
		{ 
			// horizontal dragging
			// Hint: read carefully the following block to implement vertical dragging.
			if(cursorOnCowBoundingBox)
			{
				Ray ray;
				screenCoordToRay(x, y, ray);
				PickInfo &pp=pickInfo;
				Plane p(vector3(0,1,0), pp.cowPickPosition);
				std::pair<bool, double> c=ray.intersects(p);

				vector3 currentPos=ray.getPoint(c.second);	

				matrix4 T;
				T.setTranslation(currentPos-pp.cowPickPosition, false);
				cow2wld.mult(T, pp.cowPickConfiguration);
			}
		}
	}
	else
	{
		Ray ray;
		screenCoordToRay(x, y, ray);

		std::vector<Plane> planes;
		vector3 bbmin(cow->bbmin.x, cow->bbmin.y, cow->bbmin.z);
		vector3 bbmax(cow->bbmax.x, cow->bbmax.y, cow->bbmax.z);

		planes.push_back(makePlane(bbmin, bbmax, vector3(0,1,0)));
		planes.push_back(makePlane(bbmin, bbmax, vector3(0,-1,0)));
		planes.push_back(makePlane(bbmin, bbmax, vector3(1,0,0)));
		planes.push_back(makePlane(bbmin, bbmax, vector3(-1,0,0)));
		planes.push_back(makePlane(bbmin, bbmax, vector3(0,0,1)));
		planes.push_back(makePlane(bbmin, bbmax, vector3(0,0,-1)));


		std::pair<bool,double> o=ray.intersects(planes);
		cursorOnCowBoundingBox=o.first;
		PickInfo &pp=pickInfo;
		pp.cursorRayT=o.second;
		pp.cowPickPosition=ray.getPoint(pp.cursorRayT);
		pp.cowPickConfiguration=cow2wld;
		matrix4 invWorld;
		invWorld.inverse(cow2wld);
		// the local position (relative to the cow frame) of the pick position.
		pp.cowPickPositionLocal=invWorld*pp.cowPickPosition;
	}

}

/*********************************************************************************
* Call this part whenever user types keyboard. 
**********************************************************************************/
#if GLFW_VERSION_MAJOR==3
void onKeyPress(GLFWwindow *__win, int key, int __scancode, int action, int __mods)
#else
void onKeyPress( int key, int action)
#endif
{
	if (action==GLFW_RELEASE)
		return 	; // do nothing
	// If 'c' or space bar are pressed, alter the camera.
	// If a number is pressed, alter the camera corresponding the number.
	if ((key == ' ') || (key == 'c'))
	{    
		printf( "Toggle camera %d\n", cameraIndex );
		cameraIndex += 1;
	}      
	else if ((key >= '0') && (key <= '9'))
		cameraIndex = key - '0';

	if (cameraIndex >= (int)wld2cam.size() )
		cameraIndex = 0;
}
void screenCoordToRay(int x, int y, Ray& ray)
{
	int height , width;
#if GLFW_VERSION_MAJOR==3
	glfwGetWindowSize(g_window, &width, &height);
#else
	glfwGetWindowSize(&width, &height);
#endif

	matrix4 matProjection;
	matProjection.getCurrentOpenGLmatrix(GL_PROJECTION_MATRIX);
	matProjection*=wld2cam[cameraIndex];
	matrix4 invMatProjection;
	invMatProjection.inverse(matProjection);

	vector3 vecAfterProjection, vecAfterProjection2;
	// -1<=v.x<1 when 0<=x<width
	// -1<=v.y<1 when 0<=y<height
	vecAfterProjection.x = ((double)(x - 0)/(double)width)*2.0-1.0;
	vecAfterProjection.y = ((double)(y - 0)/(double)height)*2.0-1.0;
	vecAfterProjection.y*=-1;
	vecAfterProjection.z = -10;

	//std::cout<<"cowPosition in clip coordinate (NDC)"<<matProjection*cow2wld.getTranslation()<<std::endl;
	
	vector3 vecBeforeProjection=invMatProjection*vecAfterProjection;

	// camera position
	ray.origin()=cam2wld[cameraIndex].getTranslation();
	ray.direction()=vecBeforeProjection-ray.origin();
	ray.direction().normalize();

	//std::cout<<"dir" <<ray.direction()<<std::endl;

}

//------------------------------------------------------------------------------
int main( int argc, char* argv[] )
{
	width = 800;
	height = 600;
	int BPP=32;
	glfwInit(); // Initialize openGL.
#if GLFW_VERSION_MAJOR==3
	GLFWwindow* window = glfwCreateWindow(800, 600, "OpenGL Tutorialal", NULL, NULL);
	g_window=window;
	if(!window) {glfwTerminate(); return 1;}
	glfwMakeContextCurrent(window);

	initialize();									// Initialize the other thing.

	glfwSetKeyCallback(window, onKeyPress);
	glfwSetMouseButtonCallback(window, onMouseButton);
	glfwSetCursorPosCallback(window, onMouseDrag);

	while (!glfwWindowShouldClose(window))
	{
		display();
        glfwSwapBuffers(window);
		glfwPollEvents();
	}
#else
	// Create a window (8-bit depth-buffer, no alpha and stencil buffers, windowed)
   	glfwOpenWindow(width, height, BPP/4, BPP/4, BPP/4, 1, 8, 1, GLFW_WINDOW) ;
	glfwSetWindowTitle("Simple Scene");				// Make window whose name is "Simple Scene".
	int rv,gv,bv;
	glGetIntegerv(GL_RED_BITS,&rv);					// Get the depth of red bits from GL.
	glGetIntegerv(GL_GREEN_BITS,&gv);				// Get the depth of green bits from GL.
	glGetIntegerv(GL_BLUE_BITS,&bv);				// Get the depth of blue bits from GL.
	printf( "Pixel depth = %d : %d : %d\n", rv, gv, bv );
	initialize();									// Initialize the other thing.

	glfwSetKeyCallback(onKeyPress);					// Register onKeyPress function to call that when user presses the keyboard.
	glfwSetMouseButtonCallback(onMouseButton);		// Register onMouseButton function to call that when user moves mouse.
	glfwSetMousePosCallback(onMouseDrag);			// Register onMouseDrag function to call that when user drags mouse.

	while(glfwGetWindowParam(GLFW_OPENED) )
	{
		display();
		glfwSwapBuffers();
	}
#endif
	return 0;
}
