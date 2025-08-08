/*

	Author: Hugo Devant
	Copyright: DigiPen Institute of Technology
	Sources: cppreference

*/

#ifndef MESSAGING
#define MESSAGING
#include "System.h"
#include "Messaging/Events/Event.h"
#include <functional>
#include <any>
#include <variant>

#include "Events/CollisionEnter.h"
#include "Events/CollisionStay.h"
#include "Events/CollisionExit.h"
#include "Events/EntityCreation.h"
#include "Events/EntityDeletion.h"
#include "Events/EntityState.h"
#include "Events/MouseClick.h"
#include "Events/ButtonPress.h"
#include "Events/AudioEvent.h"
#include "Events/Pause.h"
#include "Events/ParticleEmitterBase.h"
#include "Events/LevelUp.h"



/*
	How to create & use a new event:

	1. Create YourEventClass.h
	2. Declare YourEventClass
	3. Derive from the base class (Event) - details can be found in src/Systems/Messaging/Events/Event.h
	4. Add any data you want to be held in the event - these should be constant
	5. Declare & define your Register function
	6. Call the RegisterEventCreator function within it, and pass in a function that will create your event (usually your constructor)
	7. Call the REGISTER macro at the bottom of your class' private variables
	8. Register listeners using the RegisterEventFunc function and the name you registered your event as

	For an example, see the CollisionEnter event - details can be found in src/Systems/Messaging/Events/CollisionEnter.h
*/

/*
	@brief System to facilitate and minimize dependencies for inter-system communication.
*/
class MessagingSystem : public System {
public:

	void Init() override;

	static void Create();

	~MessagingSystem() override;

	/*
		@brief Function for registering for an Event
		@details 
		The function taken in as a parameter should generally be a lambda that captures a reference to the object you are calling it from, that then calls the function you want to be called when the Event is triggered
		@param e: The name of the event you want to register for.
		@param func: The function you want to register for that event. Takes in an Event.
	*/
	static void RegisterEventFunc(const std::string& e, std::function<void(const Event*)> func);

	/*
		@brief Registers a function to check if there exists a T associated with the ID that is passed in.
		@details
		The function taken in as a parameter should generally be a lambda that captures a reference to the object you are calling it from, that then calls the function you want to be called to check if a T exists associated with the ID.
		@tparam T: The type that can be checked to see if there exists an instance of it associated with the given ID.
		@param func: The function you want to register for this query. Takes in an ID and returns whether or not it exists.
	*/
	template <typename T>
	constexpr static void RegisterQueryFunc(std::function<bool(const int&)> func) {
		GetInstance()->queryFuncs[std::type_index(typeid(T))] = func;
	}

	/*
		@brief Registers a function that gets a T associated with the ID that is passed in.
		@details
		The function taken in as a parameter should generally be a lambda that captures a reference to the object you are calling it from, that then calls the function you want to be called to return a T that is associated with the ID.
		@tparam T: The type that should be retrieved and returned. Needs to derive from Component.
		@param func: The function you want to register for the request. Takes in an ID and returns a T*.
	*/
	template <typename T>
	constexpr static void RegisterRequestFunc(std::function<std::any(const int&)> func) {
		GetInstance()->requestFuncs[std::type_index(typeid(T))] = func;
	}

	/*
		@brief Registers a function that creates a T from an archetype with a new ID.
		@details
		The function take in as a parameter should generally be a lambda that captures a reference to the object you are calling it from, that then calls the function you want to be called.
		@tparam T: The type that should be created.
		@param func: The function you want to register to create the T. Takes in the name of the archetype to create from and the ID the new T should be associated with. Doesn't return anything.
	*/
	template <typename T>
	constexpr static void RegisterCreateFunc(std::function <void(const std::string&, const int&) > func) {
		GetInstance()->createFuncs[std::type_index(typeid(T))] = func;
	}

	/*
		@brief Registers a function that creates an Event and returns it
		@details
		The function taken in as a parameter should generally be a lambda that captures a reference to the object you are calling it from that then calls the Event constructor.
		@param e: The name of the event. For ease of use, use all caps.
		@param func: The function you want to register to create the event. Takes in a vector of parameters of different types.
	*/
	static void RegisterEventCreator(const std::string& e, std::function<Event*(std::vector<std::any>&)> func);

