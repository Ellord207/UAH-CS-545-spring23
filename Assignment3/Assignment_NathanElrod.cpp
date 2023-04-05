#include "pch.h"
#include <GL/glew.h>
#include <GL/freeglut.h>
#include "OpenGL445Setup.h"
/********************************** Assignment 3: Nathan Elrod ********************************
* Scene acts as the orchestrator of the graphics objects, event handling, and scene state.  The 
*	possible states are defined by SceneState.  All objects are stored in scene as either
* 	objs_actors or objs_ui. This program considers actors as objects that exist in the scene
* 	(e.g. Ring and Post) and UI objects (e.g. text)  as existing on a single z plane (-1).
* 	scene.setupEnvironment registers all the handlers (i.e. display_func, timer_handler,
* 	mouse_handler, keyboard_handler).
* 
* User inputs are managed by 2 event handlers.
* 	keyboard_handler manages the keyboard presses.  These effect velocity inputs and allows
* 		toggling between camera perspectives.
* 	mouse_handler handles mouse clicks that triggers, “GO”.
* 
* Rendering graphics to the scene is handled by display_func. This handler ensures a consistent
* 	Transformation Matrix before calling the processScene and processUi.
* 	processScene applies a simple physics model to the ring by tracking it’s velocity between frames.
* 		The time between frames is calculated as delta.  Delta is used to scale the vertical velocity
* 		to create a smooth animation independent of actual frame rate.  This creates inconsistency in
* 		when the ring crosses the y=-250 axis.
* 	procssUi intiailizes an orthographic camera that is used that with the ui objects.  This ensures
* 		they are clear if the user swaps to perspective viewing.
* 
* Animation is achieved by timer_handler being triggered ever 40msec with the id TimerId::LOOP. Text
* 	prompts to the screen are draw by the object GOut.  This object will buffer the text information
* 	until render time.  timer_handler also moves the state forward after letting the ring settle with
* 	tTimerId::PAUSE.
EXTRA CREDIT
	1. Included a score field in the program's ui.
***********************************************************************************************/
#define canvas_Width 600 /* width of canvas - you may need to change this */
#define canvas_Height 600 /* height of canvas - you may need to change this */
#define canvas_Name (char*)"ASSIGNMENT_3" /* name in window top */

/*
* The basic graphics objects
*/
class GfxObject
{
public:
	/* Defines the origin of the object. */
	float originX;
	float originY;
	float originZ;

	/* Create a basic graphics object, in "3D" space.
	 * X: Defines the X position of the origin of this object.
	 * Y: Defines the Y position of the origin of this object.
	 * Z: Is not allowed and assumed to be -1.
	*/
	GfxObject(float x, float y, float z)
	{
		originX = x;
		originY = y;
		originZ = z;
	}

	// Calling this method will handle the OpenGL calls to render this object to the canvas.
	void draw(void* args)
	{
		// Ensure any higher level transforms are not disrupted by transformed applied by this object.
		glPushMatrix();
		// Translate to origin
		glTranslatef(originX, originY, originZ);

		// Draw specific geometry for this object
		_drawInLocalSpace(args);

		// Return the transform to the stack
		glPopMatrix();
	}

protected:
	virtual inline void _drawInLocalSpace(void* args) = 0;
};

// Assumes origin at bottom left.  Only work in 2D space ignoring the z axis
class CollisionGfxObject : virtual public GfxObject
{
public:
	virtual inline float width() = 0;
	virtual inline float height() = 0;

	// Test that 2 Collision Objects have overlapping area
	bool testCollision(CollisionGfxObject* b)
	{
		auto a = this;
		// Test overlaping on both the x and y axis.
		// test along the x axis
		return (
			a->originX + a->width() >= b->originX &&
			a->originX <= b->originX + b->width() &&
			// test along the y axis
			a->originY + a->height() >= b->originY &&
			a->originY <= b->originY + b->height()
			);
	}

	// Test that this Collision Object engulfs the other Collision Object
	bool testEngulfs(CollisionGfxObject* b)
	{
		auto a = this;
		// Test overlaping on both the x and y axis.
		// test along the x axis
		return (
			a->originX + a->width() >= b->originX + b->width() &&
			a->originX <= b->originX &&
			// test along the y axis
			a->originY + a->height() >= b->originY + b->height() &&
			a->originY <= b->originY
			);
	}

