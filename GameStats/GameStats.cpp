#include "stdafx.h"
#include "GameStats.h"
#include <set>

// match statement for std::visit
template <typename...T>
struct cases : T...{
	using T::operator()...;
};

void GameStats::Init()
{
	MessagingSystem::RegisterEventFunc("GAMEOVER", [&](const Event* e) { UpdateStats("Session", "Game"), playing = false /*, ResetStats("Game")*/; });
	MessagingSystem::RegisterEventFunc("RESTART", [&](const Event* e) { UpdateStats("Session", "Game"), ResetStats("Game"); });
	//MessagingSystem::RegisterEventFunc("GAMEPLAY", [&](const Event* e) { playing = true; });
	MessagingSystem::RegisterEventFunc("BUTTON_CLICK", [&](const Event* e) { ButtonClickHandler(dynamic_cast<const ButtonPress*>(e)); });
}

void GameStats::Update(float dt)
{
	if (playing) IncrementStat("Time Alive", dt);;
}

void GameStats::SetStat(const std::string& statName, const STAT_TYPES& val, const std::string& statGroup) {
	if (statName.empty()) {
		Tracing::Trace(Tracing::ERROR, "Attempting to set a=stat with no name");
	}
	else {
		auto ref = Get();
		if (!ref->stats.contains(statGroup)) {
			Tracing::Trace(Tracing::ERROR, "Attempting to set a stat in a stat group that does not exist");
		}
		else {
			if (ref->stats[statGroup].contains(statName)) {
				ref->stats[statGroup][statName] = val;
			}
			else {
				Tracing::Trace(Tracing::ERROR, "Attempting to set a stat in a stat group that does not exist");
			}
		}
	}
}

const std::optional<STAT_TYPES> GameStats::GetStat(const std::string& statName, const std::string& statGroup)
{
	if (statName.empty()) {
		Tracing::Trace(Tracing::WARNING, "Attempting to get a stat with no name");
		return {};
	}
	auto ref = Get();
	if (!ref->stats.contains(statGroup)) {
		Tracing::Trace(Tracing::ERROR, "Attempting to get a stat in a stat group that does not exist");
		return {};
	}
	if (!ref->stats[statGroup].contains(statName)) {
		Tracing::Trace(Tracing::WARNING, "Attempting to get a stat with name %s that does not exist", statName.c_str());
		return {};
	}
	else {
		return std::make_optional(ref->stats[statGroup][statName]);
	}
}

const std::vector<const std::string*> GameStats::GetStatNames(const std::string& statGroup)
{
	std::vector <const std::string*> statNames = std::vector<const std::string*>();
	auto instance = Get();
	for (auto pair : instance->stats[statGroup]) {
		statNames.push_back(&pair.first);
	}
	return statNames;
}

const std::vector < const std::string*> GameStats::GetStatGroupNames(const std::string& statGroup) 
{
	std::vector < const std::string*> statNames = std::vector<const std::string*>();
	auto instance = Get();
	for (auto pair : instance->stats[statGroup]) {
		statNames.push_back(&pair.first);
	}
	return statNames;
}

void GameStats::IncrementStat(const std::string& statName, std::variant<int, float, std::string> val, const std::string& statGroup)
{
	if (statName.empty()) {
		Tracing::Trace(Tracing::ERROR, "Attempting to set a stat with no name");
	}
	else {
		auto ref = Get();
		if (!ref->stats.contains(statGroup)) {
			Tracing::Trace(Tracing::ERROR, "Attempting to set a stat in a stat group that does not exist");
		}
		if (ref->stats[statGroup].contains(statName)) {
			ref->stats[statGroup][statName] = std::visit(
				// what to do depending on the type in the variant
				cases {
					[](const float& x, const float& y) -> STAT_TYPES { return x + y; },
					[](const int& x, const int& y) -> STAT_TYPES { return x + y; },
					[](const auto& x, const auto& y) -> STAT_TYPES { return STAT_TYPES(x); }
				}, ref->stats[statGroup][statName], val);
		}
		else {
			Tracing::Trace(Tracing::ERROR, "Attempting to set a stat in a stat group that does not exist");
		}
	}
}

void GameStats::ClearStats()
{
	Get()->stats.clear(); 
}

void GameStats::ResetStats(const std::string& statGroup)
{
	if (statGroup.empty()) {
		Tracing::Trace(Tracing::WARNING, "Attempting to clear a stat group with no name");
		return;
	}
	auto ref = Get();
	if (!ref->stats.contains(statGroup)) {
		Tracing::Trace(Tracing::WARNING, "Attempting to clear a stat group that does not exist");
	}
	else {
		for (auto& pair : ref->stats[statGroup]) {
			pair.second = std::visit(
				// what to do depending on the type in the variant
				cases{
					[](const float&) -> STAT_TYPES { return STAT_TYPES(0.f); },
					[](const int&) -> STAT_TYPES { return STAT_TYPES(0); },
					[](const auto&) -> STAT_TYPES { return STAT_TYPES(std::string()); }
				}, pair.second);
		}
	}
}

void GameStats::ResetAllStats()
{
	auto ref = Get();
	for (auto& stat : ref->stats)
	{ // For the overarching stat type
		for (auto& pair : stat.second)
		{ // For the specific stat type
			pair.second = std::visit(
				// What we do depending on what variant type it is 
				cases{
					[](const float&) -> STAT_TYPES { return STAT_TYPES(0.f); },
					[](const int&) -> STAT_TYPES { return STAT_TYPES(0); },
					[](const auto&) -> STAT_TYPES { return STAT_TYPES(std::string()); }
				},pair.second);
		}
	}
}

