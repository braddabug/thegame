#include "Path.h"
#include <cstring>

bool CopyString(const char* source, char* dest, int destLength)
{
	int i = 0;
	while (source[i] != 0 && destLength - i > 0)
	{
		dest[i] = source[i];
		i++;
	}
	dest[i] = 0;

	return source[i] == 0;
}

bool CopyString(const char* source, int sourceLength, char* dest, int destLength)
{
	int toCopy = sourceLength < (destLength - 1) ? sourceLength : (destLength - 1);
	memcpy(dest, source, toCopy);
	dest[toCopy] = 0;

	return toCopy == sourceLength;
}

bool ConcatString(const char* source, char** dest, int* destLen)
{
	const char* original = *dest;

	// find the end of dest
	while ((*dest)[0] != 0)
	{
		(*dest)++;
		(*destLen)--;
	}

	int i = 0;
	while (*destLen > 1 && source[i] != 0)
	{
		(*dest)[0] = source[i];

		(*destLen)--;
		(*dest)++;
		i++;
	}

	(*dest)[0] = 0;
	(*destLen)--;

	return source[i] == 0;
}

bool Path::GetDirectoryName(const char* path, char* dest, int destLen)
{
	if (path == nullptr || dest == nullptr || destLen == 0) return false;

	// search for the last \ or /
	auto pathLen = strlen(path);
	if (pathLen == 0)
	{
		dest[0] = 0;
		return true;
	}

	for (int i = (int)pathLen - 1; i >= 0; i--)
	{
		if (path[i] == '/' || path[i] == '\\')
		{
			return CopyString(path, i, dest, destLen);
		}
	}

	dest[0] = 0;
	return true;
}

bool Path::Combine(const char* path1, const char* path2, char* dest, int destLen)
{
	if (dest == nullptr || destLen == 0)
		return false;

	if (path1 == nullptr || path1[0] == 0)
	{
		if (path2 == nullptr || path2[0] == 0) 
		{
			dest[0] = 0;
			return true;
		}
		else
		{
			return CopyString(path2, dest, destLen);
		}
	}

	if (path2 == nullptr || path2[0] == 0)
	{
		return CopyString(path1, dest, destLen);
	}

	dest[0] = 0;
	return ConcatString(path1, &dest, &destLen) &&
		(dest[-1] == '/' || dest[-1] == '\\' || path2[0] == '/' || path2[0] == '\\' ? true : ConcatString("/", &dest, &destLen)) &&
		ConcatString(path2, &dest, &destLen);
}

bool Path::ReplaceFilename(const char* path, const char* filename, char* dest, int destLen)
{
	if (GetDirectoryName(path, dest, destLen) == false)
		return false;

	return ConcatString("/", &dest, &destLen) &&
		ConcatString(filename, &dest, &destLen);
}

bool Path::Test()
{
	char dest[256];

	if (GetDirectoryName("blah\\foo\\bar\\file.txt", dest, 256) == false ||
		strcmp(dest, "blah\\foo\\bar") != 0) 
		return false;

	if (GetDirectoryName("blah/foo/bar/file.txt", dest, 256) == false ||
		strcmp(dest, "blah/foo/bar") != 0)
		return false;

	if (GetDirectoryName("blah/foo/bar/", dest, 256) == false ||
		strcmp(dest, "blah/foo/bar") != 0)
		return false;

	if (GetDirectoryName("blah", dest, 256) == false ||
		strcmp(dest, "") != 0)
		return false;

	if (Combine("foo/", "bar", dest, 256) == false ||
		strcmp(dest, "foo/bar") != 0)
		return false;

	if (Combine("foo", "/bar", dest, 256) == false ||
		strcmp(dest, "foo/bar") != 0)
		return false;

	if (Combine("foo", "bar", dest, 256) == false ||
		strcmp(dest, "foo/bar") != 0)
		return false;

	if (ReplaceFilename("foo/bar/baz.txt", "blah.dat", dest, 256) == false ||
		strcmp(dest, "foo/bar/blah.dat") != 0)
		return false;

	return true;
}