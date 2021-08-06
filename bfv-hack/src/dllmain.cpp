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

std::mutex mtx;
std::deque<DWORD_PTR>v_cse_list;

using f_unkfunc = void(__fastcall*)(__int64);
f_unkfunc pf_unkfunc = NULL;

void __fastcall unkfunc_hooked(__int64 a1)
{
	DWORD_PTR client_soldier_entity = (DWORD_PTR)a1 - 0x338;
	//cse + 0xC90 = coords

	if (!(std::find(v_cse_list.begin(), v_cse_list.end(), client_soldier_entity) != v_cse_list.end()))
	{
		mtx.lock();
		v_cse_list.push_back(client_soldier_entity);
		mtx.unlock();
	}

	pf_unkfunc(a1);
}

class vector3 {
public:
	float x, y, z;
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

	auto func_address = 0x1420C7C90;

	MH_CreateHook((void*)func_address, unkfunc_hooked, (LPVOID*)&pf_unkfunc);
	MH_EnableHook((void*)func_address);

	while (!GetAsyncKeyState(VK_DELETE))
	{
		mtx.lock();
		if (v_cse_list.size())
		{
			std::cout << "-----START-----\n\n";
			static const auto var_name = typeid(v_cse_list).name();
			std::cout << var_name << " .size: " << v_cse_list.size() << "\n\n";
			int i = 0;
			for (auto it = v_cse_list.begin(); it < v_cse_list.end(); it++)
			{
				auto cse = *it;
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
							printf("cse: 0x%p, health: %.1f, x: %.3f, y: %.3f, z: %.3f\n", cse, health, origin.x, origin.y, origin.z);
						}
					}
				}
				else
				{
					v_cse_list.erase(it);
				}
			}
			std::cout << "\nalive players: " << i << std::endl;
			std::cout << "\n-----END-----\n\n";
		}
		mtx.unlock();
		Sleep(1000);
	}

	MH_DisableHook((void*)func_address);
	MH_RemoveHook((void*)func_address);
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
		CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)thread, hModule, NULL, NULL);

	return TRUE;
}