	// Test that this Collision Object engulfs the other Collision Object
	bool testEngulfs(float bX, float bY)
	{
		auto a = this;

		// Test overlaping on both the x and y axis.
		return (
			a->originX + a->width() >= bX &&
			a->originX <= bX &&
			// test along the y axis
			a->originY + a->height() >= bY &&
			a->originY <= bY
			);
	}

	// Test that this Collision Object engulfs the other Collision Object
	bool testEngulfsScreenSpace(float bX, float bY)
	{
		// move x and y into world space
		bX -= 300; // origin offset
		bY -= 300; // origin offset
		bY *= -1; // reverse the axis

		return testEngulfs(bX, bY);
	}

protected:
	// draws a box around the collision area
	void drawBoundingBox(void* ptr)
	{
		// debugging
		if (ptr != nullptr)
		{
			if (testEngulfs(static_cast<CollisionGfxObject*>(ptr)))
				glColor3f(1.0f, 0.0f, 0.0f);
			else
				glColor3f(0.0f, 1.0f, 0.0f);
		}

		int w = static_cast<int>(width()), h = static_cast<int>(height());
		glBegin(GL_LINE_LOOP); // Drawing a box
		glVertex3i(0, 0, -1);
		glVertex3i(w, 0, -1);
		glVertex3i(w, h, -1);
		glVertex3i(0, h, -1);
		glEnd();
	}
};

// replacement for std::cout using GLUT to render the characters.  Is not effect intial render position.
class GOut : public virtual GfxObject {
private:
	GLvoid* _font;
	// Character buffer and size
	char* _buffer;
	size_t _size;

	// resize buffer
	void resize(size_t new_size)
	{
		if (new_size > _size)
		{
			// create a new buffer
			char* new_buffer = new char[new_size];
			if (_buffer)
			{
				// copy old memeory to new buffer
				memcpy(new_buffer, _buffer, _size);
				delete[] _buffer;
			}
			// replace old buffer
			_buffer = new_buffer;
		}
	}
protected:
	// To be used for inharitance, removes the virtual arguments for gfx.  Please provide GOut::UI_Z_PLANE as GfxObject's z parameter.
	GOut() : GfxObject(0, 0, UI_Z_PLANE), _font(nullptr), _buffer(nullptr), _size(0) {};
	// Support for colors
	static const float UI_Z_PLANE;
	float primaryColor[3] = { 1.0f, 1.0f, 1.0f };
public:
	// Constructor
	GOut(float x, float y) : GfxObject(x, y, UI_Z_PLANE), _font(nullptr), _buffer(nullptr), _size(0) {}

	// Evaluates as the isReady method
	operator bool() { return isReady(); }

	// Returns if the stream is ready for writing
	bool isReady()
	{
		return _font != nullptr && _size > 0;
	}

	// Stream characters into the buffer for drawing later
	GOut& operator << (const char* str)
	{
		size_t len = strlen(str);
		resize(_size + len);
		memcpy(_buffer + _size, str, len);
		_size += len;
		return *this;
	}

	// Stream integers into the buffer for drawing later
	GOut& operator << (int number)
	{
		if (number == 0)
		{
			resize(_size + 1);
			_buffer[_size++] = '0';
			return *this;
		}

		int digitCount = 0;
		// num_digits = floor(log10(n)) + 1;  // Are we able to use cmath?
		for (int n = number; n != 0; n /= 10) digitCount++;

		bool isNegative = false;
		if (number < 0) {
			isNegative = true;
			number = -number;
		}
		resize(_size + digitCount + (isNegative ? 1: 0));
		
		if (isNegative)
		{
			_buffer[_size++] = '-';
		}
		// move index to the end of the buffer and work way backward
		_size += digitCount;
		while (number > 0) {
			int digit = number % 10;
			char ch = digit + '0';
			_buffer[--_size] = ch;
			number /= 10;
		}
		// reset size again
		_size += digitCount;
		return *this;
	}

	// clears the buffer and triggers a redraw
	GOut& clear()
	{
		// glutPostRedisplay();
		return start(nullptr);
	}

	// Only supports 1 color change perline of buffer
	GOut& setColor(float* color3fv)
	{
		void* dst;
		dst = primaryColor;

		memcpy(dst, color3fv, sizeof(float) * 3);
		return *this;
	}

	// Clears the current buffer and sets the font for the next text to draw
	GOut& start(GLvoid* font)
	{
		_font = font;
		_size = 0;
		return *this;
	}

