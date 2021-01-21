#pragma once

namespace v2
{
	struct MathJSONDeserialization
	{
		inline static v3f _v3f(JSONDeserializer* p_json_object)
		{
			v3f l_return;
			json_deser_object_field("x", p_json_object, l_return.x, FromString::afloat32);
			json_deser_object_field("y", p_json_object, l_return.y, FromString::afloat32);
			json_deser_object_field("z", p_json_object, l_return.z, FromString::afloat32);
			return l_return;
		};

		inline static quat _quat(JSONDeserializer* p_json_object)
		{
			quat l_return;
			json_deser_object_field("x", p_json_object, l_return.x, FromString::afloat32);
			json_deser_object_field("y", p_json_object, l_return.y, FromString::afloat32);
			json_deser_object_field("z", p_json_object, l_return.z, FromString::afloat32);
			json_deser_object_field("w", p_json_object, l_return.w, FromString::afloat32);
			return l_return;
		};
	};

	struct SceneTreeAsset
	{
		NTree<transform> nodes;

		//TODO -> we can replace the HeapMemory and the VectorOfVector by a VectorOfVaryingVector
		HeapMemory component_asset_heap;
		VectorOfVector<Token(SliceIndex)> node_to_components;

		inline static SceneTreeAsset allocate_default()
		{
			return SceneTreeAsset{
				NTree<transform>::allocate_default(),
				HeapMemory::allocate_default(),
				VectorOfVector<Token(SliceIndex)>::allocate_default()
			};
		};

		inline void free()
		{
			this->nodes.free();
			this->component_asset_heap.free();
			this->node_to_components.free();
		};

		inline Token(transform) add_node_without_parent(const transform& p_node_local_transform)
		{
			Token(transform) l_node = this->nodes.push_root_value(p_node_local_transform);
			this->node_to_components.push_back();
			return l_node;
		};

		inline Token(transform) add_node(const transform& p_node_local_transform, const Token(transform) p_parent)
		{
			Token(transform) l_node = this->nodes.push_value(p_node_local_transform, p_parent);
			this->node_to_components.push_back();
			return l_node;
		};

		inline void add_component(const Token(transform) p_node, const Slice<int8>& p_component_memory)
		{
			this->node_to_components.element_push_back_element(tk_v(p_node), this->component_asset_heap.allocate_element(p_component_memory));
		};
	};

	namespace SceneSerialization_const
	{
		const Slice<int8> scene_json_type = slice_int8_build_rawstr("scene");
		const int8* scene_nodes_field = "nodes";
		const int8* node_local_position_field = "local_position";
		const int8* node_local_rotation_field = "local_rotation";
		const int8* node_local_scale_field = "local_scale";
		const int8* node_components_field = "components";
		const int8* node_childs_field = "childs";

		const int8* node_component_type_field = "type";
		const int8* node_component_object_field = "object";
	};

	struct SceneDeserialization
	{
		template<class ComponentDeserializationFunc>
		inline static void json_to_SceneTreeAsset(JSONDeserializer& p_scene_deserializer, SceneTreeAsset* out_SceneAssetTree)
		{
			json_get_type_checked(p_scene_deserializer, SceneSerialization_const::scene_json_type);
			build_SceneTree_from_JSONNodes<ComponentDeserializationFunc>(p_scene_deserializer, out_SceneAssetTree);
			p_scene_deserializer.free();
		};

	private:

