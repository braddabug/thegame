#ifndef INIPARSE_H
#define INIPARSE_H

// preprocessor options:
// INIPARSE_STRTOF - overrides the default strtof implementation
// INIPARSE_MEMCPY - overrides the default memcpy implementation

#ifndef INIPARSE_STRTOF
#define INIPARSE_STRTOF strtof
#endif

#ifndef INIPARSE_STRTOL
#define INIPARSE_STRTOL strtol
#endif

#ifndef INIPARSE_MEMCPY
#define INIPARSE_MEMCPY memcpy
#endif

struct ini_context
{
	const char* source;
	const char* end;
	const char* position;
};

enum class ini_itemtype
{
	section,
	keyvalue
};

struct ini_item
{
	ini_itemtype type;
	int start;
	int end;

	union
	{
		struct
		{
			int start;
			int end;
		} section;
		struct
		{
			int key_start;
			int key_end;

			int value_start;
			int value_end;
		} keyvalue;
	};
};

const int ini_result_success = 0;
const int ini_result_end_of_section = 1;
const int ini_result_error = -1;
const int ini_result_end_of_stream = -2;
const int ini_result_syntax_error = -3;
const int ini_result_unknown_error_probably_bug = -100;

void ini_init(ini_context* ctx, const char* source, const char* end);
int ini_next(ini_context* ctx, ini_item* item);
int ini_next_within_section(ini_context* ctx, ini_item* item);

int ini_value_int(ini_context* ctx, ini_item* item, int* result);
int ini_value_float(ini_context* ctx, ini_item* item, float* result);
bool ini_section_equals(ini_context* ctx, ini_item* item, const char* name);
bool ini_key_equals(ini_context* ctx, ini_item* item, const char* key);
bool ini_value_equals(ini_context* ctx, ini_item* item, const char* value);

bool ini_key_copy(ini_context* ctx, ini_item* item, char* destination, int maxLength);
bool ini_value_copy(ini_context* ctx, ini_item* item, char* destination, int maxLength);

#endif // INIPARSE_H

#ifdef INIPARSE_IMPLEMENTATION

void ini_init(ini_context* ctx, const char* source, const char* end)
{
	ctx->source = source;
	ctx->end = end;
	ctx->position = source;
}

int ini_next(ini_context* ctx, ini_item* item)
{
	while (ctx->position != ctx->end && *ctx->position != 0)
	{
		char c = *ctx->position;

		switch (c)
		{
		case ' ':
		case '\t':
		case '\r':
		case '\n':
			ctx->position++;
			continue;
		case '[':
		{
			item->type = ini_itemtype::section;
			item->start = (int)(ctx->position - ctx->source);

			// fast forward to the end of the section heading
			while (ctx->position != ctx->end && *ctx->position != 0 && *ctx->position != '\r' && *ctx->position != '\n' && *ctx->position != ']')
				ctx->position++;

			if (*ctx->position != ']')
				return ini_result_syntax_error;

			item->end = (int)(ctx->position - ctx->source) + 1;

			item->section.start = item->start + 1;
			item->section.end = item->end - 1;

			// move past the ]
			ctx->position++;

			return ini_result_success;
		}
		case ';':
		case '#':
		{
			// fast forward to the end of the line
			while (ctx->position != ctx->end && *ctx->position != 0 && *ctx->position != '\r' && *ctx->position != '\n')
				ctx->position++;

			continue;
		}
		default:
		{
			// this must be a key/value pair

			// find the end of the line
			const char* end = ctx->position;
			while (end != ctx->end && *end != 0 && *end != ';' && *end != '#' && *end != '\r' && *end != '\n')
				end++;

			// find the colon
			const char* colon = ctx->position;
			while (colon < end && *colon != ':')
				colon++;

			if (colon == ctx->end || *colon != ':')
				return ini_result_syntax_error;

			item->type = ini_itemtype::keyvalue;
			item->start = (int)(ctx->position - ctx->source);
			item->end = (int)(end - ctx->source);

			item->keyvalue.key_start = item->start;
			item->keyvalue.key_end = (int)(colon - ctx->source);

			const char* valueStart = colon + 1;
			while (valueStart < end && (*valueStart == ' ' || *valueStart == '\t'))
				valueStart++;

			item->keyvalue.value_start = (int)(valueStart - ctx->source);
			item->keyvalue.value_end = item->end;

			ctx->position = end;

			return ini_result_success;
		}
		}

		// still here? This should never happen!
		return ini_result_unknown_error_probably_bug;
	}

	return ini_result_end_of_stream;
}

