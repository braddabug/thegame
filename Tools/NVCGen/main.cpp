#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <string>
#include <sstream>
#include "iniparse.h"
#include "FileSystem.h"

struct NVC
{
	std::string Noun;
	std::string Verb;
	std::string Case;
	std::string Action;
};

struct Options
{
	std::string OutputFile;
	std::vector<std::string> InputFiles;
};

bool ParseOptions(int argc, char** argv, Options* result)
{
	result->OutputFile = "Script.cpp";

	for (int i = 1; i < argc; )
	{
		if (strcmp(argv[i], "-o") == 0)
		{
			if (i + 1 < argc)
			{
				result->OutputFile = argv[++i];
			}
			else
			{
				return false;
			}
		}
		else
		{
			result->InputFiles.push_back(argv[i]);
		}

		i++;
	}

	return true;
}

int main(int argc, char** argv)
{
	const char* filename = nullptr;

	Options params;
	if (ParseOptions(argc, argv, &params) == false)
	{
		std::cout << "Usage:" << std::endl;
		std::cout << "\tNVCGen -o output input input input" << std::endl;
		return -1;
	}

	struct SceneInfo
	{
		std::string ID;
		std::vector<NVC> Nvcs;
		std::vector<std::string> Nouns;
		std::vector<std::string> Verbs;
	};

	std::map<std::string, SceneInfo> nvcs;
	std::vector<std::string> actions;

	for (auto itr = params.InputFiles.begin(); itr != params.InputFiles.end(); ++itr)
	{
		ini_context ctx;
		ini_item item;

		File f;
		if (FileSystem::OpenAndMap((*itr).c_str(), &f) == nullptr)
		{
			std::cout << "Unable to open " << *itr << std::endl;
			return -1;
		}

		ini_init(&ctx, (char*)f.Memory, (char*)f.Memory + f.FileSize);

		SceneInfo scene;

		while (ini_next(&ctx, &item) == ini_result_success)
		{
			if (item.type == ini_itemtype::section)
			{
				if (ini_section_equals(&ctx, &item, "nvc"))
				{
					NVC nvc;
					
					while (ini_next_within_section(&ctx, &item) == ini_result_success)
					{
						if (ini_key_equals(&ctx, &item, "noun"))
							nvc.Noun = std::string(ctx.source + item.keyvalue.value_start, item.keyvalue.value_end - item.keyvalue.value_start);
						else if (ini_key_equals(&ctx, &item, "verb"))
							nvc.Verb = std::string(ctx.source + item.keyvalue.value_start, item.keyvalue.value_end - item.keyvalue.value_start);
						else if (ini_key_equals(&ctx, &item, "case"))
							nvc.Case = std::string(ctx.source + item.keyvalue.value_start, item.keyvalue.value_end - item.keyvalue.value_start);
						else if (ini_key_equals(&ctx, &item, "action"))
							nvc.Action = std::string(ctx.source + item.keyvalue.value_start, item.keyvalue.value_end - item.keyvalue.value_start);
					}

					scene.Nvcs.push_back(nvc);
				}
				else if (ini_section_equals(&ctx, &item, "global"))
				{
					while (ini_next_within_section(&ctx, &item) == ini_result_success)
					{
						if (ini_key_equals(&ctx, &item, "id"))
							scene.ID = std::string(ctx.source + item.keyvalue.value_start, item.keyvalue.value_end - item.keyvalue.value_start);
					}
				}
			}
		}

		nvcs.insert(decltype(nvcs)::value_type(scene.ID, scene));

		FileSystem::Close(&f);
	}

	// collect unique nouns and verbs
	for (auto itr = nvcs.begin(); itr != nvcs.end(); ++itr)
	{
		for (auto itr2 = (*itr).second.Nvcs.begin(); itr2 != (*itr).second.Nvcs.end(); ++itr2)
		{
			if (std::find((*itr).second.Nouns.begin(), (*itr).second.Nouns.end(), (*itr2).Noun) == (*itr).second.Nouns.end())
				(*itr).second.Nouns.push_back((*itr2).Noun);

			if (std::find((*itr).second.Verbs.begin(), (*itr).second.Verbs.end(), (*itr2).Verb) == (*itr).second.Verbs.end())
				(*itr).second.Verbs.push_back((*itr2).Verb);

			if (std::find(actions.begin(), actions.end(), (*itr2).Action) == actions.end())
				actions.push_back((*itr2).Action);
		}
	}

	// now generate the script
	std::stringstream ss2;
	std::ofstream ss3;
	std::ostream* pss = nullptr;
	

	ss3.open(params.OutputFile, std::ios::out | std::ios::trunc);
	pss = &ss3;

#define ss (*pss)


	ss << "#include \"../Common.h\"" << std::endl;
	ss << std::endl;

	ss << "bool Test(uint32 nounHash, uint32 verbHash, uint32 sceneID)" << std::endl;
	ss << "{" << std::endl;

	ss << "\tswitch(sceneID) {" << std::endl;
	for (auto itrScene = nvcs.begin(); itrScene != nvcs.end(); ++itrScene)
	{
		ss << "\t\tcase Utils::CalcHash(\"" << (*itrScene).first << "\"):" << std::endl;
		ss << "\t\tswitch(nounHash) {" << std::endl;

		for (auto itr = (*itrScene).second.Nouns.begin(); itr != (*itrScene).second.Nouns.end(); ++itr)
		{
			ss << "\t\t\tcase Utils::CalcHash(\"" << *itr << "\"):" << std::endl;
			ss << "\t\t\tswitch(verbHash) {" << std::endl;

			for (auto itr2 = (*itrScene).second.Verbs.begin(); itr2 != (*itrScene).second.Verbs.end(); ++itr2)
			{
				ss << "\t\t\t\tcase Utils::CalcHash(\"" << *itr2 << "\"):" << std::endl;

				for (auto itr3 = (*itrScene).second.Nvcs.begin(); itr3 != (*itrScene).second.Nvcs.end(); ++itr3)
				{
					if ((*itr3).Noun == *itr && (*itr3).Verb == *itr2)
					{
						if ((*itr3).Case.empty())
							ss << "\t\t\t\t\treturn true;" << std::endl;
						else
							ss << "\t\t\t\t\tif (" << (*itr3).Case << "()) return true;" << std::endl;
					}
				}
			}

			ss << "\t\t\t}" << std::endl;
			ss << "\t\t\tbreak;" << std::endl;
		}
		ss << "\t\t}" << std::endl;
		ss << "\t\tbreak;" << std::endl;
	}

	ss << "\t}" << std::endl;
	ss << "\treturn false;" << std::endl;
	ss << "}" << std::endl;
	ss << std::endl;

	ss << "uint32 GetVerbs(uint32 sceneID, uint32 nounHash, VerbInfo* verbs, uint32 maxVerbs)" << std::endl;
	ss << "{" << std::endl;
	ss << "\tuint32 verbCount = 0;" << std::endl;
	ss << std::endl;

	ss << "\tswitch (sceneID) {" << std::endl;
	for (auto itrScene = nvcs.begin(); itrScene != nvcs.end(); ++itrScene)
	{
		ss << "\t\tcase Utils::CalcHash(\"" << (*itrScene).first << "\"):" << std::endl;
		ss << "\t\tswitch (nounHash) {" << std::endl;

		for (auto itr = (*itrScene).second.Nouns.begin(); itr != (*itrScene).second.Nouns.end(); ++itr)
		{
			ss << "\t\t\tcase Utils::CalcHash(\"" << *itr << "\"):" << std::endl;

			for (auto itr3 = (*itrScene).second.Nvcs.begin(); itr3 != (*itrScene).second.Nvcs.end(); ++itr3)
			{
				if ((*itr3).Noun == *itr)
				{
					if ((*itr3).Case.empty() == false)
						ss << "\t\t\t\tif (" << (*itr3).Case << "()) {" << std::endl;

					ss << "\t\t\t\tverbs[verbCount].ActionHash = Utils::CalcHash(\"" << (*itr3).Action << "\");" << std::endl;
					ss << "\t\t\t\tverbs[verbCount++].VerbHash = Utils::CalcHash(\"" << (*itr3).Verb << "\");" << std::endl;

					if ((*itr3).Case.empty() == false)
						ss << "\t\t\t\t}" << std::endl;
				}
			}

			ss << "\t\t\tbreak;" << std::endl;
		}
		
		ss << "\t\t}" << std::endl;
		ss << "\t\tbreak;" << std::endl;
	}

	ss << "\t}" << std::endl;
	ss << "\treturn verbCount;" << std::endl;
	ss << "}" << std::endl;
	ss << std::endl;

	ss << "void DoAction(uint32 nounHash, uint32 verbHash, uint32 actionHash)" << std::endl;
	ss << "{" << std::endl;
	ss << "    switch(actionHash) {" << std::endl;

	for (auto itr = actions.begin(); itr != actions.end(); ++itr)
	{
		ss << "    case Utils::CalcHash(\"" << (*itr) << "\"): " << (*itr) << "(nounHash, verbHash); break;" << std::endl;
	}

	ss << "    }" << std::endl;
	ss << "}" << std::endl;

	if (filename == nullptr)
		std::cout << ss2.str();

	return 0;
}

#define INIPARSE_IMPLEMENTATION
#include "iniparse.h"

#define FILESYSTEM_BASIC_IMPL
#include "FileSystem.cpp"