#include "Model.h"
#include "tiny_obj_loader.h"
#include <sstream>

namespace Graphics
{
	ModelData* Model::m_data = nullptr;

	void Model::SetGlobalData(ModelData** data)
	{
		if (*data == nullptr)
		{
			*data = new ModelData();
			(*data)->Initialized = false;
		}

		m_data = *data;
	}

	int Model::LoadObj(Content::ContentManager* content, const char* filename, Model* result, void* device)
	{
		Nxna::Graphics::GraphicsDevice* gd = (Nxna::Graphics::GraphicsDevice*)device;

		if (m_data->Initialized == false)
		{
			Nxna::Graphics::ConstantBufferDesc cbDesc = {};
			cbDesc.InitialData = nullptr;
			cbDesc.ByteCount = sizeof(float) * 16;
			if (gd->CreateConstantBuffer(&cbDesc, &m_data->Constants) != Nxna::NxnaResult::Success)
			{
				printf("Unable to create constant buffer\n");
				return -1;
			}

			Nxna::Graphics::SamplerStateDesc ssDesc = NXNA_SAMPLERSTATEDESC_LINEARWRAP;
			if (gd->CreateSamplerState(&ssDesc, &m_data->SamplerState) != Nxna::NxnaResult::Success)
			{
				printf("Unable to create sampler state\n");
				return -1;
			}
		}


		File f;
		if (FileSystem::OpenAndMap(filename, &f) == nullptr)
			return false;

		std::string str((char*)f.Memory, f.FileSize);
		std::stringstream ss(str);

		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;

		tinyobj::MaterialFileReader mr("Content/Models/");

		std::string err;
		bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, &ss, &mr);

		if (!err.empty()) { // `err` may contain warning message.
			// TODO: log the error
		}

		if (!ret) {
			exit(1);
		}

