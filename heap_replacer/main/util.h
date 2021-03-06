#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdio.h>

#pragma warning(disable:6031)
#pragma warning(disable:6250)

// ONE OF THESE NEEDS TO BE DEFINED!
#if defined(FNV)
	#define HR_NAME "NVHR"
	#define HR_VERSION "1.4.0.525"
	#define HR_GAME_QPC_HOOK (void*)0x00FDF0A0
	#define HR_GECK_QPC_HOOK (void*)0x00D2320C
#elif defined(FO3)
	#define HR_NAME "F3HR"
	#define HR_VERSION "1.7.0.3"
	#define HR_GAME_QPC_HOOK (void*)0x00D9B0E4
	#define HR_GECK_QPC_HOOK (void*)0x00D03208
#endif

#define HR_MSGBOX(msg) MessageBox(NULL, HR_NAME " - " msg, "Error", NULL)
#define HR_PRINTF(msg) printf(HR_NAME " - " msg "\n")

#define TFPARAM(self, ...) self, void* _, __VA_ARGS__
#define TFCALL(self, ...) self, nullptr, __VA_ARGS__

#define ECS(v) while (InterlockedCompareExchange(v, 1, NULL));
#define LCS(v) *v = NULL;

#define BITWISE_NULLIFIER(c, v) ((DWORD)(v) & ((!(c)) - 1))
#define BITWISE_IF_ELSE(c, t, f) ((BITWISE_NULLIFIER((c), (t))) + (BITWISE_NULLIFIER((!(c)), (f))))

#define UPTRSUM(x, y) ((uintptr_t)x + (uintptr_t)y)
#define UPTRDIFF(x, y) ((uintptr_t)x - (uintptr_t)y)

#define VPTRSUM(x, y) (void*)UPTRSUM(x, y)
#define VPTRDIFF(x, y) (void*)UPTRDIFF(x, y)

#define NOINLINE __declspec(noinline)

constexpr size_t KB = 1024u * 1u;
constexpr size_t MB = 1024u * KB;
constexpr size_t GB = 1024u * MB;

// FILE* file = fopen("log.log", "w");

namespace nvhr
{

	void* __fastcall nvhr_malloc(size_t size);
	void* __fastcall nvhr_calloc(size_t count, size_t size);
	void* __fastcall nvhr_realloc(void* address, size_t size);
	void* __fastcall nvhr_recalloc(void* address, size_t count, size_t size);
	size_t __fastcall nvhr_mem_size(void* address);
	void __fastcall nvhr_free(void* address);

}

void* __cdecl operator new(size_t size)
{
	return nvhr::nvhr_malloc(size);
}

void* __cdecl operator new[](size_t size)
{
	return nvhr::nvhr_malloc(size);
}

void __cdecl operator delete(void* address)
{
	nvhr::nvhr_free(address);
}

void __cdecl operator delete[](void* address)
{
	nvhr::nvhr_free(address);
}

namespace util
{

	template <typename O, typename I>
	O force_cast(I in)
	{
		union { I in; O out; } u = { in };
		return u.out;
	};

	size_t next_power_of_2(size_t size)
	{
		if (size & (size - 1))
		{
			size--;
			size |= size >> 1;
			size |= size >> 2;
			size |= size >> 4;
			size |= size >> 8;
			size |= size >> 16;
			size++;
		}
		return size;
	}

	const size_t bit_pos[32] =
	{
		0,	9,	1,	10,	13,	21,	2,	29,
		11,	14,	16,	18,	22,	25,	3,	30,
		8,	12,	20,	28,	15,	17,	24,	07,
		19,	27,	23,	6,	26,	5,	4,	31,
	};

	size_t get_first_bit_from_power_of_2(DWORD value)
	{
		return bit_pos[(DWORD)((value - 1) * 0x07C4ACDDU) >> 27];
	}

	namespace mem
	{

