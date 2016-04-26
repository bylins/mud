#include "structs.h"

#include "utils.h"
#include "char.hpp"
#include "msdp.hpp"

void DESCRIPTOR_DATA::msdp_support(bool on)
{
	log("INFO: MSDP support enabled for client %s.\n", host);
	m_msdp_support = on;
}

void DESCRIPTOR_DATA::msdp_report(const std::string& name)
{
	if (msdp_need_report(name))
	{
		msdp::msdp_report(this, name);
	}
}

void DESCRIPTOR_DATA::string_to_client_encoding(const char* input, char* output)
{
	switch (keytable)
	{
	case KT_ALT:
		for (; *input; *output = KtoA(*input), input++, output++);
		break;

	case KT_WIN:
		for (; *input; *output = KtoW(*input), input++, output++)
		{
			if (*input == 'Ñ')
			{
				// Anton Gorev(2016 - 04 - 25): I guess it is dangerous,
				// because 0xff is the special telnet sequence IAC.
				*reinterpret_cast<unsigned char*>(output++) = 255u;
			}
		}
		break;

	case KT_WINZ:
		for (; *input; *output = KtoW2(*input), input++, output++);
		break;

	case KT_WINZ6:
		for (; *input; *output = KtoW2(*input), input++, output++);
		break;

#ifdef HAVE_ICONV
	case KT_UTF8:
		// Anton Gorev (2016-04-25): we have to be careful. String in UTF-8 encoding may
		// contain character with code 0xff which telnet interprets as IAC.
		koi_to_utf8(input, output);
		break;
#endif

	default:
		for (; *input; *output = *input, input++, output++);
		break;
	}

	if (keytable != KT_UTF8)
	{
		*output = '\0';
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
