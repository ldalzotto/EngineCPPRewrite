#pragma once

namespace v2
{
	struct RenderHeap
	{
		PoolHashedCounted<hash_t, ShaderModuleRessource> shader_modules_v2;
		PoolHashedCounted<hash_t, MeshRessource> mesh_v2;
		PoolHashedCounted<hash_t, ShaderRessource> shaders_v3;
		PoolHashedCounted<hash_t, MaterialRessource> materials;

		inline static RenderHeap allocate()
		{
			return RenderHeap{
					PoolHashedCounted<hash_t, ShaderModuleRessource>::allocate_default(),
					PoolHashedCounted<hash_t, MeshRessource>::allocate_default(),
					PoolHashedCounted<hash_t, ShaderRessource>::allocate_default(),
					PoolHashedCounted<hash_t, MaterialRessource>::allocate_default()
			};
		};

		inline void free()
		{
#if RENDER_BOUND_TEST
			assert_true(this->shaders_v3.empty());
			assert_true(this->mesh_v2.empty());
			assert_true(this->shaders_v3.empty());
			assert_true(this->materials.empty());
#endif
			this->shader_modules_v2.free();
			this->mesh_v2.free();
			this->shaders_v3.free();
			this->materials.free();
		};
	};

	struct RenderRessourceAllocator2
	{
		RenderHeap heap;

		struct MeshRendererAllocationEvent
		{
			Token(MeshRendererComponent)RessourceAllocationEvent_member_allocated_ressource;
		};

		Vector<ShaderModuleRessource::AllocationEvent> shadermodule_allocation_events;
		Vector<MeshRessource::AllocationEvent> mesh_allocation_events;
		Vector<ShaderRessource::AllocationEvent> shader_allocation_events;
		Vector<MaterialRessource::AllocationEvent> material_allocation_events;
		Vector<MeshRendererAllocationEvent> mesh_renderer_allocation_events;

		Vector<ShaderModuleRessource::FreeEvent> shadermodule_free_events;
		Vector<MeshRessource::FreeEvent> mesh_free_events;
		Vector<ShaderRessource::FreeEvent> shader_free_events;
		Vector<MaterialRessource::FreeEvent> material_free_events;
		Vector<MeshRendererComponent::FreeEvent> meshrenderer_free_events;

		PoolIndexed<MeshRendererComponent> mesh_renderers;
		CameraComponent camera_component;

		inline static RenderRessourceAllocator2 allocate()
		{
			return RenderRessourceAllocator2{
					RenderHeap::allocate(),
					Vector<ShaderModuleRessource::AllocationEvent>::allocate(0),
					Vector<MeshRessource::AllocationEvent>::allocate(0),
					Vector<ShaderRessource::AllocationEvent>::allocate(0),
					Vector<MaterialRessource::AllocationEvent>::allocate(0),
					Vector<MeshRendererAllocationEvent>::allocate(0),
					Vector<ShaderModuleRessource::FreeEvent>::allocate(0),
					Vector<MeshRessource::FreeEvent>::allocate(0),
					Vector<ShaderRessource::FreeEvent>::allocate(0),
					Vector<MaterialRessource::FreeEvent>::allocate(0),
					Vector<MeshRendererComponent::FreeEvent>::allocate(0),
					PoolIndexed<MeshRendererComponent>::allocate_default(),
					CameraComponent::build_default()
			};
		};

