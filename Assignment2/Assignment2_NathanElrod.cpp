#include "pch.h"
#include <GL/glew.h>
#include <GL/freeglut.h>
#include "OpenGL445Setup.h"
/********************************** Assignment 2: Nathan Elrod ******************************** 
* Scene acts as the orchestrator of the graphics objects, event handling, and scene state.  The
*	possible states are defined by SceneState.  Changing state with scene.setState will ensure
*	the proper event handles are registered.  All objects are stored in scene.objects.
* Keyboard inputs are handled with keyboard_handler. All inputs update the scene properties,
*	whether it be moving throug the scene states, or triggering motion of the fowl.
* Rendering graphics to the scene is handled by display_func. This handler iterates through
*	objects to draw them, and have them update themselves.  This is also the time that game-end 
*	conditions are tested.  Win conditions are determined by the state of the scene at game-end.
* Animation is achieved by timer_hanlder being triggered ever 15msec.  This was chosen to
*	produce smoother animation as performance isn’t a limiting concern.
*	Text prompts to the screen are draw by the object GOut.  This object will buffer the text
*	information until render time.
* EXTRA CREDIT:
*	1. Added gravity effect to fowl as they fly.
***********************************************************************************************/

#define canvas_Width 500 /* width of canvas - you may need to change this */
#define canvas_Height 500 /* height of canvas - you may need to change this */
#define canvas_Name (char*)"ASSIGNMENT_2" /* name in window top */
#define gravity_value (float)9.8f;

/*
* The basic graphics objects
*/
class GfxObject
{
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
	void draw(void* args)
	{
		// Ensure any higher level transforms are not disrupted by transformed applied by this object.
		glPushMatrix();
		// Translate to origin
		glTranslatef(originX, originY, 0);

		// Draw specific geometry for this object
		_draw(args);

		// Return the transform to the stack
		glPopMatrix();
	}

protected:
	virtual void _draw(void* args) = 0;
};

// Assumes origin at bottom left.  Only work in 2D space.
class CollisionGfxObject : public GfxObject
{
public:
	CollisionGfxObject(float x, float y) : GfxObject(x, y) {}

	virtual float width() = 0;
	virtual float height() = 0;

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

protected:
	// used for debugging
	void drawBoundingBox(void* ptr)
	{
		// debugging
		if (ptr == nullptr)
			glColor3f(0.0f, 0.0f, 1.0f);
		else if (testEngulfs(static_cast<CollisionGfxObject*>(ptr)))
			glColor3f(1.0f, 0.0f, 0.0f);
		else
			glColor3f(0.0f, 1.0f, 0.0f);

		int w = static_cast<int>(width()), h = static_cast<int>(height());
		glBegin(GL_LINE_LOOP); // Drawing a box
		glVertex3i(0, 0, -1);
		glVertex3i(w, 0, -1);
		glVertex3i(w, h, -1);
		glVertex3i(0, h, -1);
		glEnd();
	}
};

/* A graphics object representing a fowl */
class Fowl : public CollisionGfxObject
{
private:
	/*
	* 545 short side
	* Math for a right triangle with a hypotenuse of 5
	* Side c = 5
	* Side b = 3.53553
	* Side a = 3.53553
	*/
	const float _shortSide = 3.53553f;

	/*
	* Main body
	* Math for a right triangle with a hypotenuse of 25
	* Side c = 25
	* Side b = 18.0
	* Side a = 18.0
	*/
	const float _longSide = 18.0f;

	float* _vertices = new float[30]
	{
		_longSide - _shortSide	, -_shortSide	, -1.0f,	// Bottom Right
		_longSide				, 0.0f			, -1.0f,	// Middle right
		0.0f					, _longSide		, -1.0f,	// Top
		-_longSide				, 0.0f			, -1.0f,	// Middle left
		-_longSide + _shortSide	, -_shortSide	, -1.0f,	// Bottom left
		// Back is extruded, and appended to this list at run time by constructor
	};

