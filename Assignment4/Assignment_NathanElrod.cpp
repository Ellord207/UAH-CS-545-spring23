#include "pch.h"
#include <GL/glew.h>
#include <GL/freeglut.h>
#include "OpenGL445Setup.h"
/********************************** Assignment 4: Nathan Elrod ********************************
Scene acts as the orchestrator of the graphics objects, event handling, and scene state.  The
	possible states are defined by SceneState.  All objects are stored in the scene as either
	objs_actors or objs_ui. This program considers actors as objects that exist in the scene
	(e.g. Fish and Food) and UI objects (e.g. text)  as existing on a single z plane (-1).
	scene.setupEnvironment registers all the handlers (i.e. display_func, timer_handler,
	keyboard_handler) & and starts any timer events.

Timer_handler handles all events that occur.  It will first test if the timer originated from
	the scene. If not it will then pass the event to the actor objects for handling (e.g. Food).
	All timer event ids are defined by Scene::TimerId.

The Food manages the timer events related to when it appears (regens), or spoils.  There is
	a single food actor that is “hidden” out of the viewing region before regening.
	The food tracks which events are relevant with an Id.  Food id is embedded in each timer
	id it creates.  When handling events, it first confirms the id matches what it expected,
	then processes the event.

The Fish manages when it eats the food.  It communicates to the scene that the score should
	increase and to the food to begin regening.

User inputs are managed by keyboard_handler and enable the movement of the fish.

Rendering graphics to the scene is handled by display_func. This handler ensures a
	consistent Transformation Matrix before calling the processScene and processUi.
processScene applies a simply draws the actor objects of the scene.
procssUi initializes an orthographic camera that is used with the ui objects.  This
	ensures simpler math moving from OpenGL coordinates to screen space.

Animation is achieved by timer_handler being triggered every 40msec with the id
	TimerId::LOOP. Text prompts to the screen are drawn by the object GOut.  This object
	will buffer the text information until render time.

When the game time is completed, the scene will freeze inputs, the “Time Remaining” ui,
	and Food events.

EXTRA CREDIT
	1. The fish's tail changes direction "follow" the fish's movements.
***********************************************************************************************/
#define canvas_Width 640 /* width of canvas - you may need to change this */
#define canvas_Height 640 /* height of canvas - you may need to change this */
#define canvas_Name (char*)"ASSIGNMENT_4" /* name in window top */

/*
* The basic graphics objects
*/
class GfxObject
{
private:
	// Square Root
	float sqrt(float n) {
		float x = n;
		float y = 1;
		float e = 0.001; // the precision of the result
		while (x - y > e) {
			x = (x + y) / 2;
			y = n / x;
		}
		return x;
	}
	// Power
	float pow(float n, UINT pow) {
		float x = n;
		for (int i = 1; i < pow; i++)
			x *= n;
		return x;
	}
public:
	/* Defines the origin of the object. */
	float originX;
	float originY;
	float originZ;

