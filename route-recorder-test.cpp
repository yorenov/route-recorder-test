#include "SAMPFUNCS_API.h"
#include "game_api.h"
#include <fstream>
#include <format>

#include <filesystem>
std::string directoryPath = "C:\\Yorenov-Cheat\\route-recorder-test\\";

SAMPFUNCS *SF = new SAMPFUNCS();

struct RoutePoint {
	float position[3];
	float moveSpeed[3];
	float quaternion[4];
	int32_t specialAction;
	int32_t specialKey;
	uint8_t currentWeapon;
	int32_t leftStickY;
	int32_t leftStickX;
	DWORD delay;
};

namespace settings_recordroute {
	bool record = false;
	bool render = false;
	bool cycle = false; // Цикличность воспроизведения
	bool tp_point = true; // ТП на точку при воспроизведении
	int last_point = 0;
	DWORD last_point_time = 0;
	std::string last_filename;
	std::vector<RoutePoint> route_points;
	std::ofstream wRoute_file;
	std::ifstream rRoute_file;
};

void sendOnfoot(OnFootData sync) {
	BitStream bsActorSync;
	bsActorSync.Write((BYTE)ID_PLAYER_SYNC);
	bsActorSync.Write((PCHAR)&sync, sizeof(OnFootData));
	SF->getRakNet()->SendPacket(&bsActorSync);
}

void CALLBACK render_route(std::string param)
{
	if (settings_recordroute::render) {
		settings_recordroute::render = false;
		SF->getSAMP()->getChat()->AddChatMessage(-1, "[{CECECE}Route Recorder by {1E5C3D}Yorenov{FFFFFF}]: Воспроизведение остановлено");
		return;
	}
	if (std::filesystem::exists(directoryPath + param)) {
		settings_recordroute::rRoute_file.open(directoryPath + param);
		settings_recordroute::route_points.clear();
		std::string line;
		RoutePoint point;
		while (std::getline(settings_recordroute::rRoute_file, line)) {
			sscanf_s(line.c_str(), "%f|%f|%f|%f|%f|%f|%f|%f|%f|%f|%d|%d|%hhd|%d|%d|%d", &point.position[0], &point.position[1], &point.position[2],
				&point.moveSpeed[0], &point.moveSpeed[1], &point.moveSpeed[2],
				&point.quaternion[0], &point.quaternion[1], &point.quaternion[2], &point.quaternion[3],
				&point.specialAction, &point.specialKey, &point.currentWeapon, &point.leftStickY, &point.leftStickX, &point.delay);
			settings_recordroute::route_points.push_back(point);
		}
		SF->getSAMP()->getChat()->AddChatMessage(-1, "[{CECECE}Route Recorder by {1E5C3D}Yorenov{FFFFFF}]: Маршрут '%s' | Точек: %d", param.c_str(), settings_recordroute::route_points.size());
		settings_recordroute::rRoute_file.close();
		settings_recordroute::last_point_time = GetTickCount();
		settings_recordroute::last_point = 0;
		settings_recordroute::render = true;
	}
	else
	{
		SF->getSAMP()->getChat()->AddChatMessage(-1, "[{CECECE}Route Recorder by {1E5C3D}Yorenov{FFFFFF}]: Маршрут '%s' не найден", param.c_str());
	}
}

void CALLBACK record_route(std::string param)
{
	if (param.empty()) return;
	if (settings_recordroute::record) {
		settings_recordroute::wRoute_file.close();
		SF->getSAMP()->getChat()->AddChatMessage(-1, "[{CECECE}Route Recorder by {1E5C3D}Yorenov{FFFFFF}]: Маршрут был сохранен");
		settings_recordroute::record = false;
		return;
	}

	if (std::filesystem::exists(directoryPath + param) && settings_recordroute::last_filename != param) {
		SF->getSAMP()->getChat()->AddChatMessage(-1, "[{CECECE}Route Recorder by {1E5C3D}Yorenov{FFFFFF}]: Файл уже существует. Отправь еще раз чтобы перезаписать");
		settings_recordroute::last_filename = param;
	}
	else
	{
		if (!std::filesystem::exists("C:\\Yorenov-Cheat\\route-recorder-test"))
		{
			if (std::filesystem::create_directories("C:\\Yorenov-Cheat\\route-recorder-test")) {
				SF->getSAMP()->getChat()->AddChatMessage(-1, "[{CECECE}Route Recorder by {1E5C3D}Yorenov{FFFFFF}]: У вас не был создан путь, программа создала его, начните запись еще раз! (%s)", directoryPath.c_str());
			}
			else {
				SF->getSAMP()->getChat()->AddChatMessage(-1, "[{CECECE}Route Recorder by {1E5C3D}Yorenov{FFFFFF}]: Ошибка! У вас нет пути, создайте его сами по %s", directoryPath.c_str());
			}
		}
		else {
			SF->getSAMP()->getChat()->AddChatMessage(-1, "[{CECECE}Route Recorder by {1E5C3D}Yorenov{FFFFFF}]: Запись маршрута '%s'", param.c_str());
			settings_recordroute::wRoute_file.open(directoryPath + param);
			settings_recordroute::last_point_time = GetTickCount();
			settings_recordroute::record = true;
		}
	}
}

