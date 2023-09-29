#include "am/string/string.h"

#include "stringwrapper.h"
#include <cwchar>

bool String::IsNullOrEmpty(const char* string)
{
	return !string || string[0] == '\0';
}

bool String::IsNullOrEmpty(const wchar_t* string)
{
	return !string || string[0] == L'\0';
}

u32 String::Length(const char* string)
{
	return StringWrapper(string).Length();
}

u32 String::Length(const wchar_t* string)
{
	return StringWrapper(string).Length();
}

const char* String::FormatTemp(const char* fmt, ...)
{
	static thread_local char buffer[STRING_TEMP_BUFFER_SIZE];
	va_list args;
	va_start(args, fmt);
	FormatVA(buffer, STRING_TEMP_BUFFER_SIZE, fmt, args);
	va_end(args);
	return buffer;
}

const wchar_t* String::FormatTemp(const wchar_t* fmt, ...)
{
	static thread_local wchar_t buffer[STRING_TEMP_BUFFER_SIZE];
	va_list args;
	va_start(args, fmt);
	FormatVA(buffer, STRING_TEMP_BUFFER_SIZE, fmt, args);
	va_end(args);
	return buffer;
}

void String::WideToUtf8(char* destination, int destinationSize, const wchar_t* source)
{
	WideCharToMultiByte(CP_UTF8, 0, source, -1, destination, destinationSize, NULL, NULL);
}

void String::Utf8ToWide(wchar_t* destination, int destinationSize, const char* source)
{
	MultiByteToWideChar(CP_UTF8, 0, source, -1, destination, destinationSize);
}

void String::Copy(char* destination, int destinationSize, const char* source, int length)
{
	u32 copySize = GetCopySize(Length(source), length);
	memcpy_s(destination, destinationSize, source, copySize);
	destination[copySize - 1] = '\0';
}

void String::Copy(wchar_t* destination, int destinationSize, const wchar_t* source, int length)
{
	u32 copySize = GetCopySize(Length(source), length);
	wmemcpy_s(destination, destinationSize, source, copySize);
	destination[copySize - 1] = '\0';
}

void String::ToWide(wchar_t* destination, int destinationSize, const char* source)
{
	int i = 0;
	for (; source[i]; i++)
	{
		destination[i] = static_cast<wchar_t>(source[i]);
		AM_ASSERT(i < destinationSize, "String::ToWide() -> Buffer is too small!");
	}
	destination[i] = 0;
}

void String::ToAnsi(char* destination, int destinationSize, const wchar_t* source)
{
	// For unsupported characters
	constexpr char blankChar = ' ';

	int i = 0;
	for (; source[i]; i++)
	{
		wchar_t c = source[i];
		destination[i] = static_cast<char>(c > 255 ? blankChar : c);
		AM_ASSERT(i < destinationSize, "String::ToAnsi() -> Buffer is too small!");
	}
	destination[i] = 0;
}

const char* String::ToUtf8Temp(const wchar_t* source)
{
	static thread_local char buffer[STRING_TEMP_BUFFER_SIZE];
	WideToUtf8(buffer, STRING_TEMP_BUFFER_SIZE, source);
	return buffer;
}

const wchar_t* String::ToWideTemp(const char* source)
{
	static thread_local wchar_t buffer[STRING_TEMP_BUFFER_SIZE];
	ToWide(buffer, STRING_TEMP_BUFFER_SIZE, source);
	return buffer;
}

const char* String::ToAnsiTemp(const wchar_t* source)
{
	static thread_local char buffer[STRING_TEMP_BUFFER_SIZE];
	ToAnsi(buffer, STRING_TEMP_BUFFER_SIZE, source);
	return buffer;
}

bool String::Equals(ConstString left, ConstString right, bool ignoreCase)
{
	return StringWrapper(left).Equals(right, ignoreCase);
}

bool String::Equals(ConstWString left, ConstWString right, bool ignoreCase)
{
	return StringWrapper(left).Equals(right, ignoreCase);
}

void String::FormatVA(char* destination, int destinationSize, const char* fmt, va_list args)
{
	vsprintf_s(destination, destinationSize, fmt, args);
}

void String::FormatVA(wchar_t* destination, int destinationSize, const wchar_t* fmt, va_list args)
{
	vswprintf_s(destination, destinationSize, fmt, args);
}
