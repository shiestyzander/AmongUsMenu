#include "pch-il2cpp.h"
#include "replay.hpp"
#include "DirectX.h"
#include "state.hpp"
#include "gui-helpers.hpp"
#include <sstream>

namespace Replay
{
	std::mutex replayEventMutex;

	// TODO: improve this by building it dynamically based on the EVENT_TYPES enum
	std::vector<std::pair<const char*, bool>> event_filter =
	{
		{"Kill", false},
		{"Vent", false},
		{"Task", false},
		{"Report", false},
		{"Meeting", false},
		{"Vote", false},
		{"Cheat", false},
		{"Disconnect", false},
		{"Shapeshift", false},
		{"Protect", false},
		{"Walk", false}
	};

	std::vector<std::pair<PlayerSelection, bool>> player_filter;

	ImU32 GetReplayPlayerColor(uint8_t colorId) {
		return ImGui::ColorConvertFloat4ToU32(AmongUsColorToImVec4(GetPlayerColor(colorId)));
	}

	void SquareConstraint(ImGuiSizeCallbackData* data)
	{
		data->DesiredSize = ImVec2(data->DesiredSize.x, data->DesiredSize.y);
	}

	bool init = false;
	void Init()
	{
		ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(FLT_MAX, FLT_MAX), SquareConstraint);
		ImGui::SetNextWindowBgAlpha(1.F);

		if (!init)
		{
			// setup player_filter list based on MAX_PLAYERS definition
			for (int i = 0; i < MAX_PLAYERS; i++) {
				Replay::player_filter.push_back({ PlayerSelection(), false });
			}
			init = true;
		}
	}

	
	void Render()
	{
		Replay::Init();

		int MapType = State.mapType;
		ImGui::SetNextWindowSize(ImVec2(560, 400), ImGuiCond_None);

		ImGui::Begin("Replay", &State.ShowReplay, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse);

		ImGui::BeginChild("console#filter", ImVec2(560, 20), true);
		ImGui::Text("Event Filter: ");
		ImGui::SameLine();
		CustomListBoxIntMultiple("Event Types", &Replay::event_filter, 100.f);
		if (IsInGame()) {
			ImGui::SameLine(0.f, 5.f);
			ImGui::Text("Player Filter: ");
			ImGui::SameLine();
			CustomListBoxPlayerSelectionMultiple("Players", &Replay::player_filter, 150.f);
		}
		ImGui::EndChild();
		ImGui::Separator();

		// TODO: Size it for map and calculate center of the parent window for proper map placement
		ImGui::BeginChild("ReplayMap");

		ImVec2 winpos = ImGui::GetWindowPos();
		ImDrawList* drawList = ImGui::GetWindowDrawList();

		ImGui::Image((void*)maps[MapType].mapImage.shaderResourceView,
			ImVec2((float)maps[MapType].mapImage.imageWidth * 0.5f + 2.5f, (float)maps[MapType].mapImage.imageHeight * 0.5f + 2.5f),
			(State.FlipSkeld && MapType == 0) ? ImVec2(1.0f, 0.0f) : ImVec2(0.0f, 0.0f),
			(State.FlipSkeld && MapType == 0) ? ImVec2(0.0f, 1.0f) : ImVec2(1.0f, 1.0f),
			State.SelectedReplayMapColor);

		// for each player
		for (int n = 0; n < MAX_PLAYERS; n++)
		{
			bool playerFound = false, anyPlayerFilterSelected = false;
			for (auto player : Replay::player_filter) {
				if (player.second
					&& player.first.has_value()
					&& player.first.get_PlayerId() == n)
				{
					playerFound = true;
					anyPlayerFilterSelected = true;
					break;
				}
				else if (player.second)
					anyPlayerFilterSelected = true;
			}

			if (!playerFound && anyPlayerFilterSelected)
				continue;


			bool isUsingEventFilter = false;
			for (int t = 0; t < Replay::event_filter.size(); t++)
			{
				if (Replay::event_filter[t].second == true)
				{
					isUsingEventFilter = true;
					break;
				}
			}
			// for each event type
			for (int m = 0; m < EVENT_TYPES_SIZE; m++)
			{
				// IMPORTANT:
				// Replay::event_filter must be in same order as EVENT_TYPES enum defined in _events.h
				if (Replay::event_filter[m].second == false && isUsingEventFilter == true)
					continue;

				// for each entry in event vector
				for (int i = 0; i < State.events[n][m].size(); i++)
				{
					std::lock_guard<std::mutex> replayLock(Replay::replayEventMutex);
					EventInterface* e = State.events[n][m].at(i);

					if (e->getType() == EVENT_TYPES::EVENT_WALK)
					{
						auto walk_event = dynamic_cast<WalkEvent*>(e);
						auto position = walk_event->GetPosition();
						float mapX = maps[MapType].x_offset + (position.x * maps[MapType].scale) + winpos.x;
						float mapY = maps[MapType].y_offset - (position.y * maps[MapType].scale) + winpos.y;

						if (i + 1 >= State.events[n][m].size())
						{
							drawList->AddCircleFilled(ImVec2(mapX, mapY), 4.5F, GetReplayPlayerColor(e->getSource().colorId));
							continue;
						}

						EventInterface* e2 = State.events[n][m].at(i + 1); // get position of next walk_event
						auto walk_event2 = dynamic_cast<WalkEvent*>(e2);
						auto position2 = walk_event2->GetPosition();
						float mapX2 = maps[MapType].x_offset + (position2.x * maps[MapType].scale) + winpos.x;
						float mapY2 = maps[MapType].y_offset - (position2.y * maps[MapType].scale) + winpos.y;

						drawList->AddLine(ImVec2(mapX, mapY), ImVec2(mapX2, mapY2), GetReplayPlayerColor(e->getSource().colorId));
					}
				}
			}
		}

		ImGui::EndChild();

		// new child (control)
		// slider based on chronos timestamp from beginning of round until now (live)

		ImGui::End();
	}
}