#include "pch.h"
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>
#include "OpenGL445Setup.h"
/********************************** Assignment 1: Nathan Elrod ********************************
* There 2 Major structures:
* 1. GfxObject Class & its children
*	This class is the base "graphics object" of my Scene.  The object has an origin describing where in the scene
*		the object exists, and a "draw" method that makes the OpenGL calls to add the graphics to the canvas.
*	There are 3 children objects (Fowl, Towers, Ys(slingshots)).
*	Fowl has a "process" method which updates the object for the next frames.
* 2. The Scene Class and it's GLUT event handlers
*	This class tracks the object and state of the scene and the GLUT handlers for the events needed to interact with it.
*	The scene has 3 states, waiting, forward and backward.  The Keybourd handler checks this state to know which id to
*		pass to glutTimerFunc.
*	It assumes the canvas and display handler have all been initialized before calling "promptState" for the first time.
* Event Handling:
*	The Scene assumes that glutDisplayFunc(...) has been set.  The display handler ONLY clears the current canvas and
*		draws the objects currently in the scene.  Once scene.promptState(...) is called, the scene will
*		pause until a key has been pressed and handled by the glutKeyboardFunc(...) registered handler.  This handler
*		will change the state of the scene and start a timer using glutTimerFunc(...).  This timer will trigger events
*		twice every 30-40 msec.  This was choose to have a smoother animation as computation is not a reasonable
*		concern in this assignment.  The timer event handler will first test that the Fowl haven't "collided" with the 
*		tower, and if not, will update the fowl graphics objects (position and scale).  Once this is done, the handler
*		start a timer for the next frame using glutTimerFunc and trigger a new display event in GLUT using
*		glutPostRedisplay().  The timer has 2 ids that determines if the scene should animate left(forward) or
*		right(backward).  Once a "collision" has occurred the timer event handler will prompt for the user to 
*		send the fowl back to the left.  The leftward animation follows the same event handling layed out, but
*		negating the x offset (movement) and scaling factor delta.  
* EXTRA CREDIT:
*	1. Added frowny faces and tufts of hair to the fowl.
*	2. The user is prompted to start the animation moving leftward once the fowl collides with the tower.
*	3. The Fowl will shrink as they move rightward, and grow as the movey leftward.
***********************************************************************************************/

#define canvas_Width 480 /* width of canvas - you may need to change this */
#define canvas_Height 480 /* height of canvas - you may need to change this */
#define canvas_Name (char*)"ASSIGNMENT_1" /* name in window top */

/*
* The basic graphics objects
*/
class GfxObject
{
protected:
	/* A helper method that wraps the OpenGL glVectex3f.
	* Arguments X & Y are defined by the object's space & are translated to the object's origin before being added as vertices.
	* Arguments are converted to GLfloat before being added.
	* Z is assumed to be -1
	*/
	void addVertexf(float x, float y)
	{
		glVertex3f((GLfloat)(originX + x), (GLfloat)(originY + y), -1.0);
	}

	/* A helper method that wraps the OpenGL glVectex3f.
	* Arguments X & Y are defined by the object's space & are translated to the object's origin before being added as vertics.
	* Z is assumed to be -1
	*/
	void addVertexi(int x, int y)
	{
		glVertex3i(static_cast<int>(originX) + x, static_cast<int>(originY) + y, -1);
	}
public:
	/* Defines the origin of the object. */
	float originX;
	float originY;

	/* Create a basic graphics object, in "3D" space.
	 * X: Defines the X position of the origin of this object.
	 * Y: Defines the Y position of the origin of this object.
	 * Z: Is not allowed and assumed to be -1.
	*/ 
	GfxObject(float x, float y)
	{
		originX = x;
		originY = y;
	}

	// Calling this method will handle the OpenGL calls to render this object to the canvas.
	virtual void draw() = 0;
};

/* A graphics object representing a fowl.
* Note: The origin defines the inexact CENTER of the object.
*	This was done so that the shrinking of the fowl would look it's best.
*/
class Fowl : public GfxObject
{
private:
	// EXTRA CREDIT
	// The current amount to scale the vertex of the fowl
	float _scaleFactor = 1.0;
	/*
	* 545 short side
	* Math for a right triangle with a hypotenuse of 5
	* Side c = 5
	* Side b = 3.53553
	* Side a = 3.53553
	*/
	float _shortSide = 3.53553;

	/*
	* Main body
	* Math for a right triangle with a hypotenuse of 20
	* Side c = 20
	* Side b = 14.14214
	* Side a = 14.14214
	*/
	float _longSide = 14.14214;

protected:
	// A wrapper the calls the parent addVertexf method after apply the scale factor
	void addVertexf(float x, float y)
	{
		// EXTRA CREDIT

		// Calling the parent after scaling the x * y
		// By just scaling the x,y in object space (without the origin) the 
		//  object maintains its position while having the vertices scaled to be closer to each other.
		GfxObject::addVertexf(x * _scaleFactor, y * _scaleFactor);
	}
public:
	// The arguments initialize the origin of the object
	Fowl(float x, float y) : GfxObject(x,y) {}