int ini_next_within_section(ini_context* ctx, ini_item* item)
{
	ini_item i;
	const char* position = ctx->position;
	int result = ini_next(ctx, &i);

	if (result == ini_result_success)
	{
		if (i.type == ini_itemtype::keyvalue)
		{
			*item = i;
			return ini_result_success;
		}

		// rewind
		ctx->position = position;

		return ini_result_end_of_section;
	}

	return result;
}

int ini_value_int(ini_context* ctx, ini_item* item, int* result)
{
	if (item == nullptr || item->type != ini_itemtype::keyvalue)
		return ini_result_error;

	char buffer[20];
	int len = item->keyvalue.value_end - item->keyvalue.value_start;
	INIPARSE_MEMCPY(buffer, ctx->source + item->keyvalue.value_start, len);
	buffer[len] = 0;

	char* end;
	int value = INIPARSE_STRTOL(buffer, &end, 10);

	if (value == 0 && end == buffer)
		return ini_result_error;

	*result = value;
	return ini_result_success;
}

int ini_value_float(ini_context* ctx, ini_item* item, float* result)
{
	if (item == nullptr || item->type != ini_itemtype::keyvalue)
		return ini_result_error;

	char buffer[20];
	int len = item->keyvalue.value_end - item->keyvalue.value_start;
	INIPARSE_MEMCPY(buffer, ctx->source + item->keyvalue.value_start, len);
	buffer[len] = 0;

	char* end;
	float value = INIPARSE_STRTOF(buffer, &end);

	if (value == 0 && end == buffer)
		return ini_result_error;

	*result = value;
	return ini_result_success;
}

bool ini_section_equals(ini_context* ctx, ini_item* item, const char* name)
{
	if (ctx == nullptr || item == nullptr || name == nullptr || item->type != ini_itemtype::section)
		return false;

	for (int i = 0; i < item->section.end - item->section.start; i++)
	{
		if (name[i] == 0 || ctx->source[item->section.start + i] != name[i])
			return false;
	}

	return true;
}

bool ini_key_equals(ini_context* ctx, ini_item* item, const char* key)
{
	if (ctx == nullptr || item == nullptr || key == nullptr || item->type != ini_itemtype::keyvalue)
		return false;

	for (int i = 0; i < item->keyvalue.key_end - item->keyvalue.key_start; i++)
	{
		if (key[i] == 0 || ctx->source[item->keyvalue.key_start + i] != key[i])
			return false;
	}

	return true;
}

bool ini_value_equals(ini_context* ctx, ini_item* item, const char* value)
{
	if (ctx == nullptr || item == nullptr || value == nullptr || item->type != ini_itemtype::keyvalue)
		return false;

	for (int i = 0; i < item->keyvalue.value_end - item->keyvalue.value_start; i++)
	{
		if (value[i] == 0 || ctx->source[item->keyvalue.value_start + i] != value[i])
			return false;
	}

	return true;
}

bool ini_copy(ini_context* ctx, int start, int end, char* destination, int maxLength)
{
	int len = end - start;
	int i = 0;

	while (i < maxLength - 1 && i < len)
	{
		destination[i] = ctx->source[start + i];

		i++;
	}
	destination[i] = 0;

	return i == len;
}

bool ini_key_copy(ini_context* ctx, ini_item* item, char* destination, int maxLength)
{
	if (destination == nullptr || maxLength == 0)
		return false;

	if (ctx == nullptr || item == nullptr || item->type != ini_itemtype::keyvalue)
	{
		destination[0] = 0;
		return false;
	}

	return ini_copy(ctx, item->keyvalue.key_start, item->keyvalue.key_end, destination, maxLength);
}

bool ini_value_copy(ini_context* ctx, ini_item* item, char* destination, int maxLength)
{

	if (destination == nullptr || maxLength == 0)
		return false;

	if (ctx == nullptr || item == nullptr || item->type != ini_itemtype::keyvalue)
	{
		destination[0] = 0;
		return false;
	}

	return ini_copy(ctx, item->keyvalue.value_start, item->keyvalue.value_end, destination, maxLength);
}

#endif // INIPARSE_IMPLEMENTATION