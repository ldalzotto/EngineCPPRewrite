#pragma once

inline uimax FrameCount(Engine& p_engine)
{
    return p_engine.clock.framecount;
};

inline Token(Node) CreateNode(Engine& p_engine, const transform& p_transform, const Token(Node) p_parent)
{
    return p_engine.scene.add_node(p_transform, p_parent);
};

inline Token(Node) CreateNode(Engine& p_engine, const transform& p_transform)
{
    return p_engine.scene.add_node(p_transform, Scene_const::root_node);
};

inline void RemoveNode(Engine& p_engine, const Token(Node) p_node)
{
    p_engine.scene.remove_node(p_engine.scene.get_node(p_node));
};

inline void NodeAddWorldRotation(Engine& p_engine, const Token(Node) p_node, const quat& p_delta_rotation)
{
    NTree<Node>::Resolve l_node = p_engine.scene.get_node(p_node);
    p_engine.scene.tree.add_worldrotation(l_node, p_delta_rotation);
};

inline CameraComponent& NodeAddCamera(Engine& p_engine, const Token(Node) p_node, const CameraComponent::Asset& p_camera_asset)
{
    p_engine.scene_middleware.render_middleware.allocate_camera_inline(p_camera_asset, p_node);
    p_engine.scene.add_node_component_by_value(p_node, CameraComponentAsset_SceneCommunication::construct_nodecomponent());
    return p_engine.scene_middleware.render_middleware.camera_component;
};

inline Token(MeshRendererComponent) NodeAddMeshRenderer(Engine& p_engine, const Token(Node) p_node, const hash_t p_material_id, const hash_t p_mesh_id)
{
    Token(MeshRendererComponent) l_mesh_renderer = RenderMiddleWare_AllocationComposition::allocate_meshrenderer_database_and_load_dependecies(
        p_engine.scene_middleware.render_middleware, p_engine.renderer_ressource_allocator, p_engine.asset_database, MeshRendererComponent::AssetDependencies{p_material_id, p_mesh_id}, p_node);
    p_engine.scene.add_node_component_by_value(p_node, MeshRendererComponentAsset_SceneCommunication::construct_nodecomponent(l_mesh_renderer));
    return l_mesh_renderer;
};


