#pragma once
class Keyboard
{
public:
	Keyboard(void);
	virtual ~Keyboard(void);

	/// Key definitions (may be remapped internally)
	const static int UP = 1;
	const static int DOWN = 2;
	const static int LEFT = 3;
	const static int RIGHT = 4;
	const static int LAST_ID = 4;

	/// Receive the current state asynchronously
	/// from the keyboard
	void prepare(void);

	/// Get the key state that was last used
	bool keyDown(int key);

private:
	/// key states of the directional keys
	bool keyState[LAST_ID + 1];
};