	// Returns the width of this object float.
	float width() { return _longSide * 2.0 * _scaleFactor; }
	// Returns the height of this object as a float.  Yes, this method is unused, but it feels wrong to have a width method and no height method.
	float height() { return _longSide + _shortSide * _scaleFactor;; }

	// Updates the x position and scale factor
	void process(float xOffset, float scaleOffset)
	{
		originX += xOffset;
		_scaleFactor += scaleOffset;
	}

	// Calling this method will handle the OpenGL calls to render this object to the canvas.
	void draw()
	{
		// Main Body
		glColor3f(1.0, 0.87, 0.0); // Yellowish orange
		glBegin(GL_LINE_LOOP);
			addVertexf(+_longSide - _shortSide	, -_shortSide);	// Bottom Right
			addVertexf(+_longSide				, 0			 );	// Middle right
			addVertexf(0						, _longSide	 );	// Top
			addVertexf(-_longSide				, 0			 );	// Middle left
			addVertexf(-_longSide + _shortSide	, -_shortSide); // Bottom left
			// GL Loop will draw the line from bottom right to left
		glEnd();
		// EXTRA CREDIT
		// Hair tufts
		glBegin(GL_LINES);
			addVertexf(0	, _longSide		 );
			addVertexf(-2.7	, _longSide + 2.0); // points a little left

			addVertexf(0	, _longSide		 );
			addVertexf(1.5	, _longSide + 3.0); // points up

			addVertexf(0	, _longSide		 );
			addVertexf(3.0	, _longSide + 1.3); // points a little right
		
		// "Angry" Face
		// Eyes
			addVertexf(-1.0	, +8.0	); // left Eye
			addVertexf(-1.0	, +5.0	);

			addVertexf(+5.0	, +8.0	); // Right Eye
			addVertexf(+5.0	, +5.0	);
		glEnd();
		// Frown
		glBegin(GL_LINE_STRIP);
			addVertexf(-2.0	, -3.0	); // Left frown
			addVertexf(0	, 0		);
			addVertexf(2.0	, 0		);
			addVertexf(5.0	, -3.0	); // Right frown
		glEnd();
	}
};

// The object launching the fowl
class YShapedStructure : public GfxObject
{
public:
	// The arguments initialize the origin of the object
	YShapedStructure(float x, float y) : GfxObject(x, y) {}

	// Calling this method will handle the OpenGL calls to render this object to the canvas.
	void draw()
	{
		glColor3f(1.0, 0.0, 0.0); // Red
		glBegin(GL_LINE_LOOP);
			addVertexi(0	, 0  ); // bottom
			addVertexi(5	, 0	 );
			addVertexi(5	, 150);
			addVertexi(35	, 200); // Right arm
			addVertexi(30	, 205);
			addVertexi(3	, 155); // Center of the "sling shot"
			addVertexi(-40	, 220); // Left arm
			addVertexi(-45	, 215);
			addVertexi(0	, 150);
		glEnd();
	}
};

// The object the fowl are flying towards
class TowerLikeStructure : public GfxObject
{
public:
	// The arguments initialize the origin of the object
	TowerLikeStructure(float x, float y) : GfxObject(x, y) {}

	// Returns the width of this object float.
	int width() { return 5; }
	// Returns the height of this object as a float.  Yes, this method is unused, but it feels wrong to have a width method and no height method.
	int height() { return 300; }

	// Calling this method will handle the OpenGL calls to render this object to the canvas.
	void draw()
	{
		glColor3f(0.0, 1.0, 0.0); // Green
		glBegin(GL_LINE_LOOP); // Drawing a box
			addVertexi(0	,	0);
			addVertexi(width(), 0);
			addVertexi(width(), height());
			addVertexi(0	,	height());
		glEnd();
	}
};

// Handles the Graphics Objects, states related to the scene, and handlers for GLUT Events.  Created instanced.
class Scene {
private:
	// The number of milliseconds the timer will count before triggering an event.
	int const static _timerMsec = 15;
public:
	// State of the scene.  Used for handling keyboard events
	enum SceneState {
		SCENE_WAITING,
		SCENE_FORWARD,
		// EXTRA CREDIT
		SCENE_BACKWARD
	};

	// Timer Ids signaling how the timer should be handled
	enum TimerIds {
		ANIMATE_FORWARD = 0,
		// EXTRA CREDIT
		ANIMATE_BACKWARD = 1
	};

	// Current state of the scene
	SceneState state = SCENE_WAITING;

	// The number of objects in my scene.
	int const count = 5;
	// The graphics objects in my scene.
	GfxObject** objects = new GfxObject * [count];