	unsigned int _indicesFront[5] = { 0 ,1, 2, 3, 4 };
	unsigned int _indicesBack[5] = { 5, 6, 7, 8, 9 };
	unsigned int _indicesSides[10] =
	{
		0, 5,
		1, 6,
		2, 7,
		3, 8,
		4, 9, 
	};
public:
	// The arguments initialize the origin of the object
	Fowl(float x, float y) : CollisionGfxObject(x, y)
	{
		// Create extruded geometry
		float* extruded = _vertices + 15;
		// the first 15 vertices
		memcpy(extruded, _vertices, sizeof(float) * 15);
		// extrude along the z by adding the extude to the vertex
		float translationZ = -5.0f;
		for (int i = 2; i < 15; i += 3)
			extruded[i] += translationZ;
	}

	// Returns the width of this object float.
	float width() { return _longSide * 2.0f; }
	// Returns the height of this object as a float.  Yes, this method is unused, but it feels wrong to have a width method and no height method.
	float height() { return _longSide + _shortSide; }

	// Updates the x position and scale factor
	void process(float xOffset, float yOffset)
	{
		// EXTRA CREDIT
		float gravity = gravity_value;
		gravity *= (15.0f/1000.0f); // per-frame

		originX += xOffset;
		originY += yOffset - gravity;
	}
protected:
	// Calling this method will handle the OpenGL calls to render this object to the canvas.
	void _draw(void*)
	{
		// This was originally designed with the origin in the center.  My collision detection assumes origin to
		//	be at the bottom left of the object.  This translation fixes that without me redefining the vertics
		glTranslatef(width() / 2.0f, _shortSide, 0.0f);
		glColor3f(1.0f, 0.87f, 0.0f); // Yellowish orange

		// enable veterx array and set the vertex to be referenced
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, 0, _vertices);

		// draw the 3d body
		glDrawElements(GL_LINE_LOOP, 5, GL_UNSIGNED_INT, _indicesFront);
		glDrawElements(GL_LINE_LOOP, 5, GL_UNSIGNED_INT, _indicesBack);
		glDrawElements(GL_LINES, 10, GL_UNSIGNED_INT, _indicesSides);

		// Turn off vetex array
		glDisableClientState(GL_VERTEX_ARRAY);

		// Hair tufts
		glBegin(GL_LINES);
			glVertex3f(0.0f	, _longSide		, -1.0f);
			glVertex3f(-2.7f	, _longSide + 2.0f, -1.0f); // points a little left

			glVertex3f(0.0f	, _longSide		, -1.0f);
			glVertex3f(1.5f	, _longSide + 3.0f, -1.0f); // points up

			glVertex3f(0.0f	, _longSide		, -1.0f);
			glVertex3f(3.0f	, _longSide + 1.3f, -1.0f); // points a little right
		
		// "Angry" Face
		// Notice the swap to intergers, rather than the normal floats
		// Eyes
			glVertex3i(-1	, 8 , -1); // left Eye
			glVertex3i(-1	, 5 , -1);

			glVertex3i(5	, 8	, -1); // Right Eye
			glVertex3i(5	, 5	, -1);
		glEnd();
		// Frown
		glBegin(GL_LINE_STRIP);
			glVertex3i(-2	, -3	, -1); // Left frown
			glVertex3i(0	, 0	, -1);
			glVertex3i(2	, 0	, -1);
			glVertex3i(5	, -3	, -1); // Right frown
		glEnd();
	}
};

// The object for the fowl to fly towards
class Egg : public CollisionGfxObject
{
private:
	// I created this by over laying a image of an egg with a grid and choosing vertices that aligned to egg's edge.
	//	Since the scale of the egg was more than 5, it needs to be scaled down
	float const _scaleFactor = 0.25f;
	int* _vertices = new int[30]
	{
		10, 0, 0, // bottom
		15, 2, 0,
		18, 5, 0,
		20, 7, 0,
		23, 9, 0,
		23, 18, 0,
		22, 23, 0,
		19, 27, 0,
		15, 31, 0,
		10, 32, 0, // top
	};
	int* _indices = new int[10]
	{
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9
	};
public:
	Egg(float x, float y) : CollisionGfxObject(x, y) {}

	float width() { return (21.0f * _scaleFactor) + 10; }
	float height() { return (32.0f * _scaleFactor) + 10; }

