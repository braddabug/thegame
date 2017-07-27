#ifndef GUI_TEXTPRINTER_H
#define GUI_TEXTPRINTER_H

#include "../MyNxna2.h"

class SpriteBatchHelper;

namespace Gui
{
	enum class FontType
	{
		Default,
		Console
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
		int DefaultCharacterInfoIndex;
		float LineHeight;
		
		Nxna::Graphics::Texture2D Texture;
	};

	struct TextPrinterData
	{
		Font* DefaultFont;
		Font* ConsoleFont;
	};

	class TextPrinter
	{
		static TextPrinterData* m_data;

	public:
		static void SetGlobalData(TextPrinterData** m_data);
		static bool Init(Nxna::Graphics::GraphicsDevice* device);
		static void Shutdown();

		static Font* GetFont(FontType type);

		static Nxna::Vector2 MeasureString(Font* font, const char* text, const char* end = nullptr);
		static void PrintScreen(SpriteBatchHelper* sb, float x, float y, Font* font, const char* text, Nxna::PackedColor color = NXNA_GET_PACKED_COLOR_RGB_BYTES(255, 255, 255));

	private:
		static bool createFont(Nxna::Graphics::GraphicsDevice* device, const char* path, float size, int firstCharacter, int lastCharacter, int defaultCharacter, Font** result);
	};
}

#endif // GUI_TEXTPRINTER_H
