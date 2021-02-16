
#include "Render/render.hpp"

// #define RENDER_DOC_DEBUG

#ifdef RENDER_DOC_DEBUG

#include "TestCommon/renderdoc_app.h"

RENDERDOC_API_1_1_0* rdoc_api = NULL;

#endif


namespace v2
{

	inline void bufferstep_test()
	{
		GPUContext l_ctx = GPUContext::allocate();
		D3Renderer l_renderer = D3Renderer::allocate(l_ctx, ColorStep::AllocateInfo{ v3ui{ 8, 8, 1 }});

		Vertex l_test_vertices_raw[2];
		uint32 l_indices[2];
		Slice<Vertex> l_test_vertices = Slice<Vertex>::build_memory_elementnb(l_test_vertices_raw, 2);
		Slice<uint32> l_test_indices = Slice<uint32>::build_memory_elementnb(l_indices, 2);

		Token(RenderableObject) l_renderable_object =
				D3RendererAllocatorComposition::allocate_renderable_object_with_mesh(l_ctx.buffer_allocator, l_ctx.graphics_allocator, l_renderer.allocator,
						l_test_vertices, l_test_indices);
		l_renderer.heap().push_modeupdateevent(D3RendererHeap::RenderableObject_ModelUpdateEvent{ l_renderable_object, m44f_const::IDENTITY });

		l_renderer.buffer_step(l_ctx);

		assert_true(l_renderer.heap().model_update_events.Size == 0);

		l_ctx.buffer_step_and_submit();
		l_ctx.wait_for_completion();

		assert_true(
				l_ctx.buffer_allocator.get_bufferhost(
						l_ctx.graphics_allocator.heap.shader_uniform_buffer_host_parameters.get(l_renderer.allocator.heap.renderable_objects.get(l_renderable_object).model).memory
				).get_mapped_memory().compare(
						Slice<m44f>::build_asint8_memory_singleelement(&m44f_const::IDENTITY)
				)
		);

		Token(RenderableObject) l_renderable_object2 =
				D3RendererAllocatorComposition::allocate_renderable_object_with_mesh(l_ctx.buffer_allocator, l_ctx.graphics_allocator, l_renderer.allocator,
						l_test_vertices, l_test_indices);
		l_renderer.heap().push_modeupdateevent(D3RendererHeap::RenderableObject_ModelUpdateEvent{ l_renderable_object2, m44f_const::IDENTITY });

		assert_true(l_renderer.heap().model_update_events.Size == 1);

		D3RendererAllocatorComposition::free_renderable_object_with_mesh(l_ctx.buffer_allocator, l_ctx.graphics_allocator, l_renderer.allocator, l_renderable_object2);

		assert_true(l_renderer.heap().model_update_events.Size == 0);

		D3RendererAllocatorComposition::free_renderable_object_with_mesh(l_ctx.buffer_allocator, l_ctx.graphics_allocator, l_renderer.allocator, l_renderable_object);

		l_renderer.free(l_ctx);
		l_ctx.free();
	};

