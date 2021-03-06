#ifndef SPRITEBATCHHELPER_H
#define SPRITEBATCHHELPER_H

#include "MyNxna2.h"
#include "Common.h"
#include <vector>
#include <cstdio>

struct SpriteBatchData
{
	Nxna::Graphics::GraphicsDevice* Device;
	Nxna::Graphics::VertexBuffer VertexBuffer;
	Nxna::Graphics::IndexBuffer IndexBuffer;
	Nxna::Graphics::ShaderPipeline ShaderPipeline;
	Nxna::Graphics::ConstantBuffer ConstantBuffer;
	Nxna::Graphics::BlendState BlendState;
	Nxna::Graphics::RasterizerState RasterState;
	Nxna::Graphics::DepthStencilState DepthState;
	Nxna::Graphics::SamplerState SamplerState;
	uint32 Stride;
};

class SpriteBatchHelper
{
	static SpriteBatchData* m_data;
	std::vector<Nxna::Graphics::SpriteBatchSprite> m_sprites;

	static const int BATCH_SIZE = 256;
public:

	static void SetGlobalData(SpriteBatchData** data)
	{
		if (*data == nullptr)
			*data = new SpriteBatchData();

		m_data = *data;
	}

	static int Init(Nxna::Graphics::GraphicsDevice* device)
	{
		if (m_data == nullptr) return -1;

		m_data->Device = device;

		m_data->Stride;
		Nxna::Graphics::InputElement elements[3];
		Nxna::Graphics::SpriteBatch::SetupVertexElements(elements, &m_data->Stride);

		// create the vertex buffer
		Nxna::Graphics::VertexBufferDesc vbDesc = {};
		vbDesc.BufferUsage = Nxna::Graphics::Usage::Dynamic;
		vbDesc.ByteLength = m_data->Stride * BATCH_SIZE * 4;
		vbDesc.InitialData = nullptr;
		vbDesc.InitialDataByteCount = 0;
		if (device->CreateVertexBuffer(&vbDesc, &m_data->VertexBuffer) != Nxna::NxnaResult::Success)
		{
			printf("Unable to create vertex buffer\n");
			return -1;
		}

		// create the index buffer
		unsigned short indices[BATCH_SIZE * 6];
		Nxna::Graphics::SpriteBatch::FillIndexBuffer(indices, BATCH_SIZE * 6);
		Nxna::Graphics::IndexBufferDesc ibDesc = {};
		ibDesc.ElementSize = Nxna::Graphics::IndexElementSize::SixteenBits;
		ibDesc.InitialData = indices;
		ibDesc.InitialDataByteCount = sizeof(unsigned short) * BATCH_SIZE * 6;
		ibDesc.NumElements = BATCH_SIZE * 6;
		if (device->CreateIndexBuffer(&ibDesc, &m_data->IndexBuffer) != Nxna::NxnaResult::Success)
		{
			printf("Unable to create index buffer\n");
			return -1;
		}

		// create the vertex and pixel shaders
		Nxna::Graphics::ShaderBytecode vertexShaderBytecode, pixelShaderBytecode;
		Nxna::Graphics::SpriteBatch::GetShaderBytecode(device, &vertexShaderBytecode, &pixelShaderBytecode);

		Nxna::Graphics::Shader vs, ps;
		if (device->CreateShader(Nxna::Graphics::ShaderType::Vertex, vertexShaderBytecode.Bytecode, vertexShaderBytecode.BytecodeLength, &vs) != Nxna::NxnaResult::Success)
		{
			printf("Unable to create vertex shader\n");
			return -1;
		}
		if (device->CreateShader(Nxna::Graphics::ShaderType::Pixel, pixelShaderBytecode.Bytecode, pixelShaderBytecode.BytecodeLength, &ps) != Nxna::NxnaResult::Success)
		{
			printf("Unable to create pixel shader\n");
			return -1;
		}

		// now that the shaders have been created, put them into a ShaderPipeline
		Nxna::Graphics::ShaderPipelineDesc spDesc = {};
		spDesc.VertexShader = &vs;
		spDesc.PixelShader = &ps;
		spDesc.VertexShaderBytecode = vertexShaderBytecode;
		spDesc.VertexElements = elements;
		spDesc.NumElements = 3;
		if (device->CreateShaderPipeline(&spDesc, &m_data->ShaderPipeline) != Nxna::NxnaResult::Success)
		{
			printf("Unable to create shader pipeline\n");
			return -1;
		}

		// now create a constant buffer so we can send parameters to the shaders
		unsigned char constantBufferData[16 * sizeof(float)];
		Nxna::Graphics::SpriteBatch::SetupConstantBuffer(device->GetViewport(), constantBufferData, 16 * sizeof(float));
		Nxna::Graphics::ConstantBufferDesc cbDesc = {};
		cbDesc.InitialData = constantBufferData;
		cbDesc.ByteCount = sizeof(float) * 16;
		if (device->CreateConstantBuffer(&cbDesc, &m_data->ConstantBuffer) != Nxna::NxnaResult::Success)
		{
			printf("Unable to create constant buffer\n");
			return -1;
		}

		// create a blend state using default premultiplied alpha blending
		Nxna::Graphics::BlendStateDesc bd;
		bd.IndependentBlendEnabled = false;
		bd.RenderTarget[0] = NXNA_RENDERTARGETBLENDSTATEDESC_ALPHABLEND;
		if (device->CreateBlendState(&bd, &m_data->BlendState) != Nxna::NxnaResult::Success)
		{
			printf("Unable to create blend state\n");
			return -1;
		}

		// create a rasterization state using default no-culling
		Nxna::Graphics::RasterizerStateDesc rd = NXNA_RASTERIZERSTATEDESC_CULLNONE;
		if (device->CreateRasterizerState(&rd, &m_data->RasterState) != Nxna::NxnaResult::Success)
		{
			printf("Unable to create rasterizer state\n");
			return -1;
		}

		// create a depth/stencil state using read-only
		Nxna::Graphics::DepthStencilStateDesc dd = NXNA_DEPTHSTENCIL_DEPTHREAD;
		if (device->CreateDepthStencilState(&dd, &m_data->DepthState) != Nxna::NxnaResult::Success)
		{
			printf("Unable to create depth/stencil state\n");
			return -1;
		}

		// create a point-filtering sampler state
		Nxna::Graphics::SamplerStateDesc sd = NXNA_SAMPLERSTATEDESC_POINTCLAMP;
		if (device->CreateSamplerState(&sd, &m_data->SamplerState) != Nxna::NxnaResult::Success)
		{
			printf("Unable to create sampler state\n");
			return -1;
		}


		return 0;
	}

