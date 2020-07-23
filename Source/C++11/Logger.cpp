#include "Logger.h"
#include <LWCore/LWTimer.h>
#include <iostream>
#include <chrono>
#include <algorithm>
#include <ctime>
#include <cstdarg>


void DataAggregate::PushValue(uint64_t Value) {
	m_Values[m_ValueCount%MaxValues] = Value;
	m_ValueCount++;
	return;
}

void DataAggregate::GetValues(uint64_t &Lowest, uint64_t &Highest, uint64_t &Average) const {
	if (!m_ValueCount) {
		Lowest = Highest = Average = 0;
		return;
	}
	uint32_t ValueCnt = std::min<uint32_t>(m_ValueCount, MaxValues);
	Average = m_Values[0];
	Lowest = Highest =  LWTimer::ToMilliSecond(m_Values[0]);
	for (uint32_t i = 1; i < ValueCnt; i++) {
		uint64_t Val = LWTimer::ToMilliSecond(m_Values[i]);
		Average += m_Values[i];
		Lowest = std::min<uint64_t>(Lowest, Val);
		Highest = std::max<uint64_t>(Highest, Val);
	}
	Average = LWTimer::ToMilliSecond(Average / ValueCnt);
	return;
}

uint32_t DataAggregate::AppendValues(char *Buffer, uint32_t BufferLen, const char *Prepended) const {
	uint64_t Lowest, Highest, Average;
	GetValues(Lowest, Highest, Average);
	return snprintf(Buffer, BufferLen, "%s %llums %llums %llums\n", Prepended, Lowest, Highest, Average);
}

uint32_t CurrentLogLevel = LOG_CRITICAL;
void SetLogLevel(uint32_t Level) {
	CurrentLogLevel = Level;
	return;
}

void Log(const char *Text, uint32_t LogLevel) {
	if (CurrentLogLevel < LogLevel) return;
	char Buffer[256];
	const char *Log[] = { "EVENT:", "WARN:", "CRIT:" };
	LogLevel = std::min<uint32_t>(LogLevel, 2);
	std::chrono::system_clock::time_point tp = std::chrono::system_clock::now();
	time_t Time = std::chrono::system_clock::to_time_t(tp);
	tm t;
	localtime_s(&t, &Time);
	snprintf(Buffer, sizeof(Buffer), "[%d/%d/%d %d:%d:%d]%s %s", t.tm_mon + 1, t.tm_mday, (t.tm_year + 1900), t.tm_hour, t.tm_min, t.tm_sec, Log[LogLevel], Text);

	std::cout << Buffer << std::endl;
	return;
}

void LogEvent(const char *Text) {
	return Log(Text, LOG_EVENT);
}

void LogWarn(const char *Text) {
	return Log(Text, LOG_WARN);
}

void LogCritical(const char *Text) {
	return Log(Text, LOG_CRITICAL);
}

void LogEventf(const char *Fmt, ...) {
	char Buffer[256];
	va_list lst;
	va_start(lst, Fmt);
	vsnprintf(Buffer, sizeof(Buffer), Fmt, lst);
	va_end(lst);
	return Log(Buffer, LOG_EVENT);
}

void LogWarnf(const char *Fmt, ...) {
	char Buffer[256];
	va_list lst;
	va_start(lst, Fmt);
	vsnprintf(Buffer, sizeof(Buffer), Fmt, lst);
	va_end(lst);
	return Log(Buffer, LOG_WARN);
}

void LogCriticalf(const char *Fmt, ...) {
	char Buffer[256];
	va_list lst;
	va_start(lst, Fmt);
	vsnprintf(Buffer, sizeof(Buffer), Fmt, lst);
	va_end(lst);
	return Log(Buffer, LOG_CRITICAL);
}