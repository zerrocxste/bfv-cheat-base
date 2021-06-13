#ifdef _WIN64
#define DWORD_OF_BITNESS DWORD64
#define PTRMAXVAL ((PVOID)0x000F000000000000)
#elif _WIN32
#define DWORD_OF_BITNESS DWORD
#define PTRMAXVAL ((PVOID)0xFFF00000)
#endif

class CShellCodeHelper
{
private:
	BYTE* m_saved_original_bytes;
	BYTE* m_jmp_bytes;
	void* m_start_address;
	void* m_shellcode_allocated_memory;
	int m_replace_byte_instruction_length;
	bool status_ok;

	bool patch_instruction(void* instruction_address, void* instruction_bytes, int sizeof_instruction_byte)
	{
		DWORD old_proctection = NULL;

		if (VirtualProtect(instruction_address, sizeof_instruction_byte, PAGE_EXECUTE_READWRITE, &old_proctection))
		{
			memcpy(instruction_address, instruction_bytes, sizeof_instruction_byte);

			VirtualProtect(instruction_address, sizeof_instruction_byte, old_proctection, NULL);

			FlushInstructionCache(GetCurrentProcess(), instruction_address, sizeof_instruction_byte);

			return true;
		}
		return false;
	}
public:
	bool setup(void* address, void* my_heap = NULL)
	{
		if (address == NULL)
			return false;

		m_start_address = address;

		if (my_heap != NULL)
		{
			m_shellcode_allocated_memory = my_heap;
		}
		else
		{
			DWORD_OF_BITNESS search_offset = 0x10000000;
			DWORD_OF_BITNESS allocation_address = (DWORD_OF_BITNESS)address - search_offset;
			DWORD_OF_BITNESS end = (DWORD_OF_BITNESS)address;

			if ((DWORD_OF_BITNESS)address < search_offset)
			{
				search_offset = (DWORD_OF_BITNESS)address;
				allocation_address = (DWORD_OF_BITNESS)address - search_offset;
			}

			MEMORY_BASIC_INFORMATION mbi{};
			ZeroMemory(&mbi, sizeof(MEMORY_BASIC_INFORMATION));
		retry:
			while (allocation_address < end && VirtualQuery((void*)allocation_address, &mbi, sizeof(mbi)))
			{
				if (mbi.State == MEM_FREE)
				{
#ifdef _WIN64
					printf("[+]%s. Found free memory region: 0x%I64X\n", __FUNCTION__, mbi.BaseAddress);
#else
					printf("[+]%s. Found free memory region: 0x%I32X\n", __FUNCTION__, mbi.BaseAddress);
#endif // _WIN64
					allocation_address = (DWORD64)mbi.BaseAddress;
					break;
				}
				else
				{
#ifdef _WIN64
					printf("[-]%s. Non avalible region: 0x%I64X\n", __FUNCTION__, mbi.BaseAddress);
#else
					printf("[-]%s. Non avalible region: 0x%I32X\n", __FUNCTION__, mbi.BaseAddress);
#endif // _WIN64
					allocation_address += mbi.RegionSize;
				}
			}

			m_shellcode_allocated_memory = VirtualAlloc((void*)allocation_address, 1000, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);

			if (m_shellcode_allocated_memory == NULL)
			{
				if (allocation_address < end)
				{
					printf("[+]%s. Failed allocate memory, retry...\n", __FUNCTION__);
					allocation_address += mbi.RegionSize;
					goto retry;
				}
				printf("[+]%s. Failed allocate memory\n", __FUNCTION__);
				return false;
			}
		}

#ifdef _WIN64
		printf("[+]%s. Shellcode allocated page: 0x%I64X\n", __FUNCTION__, (DWORD_OF_BITNESS)m_shellcode_allocated_memory);
#else
		printf("[+]%s. Shellcode allocated page: 0x%I32X\n", __FUNCTION__, (DWORD_OF_BITNESS)m_shellcode_allocated_memory);
#endif // _WIN64

		status_ok = true;
		return true;
	}

	bool patch(void* shellcode, int sizeof_shellcode, int replace_byte_instruction_length)
	{
		if (shellcode == NULL || replace_byte_instruction_length == NULL || !status_ok)
		{
			printf("[+]%s. U are autistic\n", __FUNCTION__);
			return false;
		}

		m_replace_byte_instruction_length = replace_byte_instruction_length;

		memcpy(m_shellcode_allocated_memory, shellcode, sizeof_shellcode);

		memset((void*)((DWORD_OF_BITNESS)m_shellcode_allocated_memory + sizeof_shellcode), 0xe9, 1);
		DWORD relative_addr_jmp_back = (((DWORD)m_start_address - (DWORD)m_shellcode_allocated_memory) - (DWORD)(sizeof_shellcode - 1));
		memcpy((void*)((DWORD_OF_BITNESS)m_shellcode_allocated_memory + sizeof_shellcode + 1), &relative_addr_jmp_back, 4);

		m_jmp_bytes = new BYTE[replace_byte_instruction_length];
		m_saved_original_bytes = new BYTE[replace_byte_instruction_length];

		memcpy(m_saved_original_bytes, m_start_address, replace_byte_instruction_length);
		if (replace_byte_instruction_length > 5)
		{
			memset(m_jmp_bytes + 0x5, 0x90, (int)(replace_byte_instruction_length - 5));
			memcpy(m_saved_original_bytes + 0x5, (void*)((DWORD_OF_BITNESS)m_start_address + 0x5), (int)(replace_byte_instruction_length - 5));
		}

		m_jmp_bytes[0] = 0xe9;
		DWORD relative_address = (((DWORD)m_shellcode_allocated_memory - (DWORD)m_start_address) - (DWORD)5);
		memcpy(m_jmp_bytes + 0x1, &relative_address, 4);

		return patch_instruction(m_start_address, m_jmp_bytes, replace_byte_instruction_length);
	}

	bool disable()
	{
		if (m_jmp_bytes == NULL || m_saved_original_bytes == NULL || !status_ok)
			return false;

		return patch_instruction(m_start_address, m_saved_original_bytes, m_replace_byte_instruction_length);
	}

	void cleanup()
	{
		m_saved_original_bytes = NULL;
		m_jmp_bytes = NULL;
		m_start_address = NULL;
	}

	void* get_allocated_memory_address()
	{
		return m_shellcode_allocated_memory;
	}
};
