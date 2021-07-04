#include "pch.h"
#include "Utils.h"

std::tstring StringFormatter::format_v(LPCTSTR fmt, va_list args) const
{
	TCHAR str[1000];
	_vstprintf_s(str, fmt, args);
	return str;
}

std::tstring StringFormatter::format(LPCTSTR fmt, ...) const
{
	va_list args;
	va_start(args, fmt);
	auto str = format_v(fmt, args);
	va_end(args);
	return str;
}

void Logger::log(LPCTSTR fmt, ...) const
{
	va_list args;
	va_start(args, fmt);
	m_writer(format_v(fmt, args).c_str());
	va_end(args);
}
