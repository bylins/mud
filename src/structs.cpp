#include "structs.h"

#include "utils.h"

void asciiflag_conv(const char *flag, void *to)
{
	int *flags = (int *)to;
	int is_number = 1, block = 0, i;
	const char *p;

	for (p = flag; *p; p += i + 1)
	{
		i = 0;
		if (islower(*p))
		{
			if (*(p + 1) >= '0' && *(p + 1) <= '9')
			{
				block = (int)* (p + 1) - '0';
				i = 1;
			}
			else
				block = 0;
			*(flags + block) |= (0x3FFFFFFF & (1 << (*p - 'a')));
		}
		else if (isupper(*p))
		{
			if (*(p + 1) >= '0' && *(p + 1) <= '9')
			{
				block = (int)* (p + 1) - '0';
				i = 1;
			}
			else
				block = 0;
			*(flags + block) |= (0x3FFFFFFF & (1 << (26 + (*p - 'A'))));
		}
		if (!isdigit(*p))
			is_number = 0;
	}

	if (is_number)
	{
		is_number = atol(flag);
		block = is_number < INT_ONE ? 0 : is_number < INT_TWO ? 1 : is_number < INT_THREE ? 2 : 3;
		*(flags + block) = is_number & 0x3FFFFFFF;
	}
}

void FLAG_DATA::from_string(const char *flag)
{
	uint32_t is_number = 1;
	int i;

	for (const char* p = flag; *p; p += i + 1)
	{
		i = 0;
		if (islower(*p))
		{
			size_t block = 0;
			if (*(p + 1) >= '0' && *(p + 1) <= '9')
			{
				block = (int)* (p + 1) - '0';
				i = 1;
			}
			m_flags[block] |= (0x3FFFFFFF & (1 << (*p - 'a')));
		}
		else if (isupper(*p))
		{
			size_t block = 0;
			if (*(p + 1) >= '0' && *(p + 1) <= '9')
			{
				block = (int)* (p + 1) - '0';
				i = 1;
			}
			m_flags[block] |= (0x3FFFFFFF & (1 << (26 + (*p - 'A'))));
		}

		if (!isdigit(*p))
		{
			is_number = 0;
		}
	}

	if (is_number)
	{
		is_number = atol(flag);
		const size_t block = is_number >> 30;
		m_flags[block] = is_number & 0x3FFFFFFF;
	}
}

void FLAG_DATA::tascii(int num_planes, char* ascii) const
{
	bool found = false;

	for (int i = 0; i < num_planes; i++)
	{
		for (int c = 0; c < 30; c++)
		{
			if (m_flags[i] & (1 << c))
			{
				found = true;
				sprintf(ascii + strlen(ascii), "%c%d", c < 26 ? c + 'a' : c - 26 + 'A', i);
			}
		}
	}

	strcat(ascii, found ? " " : "0 ");
}

void tascii(const uint32_t* pointer, int num_planes, char* ascii)
{
	bool found = false;

	for (int i = 0; i < num_planes; i++)
	{
		for (int c = 0; c < 30; c++)
		{
			if (pointer[i] & (1 << c))
			{
				found = true;
				sprintf(ascii + strlen(ascii), "%c%d", c < 26 ? c + 'a' : c - 26 + 'A', i);
			}
		}
	}

	strcat(ascii, found ? " " : "0 ");
}

bool sprintbitwd(bitvector_t bitvector, const char *names[], char *result, const char *div, const int print_flag)
{
	long nr = 0;
	int fail = 0;
	int plane = 0;
	char c = 'a';

	*result = '\0';

	fail = bitvector >> 30;
	bitvector &= 0x3FFFFFFF;
	while (fail)
	{
		if (*names[nr] == '\n')
		{
			fail--;
			plane++;
		}
		nr++;
	}

	bool first = true;
	for (; bitvector; bitvector >>= 1)
	{
		if (IS_SET(bitvector, 1))
		{
			if (!first)
			{
				strcat(result, div);
			}
			first = false;

			if (*names[nr] != '\n')
			{
				if (print_flag == 1)
				{
					sprintf(result + strlen(result), "%c%d:", c, plane);
				}
				if ((print_flag == 2) && (!strcmp(names[nr], "UNUSED")))
				{
					sprintf(result + strlen(result), "%ld:", nr + 1);
				}
				strcat(result, names[nr]);
			}
			else
			{
				if (print_flag == 2)
				{
					sprintf(result + strlen(result), "%ld:", nr + 1);
				}
				else if (print_flag == 1)
				{
					sprintf(result + strlen(result), "%c%d:", c, plane);
				}
				strcat(result, "UNDEF");
			}
		}
		if (print_flag == 1)
		{
			c++;
			if (c > 'z')
			{
				c = 'A';
			}
		}
		if (*names[nr] != '\n')
		{
			nr++;
		}
	}

	return '\0' != *result;
}

bool FLAG_DATA::sprintbits(const char *names[], char *result, const char *div, const int print_flag) const
{
	bool have_flags = false;
	char buffer[MAX_STRING_LENGTH];
	*result = '\0';

	for (int i = 0; i < 4; i++)
	{
		if (sprintbitwd(m_flags[i] | (i << 30), names, buffer, div, print_flag))
		{
			if ('\0' != *result)	// We don't need to calculate length of result. We just want to know if it is not empty.
			{
				strcat(result, div);
			}
			strcat(result, buffer);
			have_flags = true;
		}
	}

	return have_flags;
}