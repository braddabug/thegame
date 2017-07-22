#ifndef PATH_H
#define PATH_H

class Path
{
public:
	// Gets the filename portion of the path
	static bool GetFileName(const char* path, char* dest, int destLen);

	// gets the extension of the filename, including the "." (empty string if no extension)
	static bool GetExtension(const char* path, char* dest, int destLen);

	// Gets the parent directory portion of the path
	static bool GetDirectoryName(const char* path, char* dest, int destLen);

	// combines two strings into a path
	static bool Combine(const char* path1, const char* path2, char* dest, int destLen);

	static bool ReplaceFilename(const char* path, const char* filename, char* dest, int destLen);

	static bool Test();
};

#endif // PATH_H
