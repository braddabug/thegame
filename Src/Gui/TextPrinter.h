#ifndef GUI_TEXTPRINTER_H
#define GUI_TEXTPRINTER_H

#include "../MyNxna2.h"

class SpriteBatchHelper;

namespace Gui
{
	enum class FontType
	{
		Default
	};

	struct Font
	{
		struct CharInfo
		{
			float SrcX, SrcY, SrcW, SrcH;
			float ScreenW, ScreenH;
			float XAdvance;
			float XOffset;
			float YOffset;
		};

		CharInfo* Characters;
		int* CharacterMap;
		int NumCharacters;
	};

	struct TextPrinterData
	{
		Nxna::Graphics::Texture2D Texture;
		Font* DefaultFont;
	};

	class TextPrinter
	{
	public:
		static bool Init(Nxna::Graphics::GraphicsDevice* device, TextPrinterData* result);
		static void Shutdown(TextPrinterData* data);

		static void PrintScreen(SpriteBatchHelper* sb, TextPrinterData* data, float x, float y, FontType font, const char* text);

	private:
		static bool createFont(Nxna::Graphics::GraphicsDevice* device, TextPrinterData* data, const char* path, float size, Font** result);
	};
}

#endif // GUI_TEXTPRINTER_H