		inline void free()
		{
			this->heap.free();

#if RENDER_BOUND_TEST
			assert_true(this->shadermodule_allocation_events.empty());
			assert_true(this->mesh_allocation_events.empty());
			assert_true(this->shader_allocation_events.empty());
			assert_true(this->material_allocation_events.empty());
			assert_true(this->mesh_renderer_allocation_events.empty());
			assert_true(this->shadermodule_free_events.empty());
			assert_true(this->mesh_free_events.empty());
			assert_true(this->shader_free_events.empty());
			assert_true(this->material_free_events.empty());
			assert_true(this->meshrenderer_free_events.empty());
			assert_true(!this->mesh_renderers.has_allocated_elements());
			assert_true(!this->camera_component.allocated);
#endif

			this->shadermodule_allocation_events.free();
			this->mesh_allocation_events.free();
			this->shader_allocation_events.free();
			this->material_allocation_events.free();
			this->mesh_renderer_allocation_events.free();
			this->shadermodule_free_events.free();
			this->mesh_free_events.free();
			this->shader_free_events.free();
			this->material_free_events.free();
			this->meshrenderer_free_events.free();
			this->mesh_renderers.free();
		};

		inline void step(D3Renderer& p_renderer, GPUContext& p_gpu_context)
		{
			for (loop_reverse(i, 0, this->meshrenderer_free_events.Size))
			{
				auto& l_event = this->meshrenderer_free_events.get(i);
				MeshRendererComponent& l_mesh_renderer = this->mesh_renderers.get(l_event.component);
				MaterialRessource& l_linked_material = this->heap.materials.pool.get(l_mesh_renderer.dependencies.material);
				p_renderer.allocator.heap.unlink_material_with_renderable_object(l_linked_material.material, l_mesh_renderer.renderable_object);
				D3RendererAllocatorComposition::free_renderable_object_with_buffers(p_gpu_context.buffer_memory, p_gpu_context.graphics_allocator, p_renderer.allocator, l_mesh_renderer.renderable_object);
				this->mesh_renderers.release_element(l_event.component);
				this->meshrenderer_free_events.pop_back();
			}

			for (loop_reverse(i, 0, this->material_free_events.Size))
			{
				auto& l_event = this->material_free_events.get(i);
				MaterialRessource& l_ressource = this->heap.materials.pool.get(l_event.ressource);
				ShaderRessource& l_linked_shader = this->heap.shaders_v3.pool.get(l_ressource.dependencies.shader);
				p_renderer.allocator.heap.unlink_shader_with_material(l_linked_shader.shader, l_ressource.material);
				D3RendererAllocatorComposition::free_material_with_parameters(p_gpu_context.buffer_memory, p_gpu_context.graphics_allocator, p_renderer.allocator, l_ressource.material);
				this->heap.materials.pool.release_element(l_event.ressource);
				this->material_free_events.pop_back();
			}

			for (loop_reverse(i, 0, this->shader_free_events.Size))
			{
				auto& l_event = this->shader_free_events.get(i);
				ShaderRessource& l_ressource = this->heap.shaders_v3.pool.get(l_event.ressource);
				D3RendererAllocatorComposition::free_shader_with_shaderlayout(p_gpu_context.graphics_allocator, p_renderer.allocator, l_ressource.shader);
				this->heap.shaders_v3.pool.release_element(l_event.ressource);
				this->shader_free_events.pop_back();
			}

			for (loop_reverse(i, 0, this->mesh_free_events.Size))
			{
				auto& l_event = this->mesh_free_events.get(i);
				MeshRessource& l_ressource = this->heap.mesh_v2.pool.get(l_event.ressource);
				D3RendererAllocatorComposition::free_mesh_with_buffers(p_gpu_context.buffer_memory, p_renderer.allocator, l_ressource.mesh);
				this->heap.mesh_v2.pool.release_element(l_event.ressource);
				this->mesh_free_events.pop_back();
			}

			for (loop_reverse(i, 0, this->shadermodule_free_events.Size))
			{
				auto& l_event = this->shadermodule_free_events.get(i);
				ShaderModuleRessource& l_ressource = this->heap.shader_modules_v2.pool.get(l_event.ressource);
				p_gpu_context.graphics_allocator.free_shader_module(l_ressource.shader_module);
				this->heap.shader_modules_v2.pool.release_element(l_event.ressource);
				this->shadermodule_free_events.pop_back();
			}

			for (loop_reverse(i, 0, this->shadermodule_allocation_events.Size))
			{
				auto& l_event = this->shadermodule_allocation_events.get(i);
				ShaderModuleRessource& l_ressource = this->heap.shader_modules_v2.pool.get(l_event.allocated_ressource);
				l_ressource.shader_module = p_gpu_context.graphics_allocator.allocate_shader_module(l_event.asset.compiled_shader.slice);
				l_ressource.header.allocated = 1;
				l_event.asset.compiled_shader.free();
				this->shadermodule_allocation_events.pop_back();
			}

			for (loop_reverse(i, 0, this->mesh_allocation_events.Size))
			{
				auto& l_event = this->mesh_allocation_events.get(i);
				MeshRessource& l_ressource = this->heap.mesh_v2.pool.get(l_event.allocated_ressource);
				l_ressource.mesh = D3RendererAllocatorComposition::allocate_mesh_with_buffers(p_gpu_context.buffer_memory, p_renderer.allocator, l_event.asset.initial_vertices.slice, l_event.asset.initial_indices.slice);
				l_ressource.header.allocated = 1;
				l_event.asset.initial_vertices.free();
				l_event.asset.initial_indices.free();
				this->mesh_allocation_events.pop_back();
			}

			for (loop_reverse(i, 0, this->shader_allocation_events.Size))
			{
				auto& l_event = this->shader_allocation_events.get(i);
				ShaderRessource& l_ressource = this->heap.shaders_v3.pool.get(l_event.allocated_ressource);

				ShaderModuleRessource& l_vertex_shader = this->heap.shader_modules_v2.pool.get(l_ressource.dependencies.vertex_shader);
				ShaderModuleRessource& l_fragment_shader = this->heap.shader_modules_v2.pool.get(l_ressource.dependencies.fragment_shader);

				l_ressource.shader = D3RendererAllocatorComposition::allocate_colorstep_shader_with_shaderlayout(
						p_gpu_context.graphics_allocator, p_renderer.allocator, l_event.asset.specific_parameters.slice, l_event.asset.execution_order,
						p_gpu_context.graphics_allocator.heap.graphics_pass.get(p_renderer.color_step.pass),
						l_event.asset.shader_configuration,
						p_gpu_context.graphics_allocator.heap.shader_modules.get(l_vertex_shader.shader_module),
						p_gpu_context.graphics_allocator.heap.shader_modules.get(l_fragment_shader.shader_module)
				);
				l_event.asset.specific_parameters.free();
				l_ressource.header.allocated = 1;
				this->shader_allocation_events.pop_back();
			}

			for (loop_reverse(i, 0, this->material_allocation_events.Size))
			{
				auto& l_event = this->material_allocation_events.get(i);
				MaterialRessource& l_ressource = this->heap.materials.pool.get(l_event.allocated_ressource);

				ShaderRessource& l_shader = this->heap.shaders_v3.pool.get(l_ressource.dependencies.shader);

				l_ressource.material = p_renderer.allocator.allocate_material(Material::allocate_empty(p_gpu_context.graphics_allocator, 1));
				p_renderer.allocator.heap.link_shader_with_material(l_shader.shader, l_ressource.material);
				l_ressource.header.allocated = 1;
				this->material_allocation_events.pop_back();
			}

			for (loop_reverse(i, 0, this->mesh_renderer_allocation_events.Size))
			{
				auto& l_event = this->mesh_renderer_allocation_events.get(i);
				MeshRendererComponent& l_mesh_renderer = this->mesh_renderers.get(l_event.allocated_ressource);

				l_mesh_renderer.renderable_object = D3RendererAllocatorComposition::allocate_renderable_object_with_buffers(p_gpu_context.buffer_memory, p_gpu_context.graphics_allocator, p_renderer.allocator,
						this->heap.mesh_v2.pool.get(l_mesh_renderer.dependencies.mesh).mesh);
				p_renderer.allocator.heap.link_material_with_renderable_object(this->heap.materials.pool.get(l_mesh_renderer.dependencies.material).material,
						l_mesh_renderer.renderable_object);
				l_mesh_renderer.allocated = 1;

				this->mesh_renderer_allocation_events.pop_back();
			}
		};

