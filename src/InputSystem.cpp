#include <InputSystem.h>

#include <cstring>
#include <map>
#include <format>

#include <imgui.h>
#include <SDL3/SDL.h>
#include "SDL3/SDL_keyboard.h"
#include "SDL3/SDL_mouse.h"
#include "SDL3/SDL_scancode.h"

#include <spdlog/spdlog.h>

#include <Engine.h>
#include <TimeSystem.h>
#include <Graphics.h>

constexpr int MouseButtonOffset = 512;

std::map<Key, int> keyToSDL {
	{ Key::Space          , SDL_SCANCODE_SPACE },
	{ Key::Apostrophe     , SDL_SCANCODE_APOSTROPHE },
	{ Key::Comma          , SDL_SCANCODE_COMMA },
	{ Key::Minus          , SDL_SCANCODE_MINUS },
	{ Key::Period         , SDL_SCANCODE_PERIOD },
	{ Key::Slash          , SDL_SCANCODE_SLASH },
	{ Key::Alpha0         , SDL_SCANCODE_0 },
	{ Key::Alpha1         , SDL_SCANCODE_1 },
	{ Key::Alpha2         , SDL_SCANCODE_2 },
	{ Key::Alpha3         , SDL_SCANCODE_3 },
	{ Key::Alpha4         , SDL_SCANCODE_4 },
	{ Key::Alpha5         , SDL_SCANCODE_5 },
	{ Key::Alpha6         , SDL_SCANCODE_6 },
	{ Key::Alpha7         , SDL_SCANCODE_7 },
	{ Key::Alpha8         , SDL_SCANCODE_8 },
	{ Key::Alpha9         , SDL_SCANCODE_9 },
	{ Key::Semicolon      , SDL_SCANCODE_SEMICOLON },
	{ Key::Equal          , SDL_SCANCODE_EQUALS },
	{ Key::A              , SDL_SCANCODE_A },
	{ Key::B              , SDL_SCANCODE_B },
	{ Key::C              , SDL_SCANCODE_C },
	{ Key::D              , SDL_SCANCODE_D },
	{ Key::E              , SDL_SCANCODE_E },
	{ Key::F              , SDL_SCANCODE_F },
	{ Key::G              , SDL_SCANCODE_G },
	{ Key::H              , SDL_SCANCODE_H },
	{ Key::I              , SDL_SCANCODE_I },
	{ Key::J              , SDL_SCANCODE_J },
	{ Key::K              , SDL_SCANCODE_K },
	{ Key::L              , SDL_SCANCODE_L },
	{ Key::M              , SDL_SCANCODE_M },
	{ Key::N              , SDL_SCANCODE_N },
	{ Key::O              , SDL_SCANCODE_O },
	{ Key::P              , SDL_SCANCODE_P },
	{ Key::Q              , SDL_SCANCODE_Q },
	{ Key::R              , SDL_SCANCODE_R },
	{ Key::S              , SDL_SCANCODE_S },
	{ Key::T              , SDL_SCANCODE_T },
	{ Key::U              , SDL_SCANCODE_U },
	{ Key::V              , SDL_SCANCODE_V },
	{ Key::W              , SDL_SCANCODE_W },
	{ Key::X              , SDL_SCANCODE_X },
	{ Key::Y              , SDL_SCANCODE_Y },
	{ Key::Z              , SDL_SCANCODE_Z },
	{ Key::LeftBracket    , SDL_SCANCODE_LEFTBRACKET },
	{ Key::Backslash      , SDL_SCANCODE_BACKSLASH },
	{ Key::RightBracket   , SDL_SCANCODE_RIGHTBRACKET },
	{ Key::Backtick       , SDL_SCANCODE_GRAVE },
	{ Key::Escape         , SDL_SCANCODE_ESCAPE },
	{ Key::Enter          , SDL_SCANCODE_RETURN },
	{ Key::Tab            , SDL_SCANCODE_TAB },
	{ Key::Backspace      , SDL_SCANCODE_BACKSPACE },
	{ Key::Insert         , SDL_SCANCODE_INSERT },
	{ Key::Delete         , SDL_SCANCODE_DELETE },
	{ Key::Right          , SDL_SCANCODE_RIGHT },
	{ Key::Left           , SDL_SCANCODE_LEFT },
	{ Key::Down           , SDL_SCANCODE_DOWN },
	{ Key::Up             , SDL_SCANCODE_UP },
	{ Key::PageUp         , SDL_SCANCODE_PAGEUP },
	{ Key::PageDown       , SDL_SCANCODE_PAGEDOWN },
	{ Key::Home           , SDL_SCANCODE_HOME },
	{ Key::End            , SDL_SCANCODE_END },
	{ Key::CapsLock       , SDL_SCANCODE_CAPSLOCK },
	{ Key::ScrollLock     , SDL_SCANCODE_SCROLLLOCK },
	{ Key::NumLock        , SDL_SCANCODE_NUMLOCKCLEAR },
	{ Key::PrintScreen    , SDL_SCANCODE_PRINTSCREEN },
	{ Key::Pause          , SDL_SCANCODE_PAUSE },
	{ Key::F1             , SDL_SCANCODE_F1 },
	{ Key::F2             , SDL_SCANCODE_F2 },
	{ Key::F3             , SDL_SCANCODE_F3 },
	{ Key::F4             , SDL_SCANCODE_F4 },
	{ Key::F5             , SDL_SCANCODE_F5 },
	{ Key::F6             , SDL_SCANCODE_F6 },
	{ Key::F7             , SDL_SCANCODE_F7 },
	{ Key::F8             , SDL_SCANCODE_F8 },
	{ Key::F9             , SDL_SCANCODE_F9 },
	{ Key::F10            , SDL_SCANCODE_F10 },
	{ Key::F11            , SDL_SCANCODE_F11 },
	{ Key::F12            , SDL_SCANCODE_F12 },
	{ Key::F13            , SDL_SCANCODE_F13 },
	{ Key::F14            , SDL_SCANCODE_F14 },
	{ Key::F15            , SDL_SCANCODE_F15 },
	{ Key::F16            , SDL_SCANCODE_F16 },
	{ Key::F17            , SDL_SCANCODE_F17 },
	{ Key::F18            , SDL_SCANCODE_F18 },
	{ Key::F19            , SDL_SCANCODE_F19 },
	{ Key::F20            , SDL_SCANCODE_F20 },
	{ Key::F21            , SDL_SCANCODE_F21 },
	{ Key::F22            , SDL_SCANCODE_F22 },
	{ Key::F23            , SDL_SCANCODE_F23 },
	{ Key::F24            , SDL_SCANCODE_F24 },
	{ Key::F25            , SDL_SCANCODE_F24 },
	{ Key::Numpad0        , SDL_SCANCODE_KP_0 },
	{ Key::Numpad1        , SDL_SCANCODE_KP_1 },
	{ Key::Numpad2        , SDL_SCANCODE_KP_2 },
	{ Key::Numpad3        , SDL_SCANCODE_KP_3 },
	{ Key::Numpad4        , SDL_SCANCODE_KP_4 },
	{ Key::Numpad5        , SDL_SCANCODE_KP_5 },
	{ Key::Numpad6        , SDL_SCANCODE_KP_6 },
	{ Key::Numpad7        , SDL_SCANCODE_KP_7 },
	{ Key::Numpad8        , SDL_SCANCODE_KP_8 },
	{ Key::Numpad9        , SDL_SCANCODE_KP_9 },
	{ Key::NumpadDecimal  , SDL_SCANCODE_KP_DECIMAL },
	{ Key::NumpadDivide   , SDL_SCANCODE_KP_DIVIDE },
	{ Key::NumpadMultiply , SDL_SCANCODE_KP_MULTIPLY },
	{ Key::NumpadSubtract , SDL_SCANCODE_KP_MINUS },
	{ Key::NumpadAdd      , SDL_SCANCODE_KP_PLUS },
	{ Key::NumpadEnter    , SDL_SCANCODE_KP_ENTER },
	{ Key::NumpadEqual    , SDL_SCANCODE_KP_EQUALS },
	{ Key::LeftShift      , SDL_SCANCODE_LSHIFT },
	{ Key::LeftCtrl       , SDL_SCANCODE_LCTRL },
	{ Key::LeftAlt        , SDL_SCANCODE_LALT },
	{ Key::LeftSuper      , SDL_SCANCODE_LGUI },
	{ Key::RightShift     , SDL_SCANCODE_RSHIFT },
	{ Key::RightCtrl      , SDL_SCANCODE_RCTRL },
	{ Key::RightAlt       , SDL_SCANCODE_RALT },
	{ Key::RightSuper     , SDL_SCANCODE_RGUI },
	{ Key::Menu           , SDL_SCANCODE_MENU },
};