void GameStats::Serialize()
{
	// update from game stats in case program exits mid-game
	UpdateStats("Session", "Game");
	// this is the only place this is saved
	UpdateStats("Lifetime", "Session");

	const auto& ser = Serializer::GetInstance();
	ser->ReadFile("Data/JSONS/GameStats.json");
	std::vector<std::string> statGroups;
	std::set<std::string> statNames;
	for (const auto& statGroup : stats) {
		statGroups.push_back(statGroup.first);
		for (const auto& stat : statGroup.second) {
			statNames.insert(stat.first);
			std::visit(
				cases{
					[statGroup, stat, ser](auto val) { ser->SetData(statGroup.first + "." + stat.first, val);}
				}
			, stat.second);
		}
	}
	ser->SetData("Stat Names", statNames);
	ser->SetData("Stat Groups", statGroups);
	ser->Transcribe("Data/JSONS/GameStats.json");
	ser->CleanData();
}

void GameStats::Deserialize()
{
	auto ser = Serializer::GetInstance();
	ser->ReadFile("Data/JSONS/GameStats.json");
	const auto& stat_names = ser->GetData("Stat Names");
	const auto& groups = ser->GetData("Stat Groups");
	for (const std::string& group : groups) {
		stats.emplace(group, std::map<std::string, STAT_TYPES>());
		for (const std::string& name : stat_names) {
			// try to get the stat, if it doesn't exist move on	
			STAT_TYPES stat;
			auto balls = ser->GetData(group + "." + name);
			if (balls.is_null()) continue;
			if (balls.is_number_float()) stat = STAT_TYPES(static_cast<float>(balls));
			else if (balls.is_number_integer()) stat = STAT_TYPES(static_cast<int>(balls));
			else if (balls.is_string()) stat = STAT_TYPES(static_cast<std::string>(balls));
			stats[group].emplace(name, stat);
		}
		if (group != "Lifetime") ResetStats(group);
	}
}

void GameStats::ButtonClickHandler(const ButtonPress* ev)
{
	if (ev->command == "GAMEPLAY") {
		IncrementStat("Games Played", 1, "Session");
		UpdateStats("Session", "Game");
		ResetStats("Game");
		playing = true;
	}
	else if (ev->command == "RESETSTATS") {
		ResetStats("Game");
		ResetStats("Lifetime");
		ResetStats("Session");
	}
	else if (ev->command == "PAUSE") playing = !playing;
}

GameStats::~GameStats()
{
	ClearStats();
}


void GameStats::UpdateStats(const std::string& to, const std::string& from)
{
	// just grab refs so we don't need to do it every time
	auto ref = Get();
	auto& to_map = ref->stats[to];
	auto& from_map = ref->stats[from];
	auto from_cstr = from.c_str();
	auto to_cstr = to.c_str();
	// since there is some variation in stat names in betwween stat groups, we have to specify updates between each stat group
	if (strcmp(from_cstr, "Game") == 0 && strcmp(to_cstr, "Session") == 0) {
		if (to_map["Best Kills"] < from_map["Kills"]) {
			to_map["Best Kills"] = from_map["Kills"];
		}
		if (to_map["Best Level"] < from_map["Level"]) {
			to_map["Best Level"] = from_map["Level"];
		}
		if (to_map["Best Time"] < from_map["Time Alive"]) {
			to_map["Best Time"] = from_map["Time Alive"];
		}
		to_map["Levels Gained"] = STAT_TYPES(std::get<int>(to_map["Levels Gained"]) + std::get<int>(from_map["Level"]));
	}
	else if (strcmp(from_cstr, "Session") == 0 && strcmp(to_cstr, "Lifetime") == 0) {
		if (to_map["Best Kills"] < from_map["Best Kills"]) {
			to_map["Best Kills"] = from_map["Best Kills"];
		}
		if (to_map["Best Level"] < from_map["Best Level"]) {
			to_map["Best Level"] = from_map["Best Level"];
		}
		if (to_map["Best Time"] < from_map["Best Time"]) {
			to_map["Best Time"] = from_map["Best Time"];
		}
		to_map["Games Played"] = STAT_TYPES(std::get<int>(to_map["Games Played"]) + std::get<int>(from_map["Games Played"]));
		to_map["Levels Gained"] = STAT_TYPES(std::get<int>(to_map["Levels Gained"]) + std::get<int>(from_map["Levels Gained"]));
	}
	// if its not covered by the if cases then its not a valid stat update to call
	else {
		Tracing::Trace(Tracing::WARNING, "Attempted to make an invalid stat update");
		return;
	}
	// generic updates that are valid for all valid updates
	to_map["Kills"] = STAT_TYPES(std::get<int>(from_map["Kills"]) + std::get<int>(to_map["Kills"]));
	to_map["Time Alive"] = STAT_TYPES(std::get<float>(from_map["Time Alive"]) +std::get<float>(to_map["Time Alive"]));
}

void GameStats::Register()
{
	Engine::AddSystem<GameStats>(Get());
	Tracing::Trace(Tracing::LOG, "GameStats: Online");
}