		inline Token(ShaderModuleRessource) allocate_shadermodule_inline(const ShaderModuleRessource::InlineAllocationInput& p_shader_module)
		{
			return RessourceComposition::allocate_ressource_composition_explicit(this->heap.shader_modules_v2, this->shadermodule_allocation_events, p_shader_module.id, ShaderModuleRessource::build_from_id,
					[&p_shader_module](const Token(ShaderModuleRessource) p_allocated_ressource)
					{
						return ShaderModuleRessource::AllocationEvent::build_inline(p_shader_module.asset, p_allocated_ressource);
					});
		};

		inline void free_shadermodule(const ShaderModuleRessource& p_shader_module)
		{
			RessourceComposition::free_ressource_composition(
					this->heap.shader_modules_v2, this->shadermodule_allocation_events, this->shadermodule_free_events, p_shader_module.header,
					ShaderModuleRessource::FreeEvent::build_from_token
			);
		};

		inline Token(ShaderRessource) allocate_shader_v2_inline(const ShaderRessource::InlineAllocationInput& p_shader, const ShaderRessource::Dependencies& p_dependencies)
		{
			return RessourceComposition::allocate_ressource_composition_explicit(
					this->heap.shaders_v3, this->shader_allocation_events, p_shader.id, [&p_dependencies](hash_t p_id)
					{ return ShaderRessource{ RessourceIdentifiedHeader::build_with_id(p_id), tk_bd(ShaderIndex), ShaderRessource::Dependencies{ p_dependencies.vertex_shader, p_dependencies.fragment_shader }}; },
					[&p_shader](const Token(ShaderRessource) p_allocated_ressource)
					{
						return ShaderRessource::AllocationEvent::build_inline(p_shader.asset, p_allocated_ressource);
					}
			);
		};