	// The boarding way to write to the text buffer
	void write(GLvoid* font, const char* msg)
	{
		start(font) << msg;
	}
protected:
	// Triggers the calls to render the character.
	void _drawInLocalSpace(void* args)
	{
		if (isReady())
		{
			// Posision will be handled by the transform.  Just resets the raster posision to the "start"
			glColor3fv(primaryColor);
			glRasterPos2i(0, 0);
			for (size_t i = 0; i < _size; i++)
			{
				glutBitmapCharacter(_font, _buffer[i]);
			}
		}
	}
};
// The z plane which the UI elements will exist on
const float GOut::UI_Z_PLANE = -1.0f;

// Acts as a GOut object, but includes methods to know if the mouse hovering things object and renders a bounding box around it.
class UIObject : public GOut, public CollisionGfxObject
{
private:
	float _w, _h;
public:
	// initializes the x/y of the text and height and width of the bounding box
	UIObject(float x, float y, float h, float w) : GOut(/*origin defined by GfxObject*/), _h(h), _w(w), GfxObject(x, y, GOut::UI_Z_PLANE) {}

	float inline width() { return _w; }
	float inline height() { return _h; }
protected:
	// Triggers the calls to render the character.
	void _drawInLocalSpace(void* args)
	{
		glColor3fv(primaryColor);
		GOut::_drawInLocalSpace(args);
		drawBoundingBox(args);
	}
};

class Post : public CollisionGfxObject {
public:
	Post(float x, float y, float z) : GfxObject(x /*Offset for hitbox*/ - 6.0f, y /*Offset for hitbox*/ + 6, z) {}

	inline float width() { return 12; }
	inline float height() { return 12 * 4; }

protected:
	// Triggers the calls to render the character.
	inline void _drawInLocalSpace(void* args)
	{
		glColor3f(0.0f, 0.7f, 1.0f);
		// Collision logic requires bottom left to be the origin.
		//  Since the shape is drawn at the origin and the "start location" is
		//	defined in the assignment, the best way to do this is hide
		//	that the origin is actually being moved.  The graphics are
		//	translated back to complete the hack
		glTranslatef(6.0f, -6.0f, 0.0f);
		glRotatef(30.0f, 0.0f, 1.0f, 0.0f);
		glutWireCube(12);
		for (int i = 0; i < 3; i++)
		{
			glTranslatef(0.0f, 12.0f, 0.0f);
			glutWireCube(12);
		}
	}
};

class Tori : public CollisionGfxObject {
public:
	Tori(float x, float y, float z) : GfxObject(x /*Offset for hitbox*/ - 15, y /*Offset for hitbox*/ - 20, z) {}

	inline float width() { return 30; }
	inline float height() { return 30; }

	float* color = new float[3] { 1.0f, 1.0f, 0.0f, };
protected:
	// Triggers the calls to render the character.
	inline void _drawInLocalSpace(void* args)
	{
		glColor3fv(color);
		// Collision logic requires bottom left to be the origin.
		//  Since the shape is drawn at the origin and the "start location" is
		//	defined in the assignment, the best way to do this is hide
		//	that the origin is actually being moved.  The graphics are
		//	translated back to complete the hack
		glTranslatef(15.0f, 20.0f, 0.0f);

		glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
		glutWireTorus(15.0, 45.0, 12, 12);
	}
};

// Handles the Graphics Objects, states related to the scene, and handlers for GLUT Events.  Created instanced.
class Scene {
private:
	// The number of milliseconds the timer will count before triggering an event.
	int const static _timerMsec = 40;
	int lastFrame = 0; // time the last frame was drawn
	bool orthoMode = true; // toggle between ortho and persective
	int score = 0; // player's current score
	int throwNum = 0; // number of rings tossed
	float velocityv[2] = { 5, 0 }; // current velocity store as [x, y]

	// Object Handles
	GOut* msg_score, * msg_status;
	UIObject * msg_go, * msg_velocity;
	Tori *ring;
	Post* post;
public:
	enum SceneState {
		WAIT, // Waiting for GO
		THROW,// Ring is flying, waiting to pass y=-240
		SETTLEING,// Ring is falling stright down to y=-300
		SETTLED,// Pausing for 1 sec before resetting the ring
		DONE, // Game Over
	};
	enum TimerId {
		LOOP = 0, // Rendering event
		PAUSE = 10,// Timer pause during settled state
	};
	// State of the scene
	SceneState state = SceneState::WAIT;
	// The number of objects in my scene.
	static const int ACTOR_COUNT = 2;
	static const int UI_COUNT = 6;
	// The graphics objects in my scene.
	GfxObject** objs_actors;
	GfxObject** objs_ui;


