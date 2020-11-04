#pragma once

#include <string>

class StringHelper
{
public:
	static BOOL ConvertWide2Multibyte(WCHAR* source, char** destination)
	{
		UINT32 len = WideCharToMultiByte(CP_ACP, 0, source, (INT32)wcslen(source), NULL, NULL, NULL, NULL);
		(*destination) = new char[NULL, len + 1];
		::ZeroMemory((*destination), (len + 1) * sizeof(char));
		WideCharToMultiByte(CP_ACP, 0, source, -1, (*destination), len, NULL, NULL);
		return TRUE;
	}

	static BOOL ConvertMultibyte2Wide(char* source, WCHAR** destination)
	{
		UINT32 len = MultiByteToWideChar(CP_ACP, 0, source, (INT32)strlen(source), NULL, NULL);
		(*destination) = SysAllocStringLen(NULL, len + 1);
		::ZeroMemory((*destination), (len + 1) * sizeof(WCHAR));
		MultiByteToWideChar(CP_ACP, 0, source, -1, (*destination), len);

		return TRUE;
	}

	static std::string Trim(std::string& s, const std::string& drop = " \t\n\v")
	{
		std::string r = s.erase(s.find_last_not_of(drop) + 1);
		return r.erase(0, r.find_first_not_of(drop));
	}
	static std::string RTrim(std::string s, const std::string& drop = " \t\n\v")
	{
		return s.erase(s.find_last_not_of(drop) + 1);
	}

	static std::string LTrim(std::string s, const std::string& drop = " \t\n\v")
	{
		return s.erase(0, s.find_first_not_of(drop));
	}

};