		void* winapi_alloc(size_t size)
		{
			return VirtualAlloc(nullptr, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
		}

		void* winapi_malloc(size_t size)
		{
			return winapi_alloc(size);
		}

		void* winapi_calloc(size_t count, size_t size)
		{
			return winapi_alloc(count * size);
		}

		void winapi_free(void* address)
		{
			VirtualFree(address, NULL, MEM_RELEASE);
		}

		void memset8(void* destination, BYTE value, size_t count)
		{
			BYTE* position = (BYTE*)destination;
			for (size_t i = 0; i < count; i++) { *position++ = value; }
		}

		void memset16(void* destination, WORD value, size_t count)
		{
			WORD* position = (WORD*)destination;
			for (size_t i = 0; i < count; i++) { *position++ = value; }
		}

		void memset32(void* destination, DWORD value, size_t count)
		{
			DWORD* position = (DWORD*)destination;
			for (size_t i = 0; i < count; i++) { *position++ = value; }
		}

		void patch_bytes(uintptr_t address, BYTE* data, DWORD size)
		{
			DWORD p;
			VirtualProtect((void*)address, size, PAGE_EXECUTE_READWRITE, &p);
			memcpy((void*)address, data, size);
			VirtualProtect((void*)address, size, p, &p);
			FlushInstructionCache(GetCurrentProcess(), (void*)address, size);
		}

		void patch_BYTE(uintptr_t address, BYTE data)
		{
			patch_bytes(address, &data, 1);
		}

		void patch_WORD(uintptr_t address, WORD data)
		{
			BYTE bytes[2];
			*(WORD*)bytes = data;
			patch_bytes(address, bytes, 2);
		}

		void patch_DWORD(uintptr_t address, DWORD data)
		{
			BYTE bytes[4];
			*(DWORD*)bytes = data;
			patch_bytes(address, bytes, 4);
		}

		void patch_call(uintptr_t address, void* destination)
		{
			BYTE bytes[5];
			bytes[0] = 0xE8;
			*(DWORD*)((DWORD)bytes + 1) = (DWORD)destination - (DWORD)address - 5;
			patch_bytes(address, bytes, 5);
		}

		void patch_jmp(uintptr_t address, void* destination)
		{
			BYTE bytes[5];
			bytes[0] = 0xE9;
			*(DWORD*)((DWORD)bytes + 1) = (DWORD)destination - (DWORD)address - 5;
			patch_bytes(address, bytes, 5);
		}

		void patch_ret(uintptr_t address)
		{
			BYTE bytes = 0xC3;
			patch_bytes(address, &bytes, 1);
		}

		void patch_ret_nullptr(uintptr_t address)
		{
			patch_bytes(address, (BYTE*)"\x31\xC0", 2);
			patch_ret(address + 2);
		}

		void patch_ret_nullptr(uintptr_t address, size_t argc)
		{
			patch_bytes(address, (BYTE*)"\x83\xC4", 2);
			patch_BYTE(address + 2, (BYTE)(argc * 4));
			patch_ret_nullptr(address + 3);
		}

		void patch_ret(uintptr_t address, size_t argc)
		{
			BYTE bytes = 0xC2;
			patch_bytes(address, &bytes, 1);
			patch_WORD(address + 1, (WORD)(4 * argc));
		}

		void patch_bp(uintptr_t address)
		{
			BYTE bytes = 0xCC;
			patch_bytes(address, &bytes, 1);
		}

		void patch_nops(uintptr_t address, size_t count)
		{
			DWORD p;
			VirtualProtect((void*)address, count, PAGE_EXECUTE_READWRITE, &p);
			memset8((void*)address, 0x90, count);
			VirtualProtect((void*)address, count, p, &p);
			FlushInstructionCache(GetCurrentProcess(), (void*)address, count);
		}

		void patch_nop_call(uintptr_t address)
		{
			patch_nops(address, 5);
		}

	}

	size_t align(size_t size, size_t alignment)
	{
		return BITWISE_IF_ELSE(size & (alignment - 1), (size + (alignment - 1)) & ~(alignment - 1), size);
	}

	void* align(void* address, size_t alignment)
	{
		return (void*)align((uintptr_t)address, alignment);
	}

	void* get_IAT_address(BYTE* base, const char* dll_name, const char* search)
	{
		IMAGE_DOS_HEADER* dos_header = (IMAGE_DOS_HEADER*)base;
		IMAGE_NT_HEADERS* nt_headers = (IMAGE_NT_HEADERS*)(base + dos_header->e_lfanew);
		IMAGE_DATA_DIRECTORY section = nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
		IMAGE_IMPORT_DESCRIPTOR* import_descriptor = (IMAGE_IMPORT_DESCRIPTOR*)(base + section.VirtualAddress);
		for (size_t i = 0; import_descriptor[i].Name != NULL; i++)
		{
			if (!_stricmp((char*)(base + import_descriptor[i].Name), dll_name))
			{
				if (!import_descriptor[i].FirstThunk) { return nullptr; }
				IMAGE_THUNK_DATA* name_table = (IMAGE_THUNK_DATA*)(base + import_descriptor[i].OriginalFirstThunk);
				IMAGE_THUNK_DATA* import_table = (IMAGE_THUNK_DATA*)(base + import_descriptor[i].FirstThunk);
				for (; name_table->u1.Ordinal != NULL; ++name_table, ++import_table)
				{
					if (!IMAGE_SNAP_BY_ORDINAL(name_table->u1.Ordinal))
					{
						IMAGE_IMPORT_BY_NAME* import_name = (IMAGE_IMPORT_BY_NAME*)(base + name_table->u1.ForwarderString);
						char* func_name = &import_name->Name[0];
						if (!_stricmp(func_name, search)) { return &import_table->u1.AddressOfData; }
					}
				}
			}
		}
		return nullptr;
	}

	bool is_LAA(BYTE* base)
	{
		IMAGE_DOS_HEADER* dos_header = (IMAGE_DOS_HEADER*)base;
		IMAGE_NT_HEADERS* nt_headers = (IMAGE_NT_HEADERS*)(base + dos_header->e_lfanew);
		return nt_headers->FileHeader.Characteristics & IMAGE_FILE_LARGE_ADDRESS_AWARE;
	}

	bool file_exists(const char* name)
	{
		FILE* file;
		if (file = fopen(name, "r"))
		{
			fclose(file);
			return true;
		}
		return false;
	}

	void create_console()
	{
		AllocConsole();
		freopen("CONIN$", "r", stdin);
		freopen("CONOUT$", "w", stderr);
		freopen("CONOUT$", "w", stdout);
	}

}
