#pragma once
/**********************************************************
* OpenGL445Setup.h
* 
* This header file contains initialization function calls and set-ups
for basic 3D CS 445/545 Open GL (Mesa) Programs that use the GLUT/freeglut.
The initializations inclove defininga callback gandler (my_reshape_function)
that sets viewing parametes for othographic  3D display.

	TSN 01/2022 version - for OpenGL 4.3 w/legacy compantiblity
	Updated NVE 3/2023

***********************************************************/

/* reshape the callback handler - defines viewing parameters (projection) */
void my_3d_projection(int width, int height)
{
	GLfloat width_bound, height_bound;
	width_bound = (GLfloat)width; height_bound = (GLfloat)height;
	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-width_bound/2, width_bound/2, -height_bound/2, height_bound/2, 1, 100.0);
	//glFrustum(-300.0, 300.0, -300.0, 300.0, -1.0, -100.0);
	glMatrixMode(GL_MODELVIEW);
}

#define STRT_X_POS 25
#define STRT_Y_POS 25

/* initializaion routine */

void my_setup(int width, int height, char* window_name_str)
{
	// Allow for current OpenGL4.3 but backwards compatiblity to legacy GL 2.1
	glutInitContextVersion(4, 3);
	glutInitContextProfile(GLUT_COMPATIBILITY_PROFILE);
	// To get double buffering, uncomment the following line
	glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB);
	// below code line does single buffering if above line is uncommented,
	// the single buffering line will have to be commented out
	// glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);

	glutInitWindowSize(width, height);
	glutInitWindowPosition(STRT_X_POS, STRT_Y_POS);
	
	glutCreateWindow(window_name_str);
	glewExperimental = GL_TRUE;
	glewInit();

	glutReshapeFunc(my_3d_projection);
}
