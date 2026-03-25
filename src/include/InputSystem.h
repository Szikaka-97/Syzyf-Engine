#pragma once

#include <string>
#include <map>
#include <glm/glm.hpp>

#include <SceneComponent.h>
#include <Debug.h>

struct GLFWwindow;

enum class Key {
	Space          = 32,
	Apostrophe     = 39,  /* ' */
	Comma          = 44,  /* , */
	Minus          = 45,  /* - */
	Period         = 46,  /* . */
	Slash          = 47,  /* / */
	Alpha0         = 48,
	Alpha1         = 49,
	Alpha2         = 50,
	Alpha3         = 51,
	Alpha4         = 52,
	Alpha5         = 53,
	Alpha6         = 54,
	Alpha7         = 55,
	Alpha8         = 56,
	Alpha9         = 57,
	Semicolon      = 59,  /* ; */
	Equal          = 61,  /* = */
	A              = 65,
	B              = 66,
	C              = 67,
	D              = 68,
	E              = 69,
	F              = 70,
	G              = 71,
	H              = 72,
	I              = 73,
	J              = 74,
	K              = 75,
	L              = 76,
	M              = 77,
	N              = 78,
	O              = 79,
	P              = 80,
	Q              = 81,
	R              = 82,
	S              = 83,
	T              = 84,
	U              = 85,
	V              = 86,
	W              = 87,
	X              = 88,
	Y              = 89,
	Z              = 90,
	LeftBracket    = 91,  /* [ */
	Backslash      = 92,  /* \ */
	RightBracket   = 93,  /* ] */
	Backtick       = 96,  /* ` */
	Escape         = 256,
	Enter          = 257,
	Tab            = 258,
	Backspace      = 259,
	Insert         = 260,
	Delete         = 261,
	Right          = 262,
	Left           = 263,
	Down           = 264,
	Up             = 265,
	PageUp         = 266,
	PageDown       = 267,
	Home           = 268,
	End            = 269,
	CapsLock       = 280,
	ScrollLock     = 281,
	NumLock        = 282,
	PrintScreen    = 283,
	Pause          = 284,
	F1             = 290,
	F2             = 291,
	F3             = 292,
	F4             = 293,
	F5             = 294,
	F6             = 295,
	F7             = 296,
	F8             = 297,
	F9             = 298,
	F10            = 299,
	F11            = 300,
	F12            = 301,
	F13            = 302,
	F14            = 303,
	F15            = 304,
	F16            = 305,
	F17            = 306,
	F18            = 307,
	F19            = 308,
	F20            = 309,
	F21            = 310,
	F22            = 311,
	F23            = 312,
	F24            = 313,
	F25            = 314,
	Numpad0        = 320,
	Numpad1        = 321,
	Numpad2        = 322,
	Numpad3        = 323,
	Numpad4        = 324,
	Numpad5        = 325,
	Numpad6        = 326,
	Numpad7        = 327,
	Numpad8        = 328,
	Numpad9        = 329,
	NumpadDecimal  = 330,
	NumpadDivide   = 331,
	NumpadMultiply = 332,
	NumpadSubtract = 333,
	NumpadAdd      = 334,
	NumpadEnter    = 335,
	NumpadEqual    = 336,
	LeftShift      = 340,
	LeftCtrl       = 341,
	LeftAlt        = 342,
	LeftSuper      = 343,
	RightShift     = 344,
	RightCtrl      = 345,
	RightAlt       = 346,
	RightSuper     = 347,
	Menu           = 348,
};

enum class MouseButton {
	Left   = 0,
	Right  = 1,
	Middle = 2,
	Mouse4 = 3,
	Mouse5 = 4,
	Mouse6 = 5,
	Mouse7 = 6,
	Mouse8 = 7,
};

class InputSystem : public SceneComponent, public ImGuiDrawable {
private:
	struct KeyBitMask {
		constexpr static int TimeBits = 29;
		constexpr static int FractionalTimeBits = 10;

		uint32_t value;

		KeyBitMask();

		KeyBitMask(uint32_t value);

		inline bool GetKeyDownBit() const {
			return value & 1;
		}

		inline bool GetKeyPressedBit() const {
			return value & 2;
		}

		inline bool GetKeyUpBit() const {
			return value & 4;
		}

		inline float GetPressTime() const {
			int pressTime = this->value >> (32 - TimeBits);
			return ((float) pressTime) / (1 << FractionalTimeBits);
		}

		inline void SetKeyDownBit(bool set) {
			value &= ~1;
			value |= set;
		}

		inline void SetKeyPressedBit(bool set) {
			value &= ~2;
			value |= ((uint8_t) set) << 1;
		}

		inline void SetKeyUpBit(bool set) {
			value &= ~4;
			value |= ((uint8_t) set) << 2;
		}

		inline void SetPressTime(float pressTime) {
			int timeRep = (int) (pressTime * (1 << FractionalTimeBits)) << (32 - TimeBits);
			
			this->value &= (1 << (32 - TimeBits)) - 1;
			this->value |= timeRep;
		}
	};

	std::map<int, KeyBitMask> keys;
	glm::vec2 prevMouseMovement;
	bool mouseLocked;
public:
	InputSystem(Scene* scene);

	bool KeyDown(Key key) const;
	bool KeyDown(const std::string& key) const;
	bool KeyDown(char key) const;

	bool KeyPressed(Key key) const;
	bool KeyPressed(const std::string& key) const;
	bool KeyPressed(char key) const;

	bool KeyUp(Key key) const;
	bool KeyUp(const std::string& key) const;
	bool KeyUp(char key) const;

	bool ButtonDown(MouseButton button) const;
	bool ButtonDown(int button) const;

	bool ButtonPressed(MouseButton button) const;
	bool ButtonPressed(int button) const;

	bool ButtonUp(MouseButton button) const;
	bool ButtonUp(int button) const;

	bool MouseLocked();

	void SetMouseLocked(bool locked);

	glm::vec2 GetMouseMovement();
	glm::vec2 GetMousePosition();

	virtual void OnPreUpdate();

	virtual int Order();

	virtual void DrawImGui();
};