	/* Create a basic graphics object, in "3D" space.
	 * X: Defines the X position of the origin of this object.
	 * Y: Defines the Y position of the origin of this object.
	 * Z: Defines the Z position of the origin of this object.
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
		// Ensure any higher-level transforms are not disrupted by this object's transformation.
		glPushMatrix();
		// Translate to origin
		glTranslatef(originX, originY, originZ);

		// Draw specific geometry for this object
		_drawInLocalSpace(args);

		// Return the transform from the stack
		glPopMatrix();
	}

	// gets the distance on a (x,y) plane between this object and arg b.
	inline float getDistance(GfxObject* b)
	{
		float xDiff = b->originX - originX;
		float yDiff = b->originY - originY;
		float distance = sqrt(pow(xDiff, 2) + pow(yDiff, 2));
		return distance;
	}

protected:
	virtual inline void _drawInLocalSpace(void* args) = 0;
};

enum DirectionDetails : UINT16 {
	Details_Neutral = 0x00,
	Details_Left = 0x01,
	Details_Right = 0x02,
	Details_Top = 0x04,
	Details_Bottom = 0x08,
};

// Replacement for std::cout using GLUT to render the characters.  Is not affected by the raster position.
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
	// To be used for inheritance, removes the virtual arguments for gfx.  Please provide GOut::UI_Z_PLANE as GfxObject's z parameter.
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
protected:
	// Triggers the calls to render the character.
	void _drawInLocalSpace(void* args)
	{
		if (isReady())
		{
			// Position  will be handled by the transform.  Just resets the raster position  to the "start"
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

class Fan : public GfxObject {
private:
	float rotation = 0;
	float rotationPerMillisecond = 360.0f / 2000.0f;
public:
	Fan(float x, float y, float z) : GfxObject(x, y, z) {}

protected:
	// Triggers the calls to render the character.
	inline void _drawInLocalSpace(void* args)
	{
		// time difference since last frame
		int delta = *static_cast<int*>(args);
		// calculate new rotation
		rotation += rotationPerMillisecond * delta;

		// float modulo
		if (rotation >= 360.0f) rotation -= 360.0f;

		glColor3f(0.5f, 0.5f, 0.0f);
		// scale first (vertics are defined as units)
		glScalef(50.0f, 50.0f, 1.0f);
		// rotate to the proper rotation
		glRotatef(static_cast<float>(rotation), 0.0f, 0.0f, 1.0f);
		// loop through rendering each fan blade
		for(int i = 0; i< 6; i++)
		{
			glBegin(GL_TRIANGLES);
			glVertex3f(0.0f, 0.0f, 0.0f);
			glVertex3f(0.3f, 1.0f, 0.0f);
			glVertex3f(-0.3f, 0.75f, 0.0f);
			glEnd();
			// rotate for the next fan blade
			glRotatef(360.0f/6.0f, 0.0f, 0.0f, 1.0f);
		}
	}
};

// Contains the bounds of the play area
class Tank : public GfxObject {
public:
	Tank(float x, float y, float z) : GfxObject(x, y, z) {}

	inline float width() { return 250; }
	inline float height() { return 250; }
	inline float depth() { return 250; }

protected:
	// Triggers the calls to render the character.
	inline void _drawInLocalSpace(void* args)
	{
		glColor3f(1.0f, 0.7f, 1.0f);
		glutWireCube(250);
	}
};

// Food Object.  Handles when it should be spoiling and appearing
class Food : public GfxObject {
private:
	int id = 0;
	inline void randPosition()
	{
		originX = static_cast<float>(rand() % 200 - 100);
		originY = static_cast<float>(rand() % 200 - 100);
		originZ = static_cast<float>(rand() % 1) - 175.5f;
	}
	inline void hide()
	{
		// out of sight
		originY = -1000.0f;
		originZ = 1000.0f;
	}
public:
	Food(float x, float y, float z) : GfxObject(x,y,z) {}

	enum EventIds : int
	{
		REGEN = 1, // 1 second delay before food spawn
		SPOILED = 2,// food didn't get eaten in time
	};

	void startRegen()
	{
		// id needs to stay between [0, 99]
		id++;
		id %= 100;
		hide();

		// set a timer for the food to regen
		setTimer(1000, (REGEN * 100) + id);
	}
	void timerEvent(int eventId)
	{
		// event id is defined as: #**
		//	# is the event id (REGEN: 1, SPOILED: 2)
		//	** is the id of the food from 0-99
		// test if the event is for this object
		if (id != (eventId % 100))
			return;
		if (SPOILED == (eventId / 100))
		{
			id++;
			startRegen();
		}
		else if (REGEN == (eventId / 100))
		{
			randPosition();
			// set a timer to spoil
			setTimer(3000, (SPOILED * 100) + id);
		}
	}
	// will be defined after timer handler as to reference it
	void setTimer(int timeMsec, int id);
protected:
	// Triggers the calls to render the character.
	inline void _drawInLocalSpace(void* args)
	{
		glColor3f(1.0f, 1.0f, 1.0f);
		glutWireSphere(5.0, 8, 5);
	}
};

// Structure of arguments to the pass the fish object when drawing
struct FishDrawArgs
{
	bool didEat;
	Food* food;
};

// Fish object manages testing for collision with food when drawing
class Fish : public GfxObject {
private:
	bool _facingLeft = false; // track the direction the fish is facing
public:
	Fish(float x, float y, float z) : GfxObject(x,y,z) {}

	inline float width() { return 90.0f; }
	inline float height() { return 25.0f; }

	// move the fish within the tank
	void swim(DirectionDetails direction, int moveStep, Tank* containingObj)
	{
		if (direction & Details_Right)
		{
			_facingLeft = false;
			originX += moveStep;
			// limit movement
			float aRightSide = originX + (width() / 2);
			float bRightSide = containingObj->originX + (containingObj->width() / 2);
			if (aRightSide > bRightSide)
				originX = bRightSide - (width() / 2);
		}
		if (direction & Details_Left)
		{
			_facingLeft = true;
			originX -= moveStep;
			// limit movement
			float aLeftSide = originX - (width() / 2);
			float bLeftSide = containingObj->originX - (containingObj->width() / 2);
			if (aLeftSide < bLeftSide)
				originX = bLeftSide + (width() / 2);
		}

		if (direction & Details_Top)
		{
			originY += moveStep;
			// limit movement
			float aTopSide = originY + (height() / 2);
			float bTopSide = containingObj->originY + (containingObj->height() / 2);
			if (aTopSide > bTopSide)
				originY = bTopSide - (height() / 2);
		}
		if (direction & Details_Bottom)
		{
			originY -= moveStep;
			// limit movement
			float aBottomSide = originY - (height() / 2);
			float bBottomSide = containingObj->originY - (containingObj->height() / 2);
			if (aBottomSide < bBottomSide)
				originY = bBottomSide + (height() / 2);
		}
	}
protected:
	// Triggers the calls to render the character.
	inline void _drawInLocalSpace(void* args)
	{
		FishDrawArgs* fishArgs = static_cast<FishDrawArgs*>(args);
		Food* food = fishArgs->food;
		// set didEat to report to the scene to increase the score
		fishArgs->didEat = getDistance(food) < 50.0f;
		if (fishArgs->didEat)
			food->startRegen();
		
		glColor3f(0.9f, 0.5f, 0.2f);

		// reflect along the x if facing left
		glScalef(_facingLeft ? -1.0f : 1.0f, 1.0f, 1.0f);
		glBegin(GL_LINE_LOOP);
		glVertex3f(-30.0f, 0.0f, 0.0f);
		glVertex3f(-45.0f, 7.5f, 0.0f);
		glVertex3f(-45.0f, -7.5f, 0.0f);
		glEnd();

		glTranslatef(7.5f, 0.0f, 0.0f);
		glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
		glScalef(37.5f, 12.5f, 12.5f);
		glutWireOctahedron();
	}
};

// Handles the Graphics Objects, states related to the scene, and handlers for GLUT Events.  Created instanced.
class Scene
{
private:
	// The number of milliseconds the timer will count before triggering an event.
	int const static _timerMsec = 40;
	int const static _gameTimeMsec = 30 * 1000;
	int lastFrame = 0; // time the last frame was drawn
	int startTime = 0;
	int score = 0; // player's current score

	// Object Handles
	GOut* msg_score, * msg_timer;
	GfxObject* fish, *tank, *food, *fan;
public:
	enum SceneState {
		PLAY,
		GAME_OVER, // Game Over
	};
	enum TimerId : int {
		TIMER_UP = -2,
		LOOP = -1, // Rendering event
		FOOD_REGEN = Food::REGEN,
		FOOD_SPOILED = Food::SPOILED,
	};
	// State of the scene
	SceneState state = SceneState::PLAY;
	// The number of objects in my scene.
	static const int ACTOR_COUNT = 4;
	static const int UI_COUNT = 2;
	// The graphics objects in my scene.
	GfxObject** objs_actors;
	GfxObject** objs_ui;


	// Initializes the Scene and its objects
	Scene()
	{
		lastFrame = 0;
		// actors exist in the model space and are drawn there
		objs_actors = new GfxObject * [ACTOR_COUNT]
		{
			tank = new Tank(0.0f, 0.0f, -175.0f),
			fish = new Fish(0.0f, 0.0f, -175.0f),
			food = new Food(0.0f, 0.0f, -175.0f),
			fan = new Fan(0.0f, 0.0f, -250.0f)
		};
		// ui elements are drawn by a different camera on a single z plane
		objs_ui = new GfxObject * [UI_COUNT]
		{
			msg_score = new GOut(-280.0f, 260.0f),
			msg_timer = new GOut(200.0f, 260.0f),
		};
	}

	// Draw all actor objects and update if nessisary
	inline void processScene(int delta)
	{
		fan->draw(&delta);
		food->draw(nullptr);
		
		FishDrawArgs* fishArgs = new FishDrawArgs();
		fishArgs->didEat = false;
		fishArgs->food = dynamic_cast<Food*>(food);
		fish->draw(static_cast<void*>(fishArgs));
		if (fishArgs->didEat && food)
			score += 10;

		delete fishArgs;

		tank->draw(nullptr);
	}

	// Draw all ui objects and update if nessisary
	inline void processUi(int time)
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
		msg_score->start(GLUT_BITMAP_HELVETICA_18) << "Score " << score;
		int timeRemaining = ((_gameTimeMsec - (time - startTime)) / 1000);
		msg_timer->start(GLUT_BITMAP_HELVETICA_18) << "Timer " << (state == GAME_OVER ? 0 : timeRemaining);

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

	// Draws and updates all the objects (both actors and ui) in the scene
	void static display_func()
	{
		/* this callback is automatically called whenever a window needs to
		be displayed or redisplayed.
		Usually this function contains nearly all of the drawing commands;
		the drawing/animation statements should go in this function. */

