#include "pch.h"
#include "Utils.h"

template<size_t size>
std::tstring StringFormatter::format_v(LPCTSTR fmt, va_list args) const
{
	TCHAR str[size];
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

std::tstring StringFormatter::format(HRESULT hr, ...) const
{
	va_list args;
	va_start(args, hr);

	std::tstring msg;
	DWORD dwFlags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM;
	DWORD dwLanguageId = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);
	LPTSTR lpBuffer;
	auto size = FormatMessage(dwFlags, nullptr, hr, dwLanguageId, (LPTSTR)&lpBuffer, 100, &args);
	va_end(args);
	if(size) {
		msg.assign(lpBuffer, size);
		LocalFree(lpBuffer);
	} else {
		msg = format(_T("Error code: 0x%p"), hr);
	}
	return msg;
}

std::tstring StringFormatter::toString(REFGUID guid) const
{
	LPOLESTR olestr;
	auto hr = StringFromCLSID(guid, &olestr);
	if(SUCCEEDED(hr)) {
		CW2T str(olestr);
		CoTaskMemFree(olestr);
		return std::tstring((LPCTSTR)str);
	} else {
		return format(_T("StringFromCLSID() failed. HRESULT=0x%p"), hr);
	}
}

void Logger::log(LPCTSTR fmt, ...) const
{
	va_list args;
	va_start(args, fmt);
	m_writer(format_v(fmt, args).c_str());
	va_end(args);
}