	void process(Fowl* leadFlow)
	{
		// tests if the lead fowl close enough
		bool isActive = leadFlow->originX >= canvas_Width - 125.0f;
		if (!isActive) return;

		int movement = 5.0f;
		originY -= movement;
	}
protected:
	void _draw(void* ptr)
	{
		glPolygonMode(GL_FRONT, GL_FILL);
		// center in collision box
		glTranslatef(5.0f, 5.0f, -1.0f);
		glScalef(_scaleFactor, _scaleFactor, 1.0f);
		glColor3f(1.0f, 1.0f, 1.0f); // white
		// setup vertex pointer
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_INT, 0, _vertices);

		// draw the 3d body
		glDrawElements(GL_TRIANGLE_FAN, 10, GL_UNSIGNED_INT, _indices);
		glTranslatef(20.0f, 0.0f, 0.0f);
		glScalef(-1.0f, 1.0f, 1.0f); // rotate
		glDrawElements(GL_TRIANGLE_FAN, 10, GL_UNSIGNED_INT, _indices);

		// Turn off vetex array
		glDisableClientState(GL_VERTEX_ARRAY);
	}
};

// The object launching the fowl
class YShapedStructure : public GfxObject
{
public:
	// The arguments initialize the origin of the object
	YShapedStructure(float x, float y) : GfxObject(x, y) {}
protected:
	// Calling this method will handle the OpenGL calls to render this object to the canvas.
	void _draw(void* args)
	{
		glColor3f(1.0, 0.0, 0.0); // Red
		glBegin(GL_LINE_LOOP);
			glVertex3i(0	, 0		, -1); // bottom
			glVertex3i(5	, 0		, -1);
			glVertex3i(5	, 150	, -1);
			glVertex3i(35	, 200	, -1); // Right arm
			glVertex3i(30	, 205	, -1);
			glVertex3i(3	, 155	, -1); // Center of the "sling shot"
			glVertex3i(-40	, 220	, -1); // Left arm
			glVertex3i(-45	, 215	, -1);
			glVertex3i(0	, 150	, -1);
		glEnd();
	}
};

// The object for the fowl to fly towards
class Nest : public CollisionGfxObject
{
private:
	int* _vertices = new int[18]
	{
		0, 5, 0,
		0, 0, 0,
		30, 0, 0,
		30, 5, 0,
		15, 0, 0,
		15, -5, 0,
	};
	int* _indicesNest = new int[4]
	{
		0, 1, 2, 3,
	};
	int* _indicesSteam = new int[2]
	{
		4, 5,
	};

public:
	Nest(float x, float y) : CollisionGfxObject(x, y) {}

	float width() { return 45; }
	float height() { return 45; }
protected:
	void _draw(void* ptr)
	{
		float spacing = 46.0f;

		glTranslatef(8.0f, 4.0f, -1.0f);
		glColor3f(1.0f, 0.6f, 0.0f);
		// actual graphics
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_INT, 0, _vertices);

		// draw the 3d body
		glDrawElements(GL_LINE_STRIP, 4, GL_UNSIGNED_INT, _indicesNest);
		glDrawElements(GL_LINES, 2, GL_UNSIGNED_INT, _indicesSteam);

		// shift and draw again
		glTranslatef(-spacing, 0.0f, 0.0f);
		glDrawElements(GL_LINE_STRIP, 4, GL_UNSIGNED_INT, _indicesNest);
		glDrawElements(GL_LINES, 2, GL_UNSIGNED_INT, _indicesSteam);

		// shift and draw again
		glTranslatef(-spacing, 0.0f, 0.0f);
		glDrawElements(GL_LINE_STRIP, 4, GL_UNSIGNED_INT, _indicesNest);
		glDrawElements(GL_LINES, 2, GL_UNSIGNED_INT, _indicesSteam);

		// Turn off vetex array
		glDisableClientState(GL_VERTEX_ARRAY);
	}
};

// replacement for std::cout using GLUT to render the characters.  Is not effect intial render position.
class GOut : public GfxObject {
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
			char* new_buffer = new char[new_size];
			if (_buffer)
			{
				memcpy(new_buffer, _buffer, _size);
				delete[] _buffer;
			}
			_buffer = new_buffer;
		}
	}
	// Support for colors
	float primaryColor[3] = { 1.0f, 1.0f, 1.0f };
	float secondaryColor[3] = { 1.0f, 1.0f, 1.0f };
	int secondaryIndex = -1;
