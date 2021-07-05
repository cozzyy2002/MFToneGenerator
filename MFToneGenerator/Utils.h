#pragma once

template<typename T>
struct ValueName {
    T value;
    LPCTSTR name;

    template<size_t size>
    static const ValueName* find(const ValueName(&list)[size], T& value) {
        for(auto& d : list) {
            if(d.value == value) { return &d; }
        }
        return nullptr;
    }
};

class StringFormatter
{
public:
	std::tstring format_v(LPCTSTR fmt, va_list args) const;
	std::tstring format(LPCTSTR fmt, ...) const;
    std::tstring format(HRESULT hr, ...) const;
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