	// Initializes the Scene and it's objects
	Scene()
	{
		lastFrame = 0;
		GOut* msg_static_info = nullptr, *msg_static_velocity = nullptr;
		// actors exist in the model space and are drawn there
		objs_actors = new GfxObject * [ACTOR_COUNT]
		{
			post = new Post(250.0f, -300.0f, -50.0f),
			ring = new Tori(-55.0f, 260.0f, -50.0f),
			// new Fireworks(0, 0, 0),
		};
		// ui elements are drawn by a different camera on the a single z plane
		objs_ui = new GfxObject * [UI_COUNT]
		{
			msg_static_info = new GOut(-200, 287),
			msg_static_velocity = new GOut(-287.0f, -270.0),
			msg_velocity = new UIObject(-280.0f, -250.0f, 19.0f, 35.0f),
			msg_go = new UIObject(-220.0f, -250.0f, 19.0f, 35.0f),
			msg_score = new GOut(-280, -100),
			msg_status = new GOut(-80.0f, 260.0f),
		};
		// No need for a scene handle. Messages are unchanging
		msg_static_info->start(GLUT_BITMAP_9_BY_15) << "Enter Toss Velocity and then GO to Toss";
		msg_static_velocity->start(GLUT_BITMAP_HELVETICA_10) << "VELOCITY";
		msg_go->start(GLUT_BITMAP_HELVETICA_18) << "GO";
	}

	// Draw all actor objects and update if nessisary
	inline void processScene()
	{
		int time = glutGet(GLUT_ELAPSED_TIME);
		int delta = time - lastFrame; // delta since last frame to animate smoothly independant of frame rate
		lastFrame = time;

		// This block applies the simple physics model to the ring
		if (state != SceneState::WAIT)
		{
			// add to my vertical volocity due to gravity
			const static float gravity_value = -32.0f;  //ft/s^2

			// apply gravity
			velocityv[1] += (gravity_value * static_cast<float>(delta) / 1000.0f);
			
			ring->originX += velocityv[0]; // apply horizonal velocity
			if (ring->originX > 300) ring->originX = 300; // limit it in canvas
			ring->originY += velocityv[1]; // apply vertical velocity
			if (ring->originY < -300)
			{
				ring->originY = -300; // limit it in canvas
				if (scene.state == SceneState::SETTLEING)
				{
					glutTimerFunc(1000, Scene::timer_handler, TimerId::PAUSE);
					scene.state = SceneState::SETTLED;
				}
			}

			// test for post
			if (state == SceneState::THROW && (ring->originY /*removing offset for collision */ + 20) <= -250)
			{
				scene.state = SceneState::SETTLEING;
				if (ring->testCollision(post)) // collision is setup in such a way that it will return true if the ring is with-in 15 of post center
					score += 10;
				velocityv[0] = 0; // remove horizontal velocity so the ring drops down
			}
		}

		void* arg = nullptr;
		for (int i = 0; i < scene.ACTOR_COUNT; i++)
		{
			GfxObject* obj = scene.objs_actors[i];
			arg = nullptr;

			// still draw the scene if should process is false
			obj->draw(arg);
		}
	}

	// Draw all ui objects and update if nessisary
	inline void processUi()
	{
		// Setup camera for UI
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glOrtho(
			-(GLfloat)canvas_Width / 2.0f, (GLfloat)canvas_Width / 2.0f,
			-(GLfloat)canvas_Width / 2.0f, (GLfloat)canvas_Width / 2.0f,
			(GLdouble)0.5, (GLdouble)5.0
		);

		// Update active ui elements
		msg_score->start(GLUT_BITMAP_HELVETICA_18) << "SCORE: " << score;
		msg_velocity->start(GLUT_BITMAP_HELVETICA_18) << velocityv[0];

		glMatrixMode(GL_MODELVIEW);
		// Unnssisary, but recommended
		glPushMatrix();
		glLoadIdentity();

		void* arg = nullptr;
		for (int i = 0; i < scene.UI_COUNT; i++)
		{
			GfxObject* obj = scene.objs_ui[i];

			arg = nullptr;

			// still draw the scene if should process is false
			obj->draw(arg);
		}
		glPopMatrix();

		glMatrixMode(GL_PROJECTION);
		glPopMatrix();

		glMatrixMode(GL_MODELVIEW);
	}

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

