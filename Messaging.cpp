/*

	Author: Hugo Devant
	Copyright: Digipen
	Sources: 

*/

#include "stdafx.h"

void MessagingSystem::Register()
{
	Engine::AddSystem<MessagingSystem>(GetInstance());
	Tracing::Trace(Tracing::LOG, "MessagingSystem: Online");
}

MessagingSystem::MessagingSystem() : System(SYS_Messaging) {}

MessagingSystem* MessagingSystem::GetInstance() {
	if (!instance) Create();
	return instance;
}

void MessagingSystem::Create() {
	if (!instance) instance = new MessagingSystem();
}

void MessagingSystem::Init() {
}

/*
	Usage: pass in the event type as well as  ([&](const Event* event) { yourfunctionhere(event); }) 
*/
void MessagingSystem::RegisterEventFunc(const std::string& e, std::function<void(const Event*)> func) {
	GetInstance()->eventFuncs[e].push_back(func);
}


/*

	Used to pass in a function that creates the given event

	Usage: look at CollisionEnter for now

*/
void MessagingSystem::RegisterEventCreator(const std::string& e, std::function<Event*(std::vector<std::any>&)> func) {
	GetInstance()->eventCreators[e] = func;
}



MessagingSystem::~MessagingSystem() {
	/*registry.clear();*/
	eventCreators.clear();
	instance = nullptr;
}