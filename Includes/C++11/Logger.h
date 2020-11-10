#ifndef LOGGER_H
#define LOGGER_H
#include <LWCore/LWTypes.h>

#define LOG_EVENT 0
#define LOG_WARN 1
#define LOG_CRITICAL 2

struct DataAggregate {
	static const uint32_t MaxValues = 120;
	uint64_t m_Values[MaxValues];
	uint32_t m_ValueCount = 0;

	void PushValue(uint64_t Value);

	void GetValues(uint64_t &Lowest, uint64_t &Highest, uint64_t &Average) const;

	uint32_t AppendValues(char *Buffer, uint32_t BufferLen, const char *Prepended) const;
};

void SetLogLevel(uint32_t Level);

void LogEvent(const LWUTF8Iterator &Text);

void LogWarn(const LWUTF8Iterator &Text);

void LogCritical(const LWUTF8Iterator &Text);

#endif