		inline void free_shader(const ShaderRessource& p_shader)
		{
			RessourceComposition::free_ressource_composition_explicit(
					this->heap.shaders_v3, this->shader_allocation_events, this->shader_free_events, p_shader.header, ShaderRessource::FreeEvent::build_from_token, RessourceComposition::AllocationEventFoundSlot::FreeAsset{}
			);
		};

		inline Token(MeshRessource) allocate_mesh_inline(const MeshRessource::InlineAllocationInput& p_mesh)
		{
			return RessourceComposition::allocate_ressource_composition_explicit(
					this->heap.mesh_v2, this->mesh_allocation_events, p_mesh.id, MeshRessource::build_from_id, [&p_mesh](const Token(MeshRessource) p_allocated_ressource)
					{
						return MeshRessource::AllocationEvent::build_inline(p_mesh.asset, p_allocated_ressource);
					}
			);
		};

		inline void free_mesh(const MeshRessource& p_mesh)
		{
			RessourceComposition::free_ressource_composition_explicit(
					this->heap.mesh_v2, this->mesh_allocation_events, this->mesh_free_events, p_mesh.header, MeshRessource::FreeEvent::build_from_token, RessourceComposition::AllocationEventFoundSlot::FreeAsset{}
			);
		};

		inline Token(MaterialRessource) allocate_material_inline(const MaterialRessource::InlineRessourceInput& p_material_ressource, const MaterialRessource::Dependencies& p_dependencies)
		{
			return RessourceComposition::allocate_ressource_composition_explicit(
					this->heap.materials, this->material_allocation_events, p_material_ressource.id, [&p_dependencies](const hash_t p_id)
					{
						return MaterialRessource{ RessourceIdentifiedHeader::build_with_id(p_id), tk_bd(Material), p_dependencies };
					}, [&p_material_ressource](const Token(MaterialRessource) p_allocated_ressource)
					{
						return MaterialRessource::AllocationEvent::build_inline(p_material_ressource.asset, p_allocated_ressource);
					}
			);
		};

