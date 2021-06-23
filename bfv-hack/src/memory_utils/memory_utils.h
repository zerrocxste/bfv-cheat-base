//memory utils
//author: zerrocxste

namespace memory_utils
{
#ifdef _WIN64
#define DWORD_OF_BITNESS DWORD64
#define PTRMAXVAL ((PVOID)0x000F000000000000)
#elif _WIN32
#define DWORD_OF_BITNESS DWORD
#define PTRMAXVAL ((PVOID)0xFFF00000)
#endif

	extern bool is_valid_ptr(PVOID ptr);

	extern HMODULE get_base();

	extern DWORD_OF_BITNESS get_base_address();

	extern DWORD_OF_BITNESS get_module_size(DWORD_OF_BITNESS address);

	extern DWORD_OF_BITNESS find_pattern(HMODULE module, const char* pattern, const char* mask, DWORD scan_speed = 0x1);

	extern DWORD_OF_BITNESS find_pattern_in_heap(const char* pattern, const char* mask, DWORD scan_speed = 0x1);

	extern DWORD_OF_BITNESS find_pattern_in_memory(const char* pattern, const char* mask, DWORD_OF_BITNESS base, DWORD_OF_BITNESS size, DWORD scan_speed = 0x1);

	extern std::vector<DWORD_OF_BITNESS> find_pattern_in_memory_array(const char* pattern, const char* mask, DWORD_OF_BITNESS base, DWORD_OF_BITNESS size, DWORD scan_speed = 0x1);

	extern std::vector<DWORD_OF_BITNESS> find_pattern_in_heap_array(const char* pattern, const char* mask, DWORD scan_speed = 0x1);

	template<class T>
	T read_pointer(std::vector<DWORD_OF_BITNESS>address)
	{
		if (address.empty())
			return NULL;

		DWORD_OF_BITNESS relative_address = address[0];

		auto length_array = address.size() - 1;

		if (length_array == 0 && is_valid_ptr((LPVOID)relative_address))
			return (T)relative_address;

		for (int i = 1; i < length_array + 1; i++)
		{
			if (is_valid_ptr((LPVOID)relative_address) == false)
				break;

			if (i < length_array)
				relative_address = *(DWORD_OF_BITNESS*)(relative_address + address[i]);
			else
				return (T)(relative_address + address[length_array]);
		}

		return NULL;
	}

	template<class T>
	T read_value(std::vector<DWORD_OF_BITNESS>address)
	{
		T* ptr = read_pointer<T*>(address);

		if (ptr == NULL)
			return T();

		return *(T*)ptr;
	}

	template<class T>
	bool write(std::vector<DWORD_OF_BITNESS>address, T my_value)
	{
		T* ptr = read_pointer<T*>(address);

		if (ptr == NULL)
			return false;

		*ptr = my_value;

		return true;
	}

	extern char* read_string(std::vector<DWORD_OF_BITNESS> address);
	extern wchar_t* read_wstring(std::vector<DWORD_OF_BITNESS>address);

	extern bool write_string(std::vector<DWORD_OF_BITNESS> address, char* my_value);

	extern void patch_instruction(DWORD_OF_BITNESS instruction_address, const char* instruction_bytes, int sizeof_instruction_byte);
	extern void fill_memory_region(DWORD_OF_BITNESS instruction_address, int byte, int sizeof_instruction_byte);
}