		glPolygonMode(GL_FRONT_AND_BACK , GL_LINE);
		
		// Unnessisary, but recommended
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		// Draw Objects
		scene.processScene();

		// Draw UI elements
		scene.processUi();

		glutSwapBuffers();
	}

	// Triggered by a timer giving ~30 fps
	void static timer_handler(int id)
	{
		// trigger re-render (glutDisplayFunc)
		glutPostRedisplay();

		if (id == TimerId::LOOP && scene.state != SceneState::DONE)
			// set timer for next frame
			glutTimerFunc(_timerMsec, timer_handler, TimerId::LOOP);

		if (id == TimerId::PAUSE)
		{
			// reset game
			scene.ring->originX = -55.0f;
			scene.ring->originY = 260.0f;
			scene.ring->originZ = -50.0f;

			scene.velocityv[0] = 5;
			scene.velocityv[1] = 0;
			scene.state = SceneState::WAIT;
			scene.throwNum++;

			switch (scene.throwNum)
			{
			case 0:
				scene.ring->color[0] = 1.0f;
				scene.ring->color[1] = 1.0f;
				scene.ring->color[2] = 0.0f;
				break;
			case 1:
				scene.ring->color[0] = 1.0f;
				scene.ring->color[1] = 0.78f;
				scene.ring->color[2] = 0.0f;
				break;
			case 2:
				scene.ring->color[0] = 0.0f;
				scene.ring->color[1] = 0.2f;
				scene.ring->color[2] = 1.0f;
				break;
				// Game over
			default:
				scene.state = SceneState::DONE;
				scene.msg_status->start(GLUT_BITMAP_TIMES_ROMAN_24) << "GAME OVER";
				// draw ring black to hide it
				scene.ring->color[0] = 0.0f;
				scene.ring->color[1] = 0.0f;
				scene.ring->color[2] = 0.0f;
				break;
			}
		}
	}

	void static mouse_handler(int button, int state, int x, int y)
	{
		if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN
			&& scene.state == SceneState::WAIT
			&& scene.msg_go->testEngulfsScreenSpace(x, y))
		{
			scene.state = SceneState::THROW;
		}
	}

	// Handler for keyboard presses (ignores character pressed and x/y)
	void static keyboard_handler(unsigned char key, int x, int y)
	{
		switch (key)
		{
		case 'p':
		case 'P':
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			if (scene.orthoMode)
			{
				glOrtho(
					-(GLfloat)canvas_Width / 2.0f, (GLfloat)canvas_Width / 2.0f,
					-(GLfloat)canvas_Width / 2.0f, (GLfloat)canvas_Width / 2.0f,
					(GLdouble)1, (GLdouble)100.0
				);
			}
			else
			{
				// This doesn't look right, but it matches what was said in the assignment
				gluPerspective(175, 1, 1, 100);
			}
			scene.orthoMode = !scene.orthoMode;
			glMatrixMode(GL_MODELVIEW);
			break;
		default:
			// Only allow inputs during the wait state
			if (scene.state != SceneState::WAIT) break;
			UIObject* vel_ui = dynamic_cast<UIObject*>(scene.msg_velocity);
			if (vel_ui != nullptr
				&& vel_ui->testEngulfsScreenSpace(x, y) // is in area
				&& key >= '0' && key <= '9' // is number
				)
			{
				int n = key - '0';
				int hv = scene.velocityv[0];
				hv = hv % 10 * 10 + n;
				scene.velocityv[0] = hv;
			}
			break;
		}
	}

	// registers event handlers and starts the timer for the next frame
	void setupEnvironment()
	{
		glutDisplayFunc(Scene::display_func); /* registers the display callback */
		glutKeyboardFunc(Scene::keyboard_handler);
		glutMouseFunc(Scene::mouse_handler);

		scene.lastFrame = glutGet(GLUT_ELAPSED_TIME); // set a more accurate "lastFrame"

		// starts the rendering event timer
		Scene::timer_handler(TimerId::LOOP);
		scene.state = SceneState::WAIT;
	}

	// The static scene object that will be rendered
} scene;

int main(int argc, char** argv)
{
	glutInit(&argc, argv);

	my_setup(canvas_Width, canvas_Height, canvas_Name);

	scene.setupEnvironment();

	glutMainLoop(); /* execute until killed */
	return 0;
}