std::map<char, Key> charToKey {
	{ ' ', Key::Space },
	{ '\'', Key::Apostrophe },
	{ ',', Key::Comma },
	{ '-', Key::Minus },
	{ '.', Key::Period },
	{ '/', Key::Slash },
	{ '0', Key::Alpha0 },
	{ '1', Key::Alpha1 },
	{ '2', Key::Alpha2 },
	{ '3', Key::Alpha3 },
	{ '4', Key::Alpha4 },
	{ '5', Key::Alpha5 },
	{ '6', Key::Alpha6 },
	{ '7', Key::Alpha7 },
	{ '8', Key::Alpha8 },
	{ '9', Key::Alpha9 },
	{ ';', Key::Semicolon },
	{ '=', Key::Equal },
	{ 'A', Key::A },
	{ 'B', Key::B },
	{ 'C', Key::C },
	{ 'D', Key::D },
	{ 'E', Key::E },
	{ 'F', Key::F },
	{ 'G', Key::G },
	{ 'H', Key::H },
	{ 'I', Key::I },
	{ 'J', Key::J },
	{ 'K', Key::K },
	{ 'L', Key::L },
	{ 'M', Key::M },
	{ 'N', Key::N },
	{ 'O', Key::O },
	{ 'P', Key::P },
	{ 'Q', Key::Q },
	{ 'R', Key::R },
	{ 'S', Key::S },
	{ 'T', Key::T },
	{ 'U', Key::U },
	{ 'V', Key::V },
	{ 'W', Key::W },
	{ 'X', Key::X },
	{ 'Y', Key::Y },
	{ 'Z', Key::Z },
	{ '[', Key::LeftBracket },
	{ '\\', Key::Backslash },
	{ ']', Key::RightBracket },
	{ '`', Key::Backtick },
};

