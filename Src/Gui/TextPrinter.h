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
		static TextPrinterData* m_data;

	public:
		static void SetGlobalData(TextPrinterData** m_data);
		static bool Init(Nxna::Graphics::GraphicsDevice* device);
		static void Shutdown();

		static void PrintScreen(SpriteBatchHelper* sb, float x, float y, FontType font, const char* text);

	private:
		static bool createFont(Nxna::Graphics::GraphicsDevice* device, const char* path, float size, Font** result);
	};
}

#endif // GUI_TEXTPRINTER_H