		template<class ComponentDeserializationFunc>
		inline static void build_SceneTreeAsset_from_JSONNodes(JSONDeserializer& p_nodes, SceneTreeAsset* in_out_SceneAssetTree)
		{
			struct Stack
			{
				Vector<JSONDeserializer> node_object_iterator;
				Vector<Token(transform)> allocated_nodes;
				Vector<JSONDeserializer> node_childs_serializers;

				inline static Stack allocate_default()
				{
					return Stack{
					 Vector<JSONDeserializer>::allocate(0),
					 Vector<Token(transform)>::allocate(0),
					 Vector<JSONDeserializer>::allocate(0) 
					};
				};

				inline void free()
				{
					this->node_object_iterator.free();
					this->allocated_nodes.free();
					this->node_childs_serializers.free();
				};

				inline void push_stack(const Token(transform) p_allocated_node, const JSONDeserializer& p_node_object_iterator)
				{
					this->node_object_iterator.push_back_element(p_node_object_iterator);
					this->allocated_nodes.push_back_element(p_allocated_node);
					this->node_childs_serializers.push_back_element_empty();
				};

				inline void pop_stack()
				{
					this->node_object_iterator.pop_back();
					this->allocated_nodes.pop_back();
					this->node_childs_serializers.pop_back();
				};

				inline JSONDeserializer& get_node_object()
				{
					return this->node_object_iterator.get(this->node_object_iterator.Size - 1);
				};

				inline Token(transform) get_allocated_node()
				{
					return this->allocated_nodes.get(this->allocated_nodes.Size - 1);
				};
			};

			Stack l_stack = Stack::allocate_default();

			in_out_SceneAssetTree->add_node_without_parent(transform_const::ORIGIN);

			json_deser_iterate_array_start(SceneSerialization_const::scene_nodes_field, &p_nodes);
			{

				transform l_transform;
				Token(transform) l_allocated_node;
				JSONDeserializer l_object_iterator = JSONDeserializer::allocate_default();

				l_transform = deserialize_node_transform(l_object, l_object_iterator);
				l_allocated_node = in_out_SceneAssetTree->add_node(l_transform, tk_b(transform, 0));
				deserialize_components<ComponentDeserializationFunc>(l_object, l_object_iterator, l_allocated_node, in_out_SceneAssetTree);

				l_object.next_array(SceneSerialization_const::node_childs_field, &l_object_iterator);

				l_stack.push_stack(l_allocated_node, l_object_iterator.clone());

				while (l_stack.node_object_iterator.Size != 0)
				{

					JSONDeserializer& l_current_node = l_stack.get_node_object();

					JSONDeserializer l_child_node_iterator = JSONDeserializer::allocate_default();
					if (l_current_node.next_array_object(&l_child_node_iterator))
					{
						l_transform = deserialize_node_transform(l_child_node_iterator, l_object_iterator);
						l_allocated_node = in_out_SceneAssetTree->add_node(l_transform, l_stack.get_allocated_node());
						deserialize_components<ComponentDeserializationFunc>(l_child_node_iterator, l_object_iterator, l_allocated_node, in_out_SceneAssetTree);

						l_child_node_iterator.next_array(SceneSerialization_const::node_childs_field, &l_object_iterator);
						l_stack.push_stack(l_allocated_node, l_object_iterator.clone());
					}
					else
					{
						l_stack.pop_stack();
					};
					l_child_node_iterator.free();
				}
			}
			json_deser_iterate_array_end();

			l_stack.free();
		};

		inline static transform deserialize_node_transform(JSONDeserializer& p_node, JSONDeserializer& p_tmp_iterator)
		{
			transform l_tranfsorm;
			json_deser_object(SceneSerialization_const::node_local_position_field, &p_node, &p_tmp_iterator, l_tranfsorm.position, MathJSONDeserialization::_v3f);
			json_deser_object(SceneSerialization_const::node_local_rotation_field, &p_node, &p_tmp_iterator, l_tranfsorm.rotation, MathJSONDeserialization::_quat);
			json_deser_object(SceneSerialization_const::node_local_scale_field, &p_node, &p_tmp_iterator, l_tranfsorm.scale, MathJSONDeserialization::_v3f);
			return l_tranfsorm;
		};

		template<class ComponentDeserializationFunc>
		inline static void deserialize_components(JSONDeserializer& p_node, JSONDeserializer& p_tmp_iterator, const Token(transform) p_node_token, SceneTreeAsset* in_in_out_SceneAssetTreeout_scene)
		{
			json_deser_iterate_array_start(SceneSerialization_const::node_components_field, &p_node);
			{
				json_deser_iterate_array_object.next_field(SceneSerialization_const::node_component_type_field);
				Slice<int8> l_type = json_deser_iterate_array_object.get_currentfield().value;
				JSONDeserializer l_component_object_iterator = JSONDeserializer::allocate_default();
				json_deser_iterate_array_object.next_object(SceneSerialization_const::node_component_object_field, &l_component_object_iterator);
				//TODO The type should be hashed
				ComponentDeserializationFunc::push_to_scene(l_component_object_iterator, l_type, p_node_token, in_out_SceneAssetTree);
				l_component_object_iterator.free();
			}
			json_deser_iterate_array_end();
		};
	};


};