std::map<Key, const std::string> keyToString {
	{ Key::Space, "Space" },
	{ Key::Apostrophe, "Apostrophe" },
	{ Key::Comma, "Comma" },
	{ Key::Minus, "Minus" },
	{ Key::Period, "Period" },
	{ Key::Slash, "Slash" },
	{ Key::Alpha0, "0" },
	{ Key::Alpha1, "1" },
	{ Key::Alpha2, "2" },
	{ Key::Alpha3, "3" },
	{ Key::Alpha4, "4" },
	{ Key::Alpha5, "5" },
	{ Key::Alpha6, "6" },
	{ Key::Alpha7, "7" },
	{ Key::Alpha8, "8" },
	{ Key::Alpha9, "9" },
	{ Key::Semicolon, "Semicolon" },
	{ Key::Equal, "Equal" },
	{ Key::A, "A" },
	{ Key::B, "B" },
	{ Key::C, "C" },
	{ Key::D, "D" },
	{ Key::E, "E" },
	{ Key::F, "F" },
	{ Key::G, "G" },
	{ Key::H, "H" },
	{ Key::I, "I" },
	{ Key::J, "J" },
	{ Key::K, "K" },
	{ Key::L, "L" },
	{ Key::M, "M" },
	{ Key::N, "N" },
	{ Key::O, "O" },
	{ Key::P, "P" },
	{ Key::Q, "Q" },
	{ Key::R, "R" },
	{ Key::S, "S" },
	{ Key::T, "T" },
	{ Key::U, "U" },
	{ Key::V, "V" },
	{ Key::W, "W" },
	{ Key::X, "X" },
	{ Key::Y, "Y" },
	{ Key::Z, "Z" },
	{ Key::LeftBracket, "LeftBracket" },
	{ Key::Backslash, "Backslash" },
	{ Key::RightBracket, "RightBracket" },
	{ Key::Backtick, "Backtick" },
	{ Key::Escape, "Escape" },
	{ Key::Enter, "Enter" },
	{ Key::Tab, "Tab" },
	{ Key::Backspace, "Backspace" },
	{ Key::Insert, "Insert" },
	{ Key::Delete, "Delete" },
	{ Key::Right, "Right" },
	{ Key::Left, "Left" },
	{ Key::Down, "Down" },
	{ Key::Up, "Up" },
	{ Key::PageUp, "PageUp" },
	{ Key::PageDown, "PageDown" },
	{ Key::Home, "Home" },
	{ Key::End, "End" },
	{ Key::CapsLock, "CapsLock" },
	{ Key::ScrollLock, "ScrollLock" },
	{ Key::NumLock, "NumLock" },
	{ Key::PrintScreen, "PrintScreen" },
	{ Key::Pause, "Pause" },
	{ Key::F1, "F1" },
	{ Key::F2, "F2" },
	{ Key::F3, "F3" },
	{ Key::F4, "F4" },
	{ Key::F5, "F5" },
	{ Key::F6, "F6" },
	{ Key::F7, "F7" },
	{ Key::F8, "F8" },
	{ Key::F9, "F9" },
	{ Key::F10, "F10" },
	{ Key::F11, "F11" },
	{ Key::F12, "F12" },
	{ Key::F13, "F13" },
	{ Key::F14, "F14" },
	{ Key::F15, "F15" },
	{ Key::F16, "F16" },
	{ Key::F17, "F17" },
	{ Key::F18, "F18" },
	{ Key::F19, "F19" },
	{ Key::F20, "F20" },
	{ Key::F21, "F21" },
	{ Key::F22, "F22" },
	{ Key::F23, "F23" },
	{ Key::F24, "F24" },
	{ Key::F25, "F25" },
	{ Key::Numpad0, "Numpad0" },
	{ Key::Numpad1, "Numpad1" },
	{ Key::Numpad2, "Numpad2" },
	{ Key::Numpad3, "Numpad3" },
	{ Key::Numpad4, "Numpad4" },
	{ Key::Numpad5, "Numpad5" },
	{ Key::Numpad6, "Numpad6" },
	{ Key::Numpad7, "Numpad7" },
	{ Key::Numpad8, "Numpad8" },
	{ Key::Numpad9, "Numpad9" },
	{ Key::NumpadDecimal, "NumpadDecimal" },
	{ Key::NumpadDivide, "NumpadDivide" },
	{ Key::NumpadMultiply, "NumpadMultiply" },
	{ Key::NumpadSubtract, "NumpadSubtract" },
	{ Key::NumpadAdd, "NumpadAdd" },
	{ Key::NumpadEnter, "NumpadEnter" },
	{ Key::NumpadEqual, "NumpadEqual" },
	{ Key::LeftShift, "LeftShift" },
	{ Key::LeftCtrl, "LeftCtrl" },
	{ Key::LeftAlt, "LeftAlt" },
	{ Key::LeftSuper, "LeftSuper" },
	{ Key::RightShift, "RightShift" },
	{ Key::RightCtrl, "RightCtrl" },
	{ Key::RightAlt, "RightAlt" },
	{ Key::RightSuper, "RightSuper" },
	{ Key::Menu, "Menu" },
	{ (Key) ((int) MouseButton::Left   + MouseButtonOffset), "Left" },
	{ (Key) ((int) MouseButton::Right  + MouseButtonOffset), "Right" },
	{ (Key) ((int) MouseButton::Middle + MouseButtonOffset), "Middle" },
	{ (Key) ((int) MouseButton::Mouse4 + MouseButtonOffset), "Mouse 4" },
	{ (Key) ((int) MouseButton::Mouse5 + MouseButtonOffset), "Mouse 5" },
	{ (Key) ((int) MouseButton::Mouse6 + MouseButtonOffset), "Mouse 6" },
	{ (Key) ((int) MouseButton::Mouse7 + MouseButtonOffset), "Mouse 7" },
	{ (Key) ((int) MouseButton::Mouse8 + MouseButtonOffset), "Mouse 8" },
};