		inline void free_material(const MaterialRessource& p_material)
		{
			RessourceComposition::free_ressource_composition(
					this->heap.materials, this->material_allocation_events, this->material_free_events, p_material.header, [](Token(MaterialRessource) p_removed_token)
					{ return MaterialRessource::FreeEvent{ p_removed_token }; }
			);
		};

		inline Token(MeshRendererComponent) allocate_meshrenderer_inline(const MeshRendererComponent::Dependencies& p_dependencies, const Token(Node) p_scene_node)
		{
			Token(MeshRendererComponent) l_mesh_renderer = this->mesh_renderers.alloc_element(MeshRendererComponent::build(p_scene_node, p_dependencies));
			this->mesh_renderer_allocation_events.push_back_element(MeshRendererAllocationEvent{ l_mesh_renderer });
			return l_mesh_renderer;
		};

		inline void allocate_camera_inline(const CameraComponent::Asset& p_camera_component_asset, const Token(Node) p_scene_node)
		{
			this->camera_component.force_update = 1;
			this->camera_component.allocated = 1;
			this->camera_component.scene_node = p_scene_node;
			this->camera_component.asset = p_camera_component_asset;
		};

		inline void free_camera()
		{
			this->camera_component.allocated = 0;
		};
	};

	struct RenderRessourceAllocator2Composition
	{
		inline static Token(MeshRendererComponent) allocate_meshrenderer_inline_with_dependencies(RenderRessourceAllocator2& p_render_ressource_allocator,
				const ShaderModuleRessource::InlineAllocationInput& p_vertex_shader, const ShaderModuleRessource::InlineAllocationInput& p_fragment_shader,
				const ShaderRessource::InlineAllocationInput& p_shader, const MaterialRessource::InlineRessourceInput& p_material, const MeshRessource::InlineAllocationInput& p_mesh,
				const Token(Node) p_scene_node)
		{
			Token(ShaderModuleRessource) l_vertex_shader_module_ressource = p_render_ressource_allocator.allocate_shadermodule_inline(p_vertex_shader);
			Token(ShaderModuleRessource) l_fragment_shader_module_ressource = p_render_ressource_allocator.allocate_shadermodule_inline(p_fragment_shader);
			Token(ShaderRessource) l_shader_ressource = p_render_ressource_allocator.allocate_shader_v2_inline(p_shader, ShaderRessource::Dependencies{ l_vertex_shader_module_ressource, l_fragment_shader_module_ressource });
			Token(MaterialRessource) l_material_ressource = p_render_ressource_allocator.allocate_material_inline(p_material, MaterialRessource::Dependencies{ l_shader_ressource });
			Token(MeshRessource) l_mesh_ressource = p_render_ressource_allocator.allocate_mesh_inline(p_mesh);

			return p_render_ressource_allocator.allocate_meshrenderer_inline(MeshRendererComponent::Dependencies{ l_material_ressource, l_mesh_ressource }, p_scene_node);
		};

