#ifndef CONSOLECOMMAND_H
#define CONSOLECOMMAND_H

typedef void(*CommandCallback)(const char*);

struct ConsoleCommand
{
	const char* Command;
	CommandCallback Callback;
};

#endif // CONSOLECOMMAND_H