InputSystem::KeyBitMask::KeyBitMask():
value(0) { }

InputSystem::KeyBitMask::KeyBitMask(uint32_t value):
value(value) { }

InputSystem::InputSystem(Scene* scene):
SceneComponent(scene),
prevMouseMovement(glm::zero<glm::vec2>()),
mouseLocked(false) {
	keys = {
		{ (int) Key::Space, 0 },
		{ (int) Key::Apostrophe, 0 },
		{ (int) Key::Comma, 0 },
		{ (int) Key::Minus, 0 },
		{ (int) Key::Period, 0 },
		{ (int) Key::Slash, 0 },
		{ (int) Key::Alpha0, 0 },
		{ (int) Key::Alpha1, 0 },
		{ (int) Key::Alpha2, 0 },
		{ (int) Key::Alpha3, 0 },
		{ (int) Key::Alpha4, 0 },
		{ (int) Key::Alpha5, 0 },
		{ (int) Key::Alpha6, 0 },
		{ (int) Key::Alpha7, 0 },
		{ (int) Key::Alpha8, 0 },
		{ (int) Key::Alpha9, 0 },
		{ (int) Key::Semicolon, 0 },
		{ (int) Key::Equal, 0 },
		{ (int) Key::A, 0 },
		{ (int) Key::B, 0 },
		{ (int) Key::C, 0 },
		{ (int) Key::D, 0 },
		{ (int) Key::E, 0 },
		{ (int) Key::F, 0 },
		{ (int) Key::G, 0 },
		{ (int) Key::H, 0 },
		{ (int) Key::I, 0 },
		{ (int) Key::J, 0 },
		{ (int) Key::K, 0 },
		{ (int) Key::L, 0 },
		{ (int) Key::M, 0 },
		{ (int) Key::N, 0 },
		{ (int) Key::O, 0 },
		{ (int) Key::P, 0 },
		{ (int) Key::Q, 0 },
		{ (int) Key::R, 0 },
		{ (int) Key::S, 0 },
		{ (int) Key::T, 0 },
		{ (int) Key::U, 0 },
		{ (int) Key::V, 0 },
		{ (int) Key::W, 0 },
		{ (int) Key::X, 0 },
		{ (int) Key::Y, 0 },
		{ (int) Key::Z, 0 },
		{ (int) Key::LeftBracket, 0 },
		{ (int) Key::Backslash, 0 },
		{ (int) Key::RightBracket, 0 },
		{ (int) Key::Backtick, 0 },
		{ (int) Key::Escape, 0 },
		{ (int) Key::Enter, 0 },
		{ (int) Key::Tab, 0 },
		{ (int) Key::Backspace, 0 },
		{ (int) Key::Insert, 0 },
		{ (int) Key::Delete, 0 },
		{ (int) Key::Right, 0 },
		{ (int) Key::Left, 0 },
		{ (int) Key::Down, 0 },
		{ (int) Key::Up, 0 },
		{ (int) Key::PageUp, 0 },
		{ (int) Key::PageDown, 0 },
		{ (int) Key::Home, 0 },
		{ (int) Key::End, 0 },
		{ (int) Key::CapsLock, 0 },
		{ (int) Key::ScrollLock, 0 },
		{ (int) Key::NumLock, 0 },
		{ (int) Key::PrintScreen, 0 },
		{ (int) Key::Pause, 0 },
		{ (int) Key::F1, 0 },
		{ (int) Key::F2, 0 },
		{ (int) Key::F3, 0 },
		{ (int) Key::F4, 0 },
		{ (int) Key::F5, 0 },
		{ (int) Key::F6, 0 },
		{ (int) Key::F7, 0 },
		{ (int) Key::F8, 0 },
		{ (int) Key::F9, 0 },
		{ (int) Key::F10, 0 },
		{ (int) Key::F11, 0 },
		{ (int) Key::F12, 0 },
		{ (int) Key::F13, 0 },
		{ (int) Key::F14, 0 },
		{ (int) Key::F15, 0 },
		{ (int) Key::F16, 0 },
		{ (int) Key::F17, 0 },
		{ (int) Key::F18, 0 },
		{ (int) Key::F19, 0 },
		{ (int) Key::F20, 0 },
		{ (int) Key::F21, 0 },
		{ (int) Key::F22, 0 },
		{ (int) Key::F23, 0 },
		{ (int) Key::F24, 0 },
		{ (int) Key::F25, 0 },
		{ (int) Key::Numpad0, 0 },
		{ (int) Key::Numpad1, 0 },
		{ (int) Key::Numpad2, 0 },
		{ (int) Key::Numpad3, 0 },
		{ (int) Key::Numpad4, 0 },
		{ (int) Key::Numpad5, 0 },
		{ (int) Key::Numpad6, 0 },
		{ (int) Key::Numpad7, 0 },
		{ (int) Key::Numpad8, 0 },
		{ (int) Key::Numpad9, 0 },
		{ (int) Key::NumpadDecimal, 0 },
		{ (int) Key::NumpadDivide, 0 },
		{ (int) Key::NumpadMultiply, 0 },
		{ (int) Key::NumpadSubtract, 0 },
		{ (int) Key::NumpadAdd, 0 },
		{ (int) Key::NumpadEnter, 0 },
		{ (int) Key::NumpadEqual, 0 },
		{ (int) Key::LeftShift, 0 },
		{ (int) Key::LeftCtrl, 0 },
		{ (int) Key::LeftAlt, 0 },
		{ (int) Key::LeftSuper, 0 },
		{ (int) Key::RightShift, 0 },
		{ (int) Key::RightCtrl, 0 },
		{ (int) Key::RightAlt, 0 },
		{ (int) Key::RightSuper, 0 },
		{ (int) Key::Menu, 0 },
		{ (int) MouseButton::Left   + MouseButtonOffset, 0 },
		{ (int) MouseButton::Right  + MouseButtonOffset, 0 },
		{ (int) MouseButton::Middle + MouseButtonOffset, 0 },
		{ (int) MouseButton::Mouse4 + MouseButtonOffset, 0 },
		{ (int) MouseButton::Mouse5 + MouseButtonOffset, 0 },
		{ (int) MouseButton::Mouse6 + MouseButtonOffset, 0 },
		{ (int) MouseButton::Mouse7 + MouseButtonOffset, 0 },
		{ (int) MouseButton::Mouse8 + MouseButtonOffset, 0 },
	};
}

