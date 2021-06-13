#include "../includes.h"

namespace memory_utils
{
	static HMODULE base;

	HMODULE get_base()
	{
		if (!base)
			base = GetModuleHandle(NULL);
		return base;
	}

	DWORD_OF_BITNESS get_base_address()
	{
		return (DWORD_OF_BITNESS)get_base();
	}

	bool is_valid_ptr(PVOID ptr)
	{
		return (ptr >= (PVOID)0x10000) && (ptr < PTRMAXVAL) && ptr != nullptr && !IsBadReadPtr(ptr, sizeof(ptr));
	}

	void write_string(std::vector<DWORD_OF_BITNESS>address, char* value)
	{
		size_t length_array = address.size() - 1;
		DWORD_OF_BITNESS relative_address = address[0];
		for (int i = 1; i < length_array + 1; i++)
		{
			if (is_valid_ptr((LPVOID)relative_address) == false)
				return;

			if (i < length_array)
				relative_address = *(DWORD_OF_BITNESS*)(relative_address + address[i]);
			else
			{
				char* writable_address = (char*)(relative_address + address[length_array]);
				*writable_address = *value;
			}
		}
	}

	char* read_string(std::vector<DWORD_OF_BITNESS>address)
	{
		size_t length_array = address.size() - 1;
		DWORD_OF_BITNESS relative_address = address[0];
		for (int i = 1; i < length_array + 1; i++)
		{
			if (is_valid_ptr((LPVOID)relative_address) == false)
				return NULL;

			if (i < length_array)
				relative_address = *(DWORD_OF_BITNESS*)(relative_address + address[i]);
			else
			{
				char* readable_address = (char*)(relative_address + address[length_array]);
				return readable_address;
			}
		}
	}

	DWORD_OF_BITNESS get_module_size(DWORD_OF_BITNESS address)
	{
		return PIMAGE_NT_HEADERS(address + (DWORD_OF_BITNESS)PIMAGE_DOS_HEADER(address)->e_lfanew)->OptionalHeader.SizeOfImage;
	}

	/*DWORD_OF_BITNESS compare_mem(const char* pattern, const char* mask, DWORD_OF_BITNESS base, DWORD_OF_BITNESS size, const int patternLength, DWORD speed)
	{
		for (DWORD_OF_BITNESS i = 0; i < size - patternLength; i += speed)
		{
			bool found = true;
			for (DWORD_OF_BITNESS j = 0; j < patternLength; j++)
			{
				found &= pattern[j] == *(char*)(base + i + j);
			}

			if (found)
			{
				return base + i;
			}
		}

		return NULL;
	}*/

	DWORD_OF_BITNESS compare_mem(const char* pattern, const char* mask, DWORD_OF_BITNESS base, DWORD_OF_BITNESS size, const int patternLength, DWORD speed)
	{
		for (DWORD_OF_BITNESS i = 0; i < size - patternLength; i += speed)
		{
			bool found = true;
			for (DWORD_OF_BITNESS j = 0; j < patternLength; j++)
			{
				if (mask[j] == '?')
					continue;

				/*if (IsBadCodePtr((FARPROC)(base + i + j)) != NULL)
					continue;*/

				if (pattern[j] != *(char*)(base + i + j))
				{
					found = false;
					break;
				}
			}

			if (found)
			{
				return base + i;
			}
		}

		return NULL;
	}

	DWORD_OF_BITNESS find_pattern(HMODULE module, const char* pattern, const char* mask, DWORD scan_speed)
	{
		DWORD_OF_BITNESS base = (DWORD_OF_BITNESS)module;
		DWORD_OF_BITNESS size = get_module_size(base);

		DWORD_OF_BITNESS patternLength = (DWORD_OF_BITNESS)strlen(mask);

		return compare_mem(pattern, mask, base, size, patternLength, scan_speed);
	}

	DWORD_OF_BITNESS find_pattern_in_heap(const char* pattern, const char* mask, DWORD scan_speed)
	{
		SYSTEM_INFO sysInfo;
		GetSystemInfo(&sysInfo);
		DWORD_OF_BITNESS base = (DWORD_OF_BITNESS)GetProcessHeap();
		DWORD_OF_BITNESS size = (DWORD_OF_BITNESS)sysInfo.lpMaximumApplicationAddress;

		DWORD_OF_BITNESS base_start = base;
		DWORD_OF_BITNESS end_size = size;

		DWORD_OF_BITNESS patternLength = (DWORD_OF_BITNESS)strlen(mask);

		MEMORY_BASIC_INFORMATION mbi;

		while (base_start < end_size && VirtualQuery((LPCVOID)base_start, &mbi, sizeof(mbi)) != NULL)
		{
			DWORD_OF_BITNESS start = (DWORD_OF_BITNESS)mbi.BaseAddress;
			DWORD_OF_BITNESS end = mbi.RegionSize;

			bool is_accessible = (mbi.Protect == PAGE_READWRITE);
			bool is_mem_state_ok = (mbi.State == MEM_COMMIT);

			if (is_accessible && is_mem_state_ok)
			{
				auto compare_result = compare_mem(pattern, mask, start, end, patternLength, scan_speed);

				if (compare_result != NULL)
					return compare_result;
			}
			base_start += end;
		}

		return NULL;
	}

