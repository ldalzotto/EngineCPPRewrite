#pragma once

#include "AssetRessource/asset_ressource.hpp"
#include "./asset_types_json.hpp"
#include "./shader_compiler.hpp"

using namespace v2;

// TODO -> inserting asset dependencies

// TODO -> handling errors by using the shader compiler silent :)
inline Span<int8> AssetCompiler_compile_single_file(ShaderCompiler& p_shader_compiler, const File& p_asset_file)
{
    Slice<int8> l_asset_path = p_asset_file.path_slice;

    uimax l_last_dot_index = -1;
    while (l_asset_path.find(slice_int8_build_rawstr("."), &l_last_dot_index))
    {
        l_asset_path.slide(l_last_dot_index + 1);
    };

    if (l_asset_path.Size > 0)
    {
        if (l_asset_path.compare(slice_int8_build_rawstr("vert")))
        {
            Span<int8> l_buffer = p_asset_file.read_file_allocate();
            l_buffer.get(l_buffer.Capacity - 1) = (int8)NULL;
            ShaderCompiled l_compiled_shader = p_shader_compiler.compile_shader(ShaderModuleStage::VERTEX, l_buffer.slice);
            Span<int8> l_compiled_buffer = Span<int8>::allocate_slice(l_compiled_shader.get_compiled_binary());
            l_compiled_shader.free();
            l_buffer.free();
            return l_compiled_buffer;
        }
        else if (l_asset_path.compare(slice_int8_build_rawstr("frag")))
        {
            Span<int8> l_buffer = p_asset_file.read_file_allocate();
            l_buffer.get(l_buffer.Capacity - 1) = (int8)NULL;
            ShaderCompiled l_compiled_shader = p_shader_compiler.compile_shader(ShaderModuleStage::FRAGMENT, l_buffer.slice);
            Span<int8> l_compiled_buffer = Span<int8>::allocate_slice(l_compiled_shader.get_compiled_binary());
            l_compiled_shader.free();
            l_buffer.free();
            return l_compiled_buffer;
        }
        else if (l_asset_path.compare(slice_int8_build_rawstr("json")))
        {
            Span<int8> l_compiled_asset = Span<int8>::build_default();
            Span<int8> l_buffer = p_asset_file.read_file_allocate();
            String l_str = String::build_from_raw_span(l_buffer);
            JSONDeserializer l_json_deserializer = JSONDeserializer::start(l_str);

            l_json_deserializer.next_field("type");
            if (l_json_deserializer.get_currentfield().value.compare(slice_int8_build_rawstr("SHADER")))
            {
                JSONDeserializer l_json_obj_deserializer = JSONDeserializer::allocate_default();
                l_json_deserializer.next_object("val", &l_json_obj_deserializer);

                l_compiled_asset = ShaderAssetJSON::allocate_asset_from_json(&l_json_obj_deserializer);
                l_json_obj_deserializer.free();
            }

            l_json_deserializer.free();
            l_str.free();
            return l_compiled_asset;
            // TODO we want to handle json asset files, texture and models
        }
    }

    return Span<int8>::build_default();
};

inline Span<int8> AssetCompiler_compile_dependencies_of_file(ShaderCompiler& p_shader_compiler, const Slice<int8>& p_root_path, const File& p_asset_file)
{

    Slice<int8> l_asset_path = p_asset_file.path_slice;

    uimax l_last_dot_index = -1;
    while (l_asset_path.find(slice_int8_build_rawstr("."), &l_last_dot_index))
    {
        l_asset_path.slide(l_last_dot_index + 1);
    };

    if (l_asset_path.Size > 0)
    {
        if (l_asset_path.compare(slice_int8_build_rawstr("json")))
        {
            Span<int8> l_compiled_dependencies;
            Span<int8> l_buffer = p_asset_file.read_file_allocate();
            String l_str = String::build_from_raw_span(l_buffer);
            JSONDeserializer l_json_deserializer = JSONDeserializer::start(l_str);

            l_json_deserializer.next_field("type");
            if (l_json_deserializer.get_currentfield().value.compare(slice_int8_build_rawstr("SHADER")))
            {
                JSONDeserializer l_json_obj_deserializer = JSONDeserializer::allocate_default();
                l_json_deserializer.next_object("val", &l_json_obj_deserializer);

                l_compiled_dependencies = ShaderAssetJSON::allocate_dependencies_from_json(&l_json_obj_deserializer);
                l_json_obj_deserializer.free();
            }

            l_json_deserializer.free();
            l_str.free();
            return l_compiled_dependencies;
        }
    }

    return Span<int8>::build_default();
};

inline void AssetCompiler_compile_and_push_to_database_single_file(ShaderCompiler& p_shader_compiler, AssetDatabase& p_asset_database, const Slice<int8>& p_root_path,
                                                                   const Slice<int8>& p_relative_asset_path)
{
    Span<int8> l_asset_full_path = Span<int8>::allocate_slice_3(p_root_path, p_relative_asset_path, Slice<int8>::build("\0", 0, 1));
    File l_asset_file = File::open(l_asset_full_path.slice);
    Span<int8> l_compiled_asset = AssetCompiler_compile_single_file(p_shader_compiler, l_asset_file);
    if (l_compiled_asset.Memory)
    {
        p_asset_database.insert_or_update_asset_blob(p_relative_asset_path, l_compiled_asset.slice);
        l_compiled_asset.free();
    }
    Span<int8> l_compiled_dependencies = AssetCompiler_compile_dependencies_of_file(p_shader_compiler, p_root_path, l_asset_file);
    if (l_compiled_dependencies.Memory)
    {
        p_asset_database.insert_asset_dependencies_blob(l_asset_full_path.slice, l_compiled_dependencies.slice);
        l_compiled_dependencies.free();
    }

    l_asset_file.free();
    l_asset_full_path.free();
};