bool InputSystem::KeyDown(Key key) const {
	auto keyMask = this->keys.find((int) key);

	return keyMask != this->keys.end() && keyMask->second.GetKeyDownBit();
}

bool InputSystem::KeyDown(const std::string& key) const {
	auto keyCode = charToKey.find(toupper(key.front()));

	if (keyCode == charToKey.end()) {
		return false;
	}

	auto keyMask = this->keys.find((int) keyCode->second);

	return keyMask != this->keys.end() && keyMask->second.GetKeyDownBit();
}

bool InputSystem::KeyDown(char key) const {
	auto keyCode = charToKey.find(toupper(key));

	if (keyCode == charToKey.end()) {
		return false;
	}

	auto keyMask = this->keys.find((int) keyCode->second);

	return keyMask != this->keys.end() && keyMask->second.GetKeyDownBit();
}

bool InputSystem::KeyPressed(Key key) const {
	auto keyMask = this->keys.find((int) key);

	return keyMask != this->keys.end() && keyMask->second.GetKeyPressedBit();
}

bool InputSystem::KeyPressed(const std::string& key) const {
	auto keyCode = charToKey.find(toupper(key.front()));

	if (keyCode == charToKey.end()) {
		return false;
	}

	auto keyMask = this->keys.find((int) keyCode->second);

	return keyMask != this->keys.end() && keyMask->second.GetKeyPressedBit();
}

