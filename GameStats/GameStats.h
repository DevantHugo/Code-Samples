#pragma once
#include <string>
#include <map>
#include <optional>
#include "Messaging/Events/ButtonPress.h"
/*

	Author: Hugo Devant
	Copyright: DigiPen
	Sources: cppreference

*/

using STAT_TYPES = std::variant<int, float, std::string>;

/*
	Keeps track of various game stats
*/
class GameStats : public System {
public:
	void Init() override;
	/*
		@brief Basic update function, just increments the time alive stat for now.
		@param dt: The sime since the last frame.
	*/
	void Update(float dt) override;
	/*
		@brief Sets the given stat to the given value.
		@param statName: The name of the stat to set.
		@param val: The value to set the stat to.
		@param statGroup: The name of the statgroup to set the stat in. Game by default.
	*/;
	static void SetStat(const std::string& statName, const STAT_TYPES& val, const std::string& statGroup = "Game");
	/*
		@brief Gets the stat with the given name.
		@param statName: The name of the stat to get.
		@param statGroup: The name of the statgroup to set the stat in. Game by default.
		@return the value of the stat, if it exists.
	*/
	[[nodiscard]] static const std::optional<STAT_TYPES> GetStat(const std::string& statName, const std::string& statGroup = "Game");

	/*
		@brief Gets all of the stat names for a statgroup.
		@param statGroup: The name of the statgroup to get the statnames from. Game by default.
	*/
	[[nodiscard]] static const std::vector<const std::string*> GetStatNames(const std::string& statGroup = "Game");

	/*
		@brief Gets all of the stat names for a statgroup.
		@param statGroup: The name of the statgroup to get the statnames from. Game by default.
	*/
	[[nodiscard]] static const std::vector<const std::string*> GetStatGroupNames(const std::string& statGroup = "Game");
	/*
		@brief Increment a stat by the given amount
		@param statName: the name of the stat to get.
		@param val: the value to increment by
		@param statGroup: The name of the statgroup to set the stat in. Game by default.
	*/
	static void IncrementStat(const std::string& statName, std::variant<int, float, std::string> val, const std::string& statGroup = "Game");

	/*
		@brief Clears all the stats.
	*/
	static void ClearStats();
	/*
		@brief Resets the given statgroup to default values.
	*/
	static void ResetStats(const std::string& statGroup);
	/*
		@brief Resets all of the stats to their default values
	*/
	static void ResetAllStats();
	/*
		@brief Saves the stats to file.
	*/
	void Serialize() override;
	/*
		@brief Reads in stats from file.
	*/
	void Deserialize() override;

	/*
		@brief Button click handler to know when the game starts/end and is paused.
		@params ev: The button click event holding the command.
	*/
	void ButtonClickHandler(const ButtonPress* ev);
	/*
		@brief Basic dtor, just clears the stats.
	*/
	~GameStats();
private:
	static inline GameStats* instance = nullptr;
	GameStats() : System(SYS_Stats) {};
	static inline void Create() 
	{
		if (!instance) {
			instance = new GameStats();
		}
		else {
			Tracing::Trace(Tracing::WARNING, "Attempted to create multiple instances of GameStats");
		}
	}
	static void UpdateStats(const std::string& to, const std::string& from);
	static void Register();
	[[nodiscard]] static inline GameStats* Get() {
		if (!instance) {
			Create();
		}
		return instance;
	}

	// stat group ---> stat name ---> stat value
	std::map < std::string, std::map<std::string, STAT_TYPES>> stats;

	bool playing = false;

	REGISTER;
};