public:
	// Constructor
	GOut(float x, float y): GfxObject(x,y), _font(nullptr), _buffer(nullptr), _size(0) {}

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

	// clears the buffer and triggers a redraw
	GOut& clear()
	{	
		glutPostRedisplay();
		return start(nullptr);
	}

	// Only supports 1 color change perline of buffer
	GOut& setColor(float* color3fv)
	{
		void* dst;
		if (_size > 0) {
			dst = secondaryColor;
			secondaryIndex = _size;
		}
		else
		{
			dst = primaryColor;
		}

		memcpy(dst, color3fv, sizeof(float) * 3);
		return *this;
	}

	// Clears the current buffer and sets the font for the next text to draw
	GOut& start(GLvoid* font)
	{
		secondaryIndex = -1;
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
	void _draw(void* args)
	{
		if (isReady())
		{
			// Posision will be handled by the transform.  Just resets the raster posision to the "start"
			glColor3fv(primaryColor);
			glRasterPos2i(0, 0);
			for (size_t i = 0; i < _size; i++)
			{
				// If a secondary color has been set, swap when the index reaches the change point
				if (i == secondaryIndex)
				{
					// save current raster pos to reset to trigger acceptance of new color
					float pos[4] = {};
					glGetFloatv(GL_CURRENT_RASTER_POSITION, pos);
					glColor3fv(secondaryColor);
					glWindowPos3fv(pos);
				}
				glutBitmapCharacter(_font, _buffer[i]);
			}
		}
	}
};

// Handles the Graphics Objects, states related to the scene, and handlers for GLUT Events.  Created instanced.
class Scene {
private:
	// The number of milliseconds the timer will count before triggering an event.
	int const static _timerMsec = 15;

	// acts as a counter for the number of frames to apply up/down movement to the fowl
	int _animationUp = 0;
	int _animationDown = 0;

	// scene object hanldes
	GOut* gout;
	Fowl* leadFowl;
	Nest* target;
	Egg* egg;
public:

	// State of the scene.  Used for handling keyboard events
	enum SceneState {
		SCENE_START,
		SCENE_WAITING,
		SCENE_FORWARD,
		SCENE_END
	};

	// Current state of the scene
	SceneState state = SCENE_START;

	// The number of objects in my scene.
	static const int COUNT = 7;
	// The graphics objects in my scene.
	GfxObject** objects;

	// Add upward movement
	void setAnimationUp() { _animationUp = 4; }
	// Add downward movement
	void setAnimationDown() { _animationDown = 4; }

	// Initializes the Scene and it's objects
	Scene()
	{
		Fowl exampleFowl = Fowl(0,0);
		float spacer = 10.0f + exampleFowl.width();
		float posX = 10.0f;
		objects = new GfxObject * [COUNT]
		{
			new YShapedStructure(50.0f, 5.0f),
			gout = new GOut(80.0f, static_cast<float>(canvas_Height) / 2.0f), // static_cast<float>(canvas_Height) / 0.25f),
			new Fowl(posX, 217.0f),
			new Fowl(posX += spacer, 217.0f),
			leadFowl = new Fowl(posX += spacer, 217.0f),
			target = new Nest(static_cast<float>(canvas_Width) - 40.0f, 300.0f),
			egg = new Egg(target->originX - 70.0f, target->originY), // stored off screen (inactive) until needed
		};
	}