bool InputSystem::KeyPressed(char key) const {
	auto keyCode = charToKey.find(toupper(key));

	if (keyCode == charToKey.end()) {
		return false;
	}

	auto keyMask = this->keys.find((int) keyCode->second);

	return keyMask != this->keys.end() && keyMask->second.GetKeyPressedBit();
}

bool InputSystem::KeyUp(Key key) const {
	auto keyMask = this->keys.find((int) key);

	return keyMask != this->keys.end() && keyMask->second.GetKeyUpBit();
}

bool InputSystem::KeyUp(const std::string& key) const {
	auto keyCode = charToKey.find(toupper(key.front()));

	if (keyCode == charToKey.end()) {
		return false;
	}

	auto keyMask = this->keys.find((int) keyCode->second);

	return keyMask != this->keys.end() && keyMask->second.GetKeyUpBit();
}

bool InputSystem::KeyUp(char key) const {
	auto keyCode = charToKey.find(toupper(key));

	if (keyCode == charToKey.end()) {
		return false;
	}

	auto keyMask = this->keys.find((int) keyCode->second);

	return keyMask != this->keys.end() && keyMask->second.GetKeyUpBit();
}

bool InputSystem::ButtonDown(MouseButton button) const {
	auto keyMask = this->keys.find((int) button + MouseButtonOffset);

	return keyMask != this->keys.end() && keyMask->second.GetKeyDownBit();
}