		int time = glutGet(GLUT_ELAPSED_TIME);
		int delta = time - scene.lastFrame; // delta since last frame to animate smoothly independent of frame rate
		scene.lastFrame = time;

		// Using a black for the background
		glClearColor(0.0, 0.0, 0.0, 1.0);
		glClear(GL_COLOR_BUFFER_BIT);

		// The only polygons are the fan blades.
		glPolygonMode(GL_FRONT, GL_FILL);
		
		// Unnessisary, but recommended
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		// Draw Actor Objects
		scene.processScene(delta);

		// Draw UI elements
		scene.processUi(time);

		glutSwapBuffers();
	}

	// Triggered by a timer event
	void static timer_handler(int id)
	{
		// Scene Event: Render loop
		if (id == TimerId::LOOP)
		{
			// trigger re-render (glutDisplayFunc)
			glutPostRedisplay();

			// set timer for next frame
			glutTimerFunc(_timerMsec, timer_handler, TimerId::LOOP);
		}
		// Scene Event: Game Over
		else if (id == TimerId::TIMER_UP)
		{
			scene.state = GAME_OVER;
			// no more inputs
			glutKeyboardFunc(nullptr);
		}
		// Pass to other objects
		else if (scene.state != GAME_OVER)
			dynamic_cast<Food*>(scene.food)->timerEvent(id);
	}

	// Handler for keyboard presses 
	void static keyboard_handler(unsigned char key, int, int)
	{
		static const int moveStep = 10;
		UINT16 direction = 0x00;
		switch (key)
		{
		case 'h':
		case 'H':
			// Left
			direction |= DirectionDetails::Details_Left;
			break;
		case 'j':
		case 'J':
			// Right
			direction |= DirectionDetails::Details_Right;
			break;
		case 'u':
		case 'U':
			// Up
			direction |= DirectionDetails::Details_Top;
			break;
		case 'n':
		case 'N':
			// Down
			direction |= DirectionDetails::Details_Bottom;
			break;
		}
		if (direction > 0)
			dynamic_cast<Fish*>(scene.fish)
				->swim(
					static_cast<DirectionDetails>(direction),
					moveStep,
					dynamic_cast<Tank*>(scene.tank)
				);
	}

	// registers event handlers and starts the timers
	void setupEnvironment()
	{
		glutDisplayFunc(Scene::display_func); /* registers the display callback */
		glutKeyboardFunc(Scene::keyboard_handler);

		scene.startTime = scene.lastFrame = glutGet(GLUT_ELAPSED_TIME); // set a more accurate "lastFrame"

		// starts the rendering event timer
		Scene::timer_handler(TimerId::LOOP);
		scene.state = SceneState::PLAY;

		// randomize the initial position and start it spoiling
		dynamic_cast<Food*>(food)->timerEvent(Food::EventIds::REGEN);
		
		// start the time for the end of the game
		glutTimerFunc(_gameTimeMsec, timer_handler, TimerId::TIMER_UP);
	}

	// The static scene object that will be rendered
} scene;

// Declare the food's method to set timers now that the timer handler method is defined.
void Food::setTimer(int timeMsec, int id)
{
	glutTimerFunc(timeMsec, Scene::timer_handler, id);
}

int main(int argc, char** argv)
{
	glutInit(&argc, argv);

	my_setup(canvas_Width, canvas_Height, canvas_Name);

	scene.setupEnvironment();

	glutMainLoop(); /* execute until killed */
	return 0;
}