	inline void draw_test()
	{
		GPUContext l_ctx = GPUContext::allocate();
		D3Renderer l_renderer = D3Renderer::allocate(l_ctx, ColorStep::AllocateInfo{ v3ui{ 8, 8, 1 }});

#ifdef RENDER_DOC_DEBUG
		rdoc_api->StartFrameCapture(l_ctx.buffer_allocator.device.device, NULL);
#endif

		const int8* p_vertex_litteral =
				MULTILINE(\
                #version 450 \n

						layout(location = 0) in vec3 pos; \n
						layout(location = 1) in vec2 uv; \n

						struct Camera \n
				{ \n
						mat4 view; \n
						mat4 projection; \n
				}; \n

						layout(set = 0, binding = 0) uniform camera { Camera cam; }; \n
						layout(set = 2, binding = 0) uniform model { mat4 mod; }; \n

						void main()\n
				{ \n
						gl_Position = cam.projection * (cam.view * (mod * vec4(pos.xyz, 1.0f)));\n
				}\n
				);

		const int8* p_fragment_litteral =
				MULTILINE(\
                #version 450\n

						layout(location = 0) out vec4 outColor;\n

						layout(set = 1, binding = 0) uniform color { vec3 col; }; \n

						void main()\n
				{ \n
						outColor = vec4(col.xyz, 1.0f);\n
				}\n
				);

		ShaderCompiled l_vertex_shader_compiled = ShaderCompiled::compile(ShaderModuleStage::VERTEX, slice_int8_build_rawstr(p_vertex_litteral));
		ShaderCompiled l_fragment_shader_compiled = ShaderCompiled::compile(ShaderModuleStage::FRAGMENT, slice_int8_build_rawstr(p_fragment_litteral));

		Token(ShaderLayout) l_shader_layout;

		{
			ShaderLayoutParameterType l_layout_parameter_types[3] = { ShaderLayoutParameterType::UNIFORM_BUFFER_VERTEX, ShaderLayoutParameterType::UNIFORM_BUFFER_VERTEX_FRAGMENT, ShaderLayoutParameterType::UNIFORM_BUFFER_VERTEX };
			Span<ShaderLayoutParameterType> l_layout_parameter_types_span = Span<ShaderLayoutParameterType>::allocate_array<3>(l_layout_parameter_types);
			ShaderLayout::VertexInputParameter l_layout_verex_input[2] = { ShaderLayout::VertexInputParameter{ PrimitiveSerializedTypes::Type::FLOAT32_3, 0 },
																		   ShaderLayout::VertexInputParameter{ PrimitiveSerializedTypes::Type::FLOAT32_2, offsetof(Vertex, uv) }};
			Span<ShaderLayout::VertexInputParameter> l_layout_verex_input_span = Span<ShaderLayout::VertexInputParameter>::allocate_array<2>(l_layout_verex_input);
			l_shader_layout = l_ctx.graphics_allocator.allocate_shader_layout(l_layout_parameter_types_span, l_layout_verex_input_span, sizeof(Vertex));
		}

		Token(ShaderModule) l_vertex_shader_module = l_ctx.graphics_allocator.allocate_shader_module(l_vertex_shader_compiled.get_compiled_binary());
		Token(ShaderModule) l_fragment_shader_module = l_ctx.graphics_allocator.allocate_shader_module(l_fragment_shader_compiled.get_compiled_binary());

		l_vertex_shader_compiled.free();
		l_fragment_shader_compiled.free();


		ShaderAllocateInfo l_shader_allocate_info{
				l_ctx.graphics_allocator.heap.graphics_pass.get(l_renderer.color_step.pass),
				ShaderConfiguration{ 1, ShaderConfiguration::CompareOp::GreaterOrEqual },
				l_ctx.graphics_allocator.heap.shader_layouts.get(l_shader_layout),
				l_ctx.graphics_allocator.heap.shader_modules.get(l_vertex_shader_module),
				l_ctx.graphics_allocator.heap.shader_modules.get(l_fragment_shader_module)
		};

		Token(Shader) l_graphics_shader = l_ctx.graphics_allocator.allocate_shader(l_shader_allocate_info);
		Token(ShaderIndex) l_shader = l_renderer.allocator.allocate_shader(ShaderIndex{ 0, l_graphics_shader });


		Material l_red_material = Material::allocate_empty(l_ctx.graphics_allocator, 1);
		l_red_material.add_buffer_host_parameter_typed(l_ctx.graphics_allocator, l_ctx.buffer_allocator, l_ctx.graphics_allocator.heap.shader_layouts.get(l_shader_layout),
				v3f{ 1.0f, 0.0f, 0.0f });

		Material l_green_material = Material::allocate_empty(l_ctx.graphics_allocator, 1);
		l_green_material.add_buffer_host_parameter_typed(l_ctx.graphics_allocator, l_ctx.buffer_allocator, l_ctx.graphics_allocator.heap.shader_layouts.get(l_shader_layout),
				v3f{ 0.0f, 1.0f, 0.0f });

		Token(Material) l_red_material_token = l_renderer.allocator.allocate_material(l_red_material);
		Token(Material) l_green_material_token = l_renderer.allocator.allocate_material(l_green_material);

		l_renderer.heap().link_shader_with_material(l_shader, l_red_material_token);
		l_renderer.heap().link_shader_with_material(l_shader, l_green_material_token);

		Token(RenderableObject) l_obj_1, l_obj_2, l_obj_3, l_obj_4;

		{
			v3f l_positions[8] = {
					v3f{ -1.0f, -1.0f, 1.0f },
					v3f{ -1.0f, 1.0f, 1.0f },
					v3f{ -1.0f, -1.0f, -1.0f },
					v3f{ -1.0f, 1.0f, -1.0f },
					v3f{ 1.0f, -1.0f, 1.0f },
					v3f{ 1.0f, 1.0f, 1.0f },
					v3f{ 1.0f, -1.0f, -1.0f },
					v3f{ 1.0f, 1.0f, -1.0f }
			};

			v2f l_uvs[14] = {
					v2f{ 0.625f, 0.0f },
					v2f{ 0.375f, 0.25f },
					v2f{ 0.375f, 0.0f },
					v2f{ 0.625f, 0.25f },
					v2f{ 0.375f, 0.5f },
					v2f{ 0.625f, 0.5f },
					v2f{ 0.375f, 0.75f },
					v2f{ 0.625f, 0.75f },
					v2f{ 0.375f, 1.00f },
					v2f{ 0.125f, 0.75f },
					v2f{ 0.125f, 0.50f },
					v2f{ 0.875f, 0.50f },
					v2f{ 0.625f, 1.00f },
					v2f{ 0.875f, 0.75f }
			};

			Vertex l_vertices[14] = {
					Vertex{ l_positions[1], l_uvs[0] },
					Vertex{ l_positions[2], l_uvs[1] },
					Vertex{ l_positions[0], l_uvs[2] },
					Vertex{ l_positions[3], l_uvs[3] },
					Vertex{ l_positions[6], l_uvs[4] },
					Vertex{ l_positions[7], l_uvs[5] },
					Vertex{ l_positions[4], l_uvs[6] },
					Vertex{ l_positions[5], l_uvs[7] },
					Vertex{ l_positions[0], l_uvs[8] },
					Vertex{ l_positions[0], l_uvs[9] },
					Vertex{ l_positions[2], l_uvs[10] },
					Vertex{ l_positions[3], l_uvs[11] },
					Vertex{ l_positions[1], l_uvs[12] },
					Vertex{ l_positions[1], l_uvs[13] }
			};
			uint32 l_indices[14 * 3] = {
					0, 1, 2,
					3, 4, 1,
					5, 6, 4,
					7, 8, 6,
					4, 9, 10,
					11, 7, 5,
					0, 3, 1,
					3, 5, 4,
					5, 7, 6,
					7, 12, 8,
					4, 6, 9,
					11, 13, 7
			};
			l_obj_1 = D3RendererAllocatorComposition::allocate_renderable_object_with_mesh(l_ctx.buffer_allocator, l_ctx.graphics_allocator, l_renderer.allocator, Slice<Vertex>::build_memory_elementnb(l_vertices, 14),
					Slice<uint32>::build_memory_elementnb(l_indices, 14 * 3));
			l_obj_2 = D3RendererAllocatorComposition::allocate_renderable_object_with_mesh(l_ctx.buffer_allocator, l_ctx.graphics_allocator, l_renderer.allocator, Slice<Vertex>::build_memory_elementnb(l_vertices, 14),
					Slice<uint32>::build_memory_elementnb(l_indices, 14 * 3));
			l_obj_3 = D3RendererAllocatorComposition::allocate_renderable_object_with_mesh(l_ctx.buffer_allocator, l_ctx.graphics_allocator, l_renderer.allocator, Slice<Vertex>::build_memory_elementnb(l_vertices, 14),
					Slice<uint32>::build_memory_elementnb(l_indices, 14 * 3));
			l_obj_4 = D3RendererAllocatorComposition::allocate_renderable_object_with_mesh(l_ctx.buffer_allocator, l_ctx.graphics_allocator, l_renderer.allocator, Slice<Vertex>::build_memory_elementnb(l_vertices, 14),
					Slice<uint32>::build_memory_elementnb(l_indices, 14 * 3));
		}

		l_renderer.heap().link_material_with_renderable_object(l_red_material_token, l_obj_1);
		l_renderer.heap().link_material_with_renderable_object(l_red_material_token, l_obj_2);
		l_renderer.heap().link_material_with_renderable_object(l_green_material_token, l_obj_3);
		l_renderer.heap().link_material_with_renderable_object(l_green_material_token, l_obj_4);


		l_renderer.color_step.set_camera(l_ctx, Camera{ m44f_const::IDENTITY, m44f_const::IDENTITY });
		l_renderer.buffer_step(l_ctx);


		l_ctx.buffer_step_and_submit();

		GraphicsBinder l_binder = GraphicsBinder::build(l_ctx.buffer_allocator, l_ctx.graphics_allocator);
		l_binder.start();
		l_renderer.graphics_step(l_binder);
		l_binder.end();

		l_ctx.submit_graphics_binder(l_binder);
		l_ctx.wait_for_completion();


		D3RendererAllocatorComposition::free_shader_recursively_with_gpu_ressources(l_ctx.buffer_allocator, l_ctx.graphics_allocator, l_renderer.allocator, l_shader);

		l_ctx.graphics_allocator.free_shader_layout(l_shader_layout);
		l_ctx.graphics_allocator.free_shader_module(l_vertex_shader_module);
		l_ctx.graphics_allocator.free_shader_module(l_fragment_shader_module);

#ifdef RENDER_DOC_DEBUG
		rdoc_api->EndFrameCapture(l_ctx.buffer_allocator.device.device, NULL);
#endif

		l_renderer.free(l_ctx);
		l_ctx.free();
	};
};

int main()
{
#ifdef RENDER_DOC_DEBUG
	HMODULE mod = GetModuleHandleA("renderdoc.dll");

	pRENDERDOC_GetAPI RENDERDOC_GetAPI =
			(pRENDERDOC_GetAPI)GetProcAddress(mod, "RENDERDOC_GetAPI");
	int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_1_0, (void**)&rdoc_api);
	assert_true(ret == 1);
#endif

	v2::bufferstep_test();
	v2::draw_test();

	memleak_ckeck();
};
