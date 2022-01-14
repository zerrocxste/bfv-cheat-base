#include "includes.h"

namespace console
{
	FILE* output_stream = nullptr;

	void attach(const char* name)
	{
		if (AllocConsole())
		{
			SetConsoleTitle(name);
		}
		freopen_s(&output_stream, "conout$", "w", stdout);
	}

	void detach()
	{
		if (output_stream)
		{
			fclose(output_stream);
		}
		FreeConsole();
	}
}

std::uintptr_t pClientSoldierEntityLocal;
std::deque<DWORD_PTR> ClientSoldierEntityStructList;
std::mutex mtx;

using fUnkFunc1 = void(__fastcall*)(__int64);
fUnkFunc1 pfUnkFunc1 = NULL;

float __fastcall UnkFunc1_hooked(__int64 a1)
{
	std::uintptr_t client_soldier_entity = (std::uintptr_t)(a1 - 0x338);
	//cse + 0xC90 = coords

	auto res = std::find(ClientSoldierEntityStructList.begin(), ClientSoldierEntityStructList.end(), client_soldier_entity) != ClientSoldierEntityStructList.end();

	if (!res)
	{
		mtx.lock();
		ClientSoldierEntityStructList.push_back(client_soldier_entity);
		mtx.unlock();
	}

	pfUnkFunc1(a1);
}

using fUnkFunc2 = __int64(__fastcall*)(__int64, __int64, __int64);
fUnkFunc2 pfUnkFunc2 = nullptr;

__int64 __fastcall UnkFunc2_hooked(__int64 a1, __int64 a2, __int64 a3)
{
	pClientSoldierEntityLocal = (std::uintptr_t)(a1 + 0x110);

	return pfUnkFunc2(a1, a2, a3);
}

class vector3 {
public:
	float x, y, z;
};

class Matrix4x4
{
public:
	union
	{
		struct
		{
			float        _11, _12, _13, _14;
			float        _21, _22, _23, _24;
			float        _31, _32, _33, _34;
			float        _41, _42, _43, _44;
		};
		float m[4][4];
	};
};

void thread(void* arg)
{
	console::attach("debug");

	MH_Initialize();

	/*
		Address of signature = bfv.exe + 0x020C7C90
		"\x48\x89\x00\x00\x00\x57\x48\x83\xEC\x00\x48\x8B\x00\x00\x48\x8B\x00\x48\x8B\x00\x48\x8B\x00\xFF\x90", "xx???xxxx?xx??xx?xx?xx?xx"
		"48 89 ? ? ? 57 48 83 EC ? 48 8B ? ? 48 8B ? 48 8B ? 48 8B ? FF 90"
	*/

	auto Unkfunc1Address = memory_utils::pattern_scanner_module(memory_utils::get_base(), "\x48\x89\x00\x00\x00\x57\x48\x83\xEC\x00\x48\x8B\x00\x00\x48\x8B\x00\x48\x8B\x00\x48\x8B\x00\xFF\x90", "xx???xxxx?xx??xx?xx?xx?xx");
	MH_CreateHook((void*)Unkfunc1Address, &UnkFunc1_hooked, (LPVOID*)&pfUnkFunc1);
	MH_EnableHook((void*)Unkfunc1Address);

	auto Unkfunc2Address = memory_utils::pattern_scanner_module(memory_utils::get_base(), "\x40\x00\x57\x41\x00\x48\x83\xEC\x00\x48\xC7\x44\x24\x28\x00\x00\x00\x00\x48\x89\x00\x00\x00\x48\x89\x00\x00\x00\x48\x8B\x00\x4C\x8B", "x?xx?xxx?xxxxx????xx???xx???xx?xx");
	MH_CreateHook((void*)Unkfunc2Address, &UnkFunc2_hooked, (LPVOID*)&pfUnkFunc2);
	MH_EnableHook((void*)Unkfunc2Address);

	while (!GetAsyncKeyState(VK_DELETE))
	{
		auto local_cse = memory_utils::read_value<DWORD_PTR>({ pClientSoldierEntityLocal });

		mtx.lock();
		if (ClientSoldierEntityStructList.size())
		{
			std::cout << "-----START-----\n\n";
			static const auto var_name = typeid(ClientSoldierEntityStructList).name();
			std::cout << var_name << " .size: " << ClientSoldierEntityStructList.size() << "\n\n";
			int i = 0;
			for (auto it = ClientSoldierEntityStructList.begin(); it < ClientSoldierEntityStructList.end(); it++)
			{
				auto cse = *it;

				if (cse == local_cse)
					goto erase;

				if (memory_utils::is_valid_ptr((void*)cse))
				{
					auto health_component = memory_utils::read_value<DWORD_PTR>({ cse, 0x2E8 });
					if (health_component != NULL)
					{
						auto health = memory_utils::read_value<float>({ health_component, 0x20 });
						auto origin = memory_utils::read_value<vector3>({ cse, 0xC90 });
						if (health > 0.1f && health <= 150.f)
						{
							i++;
							printf("cse: 0x%I64X, health: %.1f, x: %.3f, y: %.3f, z: %.3f\n", cse, health, origin.x, origin.y, origin.z);
						}
					}
				}
				else
				{
					erase:
					ClientSoldierEntityStructList.erase(it);
				}
			}
			std::cout << "\nalive players: " << i << std::endl;
			std::cout << "\n-----END-----\n\n";
		}
		mtx.unlock();
		Sleep(1000);
	}

	MH_DisableHook(MH_ALL_HOOKS);
	MH_RemoveHook(MH_ALL_HOOKS);;
	Sleep(100);

	MH_Uninitialize();
	Sleep(100);

	FreeLibraryAndExitThread((HMODULE)arg, 0);
}

BOOL APIENTRY DllMain( HMODULE hModule,
					   DWORD  ul_reason_for_call,
					   LPVOID lpReserved
					 )
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH)
	{
		CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)thread, hModule, NULL, NULL);
	}

	return TRUE;
}