	static void Destroy()
	{
		if (m_data == nullptr || m_data->Device == nullptr)
			return;

		m_data->Device->SetShaderPipeline(nullptr);

		m_data->Device->DestroyVertexBuffer(m_data->VertexBuffer);
		m_data->Device->DestroyIndexBuffer(m_data->IndexBuffer);
		m_data->Device->DestroyConstantBuffer(&m_data->ConstantBuffer);
		m_data->Device->DestroyShaderPipeline(&m_data->ShaderPipeline);
		m_data->Device->DestroySamplerState(&m_data->SamplerState);
		m_data->Device->DestroyDepthStencilState(&m_data->DepthState);
		m_data->Device->DestroyRasterizerState(&m_data->RasterState);
		m_data->Device->DestroyBlendState(&m_data->BlendState);
	}

	void Reset()
	{
		m_sprites.clear();
	}

	void Draw(Nxna::Graphics::Texture2D* texture, int textureWidth, int textureHeight, float x, float y, float width, float height, Nxna::Color color)
	{
		Nxna::Graphics::SpriteBatchSprite sprite;
		Nxna::Graphics::SpriteBatch::WriteSprite(&sprite, texture, textureWidth, textureHeight, x, y, width, height, color);

		m_sprites.push_back(sprite);
	}

	Nxna::Graphics::SpriteBatchSprite* AddSprites(uint32 count = 1)
	{
		if (count == 0) return nullptr;

		m_sprites.resize(m_sprites.size() + count);
		return &m_sprites.back() - count + 1;
	}

	void Render()
	{
		if (m_sprites.empty()) return;

		auto device = m_data->Device;

		// TODO: sort the sprites by texture

		// set states
		device->SetBlendState(&m_data->BlendState);
		device->SetRasterizerState(&m_data->RasterState);
		device->SetDepthStencilState(&m_data->DepthState);
		device->SetConstantBuffer(m_data->ConstantBuffer, 0);
		device->SetShaderPipeline(&m_data->ShaderPipeline);
		device->SetSamplerState(0, &m_data->SamplerState);
		device->SetVertexBuffer(&m_data->VertexBuffer, 0, m_data->Stride);
		device->SetIndices(m_data->IndexBuffer);

		unsigned int spritesDrawn = 0;
		while (true)
		{
			unsigned int textureChanges[10];
			unsigned int numTextureChanges = 10;

			auto vbp = device->MapBuffer(m_data->VertexBuffer, Nxna::Graphics::MapType::WriteDiscard);
			unsigned int spritesAdded = Nxna::Graphics::SpriteBatch::FillVertexBuffer(&m_sprites[0] + spritesDrawn, nullptr, (uint32)m_sprites.size() - spritesDrawn, vbp, BATCH_SIZE * 4 * sizeof(float), textureChanges, &numTextureChanges);
			device->UnmapBuffer(m_data->VertexBuffer);

			unsigned int currentSprite = 0;
			for (unsigned int i = 0; i < numTextureChanges; i++)
			{
				device->BindTexture(&m_sprites[spritesDrawn + currentSprite].Texture, 0);

				device->DrawIndexed(Nxna::Graphics::PrimitiveType::TriangleList, 0, 0, BATCH_SIZE * 4, currentSprite * 6, textureChanges[i] * 2 * 3);
				currentSprite += textureChanges[i];
			}
			spritesDrawn += spritesAdded;

			if (spritesDrawn == m_sprites.size())
				break;
		}

		m_sprites.clear();
	}
};

#endif // SPRITEBATCHHELPER_H
