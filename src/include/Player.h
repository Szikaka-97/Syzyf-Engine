#pragma once

#include <GameObject.h>
#include <vector>
#include <glm/glm.hpp>
#include "physics/DebugRenderer.h"
#include "Scene.h"
#include <vector>

class Surface; 

class Player : public GameObject{
	public: 
		Player();
		void Update();  
	private: 
		int m_RoomID;
		std::vector<Surface*> rooms;
		bool HasRoomChanged();
		void CheckPosition();

};