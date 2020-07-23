#ifndef STACKTEXT_H
#define STACKTEXT_H
#include <cstdarg>
#include <LWPlatform/LWPlatform.h>

template<unsigned int Len>
class lStackText {
public:

	const char *operator ()(void) const {
		return m_Text;
	}

	lStackText(const char *Fmt, ...) {
		va_list lst;
		va_start(lst, Fmt);
		vsnprintf(m_Text, Len, Fmt, lst);
		va_end(lst);
	}

	lStackText() {
		*m_Text = '\0';
	}

private:
	char m_Text[Len];
};

typedef lStackText<128u> StackText;


#endif