	/*
		@brief Registers a function for a special event
		@details
		This function is for very important events: the main use right now is for Entity Deletion. The function taken in as a parameter should generally be a lambda that captures a reference to the object you are calling it from, that then calls the function you want to be called when a special event happens.
		@param e: The name of the event. For ease of use, use all caps.
		@param func: The function you want to register for the event. Takes in an ID.
	
	*/
	static void RegisterSpecialEventFunc(const std::string& e, std::function<void(const int&)> func){
		GetInstance()->specEventFuncs[e].push_back(func);
	}

	/*
		@brief Registers a function for when a T associated with the ID should activate/deactivate
		@details
		This should only be used for Entity components and behaviors. The function taken in as a parameter should generally be a lambda that captures a reference to the object you are calling it from, that then calls the function you want to be called to activate/deactivate a T associated with the ID.
		@tparam T: The type that should be activated/deactivated
		@param func: The function to be called when a T should be activated/deactivated. Takes in an ID and a bool that determines whether it should be activated or deactivated.
	*/
	template <typename T>
	static void RegisterStateChangeFunc(std::function<void(const int&, const bool&)> func) {
		GetInstance()->stateChangeFuncs[std::type_index(typeid(T))] = func;
	}

	/*
		@brief Registers a function for a special request
		@details
		This is mainly used for getting the ID of the first instance of an Entity with the given name. The function taken in as a parameter should generally be a lambda that captures a reference to the object you are calling it from, that then calls the function you want to be called to for the special request.
		@param e: The name of the request. For ease of use, use all caps.
		@param func: The function to be called for the special request. Takes in a name and, optionally, return an ID.
	*/
	static void RegisterSpecialRequestFunc(const std::string& e, std::function<std::optional<const int>(const std::string&)> func) {
		GetInstance()->specRequestFuncs[e] = func;
	}

	/*
		@brief Broadcasts an event to it's listeners
		@details
		Use either this function or the Broadcast that takes in an Event.
		@tparam Args: Deduced types of the parameter list. You do not need to pass any arguments in.
		@param e: The name of the event to be broadcasted.
		@param args: Any number of arguments of any type to be forwarded to the event creator function.

	*/
	template <typename...Args>
	static void BroadcastEvent(const std::string& e, Args&&...args){ 
		std::vector<std::any> arg = { std::forward<Args>(args)... };
		Event* ev;
		auto instance = MessagingSystem::GetInstance();
		try {
			ev = instance->eventCreators[e](arg);
		}
		catch (std::exception ex) {
  			Tracing::Trace(Tracing::ERROR, "Error when creating Event");
			Tracing::Trace(ex.what());
			return;
		}
		for (auto& func : instance->eventFuncs[e]) {
			try {
				func(ev);
			}
			catch (std::exception ex) {
				ex;
				Tracing::Trace("Error when broadcasting Event");
			}
		}
		delete ev;
	}

	/*
		@brief Broadcasts an event to it's listeners.
		@details
		Use either this function or the Broadcast that takes in variadic parameters.
		@param e: The name of the event to be broadcasted.
		@param ev: The event to be broadcasted.
	
	*/
	static void Broadcast(const std::string& e, const Event* ev) {
		auto instance = GetInstance();
		for (auto& func : instance->eventFuncs[e]) {
			try {
				func(ev);
			}
			catch (std::exception ex) {
				std::cout << ex.what() <<std::endl;
				Tracing::Trace("Error when broadcasting Event");
				Tracing::Trace(ex.what());
			}
		}
		delete ev;
	}

	/*
		@brief Broadcasts a special event.
		@param e: The name of the event to be broadcasted.
		@param i: The ID that the event is in regard to.
	*/
	static void Broadcast(const std::string& e, const int& i) {
		auto instance = GetInstance();
		for (auto& func : instance->specEventFuncs[e]) {
			try {
				func(i);
			}
			catch (std::exception) {
				Tracing::Trace("Error when broadcasting special Event");
			}
		}
	}

	/*
		@brief Function for special requests.
		@param e: The name of the special request
		@param arg: The name of the subject of the request. Generally of an Entity.
		@return Optionally returns an ID. To access, use .value() on the return value of this function.
	
	*/
	static std::optional<const int> Request(const std::string& e, const std::string& arg) {
		return GetInstance()->specRequestFuncs[e](arg);
	}

	/*
		@brief Function for queries.
		@tparam T: The type to query.
		@param id: The id to check if a T exists associated with it.
		@return Whether or not a T exists associated with the ID.
	*/
	template < typename T>
	static bool Query(const int& id) {
		return GetInstance()->queryFuncs[std::type_index(typeid(T))](id);
	}

