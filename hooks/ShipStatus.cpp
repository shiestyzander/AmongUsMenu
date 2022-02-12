#include "pch-il2cpp.h"
#include "_hooks.h"
#include "state.hpp"
#include "logger.h"
#include "utility.h"
#include "replay.hpp"
#include "profiler.h"

float dShipStatus_CalculateLightRadius(ShipStatus* __this, GameData_PlayerInfo* player, MethodInfo* method) {
	if (State.MaxVision || State.EnableZoom || State.FreeCam)
		return 10.F;
	else
		return ShipStatus_CalculateLightRadius(__this, player, method);
}

void dShipStatus_OnEnable(ShipStatus* __this, MethodInfo* method) {
	ShipStatus_OnEnable(__this, method);

	Profiler::BeginSample("ClearEvents");
	Replay::Reset();
	for (auto& flatEvt : State.flatEvents)
		flatEvt.reset();
	State.flatEvents.clear();

	for (auto& conEvt : State.consoleEvents)
		conEvt.reset();
	State.consoleEvents.clear();

	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		for (int j = 0; j < EVENT_TYPES_SIZE; j++)
		{
			for (auto& evt : State.events[i][j])
			{
				evt.reset();
			}
			State.events[i][j].clear();
		}
	}
	Profiler::EndSample("ClearEvents");

	State.selectedDoor = SystemTypes__Enum::Hallway;
	State.mapDoors.clear();
	State.pinnedDoors.clear();

	auto allDoors = __this->fields.AllDoors;

	for (il2cpp_array_size_t i = 0; i < allDoors->max_length; i++) {
		if (std::find(State.mapDoors.begin(), State.mapDoors.end(), allDoors->vector[i]->fields.Room) == State.mapDoors.end())
			State.mapDoors.push_back(allDoors->vector[i]->fields.Room);
	}

	std::sort(State.mapDoors.begin(), State.mapDoors.end());

	State.mapType = (Settings::MapType)(__this->fields.Type);

	State.userName = convert_from_string(SaveManager__TypeInfo->static_fields->lastPlayerName);
	ResetOriginalAppearance();
}