#include "Model.h"
#include "tiny_obj_loader.h"
#include <sstream>

namespace Graphics
{
	bool Model::LoadObj(Nxna::Graphics::GraphicsDevice* device, uint8* data, uint32 length, Model* result)
	{
		std::string s((char*)data, length);
		std::stringstream ss(s);

		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		  
		std::string err;
		bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, &ss);

		if (!err.empty()) { // `err` may contain warning message.
			// TODO: log the error
		}

		if (!ret) {
		  exit(1);
		}

		// count the total number of vertices
		uint32 numVertices;
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

		result->NumMeshes = shapes.size();
		result->Meshes = new ModelMesh[shapes.size()];

		// Loop over shapes
		uint32 vertexCursor = 0;
		for (size_t s = 0; s < shapes.size(); s++)
		{
			
			result->Meshes[s].FirstIndex = vertexCursor;

			// Loop over faces(polygon)
			size_t index_offset = 0;
			for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++)
			{
				int fv = shapes[s].mesh.num_face_vertices[f];
				if (fv != 3)
					return false; // model must be triangulated

				// Loop over vertices in the face.
				for (size_t v = 0; v < fv; v++)
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
					vtx.V = attrib.texcoords[2*idx.texcoord_index+1];

					vertices[vertexCursor++] = vtx;
				}

				index_offset += fv;

				// per-face material
				shapes[s].mesh.material_ids[f];
			}

			result->Meshes[s].NumTriangles = index_offset;
		}

		Nxna::Graphics::VertexBufferDesc vbDesc = {};
		vbDesc.ByteLength = numVertices * sizeof(Vertex);
		vbDesc.InitialData = vertices;
		vbDesc.InitialDataByteCount = numVertices * sizeof(Vertex);
		if (device->CreateVertexBuffer(&vbDesc, &result->Vertices) != Nxna::NxnaResult::Success)
		{
			return false;
		}

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
				outputColor = texture(Diffuse, o_diffuseCoords) + vec4(1.0, 1.0, 1.0, 1.0);
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
		if (device->CreateShader(Nxna::Graphics::ShaderType::Vertex, vertexShaderBytecode.Bytecode, vertexShaderBytecode.BytecodeLength, &vs) != Nxna::NxnaResult::Success)
		{
			printf("Unable to create vertex shader\n");
			return false;
		}
		if (device->CreateShader(Nxna::Graphics::ShaderType::Pixel, pixelShaderBytecode.Bytecode, pixelShaderBytecode.BytecodeLength, &ps) != Nxna::NxnaResult::Success)
		{
			printf("Unable to create pixel shader\n");
			return false;
		}

		// now that the shaders have been created, put them into a ShaderPipeline
		Nxna::Graphics::ShaderPipelineDesc spDesc = {};
		spDesc.VertexShader = &vs;
		spDesc.PixelShader = &ps;
		spDesc.VertexShaderBytecode = vertexShaderBytecode;
		spDesc.VertexElements = inputElements;
		spDesc.NumElements = 2;
		if (device->CreateShaderPipeline(&spDesc, &result->Pipeline) != Nxna::NxnaResult::Success)
		{
			printf("Unable to create shader pipeline\n");
			return false;
		}

		Nxna::Graphics::ConstantBufferDesc cbDesc = {};
		cbDesc.InitialData = nullptr;
		cbDesc.ByteCount = sizeof(float) * 16;
		if (device->CreateConstantBuffer(&cbDesc, &result->Constants) != Nxna::NxnaResult::Success)
		{
			printf("Unable to create constant buffer\n");
			return -1;
		}

		Nxna::Graphics::RasterizerStateDesc rsDesc = NXNA_RASTERIZERSTATEDESC_DEFAULT;
		rsDesc.FrontCounterClockwise = true;
		if (device->CreateRasterizerState(&rsDesc, &result->RasterState) != Nxna::NxnaResult::Success)
		{
			printf("Unable to create rasterizer state\n");
			return -1;
		}

	
		printf("Model loaded\n");
		return true;
	}

	void Model::Render(Nxna::Graphics::GraphicsDevice* device, Nxna::Matrix* modelview, Model* models, uint32 numModels)
	{
		
		device->SetBlendState(nullptr);
		device->SetDepthStencilState(nullptr);

		for (uint32 i = 0; i < numModels; i++)
		{
			device->SetRasterizerState(&models[i].RasterState);
			device->SetVertexBuffer(&models[i].Vertices, 0, models[i].VertexStride);
			device->SetShaderPipeline(&models[i].Pipeline);

			device->UpdateConstantBuffer(models[i].Constants, modelview->C, 16 * sizeof(float));
			device->SetConstantBuffer(models[i].Constants, 0);

			for (uint j = 0; j < models[i].NumMeshes; j++)
			{
				device->DrawPrimitives(Nxna::Graphics::PrimitiveType::TriangleList, models[i].Meshes[j].FirstIndex, models[i].Meshes[j].NumTriangles);
			}
		}
	}
}