		// count the total number of vertices
		uint32 numVertices = 0;
		for (size_t s = 0; s < shapes.size(); s++)
		{
			for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++)
			{
				int fv = shapes[s].mesh.num_face_vertices[f];
				if (fv != 3)
					return false; // model must be triangulated
				
				numVertices += fv;			
			}
		}

		struct Vertex
		{
			float X, Y, Z;
			float U, V;
		};
		Vertex* vertices = new Vertex[numVertices];

		result->NumMeshes = (uint32)shapes.size();
		result->Meshes = new ModelMesh[shapes.size()];

		// Loop over shapes
		uint32 vertexCursor = 0;
		for (size_t s = 0; s < shapes.size(); s++)
		{
			result->Meshes[s].FirstIndex = vertexCursor;

			// Loop over faces(polygon)
			uint32 index_offset = 0;
			for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++)
			{
				// Loop over vertices in the face.
				for (size_t v = 0; v < 3; v++)
				{
					Vertex vtx;

					// access to vertex
					tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
					vtx.X = attrib.vertices[3*idx.vertex_index+0];
					vtx.Y = attrib.vertices[3*idx.vertex_index+1];
					vtx.Z = attrib.vertices[3*idx.vertex_index+2];
					//float nx = attrib.normals[3*idx.normal_index+0];
					//float ny = attrib.normals[3*idx.normal_index+1];
					//float nz = attrib.normals[3*idx.normal_index+2];
					vtx.U = attrib.texcoords[2*idx.texcoord_index+0];
					vtx.V = 1.0f - attrib.texcoords[2*idx.texcoord_index+1];

					vertices[vertexCursor++] = vtx;
				}

				index_offset += 3;

				// per-face material
				result->Meshes[s].TextureIndex = shapes[s].mesh.material_ids[f];
			}

			result->Meshes[s].NumTriangles = (uint32)shapes[s].mesh.num_face_vertices.size();
		}

		Nxna::Graphics::VertexBufferDesc vbDesc = {};
		vbDesc.ByteLength = numVertices * sizeof(Vertex);
		vbDesc.InitialData = vertices;
		vbDesc.InitialDataByteCount = numVertices * sizeof(Vertex);
		if (gd->CreateVertexBuffer(&vbDesc, &result->Vertices) != Nxna::NxnaResult::Success)
		{
			delete[] vertices;
			delete[] result->Meshes;
			return false;
		}
		delete[] vertices;

		result->VertexStride = sizeof(Vertex);

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
			{ 0, Nxna::Graphics::InputElementFormat::Vector3, Nxna::Graphics::InputElementUsage::Position, 0},
			{ 3 * sizeof(float), Nxna::Graphics::InputElementFormat::Vector2, Nxna::Graphics::InputElementUsage::TextureCoordinate, 0 }
		};

		Nxna::Graphics::ShaderBytecode vertexShaderBytecode, pixelShaderBytecode;
		vertexShaderBytecode.Bytecode = glsl_vertex;
		vertexShaderBytecode.BytecodeLength = sizeof(glsl_vertex);
		pixelShaderBytecode.Bytecode = glsl_frag;
		pixelShaderBytecode.BytecodeLength = sizeof(glsl_frag);

		Nxna::Graphics::Shader vs, ps;
		if (gd->CreateShader(Nxna::Graphics::ShaderType::Vertex, vertexShaderBytecode.Bytecode, vertexShaderBytecode.BytecodeLength, &vs) != Nxna::NxnaResult::Success)
		{
			printf("Unable to create vertex shader\n");
			delete[] result->Meshes;
			return -1;
		}
		if (gd->CreateShader(Nxna::Graphics::ShaderType::Pixel, pixelShaderBytecode.Bytecode, pixelShaderBytecode.BytecodeLength, &ps) != Nxna::NxnaResult::Success)
		{
			printf("Unable to create pixel shader\n");
			delete[] result->Meshes;
			return -1;
		}

		// now that the shaders have been created, put them into a ShaderPipeline
		Nxna::Graphics::ShaderPipelineDesc spDesc = {};
		spDesc.VertexShader = &vs;
		spDesc.PixelShader = &ps;
		spDesc.VertexShaderBytecode = vertexShaderBytecode;
		spDesc.VertexElements = inputElements;
		spDesc.NumElements = 2;
		if (gd->CreateShaderPipeline(&spDesc, &result->Pipeline) != Nxna::NxnaResult::Success)
		{
			printf("Unable to create shader pipeline\n");
			delete[] result->Meshes;
			return -1;
		}

		Nxna::Graphics::RasterizerStateDesc rsDesc = NXNA_RASTERIZERSTATEDESC_DEFAULT;
		rsDesc.FrontCounterClockwise = true;
		if (gd->CreateRasterizerState(&rsDesc, &result->RasterState) != Nxna::NxnaResult::Success)
		{
			printf("Unable to create rasterizer state\n");
			delete[] result->Meshes;
			return -1;
		}

		// load the textures
		result->NumTextures = (uint32)materials.size();
		result->Textures = new Nxna::Graphics::Texture2D[result->NumTextures];
		for (size_t i = 0; i < materials.size(); i++)
		{
			Content::ContentState r;
			if (content == nullptr)
				r = Content::ContentManager::Load(materials[i].diffuse_texname.c_str(), Content::LoaderType::Texture2D, &result->Textures[i], "Content/Models/");
			else
				r = content->LoadTracked(materials[i].diffuse_texname.c_str(), Content::LoaderType::Texture2D, &result->Textures[i], "Content/Models/");

			if (r != Content::ContentState::Loaded)
			{
				printf("Error while loading %s\n", materials[i].diffuse_texname.c_str());
				result->Textures[i] = TextureLoader::GetErrorTexture(true);
			}
		}
	
		printf("Model loaded\n");
		return 0;
	}

	void Model::Render(Nxna::Graphics::GraphicsDevice* device, Nxna::Matrix* modelview, Model* models, uint32 numModels)
	{
		
		device->SetBlendState(nullptr);
		device->SetDepthStencilState(nullptr);
		device->SetSamplerState(0, &m_data->SamplerState);
		
		device->UpdateConstantBuffer(m_data->Constants, modelview->C, 16 * sizeof(float));
		device->SetConstantBuffer(m_data->Constants, 0);

		for (uint32 i = 0; i < numModels; i++)
		{
			device->SetRasterizerState(&models[i].RasterState);
			device->SetVertexBuffer(&models[i].Vertices, 0, models[i].VertexStride);
			device->SetShaderPipeline(&models[i].Pipeline);

			for (uint32 j = 0; j < models[i].NumMeshes; j++)
			{
				device->BindTexture(&models[i].Textures[models[i].Meshes[j].TextureIndex], 0);
				device->DrawPrimitives(Nxna::Graphics::PrimitiveType::TriangleList, models[i].Meshes[j].FirstIndex, models[i].Meshes[j].NumTriangles);
			}
		}
	}
}