		inline static void free_meshrenderer(RenderRessourceAllocator2& p_render_ressource_allocator, const Token(MeshRendererComponent) p_mesh_renderer)
		{
			MeshRendererComponent& l_mesh_renderer = p_render_ressource_allocator.mesh_renderers.get(p_mesh_renderer);

			MaterialRessource& l_material_ressource = p_render_ressource_allocator.heap.materials.pool.get(l_mesh_renderer.dependencies.material);
			ShaderRessource& l_shader_ressource = p_render_ressource_allocator.heap.shaders_v3.pool.get(l_material_ressource.dependencies.shader);
			MeshRessource& l_mesh_ressource = p_render_ressource_allocator.heap.mesh_v2.pool.get(l_mesh_renderer.dependencies.mesh);
			ShaderModuleRessource& l_vertex_module_ressource = p_render_ressource_allocator.heap.shader_modules_v2.pool.get(l_shader_ressource.dependencies.vertex_shader);
			ShaderModuleRessource& l_fragment_module_ressource = p_render_ressource_allocator.heap.shader_modules_v2.pool.get(l_shader_ressource.dependencies.fragment_shader);

			if (l_mesh_renderer.allocated)
			{
				p_render_ressource_allocator.meshrenderer_free_events.push_back_element(MeshRendererComponent::FreeEvent{ p_mesh_renderer });
			}
			else
			{
				RessourceComposition::remove_reference_from_allocation_events(p_render_ressource_allocator.mesh_renderer_allocation_events, p_mesh_renderer);
				p_render_ressource_allocator.mesh_renderers.release_element(p_mesh_renderer);
			}

			p_render_ressource_allocator.free_material(l_material_ressource);
			p_render_ressource_allocator.free_shader(l_shader_ressource);
			p_render_ressource_allocator.free_mesh(l_mesh_ressource);
			p_render_ressource_allocator.free_shadermodule(l_vertex_module_ressource);
			p_render_ressource_allocator.free_shadermodule(l_fragment_module_ressource);
		};

	};

	struct RenderMiddleWare
	{
		RenderRessourceAllocator2 allocator;

		inline static RenderMiddleWare allocate()
		{
			return RenderMiddleWare{
					RenderRessourceAllocator2::allocate()
			};
		};

		inline void free(D3Renderer& p_renderer, GPUContext& p_gpu_context, Scene* p_scene)
		{
			this->step(p_renderer, p_gpu_context, p_scene);

			this->allocator.free();
		};

		inline void step(D3Renderer& p_renderer, GPUContext& p_gpu_context, Scene* p_scene)
		{
			this->allocator.step(p_renderer, p_gpu_context);

			for (loop(i, 0, this->allocator.mesh_renderers.Indices.Size))
			{
				MeshRendererComponent& l_mesh_renderer = this->allocator.mesh_renderers.get(this->allocator.mesh_renderers.Indices.get(i));
				NodeEntry l_node = p_scene->get_node(l_mesh_renderer.scene_node);
				if (l_mesh_renderer.force_update || l_node.Element->state.haschanged_thisframe)
				{
					p_renderer.heap().push_modelupdateevent(D3RendererHeap::RenderableObject_ModelUpdateEvent{
							l_mesh_renderer.renderable_object,
							p_scene->tree.get_localtoworld(l_node)
					});

					l_mesh_renderer.force_update = 0;
				}
			}

			if (this->allocator.camera_component.allocated)
			{
				NodeEntry l_camera_node = p_scene->get_node(this->allocator.camera_component.scene_node);
				if (this->allocator.camera_component.force_update)
				{
					p_renderer.color_step.set_camera_projection(p_gpu_context, this->allocator.camera_component.asset.Near, this->allocator.camera_component.asset.Far, this->allocator.camera_component.asset.Fov);

					m44f l_local_to_world = p_scene->tree.get_localtoworld(l_camera_node);
					p_renderer.color_step.set_camera_view(p_gpu_context, p_scene->tree.get_worldposition(l_camera_node), l_local_to_world.Forward.Vec3, l_local_to_world.Up.Vec3);
					this->allocator.camera_component.force_update = 0;
				}
				else if (l_camera_node.Element->state.haschanged_thisframe)
				{
					m44f l_local_to_world = p_scene->tree.get_localtoworld(l_camera_node);
					p_renderer.color_step.set_camera_view(p_gpu_context, p_scene->tree.get_worldposition(l_camera_node), l_local_to_world.Forward.Vec3, l_local_to_world.Up.Vec3);
				}
			}
		};
	};

};