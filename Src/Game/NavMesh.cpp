#include "NavMesh.h"
#include "../FileSystem.h"
#include "../iniparse.h"

namespace Game
{
	bool NavMesh::Load(StringRef filename, NavMesh* result)
	{
		FoundFile f;
		if (FileFinder::OpenAndMap(filename, &f) == false)
			return false;

		ini_context ctx;
		ini_item item;

		ini_init(&ctx, (char*)f.Memory, (char*)f.Memory + f.FileSize);

		float vertices[MaxEdges * MaxPolys][3];
		uint32 vertexCount = 0;

		memset(result, 0, sizeof(NavMesh));

		while (ini_next(&ctx, &item) == ini_result_success)
		{
			if (item.type == ini_itemtype::section)
			{
				if (ini_section_equals(&ctx, &item, "nav"))
				{
					while (ini_next_within_section(&ctx, &item) == ini_result_success)
					{
						if (ini_key_equals(&ctx, &item, "v")) // vertex
						{
							float x, y, z;
							if (ini_value_float_nth(&ctx, &item, 0, &x) == ini_result_success &&
								ini_value_float_nth(&ctx, &item, 1, &y) == ini_result_success &&
								ini_value_float_nth(&ctx, &item, 2, &z) == ini_result_success)
							{
								vertices[vertexCount][0] = x;
								vertices[vertexCount][1] = y;
								vertices[vertexCount][2] = z;
								vertexCount++;
							}
							else
							{
								goto error;
							}
						}
					}
				}
				else if (ini_section_equals(&ctx, &item, "poly"))
				{
					while (ini_next_within_section(&ctx, &item) == ini_result_success)
					{
						if (ini_key_equals(&ctx, &item, "e")) // edge
						{
							for (int i = 0; i < MaxEdges; i++)
							{
								int e;
								if (ini_value_int_nth(&ctx, &item, i, &e) != ini_result_success)
								{
									if (result->Polys[result->NumPolys].NumEdges < 3)
									{
										// at least 3 edges are required
										goto error;
									}

									break;
								}
								else
								{
									if (e < vertexCount)
									{
										result->Polys[result->NumPolys].Verts[result->Polys[result->NumPolys].NumEdges].X = vertices[e][0];
										result->Polys[result->NumPolys].Verts[result->Polys[result->NumPolys].NumEdges].Y = vertices[e][1];
										result->Polys[result->NumPolys].Verts[result->Polys[result->NumPolys].NumEdges].Z = vertices[e][2];
										result->Polys[result->NumPolys].NumEdges++;
									}
									else
									{
										goto error;
									}
								}
							}
						}
					}

					result->NumPolys++;
				}
			}
		}
		

		FileFinder::Close(&f);

		return true;

	error:
		FileFinder::Close(&f);
		return false;
	}

	void NavMesh::Render(Nxna::Graphics::GraphicsDevice* device, Nxna::Matrix* modelview)
	{
		if (Drawing.DrawBuffersInitialized == false)
		{
			uint16 indices[MaxPolys * MaxEdges];

			Drawing.NumIndices = 0;
			int vertIndex = 0;
			for (int i = 0; i < NumPolys; i++)
			{
				for (int j = 0; j < Polys[i].NumEdges - 1; j++)
				{
					indices[Drawing.NumIndices++] = vertIndex;
					indices[Drawing.NumIndices++] = vertIndex + j + 1;
					indices[Drawing.NumIndices++] = vertIndex + j + 2;
				}

				vertIndex += Polys[i].NumEdges;
			}

			float verts[MaxPolys * (MaxEdges + 1) * 3];
			uint32 numVerts = 0;
			for (int i = 0; i < NumPolys; i++)
			{
				for (int j = 0; j < Polys[i].NumEdges; j++)
				{
					verts[numVerts * 3 + 0] = Polys[i].Verts[j].X;
					verts[numVerts * 3 + 1] = Polys[i].Verts[j].Y;
					verts[numVerts * 3 + 2] = Polys[i].Verts[j].Z;
					numVerts++;
				}
			}

			Nxna::Graphics::IndexBufferDesc ibDesc = {};
			ibDesc.ElementSize = Nxna::Graphics::IndexElementSize::SixteenBits;
			ibDesc.NumElements = Drawing.NumIndices;
			ibDesc.InitialData = indices;
			ibDesc.InitialDataByteCount = Drawing.NumIndices * sizeof(uint16);
			if (device->CreateIndexBuffer(&ibDesc, &Drawing.IBuffer) != Nxna::NxnaResult::Success)
				return;

			Nxna::Graphics::VertexBufferDesc vbDesc = {};
			vbDesc.ByteLength = numVerts * sizeof(float) * 3;
			vbDesc.InitialData = verts;
			vbDesc.InitialDataByteCount = numVerts * sizeof(float) * 3;
			if (device->CreateVertexBuffer(&vbDesc, &Drawing.VBuffer) != Nxna::NxnaResult::Success)
			{
				device->DestroyIndexBuffer(Drawing.IBuffer);
				return;
			}

			Nxna::Graphics::ConstantBufferDesc cbDesc = {};
			cbDesc.ByteCount = sizeof(float) * 16;
			if (device->CreateConstantBuffer(&cbDesc, &Drawing.MatrixCBuffer) != Nxna::NxnaResult::Success)
			{
				device->DestroyIndexBuffer(Drawing.IBuffer);
				device->DestroyVertexBuffer(Drawing.VBuffer);
				return;
			}

			float color[4] = { 0.0f, 0.5f, 0.5f, 0.5f };
			cbDesc.ByteCount = 16;
			cbDesc.InitialData = color;
			if (device->CreateConstantBuffer(&cbDesc, &Drawing.ColorCBuffer) != Nxna::NxnaResult::Success)
			{
				device->DestroyConstantBuffer(&Drawing.MatrixCBuffer);
				device->DestroyIndexBuffer(Drawing.IBuffer);
				device->DestroyVertexBuffer(Drawing.VBuffer);
				return;
			}

			Drawing.DrawBuffersInitialized = true;
		}

		auto blend = Graphics::ShaderLibrary::GetBlending(Graphics::BlendType::Transparent);
		device->SetBlendState(blend);

		auto shader = Graphics::ShaderLibrary::GetShader(Graphics::ShaderType::BasicWhite);
		device->SetShaderPipeline(shader);

		device->UpdateConstantBuffer(Drawing.MatrixCBuffer, modelview, sizeof(Nxna::Matrix));
		device->SetConstantBuffer(Drawing.MatrixCBuffer, 0);
		device->SetConstantBuffer(Drawing.ColorCBuffer, 1);

		device->SetVertexBuffer(&Drawing.VBuffer, 0, sizeof(float) * 3);
		device->SetIndices(Drawing.IBuffer);
		device->DrawIndexed(Nxna::Graphics::PrimitiveType::TriangleStrip, 0, 0, MaxPolys * MaxEdges, 0, Drawing.NumIndices);
	}
}