bool InputSystem::ButtonDown(int button) const {
	auto keyMask = this->keys.find(button + MouseButtonOffset);

	return keyMask != this->keys.end() && keyMask->second.GetKeyDownBit();
}

bool InputSystem::ButtonPressed(MouseButton button) const {
	auto keyMask = this->keys.find((int) button + MouseButtonOffset);

	return keyMask != this->keys.end() && keyMask->second.GetKeyPressedBit();
}

bool InputSystem::ButtonPressed(int button) const {
	auto keyMask = this->keys.find(button + MouseButtonOffset);

	return keyMask != this->keys.end() && keyMask->second.GetKeyPressedBit();
}

bool InputSystem::ButtonUp(MouseButton button) const {
	auto keyMask = this->keys.find((int) button + MouseButtonOffset);

	return keyMask != this->keys.end() && keyMask->second.GetKeyUpBit();
}

bool InputSystem::ButtonUp(int button) const {
	auto keyMask = this->keys.find(button + MouseButtonOffset);

	return keyMask != this->keys.end() && keyMask->second.GetKeyUpBit();
}

bool InputSystem::MouseLocked() {
	return this->mouseLocked;
}

void InputSystem::SetMouseLocked(bool locked) {
	static glm::vec2 mouseMovement;

	static auto mouseTransform = [](void *userdata, Uint64 timestamp, SDL_Window *window, SDL_MouseID mouseID, float *x, float *y) -> void {
		InputSystem* input = (InputSystem*) userdata;

		input->prevMouseMovement = input->prevMouseMovement.load() + glm::vec2(*x, -*y);

		*x = 0;
		*y = 0;
	};

	if (locked) {		
		this->prevMouseMovement = glm::vec2(0, 0);

		SDL_SetRelativeMouseTransform(mouseTransform, this);
		SDL_SetWindowRelativeMouseMode(Engine::GetWindow(), true);
	}
	else {
		SDL_SetWindowRelativeMouseMode(Engine::GetWindow(), false);
	}

	this->mouseLocked = locked;
}

glm::vec2 InputSystem::GetMouseMovement() {
	if (this->mouseLocked) {
		return this->prevMouseMovement;
	}

	return glm::vec2(0, 0);
}

glm::vec2 InputSystem::GetMousePosition() {
	if (this->mouseLocked) {
		return glm::vec2(0, 0);
	}
	
	return this->prevMouseMovement;
}

