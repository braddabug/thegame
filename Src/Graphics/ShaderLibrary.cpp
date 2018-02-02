#include "ShaderLibrary.h"
#include "../MemoryManager.h"

namespace Graphics
{
	struct ShaderLibraryData
	{
		Nxna::Graphics::GraphicsDevice* Device;

		Nxna::Graphics::ShaderPipeline Shaders[(int)ShaderType::LAST];
		Nxna::Graphics::BlendState Blending[(int)BlendType::LAST];
	};

	ShaderLibraryData* ShaderLibrary::m_data = nullptr;

	void ShaderLibrary::SetGlobalData(ShaderLibraryData** data, Nxna::Graphics::GraphicsDevice* device)
	{
		if (*data == nullptr)
		{
			*data = NewObject<ShaderLibraryData>(__FILE__, __LINE__);
			(*data)->Device = device;
		}

		m_data = *data;
	}

	void ShaderLibrary::Shutdown()
	{
		// TODO: destroy all loaded shaders

		g_memory->FreeTrack(m_data, __FILE__, __LINE__);
	}

	bool ShaderLibrary::LoadCoreShaders()
	{
		// basic white
		{
			const char* glsl_vertex = R"(#version 420
			uniform dataz { mat4 ModelViewProjection; };
			layout(location = 0) in vec3 position;
			out VertexOutput
			{
				vec2 o_diffuseCoords;
			};
			out gl_PerVertex { vec4 gl_Position; };
			void main()
			{
				gl_Position = ModelViewProjection * vec4(position, 1.0);
			}
		)";

			const char* glsl_frag = R"(
			layout(binding = 1) uniform dataz { vec4 Color; };
			out vec4 outputColor;
			
			void main()
			{
				outputColor = Color;
			}
		)";

			Nxna::Graphics::InputElement inputElements[] = {
				{ 0, Nxna::Graphics::InputElementFormat::Vector3, Nxna::Graphics::InputElementUsage::Position, 0 },
			};

			if (createShader((const uint8*)glsl_vertex, sizeof(glsl_vertex), (const uint8*)glsl_frag, sizeof(glsl_frag), inputElements, 1, &m_data->Shaders[(int)ShaderType::BasicWhite]) == false)
				return false;
		}

		// basic textured
		{
			const char* glsl_vertex = R"(#version 420
			uniform dataz { mat4 ModelViewProjection; };
			layout(location = 0) in vec3 position;
			layout(location = 1) in vec2 texCoords;
			out VertexOutput
			{
				vec2 o_diffuseCoords;
			};
			out gl_PerVertex { vec4 gl_Position; };
			void main()
			{
				gl_Position = ModelViewProjection * vec4(position, 1.0);
				o_diffuseCoords = texCoords;
			}
		)";

			const char* glsl_frag = R"(
			DECLARE_SAMPLER2D(0, Diffuse);
			in VertexOutput
			{
				vec2 o_diffuseCoords;
			};
			out vec4 outputColor;
			
			void main()
			{
				float v = o_diffuseCoords.x;
				outputColor = texture(Diffuse, o_diffuseCoords);
			}
		)";

			Nxna::Graphics::InputElement inputElements[] = {
				{ 0, Nxna::Graphics::InputElementFormat::Vector3, Nxna::Graphics::InputElementUsage::Position, 0 },
				{ 3 * sizeof(float), Nxna::Graphics::InputElementFormat::Vector2, Nxna::Graphics::InputElementUsage::TextureCoordinate, 0 }
			};

			if (createShader((const uint8*)glsl_vertex, sizeof(glsl_vertex), (const uint8*)glsl_frag, sizeof(glsl_frag), inputElements, 2, &m_data->Shaders[(int)ShaderType::BasicTextured]) == false)
				return false;
		}

		// create blending
		Nxna::Graphics::BlendStateDesc blend1 = NXNA_BLENDSTATEDESC_DEFAULT;
		if (m_data->Device->CreateBlendState(&blend1, &m_data->Blending[0]) != Nxna::NxnaResult::Success)
			return false;
		Nxna::Graphics::BlendStateDesc blend2 = {};
		blend2.RenderTarget[0].BlendingEnabled = true;
		blend2.RenderTarget[0].ColorSourceBlend = Nxna::Graphics::Blend::One;
		blend2.RenderTarget[0].AlphaSourceBlend = Nxna::Graphics::Blend::One;
		blend2.RenderTarget[0].ColorDestinationBlend = Nxna::Graphics::Blend::InverseSourceAlpha;
		blend2.RenderTarget[0].AlphaDestinationBlend = Nxna::Graphics::Blend::InverseSourceAlpha;
		if (m_data->Device->CreateBlendState(&blend2, &m_data->Blending[1]) != Nxna::NxnaResult::Success)
			return false;

		return true;
	}

	Nxna::Graphics::ShaderPipeline* ShaderLibrary::GetShader(ShaderType type)
	{
		return &m_data->Shaders[(int)type];
	}

	Nxna::Graphics::BlendState* ShaderLibrary::GetBlending(BlendType type)
	{
		return &m_data->Blending[(int)type];
	}

	bool ShaderLibrary::createShader(const uint8* vertexBytecode, size_t sizeOfVertexBytecode, const uint8* pixelBytecode, size_t sizeOfPixelBytecode, Nxna::Graphics::InputElement* elements, int numElements, Nxna::Graphics::ShaderPipeline* result)
	{
		Nxna::Graphics::ShaderBytecode vertexShaderBytecode, pixelShaderBytecode;
		vertexShaderBytecode.Bytecode = vertexBytecode;
		vertexShaderBytecode.BytecodeLength = (uint32)sizeOfVertexBytecode;
		pixelShaderBytecode.Bytecode = pixelBytecode;
		pixelShaderBytecode.BytecodeLength = (uint32)sizeOfPixelBytecode;

		Nxna::Graphics::Shader vs, ps;
		if (m_data->Device->CreateShader(Nxna::Graphics::ShaderType::Vertex, vertexShaderBytecode.Bytecode, vertexShaderBytecode.BytecodeLength, &vs) != Nxna::NxnaResult::Success)
		{
			return false;
		}
		if (m_data->Device->CreateShader(Nxna::Graphics::ShaderType::Pixel, pixelShaderBytecode.Bytecode, pixelShaderBytecode.BytecodeLength, &ps) != Nxna::NxnaResult::Success)
		{
			return false;
		}

		// now that the shaders have been created, put them into a ShaderPipeline
		Nxna::Graphics::ShaderPipelineDesc spDesc = {};
		spDesc.VertexShader = &vs;
		spDesc.PixelShader = &ps;
		spDesc.VertexShaderBytecode = vertexShaderBytecode;
		spDesc.VertexElements = elements;
		spDesc.NumElements = numElements;
		if (m_data->Device->CreateShaderPipeline(&spDesc, result) != Nxna::NxnaResult::Success)
		{
			return false;
		}

		return true;
	}
}