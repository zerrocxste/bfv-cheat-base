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

static std::deque<DWORD_PTR>vectored_shit;

std::mutex mtx;

using fUnkFunc = float(__fastcall*)(__int64);
fUnkFunc pfUnkFunc = NULL;

float __fastcall UnkFunc_hooked(__int64 a1, __int64 a2)
{
	DWORD_PTR client_soldier_entity = (DWORD_PTR)a1 - 0x338;
	//cse + 0xC90 = coords

	auto res = std::find(vectored_shit.begin(), vectored_shit.end(), client_soldier_entity) != vectored_shit.end();

	if (!res)
	{
		mtx.lock();
		vectored_shit.push_back(client_soldier_entity);
		mtx.unlock();
	}

	return pfUnkFunc(a1);
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

	/*
		Address of signature = bfv.exe + 0x020C7C90
		"\x48\x89\x00\x00\x00\x57\x48\x83\xEC\x00\x48\x8B\x00\x00\x48\x8B\x00\x48\x8B\x00\x48\x8B\x00\xFF\x90", "xx???xxxx?xx??xx?xx?xx?xx"
		"48 89 ? ? ? 57 48 83 EC ? 48 8B ? ? 48 8B ? 48 8B ? 48 8B ? FF 90"
	*/

	MH_Initialize();

	auto addr = 0x1420C7C90;

	MH_CreateHook((void*)addr, &UnkFunc_hooked, (LPVOID*)&pfUnkFunc);
	MH_EnableHook((void*)addr);

	while (!GetAsyncKeyState(VK_DELETE))
	{
		mtx.lock();
		if (vectored_shit.size())
		{
			std::cout << "-----START-----\n\n";
			static const auto var_name = typeid(vectored_shit).name();
			std::cout << var_name << " .size: " << vectored_shit.size() << "\n\n";
			int i = 0;
			for (auto it = vectored_shit.begin(); it < vectored_shit.end(); it++)
			{
				if (memory_utils::is_valid_ptr((void*)*it))
				{
					auto health_component = memory_utils::read_value<DWORD_PTR>({ *it, 0x2E8 });
					if (health_component != NULL)
					{
						auto health = memory_utils::read_value<float>({ health_component, 0x20 });
						auto origin = memory_utils::read_value<vector3>({ *it, 0xC90 });
						if (health > 0.1f && health <= 150.f)
						{
							i++;
							printf("cse: 0x%I64X, health: %.1f, x: %.3f, y: %.3f, z: %.3f\n", *it, health, origin.x, origin.y, origin.z);
						}
					}
				}
				else
				{
					vectored_shit.erase(it);
				}
			}
			std::cout << "\nalive players: " << i << std::endl;
			std::cout << "\n-----END-----\n\n";
		}
		mtx.unlock();
		Sleep(1000);
	}

	MH_DisableHook((void*)addr);
	MH_RemoveHook((void*)addr);
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