	/*
		@brief Function for requests.
		@tparam T: The type to get.
		@param id: The id with which the T is associated with.
		@return Pointer to the T associated with the ID.
	*/
	template < typename T>
	static T* Request(const int& id) {
		return std::any_cast<T*>(GetInstance()->requestFuncs[std::type_index(typeid(T))](id));
	}

	/*
		@brief Function for creating
		@tparam T: The type to create.
		@param id: The id that the T should be created with.
	*/
	template <typename T>
	static void Create(const std::string& name, const int& id) {
		GetInstance()->createFuncs[std::type_index(typeid(T))](name, id);
	}

	/*
		@brief Function for state changes (activation/deactivation).
		@tparam T: The type to activate/deactivate.
		@param id: The id for which the T should be activated/deactivated.
		@param s: What state the T should be changed to. True = Active, False = Inactive.
	*/
	template < typename T>
	static void Request(const int& id, const bool& s) {
		GetInstance()->stateChangeFuncs[std::type_index(typeid(T))](id, s);
	}
	void Serialize() override {}
	
	void Deserialize() override {}

private:

	static MessagingSystem* GetInstance();
	std::unordered_map<std::string, std::vector<std::function<void(const Event*)>>> eventFuncs;
	std::unordered_map<std::type_index, std::function<bool(const int&)>> queryFuncs;
	std::unordered_map<std::type_index, std::function<std::any(const int&)>> requestFuncs;
	std::unordered_map < std::type_index, std::function<void(const std::string&, const int&) >> createFuncs;
	std::unordered_map<std::type_index, std::function<void(const int&, const bool&)>> stateChangeFuncs;
	std::unordered_map<std::string, std::function<Event*(std::vector<std::any>&)>> eventCreators;
	std::unordered_map<std::string, std::vector<std::function<void(const int&)>>> specEventFuncs;;
	std::unordered_map<std::string, std::function<std::optional<const int>(const std::string&)>> specRequestFuncs;
	static inline MessagingSystem* instance = nullptr;
	MessagingSystem();
	MessagingSystem(int id);

	static void Register();
	REGISTER;
};

/*
	@brief Wrapper lambda for requests.
	@tparam T: The type to get.
	@param id: The id with which the T is associated with.
	@return Pointer to the T associated with the ID.
*/
template <typename T>
static auto Request = [](const int& id) {
	return MessagingSystem::Request<T>(id);
};

/*
	@brief Wrapper lambda for creation.
	@tparam T: The type to create.
	@param id: The id that the T should be created with.
*/
template <typename T>
static auto Create = [](const std::string& name, const int& id) {
	MessagingSystem::Create<T>(name,id);
};

/*
	@brief Wrapper lambda for activation.
	@tparam T: The type to activate.
	@param id: The id for which the T should be activated.
*/
template <typename T>
static auto _Activate = [](const int& id) {
	return MessagingSystem::Request<T>(id, true);
};

/*
	@brief Wrapper lambda for deactivation.
	@tparam T: The type to deactivate.
	@param id: The id for which the T should be deactivated.
*/
template <typename T>
static auto _Deactivate = [](const int& id) {
	return MessagingSystem::Request<T>(id, false);
};

/*
	@brief Wrapper lambda for special requests.
	@param e: The name of the special request
	@param arg: The name of the subject of the request. Generally of an Entity.
	@return Optionally returns an ID. To access, use .value() on the return value of this function.
*/
static auto SpecRequest = [](const std::string& e, const std::string s)->std::optional<const int> {
	return MessagingSystem::Request(e, s);
};

/*
	@brief Wrapper lambda for queries.
	@tparam T: The type to query.
	@param id: The id to check if a T exists associated with it.
	@return Whether or not a T exists associated with the ID.
*/
template <typename T>
static auto Query = [](const int& id) {
	return MessagingSystem::Query<T>(id);
};

/*
	@brief Wrapper lambda for broadcasting events.
	@param e: The name of the event to be broadcasted.
	@param ev: The event to be broadcasted.
*/
static auto Broadcast = [](const std::string& e, const Event* ev) {
	return MessagingSystem::Broadcast(e, ev);
};

/*
	@brief Wrapper lambda for broadcasting special events.
	@param e: The name of the event to be broadcasted.
	@param i: The ID that the event is in regard to.
*/
static auto SpecBroadcast = [](const std::string& e, const int& id) {
	return MessagingSystem::Broadcast(e, id);
};

#endif