	// The handlers are static so they exist in the global scope.
	//	(It made the compatible with GLUT, but honestly, I'm, not 100% this is why.  It's been awhile since I programmed in C++)

	// Iterates through the objects in the scene and draws each one.
	void static display_func()
	{
		/* this callback is automatically called whenever a window needs to
		be displayed or redisplayed.
		Usually this function contains nearly all of the drawing commands;
		the drawing/animation statements should go in this function. */

		// Using a black for the background
 		glClearColor(0.0, 0.0, 0.0, 1.0);
		glClear(GL_COLOR_BUFFER_BIT);
		for (int i = 0; i < scene.count; i++)
		{
			scene.objects[i]->draw();
		}

		// Was needed to be rendered on my system
		glFlush();
	}

	// Triggered by a timer giving ~30 fps
	void static timer_handler(int id)
	{
		bool isForward = id == ANIMATE_FORWARD;
		// test to see which direction the fowl are traveling
		float movementUnits = isForward ? 2.0 : -2.0;
		
		// EXTRA CREDIT
		// The amount to scale the fowl
		float scaleOffset = isForward ? -0.0015 : 0.0015;

		// Test for a "collision" with the "first" fowl
		float fowlWidth = dynamic_cast<Fowl*>(scene.objects[4])->width();
		float fowlCollisionColumn = scene.objects[4]->originX + (fowlWidth / 2.0);

		// If there is a collision prompt for next state
		float targetCollisionColumn = scene.objects[1]->originX;
		if (isForward && fowlCollisionColumn + movementUnits > targetCollisionColumn) {
			// EXTRA CREDIT
			scene.promptState("Press any 'any' key to send the fowl moving left.", SCENE_BACKWARD);
			return;
		}
		// Stops a potential crash from the fowl infantly flying off canvas
		if (!isForward && fowlCollisionColumn + movementUnits < 0)
			return;

		// set timer for next frame
		glutTimerFunc(_timerMsec, timer_handler, id);

		// Update the locations
		dynamic_cast<Fowl*>(scene.objects[2])->process(movementUnits, scaleOffset);
		dynamic_cast<Fowl*>(scene.objects[3])->process(movementUnits, scaleOffset);
		dynamic_cast<Fowl*>(scene.objects[4])->process(movementUnits, scaleOffset);

		// trigger re-render (glutDisplayFunc)
		glutPostRedisplay();
	}

	// Handler for keyboard presses (ignores character pressed and x/y)
	void static keyboard_handler(unsigned char, int, int)
	{
		int timerId = -1;
		glutKeyboardFunc(0);
		switch (scene.state)
		{
		case SCENE_FORWARD:
			timerId = ANIMATE_FORWARD;
			break;
		// EXTRA CREDIT
		case SCENE_BACKWARD:
			timerId = ANIMATE_BACKWARD;
			break;
		}
		// if the state calls for a timer, start it now.
		if (timerId != -1) {
			glutTimerFunc(_timerMsec, timer_handler, timerId);
		}
	}

	// Initializes the Scene
	Scene()
	{
		// Order of the objects matter as they are all on the same Z.
		//  Objects drawn first will be "behind" the others.
		//	As such, background objects should be drawn first.
		// --------  Background  --------
		objects[0] = new YShapedStructure(50.0, 5.0);
		objects[1] = new TowerLikeStructure(static_cast<float>(canvas_Width) - 10.0, 0);

		// --------  Foreground  --------
		float position_x = 50.0;
		float position_y = static_cast<float>(canvas_Height) / 2.0; // 190.0;
		// Create the "rear" (left) fowl
		objects[2] = new Fowl(position_x, position_y);
		float fowlWidth = dynamic_cast<Fowl*>(objects[2])->width();

		float offset = fowlWidth + 12.0;
		// Shift the position of the next one (middle)
		position_x += offset;
		objects[3] = new Fowl(position_x, position_y);
		// Shift the position of the next one (right)
		position_x += offset;
		objects[4] = new Fowl(position_x, position_y);
	}

	// Prompts the user for a input before continuing to the next state
	void promptState(const char* msg, SceneState nextState)
	{
		// Moves the state forward so the keyboard handler will know how to handle an input
		scene.state = nextState;
		// Sets the keyboard handler to wait for an input from the user.
		glutKeyboardFunc(keyboard_handler);
		// Displays prompt message on standard out.
		std::cout << msg << std::endl;
	}

// The static scene object that will be rendered
} scene;

int main(int argc, char** argv)
{
	glutInit(&argc, argv);

	my_setup(canvas_Width, canvas_Height, canvas_Name);
	glutDisplayFunc(Scene::display_func); /* registers the display callback */
	
	scene.promptState("Any Key Click Will Start", Scene::SCENE_FORWARD);

	glutMainLoop(); /* execute until killed */
	return 0;
}