	// Draw all objects and update if nessisary
	void processScene()
	{
		// The only state when any objects should be moving is "Forward" state
		bool shouldProcess = (state == SceneState::SCENE_FORWARD);

		// leadFowl is the only one that has collision we care about
		auto leadFowl = scene.leadFowl;

		// If there is a collision prompt for next state
		// compute if victory state
		if (
			shouldProcess &&
			(
				leadFowl->testCollision(egg) || // if it's collided with the egg
				(leadFowl->originX + leadFowl->width() > static_cast<float>(canvas_Width)) // if it's hit the edge
				)
			)
		{
			scene.setState(SceneState::SCENE_END);
			shouldProcess = false;
		}

		// I wish each fowl was responisble for it's own movement, but this still feels better than probagating the
		//	keybaord events to each fowl object
		float horizontalMovementUnits = 2.0f;
		float verticalMovementUnits = 4.0f;

		// react to user controls
		float offsetY = 0.0f, offsetX = horizontalMovementUnits;
		if (scene._animationUp > 0)
		{
			// reduce step counter and apply movement
			scene._animationUp--;
			offsetY += verticalMovementUnits;
		}
		if (scene._animationDown > 0)
		{
			// reduce step counter and apply movement
			scene._animationDown--;
			offsetY -= verticalMovementUnits;
		}

		// draw and update each object
		void* arg = nullptr;
		for (int i = 0; i < scene.COUNT; i++)
		{
			GfxObject* obj = scene.objects[i];

			arg = nullptr;
			auto nest = dynamic_cast<Nest*>(scene.objects[i]);
			if (nest) arg = leadFowl;

			// still draw the scene if should process is false
			obj->draw(arg);
			if (!shouldProcess) continue;
			
			// Test if fowl object and process
			auto fowl = dynamic_cast<Fowl*>(scene.objects[i]);
			if (fowl) fowl->process(offsetX, offsetY);

			// Egg needs access to the Fowl Object to know when to 'fall'
			auto egg = dynamic_cast<Egg*>(scene.objects[i]);
			if (egg) egg->process(leadFowl);
		}
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

		scene.processScene();

		// Was needed to be rendered on my system
		glFlush();
	}

	// Triggered by a timer giving ~30 fps
	void static timer_handler(int _id)
	{
		// trigger re-render (glutDisplayFunc)
		glutPostRedisplay();

		// set timer for next frame
		if (scene.state != SceneState::SCENE_END)
			glutTimerFunc(_timerMsec, timer_handler, _id);
	}

	// Handler for keyboard presses (ignores character pressed and x/y)
	void static keyboard_handler(unsigned char key, int, int)
	{
		switch (key)
		{
		// 'menu' controls
		case 'B':
		case 'b':
			// prepare for lauch
			if (scene.state == SceneState::SCENE_START)
				scene.setState(SceneState::SCENE_WAITING);
			break;
		case 'M':
		case 'm':
			// Start movements
			if (scene.state == SceneState::SCENE_WAITING) 
				scene.setState(SceneState::SCENE_FORWARD);
			break;
		// game controls
		case 'U':
		case 'u':
			// up
			if (scene.state == SceneState::SCENE_FORWARD)
				scene.setAnimationUp();
			break;
		case 'N':
		case 'n':
			// down
			if (scene.state == SceneState::SCENE_FORWARD)
				scene.setAnimationDown();
			break;
		}
	}

	// Prompts the user for a input before continuing to the next state
	void setState(SceneState nextState)
	{
		// Moves the state forward
		scene.state = nextState;

		switch (nextState)
		{
		case SceneState::SCENE_START:
			// Sets the keyboard handler to wait for an input from the user.
			glutKeyboardFunc(Scene::keyboard_handler);
			scene.gout->start(GLUT_BITMAP_8_BY_13) << "Press B Key to Begin";
			break;
		case SceneState::SCENE_WAITING:
			// clear previous prompt
			scene.gout->clear();
			break;
		case SceneState::SCENE_FORWARD:
			glutTimerFunc(_timerMsec, timer_handler, -1);
			break;
		case SceneState::SCENE_END:
			glutKeyboardFunc(nullptr);
			bool didWin = scene.target->testEngulfs(leadFowl);
			if (didWin)
			{
				scene.gout->originX += 120.0f; // move towards the center
				float secondaryColor[3] = { 0.0f, 1.0f, 0.0f };
				scene.gout->start(GLUT_BITMAP_TIMES_ROMAN_24) << "You ";
				scene.gout->setColor(secondaryColor) << "WIN";
			}
			else
			{
				float color[3] = { 1.0f, 0.0f, 0.0f };
				scene.gout->start(GLUT_BITMAP_TIMES_ROMAN_24).setColor(color) << "BETTER LUCK NEXT TIME";
			}
			break;
		}
	}

// The static scene object that will be rendered
} scene;

int main(int argc, char** argv)
{
	glutInit(&argc, argv);

	my_setup(canvas_Width, canvas_Height, canvas_Name);
	glutDisplayFunc(Scene::display_func); /* registers the display callback */

	scene.setState(Scene::SCENE_START);

	glutMainLoop(); /* execute until killed */
	return 0;
}
