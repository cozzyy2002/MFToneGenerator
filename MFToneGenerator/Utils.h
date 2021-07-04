#pragma once

class StringFormatter
{
public:
	std::tstring format_v(LPCTSTR fmt, va_list args) const;
	std::tstring format(LPCTSTR fmt, ...) const;
};

class Logger : public StringFormatter
{
public:
	Logger::Logger() : m_writer(tsm::Assert::defaultAssertFailedWriter) {}

	void log(LPCTSTR fmt, ...) const;

protected:
	using Writer = void (*)(LPCTSTR);
	Writer m_writer;
};