	std::vector<DWORD_OF_BITNESS> find_pattern_in_heap_array(const char* pattern, const char* mask, DWORD scan_speed)
	{
		std::vector<DWORD_OF_BITNESS>temp;

		SYSTEM_INFO sysInfo;
		GetSystemInfo(&sysInfo);
		DWORD_OF_BITNESS base = (DWORD_OF_BITNESS)GetProcessHeap();
		DWORD_OF_BITNESS size = (DWORD_OF_BITNESS)sysInfo.lpMaximumApplicationAddress;

		DWORD_OF_BITNESS base_start = base;
		DWORD_OF_BITNESS end_size = size;

		DWORD_OF_BITNESS patternLength = (DWORD_OF_BITNESS)strlen(mask);

		MEMORY_BASIC_INFORMATION mbi;

		while (base_start < end_size && VirtualQuery((LPCVOID)base_start, &mbi, sizeof(mbi)) != NULL)
		{
			DWORD_OF_BITNESS start = (DWORD_OF_BITNESS)mbi.BaseAddress;
			DWORD_OF_BITNESS end = mbi.RegionSize;

			bool is_accessible = (mbi.Protect == PAGE_READWRITE);
			bool is_mem_state_ok = (mbi.State == MEM_COMMIT);

			if (is_accessible && is_mem_state_ok)
			{
				auto compare_result = compare_mem(pattern, mask, start, end, patternLength, scan_speed);

				if (compare_result != NULL)
					temp.push_back(compare_result);
			}
			base_start += end;
		}

		return temp;
	}

	DWORD_OF_BITNESS find_pattern_in_memory(const char* pattern, const char* mask, DWORD_OF_BITNESS base, DWORD_OF_BITNESS size, DWORD scan_speed)
	{
		DWORD_OF_BITNESS base_start = base;
		DWORD_OF_BITNESS end_size = size;

		DWORD_OF_BITNESS patternLength = (DWORD_OF_BITNESS)strlen(mask);

		MEMORY_BASIC_INFORMATION mbi;

		while (base_start < end_size && VirtualQuery((LPCVOID)base_start, &mbi, sizeof(mbi)) != NULL)
		{
			DWORD_OF_BITNESS start = (DWORD_OF_BITNESS)mbi.BaseAddress;
			DWORD_OF_BITNESS end = mbi.RegionSize;

			bool is_accessible = (mbi.Protect == PAGE_READWRITE);
			bool is_mem_state_ok = (mbi.State == MEM_COMMIT);

			if (is_accessible && is_mem_state_ok)
			{
				auto compare_result = compare_mem(pattern, mask, start, end, patternLength, scan_speed);

				if (compare_result != NULL)
					return compare_result;
			}
			base_start += end;
		}

		return NULL;
	}

	std::vector<DWORD_OF_BITNESS> find_pattern_in_memory_array(const char* pattern, const char* mask, DWORD_OF_BITNESS base, DWORD_OF_BITNESS size, DWORD scan_speed)
	{
		std::vector<DWORD_OF_BITNESS>temp;

		DWORD_OF_BITNESS base_start = base;
		DWORD_OF_BITNESS end_size = size;

		DWORD_OF_BITNESS patternLength = (DWORD_OF_BITNESS)strlen(mask);

		MEMORY_BASIC_INFORMATION mbi;

		while (base_start < end_size && VirtualQuery((LPCVOID)base_start, &mbi, sizeof(mbi)) != NULL)
		{
			DWORD_OF_BITNESS start = (DWORD_OF_BITNESS)mbi.BaseAddress;
			DWORD_OF_BITNESS end = mbi.RegionSize;

			bool is_accessible = (mbi.Protect == PAGE_READWRITE);
			bool is_mem_state_ok = (mbi.State == MEM_COMMIT);

			if (is_accessible && is_mem_state_ok)
			{
				auto compare_result = compare_mem(pattern, mask, start, end, patternLength, scan_speed);

				if (compare_result != NULL)
					temp.push_back(compare_result);
			}
			base_start += end;
		}

		return temp;
	}

	void patch_instruction(DWORD_OF_BITNESS instruction_address, const char* instruction_bytes, int sizeof_instruction_byte)
	{
		DWORD dwOldProtection = NULL;

		VirtualProtect((LPVOID)instruction_address, sizeof_instruction_byte, PAGE_EXECUTE_READWRITE, &dwOldProtection);

		memcpy((LPVOID)instruction_address, instruction_bytes, sizeof_instruction_byte);

		VirtualProtect((LPVOID)instruction_address, sizeof_instruction_byte, dwOldProtection, NULL);

		FlushInstructionCache(GetCurrentProcess(), (LPVOID)instruction_address, sizeof_instruction_byte);
	}

	void fill_memory_region(DWORD_OF_BITNESS instruction_address, int byte, int sizeof_instruction_byte)
	{
		DWORD dwOldProtection = NULL;

		VirtualProtect((LPVOID)instruction_address, sizeof_instruction_byte, PAGE_EXECUTE_READWRITE, &dwOldProtection);

		memset((LPVOID)instruction_address, byte, sizeof_instruction_byte);

		VirtualProtect((LPVOID)instruction_address, sizeof_instruction_byte, dwOldProtection, NULL);

		FlushInstructionCache(GetCurrentProcess(), (LPVOID)instruction_address, sizeof_instruction_byte);
	}
}