bool CALLBACK onSendPacket(stRakNetHookParams* param)
{
	if (settings_recordroute::record) {
		if (param->packetId == PacketEnumeration::ID_PLAYER_SYNC)
		{
			OnFootData data;
			memset(&data, 0, sizeof(OnFootData));
			byte packet;
			param->bitStream->ResetReadPointer();
			param->bitStream->Read(packet);
			param->bitStream->Read((PCHAR)&data, sizeof(OnFootData));

			settings_recordroute::wRoute_file << std::format("{}|{}|{}|{}|{}|{}|{}|{}|{}|{}|{}|{}|{}|{}|{}|{}\n", data.position[0], data.position[1], data.position[2], 
			data.quaternion[0], data.quaternion[1], data.quaternion[2], data.quaternion[3], data.moveSpeed[0],
			data.moveSpeed[1], data.moveSpeed[2], data.specialAction, static_cast<uint8_t>(data.specialKey), static_cast<uint8_t>(data.currentWeapon), 
			data.controllerState.leftStickY, data.controllerState.leftStickX, GetTickCount() - settings_recordroute::last_point_time);

			settings_recordroute::last_point_time = GetTickCount();
		};
	}

	if (settings_recordroute::render) {
		return false;
	}
	return true;
};

void updateSyncData(OnFootData& sync, const RoutePoint& point) {
	sync.position[0] = point.position[0];
	sync.position[1] = point.position[1];
	sync.position[2] = point.position[2];

	sync.quaternion[0] = point.quaternion[0];
	sync.quaternion[1] = point.quaternion[1];
	sync.quaternion[2] = point.quaternion[2];
	sync.quaternion[3] = point.quaternion[3];

	sync.moveSpeed[0] = point.moveSpeed[0];
	sync.moveSpeed[1] = point.moveSpeed[1];
	sync.moveSpeed[2] = point.moveSpeed[2];

	sync.specialAction = point.specialAction;
	sync.specialKey = point.specialKey;
	sync.currentWeapon = point.currentWeapon;

	sync.controllerState.leftStickY = point.leftStickY;
	sync.controllerState.leftStickX = point.leftStickX;
}

void __stdcall mainloop()
{
	static bool initialized = false;
	if (!initialized)
	{
		if (GAME && GAME->GetSystemState() == eSystemState::GS_PLAYING_GAME && SF->getSAMP()->IsInitialized())
		{
			initialized = true;
			SF->getSAMP()->getChat()->AddChatMessage(-1, "SF plugin \"route-recorder-test\" was loaded | yorenov");
			SF->getSAMP()->registerChatCommand("route.record", record_route);
			SF->getSAMP()->registerChatCommand("route.run", render_route);
			SF->getSAMP()->registerChatCommand("route.cycle", [](std::string param) {
				settings_recordroute::cycle ^= true;
				if (settings_recordroute::cycle)
					SF->getSAMP()->getChat()->AddChatMessage(-1, "[{CECECE}Route Recorder by {1E5C3D}Yorenov{FFFFFF}]: Теперь записанный маршрут будет воспроизводиться по кругу");
				else 
					SF->getSAMP()->getChat()->AddChatMessage(-1, "[{CECECE}Route Recorder by {1E5C3D}Yorenov{FFFFFF}]: Теперь записанный маршрут будет воспроизводиться только 1 раз");
				});
			SF->getSAMP()->registerChatCommand("route.teleport", [](std::string param) {
				settings_recordroute::tp_point ^= true;
				if (settings_recordroute::tp_point)
					SF->getSAMP()->getChat()->AddChatMessage(-1, "[{CECECE}Route Recorder by {1E5C3D}Yorenov{FFFFFF}]: Теперь вы будете для себя телепортироваться на точки, но для других вы БЕГАЕТЕ");
				else
					SF->getSAMP()->getChat()->AddChatMessage(-1, "[{CECECE}Route Recorder by {1E5C3D}Yorenov{FFFFFF}]: Теперь вы будете для себя стоять в одном месте, но для других вы БЕГАЕТЕ");
				});

			SF->getRakNet()->registerRakNetCallback(RakNetScriptHookType::RAKHOOK_TYPE_OUTCOMING_PACKET, onSendPacket);
		}
	}
	else if (initialized)
	{
		if (settings_recordroute::render) {
			OnFootData sync;
			memset(&sync, 0, sizeof(OnFootData));

			sync = SF->getSAMP()->getPlayers()->localPlayerInfo.data->onFootData;

			if ((GetTickCount() - settings_recordroute::last_point_time) > settings_recordroute::route_points[settings_recordroute::last_point].delay) {
				const auto& currentPoint = settings_recordroute::route_points[settings_recordroute::last_point];

				updateSyncData(sync, currentPoint);

				sendOnfoot(sync);

				if (settings_recordroute::tp_point) {
					PEDSELF->SetPosition(currentPoint.position[0], currentPoint.position[1], currentPoint.position[2]);
				}

				settings_recordroute::last_point_time = GetTickCount();
				settings_recordroute::last_point++;

				if (settings_recordroute::last_point == settings_recordroute::route_points.size()) {
					if (settings_recordroute::cycle) {
						settings_recordroute::last_point = 0;
					}
					else {
						SF->getSAMP()->getChat()->AddChatMessage(-1, "[{CECECE}Route Recorder by {1E5C3D}Yorenov{FFFFFF}]: Воспроизведение завершено");
						settings_recordroute::render = false;
					}
				}
			}
		}
	}
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReasonForCall, LPVOID lpReserved)
{
	if (dwReasonForCall == DLL_PROCESS_ATTACH)
		SF->initPlugin(mainloop, hModule);
	return TRUE;
}