void InputSystem::OnPreUpdate() {
	int numKeys = 0;
	
	const bool* keyArray = SDL_GetKeyboardState(&numKeys);
	float xpos, ypos;
	SDL_MouseButtonFlags mouseState = this->mouseLocked ? SDL_GetRelativeMouseState(&xpos, &ypos) : SDL_GetMouseState(&xpos, &ypos);

	for (auto& key : this->keys) {
		int keyCode = key.first % MouseButtonOffset;

		KeyBitMask mask = key.second;

		bool pressed = (key.first < MouseButtonOffset ? keyArray[keyToSDL[(Key) keyCode]] : mouseState & (1 << keyCode));

		if (pressed ^ mask.GetKeyPressedBit()) {
			mask.SetPressTime(Time::Current());
		}

		mask.SetKeyUpBit(!pressed && mask.GetKeyPressedBit());
		mask.SetKeyDownBit(pressed && !mask.GetKeyPressedBit());
		mask.SetKeyPressedBit(pressed);
		
		key.second = mask;
	}

	
	if (!this->mouseLocked) {
		this->prevMouseMovement = glm::vec2(xpos, ypos);
	}
}

void InputSystem::OnPostUpdate() {
	if (this->mouseLocked) {
		this->prevMouseMovement = glm::vec2(0);
	}
}

int InputSystem::Order() {
	return INT_MIN;
}

void InputSystem::DrawImGui() {
	static char searchString[16] = { 0 };

	if (ImGui::TreeNode("Input System Debug")) {
		if (ImGui::TreeNode("Keys")) {
			ImGui::InputText("Search", searchString, 16);

			for (const auto& pair : this->keys) {
				if (pair.first >= MouseButtonOffset) {
					continue;
				}

				const std::string& keyName = keyToString.at((Key) pair.first);

				if (strlen(searchString) == 0 || keyName.contains(searchString)) {
					if (ImGui::TreeNode(keyName.c_str())) {
						ImGui::Text("%s", std::format("Key Down:     {}", pair.second.GetKeyDownBit()).c_str());
						ImGui::Text("%s", std::format("Key Pressed:  {}", pair.second.GetKeyPressedBit()).c_str());
						ImGui::Text("%s", std::format("Key Up:       {}", pair.second.GetKeyUpBit()).c_str());
						ImGui::Text("%s", std::format("Key Time:     {}", pair.second.GetPressTime()).c_str());

						ImGui::TreePop();
					}
				}
			}
			
			ImGui::TreePop();
		}
		
		if (ImGui::TreeNode("Mouse")) {
			for (int mouseButton = 0; mouseButton <= (int) MouseButton::Mouse8; mouseButton++) {
				const std::string& keyName = keyToString.at((Key) (mouseButton + MouseButtonOffset));
				const KeyBitMask keyValue = this->keys[mouseButton + MouseButtonOffset];
	
				if (ImGui::TreeNode(keyName.c_str())) {
					ImGui::Text("%s", std::format("Button Down:     {}", keyValue.GetKeyDownBit()).c_str());
					ImGui::Text("%s", std::format("Button Pressed:  {}", keyValue.GetKeyPressedBit()).c_str());
					ImGui::Text("%s", std::format("Button Up:       {}", keyValue.GetKeyUpBit()).c_str());
					ImGui::Text("%s", std::format("Button Time:     {}", keyValue.GetPressTime()).c_str());
	
					ImGui::TreePop();
				}
			}
			
			glm::vec2 mouseMovement = this->prevMouseMovement;
			ImGui::Text("%s", std::format("Mouse Locked: {}", this->mouseLocked).c_str());
			ImGui::Text("%s", std::format("Mouse Movement: ({:.3f}, {:.3f})",
				this->mouseLocked ? mouseMovement.x : 0,
				this->mouseLocked ? mouseMovement.y : 0
			).c_str());
	
			ImGui::Text("%s", std::format("Mouse Position: ({:.3f}, {:.3f})",
				this->mouseLocked ? 0 : mouseMovement.x,
				this->mouseLocked ? 0 : mouseMovement.y
			).c_str());
			
			ImGui::TreePop();
		}

		ImGui::TreePop();
	}
}
