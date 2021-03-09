#pragma once

namespace v2
{
enum class ShaderLayoutParameterType
{
    UNDEFINED = 0,
    UNIFORM_BUFFER_VERTEX = 1,
    UNIFORM_BUFFER_VERTEX_FRAGMENT = 2,
    TEXTURE_FRAGMENT = 3,
};

/*
     ShaderLayoutParameters are vulkan internal types of shader parameters.
     It indicates the type of parameter (BUFFER, SAMPLER) and it's binding value (always 0 in this implementation).
*/
struct ShaderLayoutParameters
{
    struct Binding0
    {
        VkDescriptorSetLayout uniformbuffer_vertex_layout;
        VkDescriptorSetLayout uniformbuffer_fragment_vertex_layout;
        VkDescriptorSetLayout texture_fragment_layout;
    };

    Binding0 parameter_set;

    static ShaderLayoutParameters allocate(gc_t const p_device);

    void free(gc_t const p_device);

    VkDescriptorSetLayout get_descriptorset_layout(const ShaderLayoutParameterType p_shader_layout_parameter_type) const;

  private:
    VkDescriptorSetLayout create_layout(gc_t const p_device, const ShaderLayoutParameterType p_shader_layout_parameter_type, const uint32 p_shader_binding);

    VkDescriptorSetLayoutBinding create_binding(const uint32 p_shader_binding, const VkDescriptorType p_descriptor_type, const VkShaderStageFlags p_shader_stage);
};

struct ShaderParameterPool
{
    VkDescriptorPool descriptor_pool;

    static ShaderParameterPool allocate(const gc_t p_device, const uimax p_max_sets);

    void free(const gc_t p_device);
};

typedef VkSampler TextureSampler;

struct TextureSamplers
{
    TextureSampler Default;

    static TextureSamplers allocate(const gc_t p_device);

    void free(const gc_t p_device);
};

/*
    The GraphicsDevice handles all operations performed in the graphics queue.
    It also store all constants related to graphics pipeline.
*/
struct GraphicsDevice
{
    GraphicsCard graphics_card;
    gc_t device;
    gcqueue_t graphics_queue;

    CommandPool command_pool;
    CommandBuffer command_buffer;

    ShaderParameterPool shaderparameter_pool;
    ShaderLayoutParameters shaderlayout_parameters;
    TextureSamplers texture_samplers;

    static GraphicsDevice allocate(GPUInstance& p_instance);

    void free();
};

typedef VkImageView ImageView_t;

/*
    A Texture is an image on which we indicate how it will be sampled in the shader.
    This involves mip_level for example.
    For now, mip_level is always 0 but in the future we want to manage multiple levels.
*/
struct TextureGPU
{
    Token(ImageGPU) Image;
    ImageView_t ImageView;

    static TextureGPU allocate(BufferMemory& p_buffer_memory, const ImageFormat& p_image_format);

    void free(BufferMemory& p_buffer_memory);
};

typedef VkRenderPass RenderPass_t;

enum class AttachmentType
{
    COLOR,
    DEPTH,
    KHR
};

/*
    Attachments are render targets of Shader.
*/
struct RenderPassAttachment
{
    AttachmentType type;
    ImageFormat image_format;
};

/*
    The RenderPass is the layout of the GraphicsPass.
    It indicates what attachments the GraphicsPass will write to.
*/
struct RenderPass
{
    RenderPass_t render_pass;

    template <uint8 AttachmentCount> static RenderPass allocate(const GraphicsDevice& p_device, const SliceN<RenderPassAttachment, AttachmentCount>& p_attachments);

    void free(const GraphicsDevice& p_device);
};

typedef VkFramebuffer FrameBuffer_t;

/*
     The GraphicsPass is the RenderPass with the allocated attachment textures
*/
struct GraphicsPass
{
#if GPU_DEBUG
    Span<RenderPassAttachment> attachement_layout;
#endif
    RenderPass render_pass;
    Token(Slice<Token(TextureGPU)>) attachment_textures;
    FrameBuffer_t frame_buffer;

    template <uint32 AttachmentCount>
    static GraphicsPass allocate(const TransferDevice& p_transfer_device, const GraphicsDevice& p_graphics_device, const Token(Slice<Token(TextureGPU)>) p_allocated_attachment_textures,
                                 const SliceN<RenderPassAttachment, AttachmentCount>& p_render_pass_attachments, const VkImageView* p_attachment_image_views);

    void free(const GraphicsDevice& p_graphics_device);
};

typedef VkPipelineLayout ShaderLayout_t;

/*
    The ShaderLayout indicate the format of all parameters of the Shader.
    It can be seen as the Reflection object of a Shader.
*/
struct ShaderLayout
{
    struct VertexInputParameter
    {
        PrimitiveSerializedTypes::Type type;
        uimax offset;
    };

    ShaderLayout_t layout;

    Span<ShaderLayoutParameterType> shader_layout_parameter_types;
    Span<VertexInputParameter> vertex_input_layout;
    uimax vertex_element_size;

    static ShaderLayout allocate(const GraphicsDevice& p_device, const Span<ShaderLayoutParameterType>& in_shaderlayout_parameter_types, const Span<VertexInputParameter>& in_vertex_input_layout,
                                 const uimax in_vertex_element_size);

    void free(const GraphicsDevice& p_device);
};

enum class ShaderModuleStage
{
    UNDEFINED = 0,
    VERTEX = 1,
    FRAGMENT = 2
};

typedef VkShaderModule ShaderModule_t;

/*
    A  ShaderModule is the compiled small unit of (vertex|fragment) shader pass.
*/
struct ShaderModule
{
    ShaderModule_t module;

    static ShaderModule allocate(const GraphicsDevice& p_graphics_device, const Slice<int8>& p_shader_compiled_str);

    static ShaderModule build_default();

    void free(const GraphicsDevice& p_graphics_device);
};

struct ShaderConfiguration
{
    enum class CompareOp
    {
        Never = 0,
        Less = 1,
        Equal = 2,
        LessOrEqual = 3,
        Greater = 4,
        NotEqual = 5,
        GreaterOrEqual = 6,
        Always = 7,
        Invalid = 8
    };

    char zwrite;
    CompareOp ztest;
};

struct ShaderAllocateInfo
{
    const GraphicsPass& graphics_pass;
    const ShaderConfiguration& shader_configuration;
    const ShaderLayout& shader_layout;
    ShaderModule vertex_shader;
    ShaderModule fragment_shader;
};

struct Shader
{
    VkPipeline shader;
    ShaderLayout layout;

    static Shader allocate(const GraphicsDevice& p_device, const ShaderAllocateInfo& p_shader_allocate_info);

    void free(const GraphicsDevice& p_device);

  private:
    static VkFormat get_primitivetype_format(const PrimitiveSerializedTypes::Type p_primitive_type);
};

#define ShadowShaderUniformBufferParameter_t(Prefix) ShadowShaderUniformBufferParameter_##Prefix
#define ShadowShaderUniformBufferParameter_member_descriptor_set descriptor_set
#define ShadowShaderUniformBufferParameter_member_memory memory
#define ShadowShaderUniformBufferParameter_method_free free
#define ShadowShaderUniformBufferParameter_methodcall_free(const_ref_buffer_memory) ShadowShaderUniformBufferParameter_method_free(const_ref_buffer_memory)

namespace ShadowShaderUniformBufferParameter
{
template <class ShadowShaderUniformBufferParameter_t(_), class ShadowBuffer_t(_)>
static ShadowShaderUniformBufferParameter_t(_) allocate(const GraphicsDevice& p_graphics_device, const VkDescriptorSetLayout p_descriptor_set_layout,
                                                        const Token(ShadowBuffer_t(_)) p_buffer_memory_token, const ShadowBuffer_t(_) & p_buffer_memory);
};

struct ShaderUniformBufferHostParameter
{
    VkDescriptorSet ShadowShaderUniformBufferParameter_member_descriptor_set;
    Token(BufferHost) ShadowShaderUniformBufferParameter_member_memory;

    static ShaderUniformBufferHostParameter allocate(const GraphicsDevice& p_graphics_device, const VkDescriptorSetLayout p_descriptor_set_layout, const Token(BufferHost) p_buffer_memory_token,
                                                     const BufferHost& p_buffer_memory);

    void ShadowShaderUniformBufferParameter_method_free(const GraphicsDevice& p_graphics_device);
};

struct ShaderUniformBufferGPUParameter
{
    VkDescriptorSet ShadowShaderUniformBufferParameter_member_descriptor_set;
    Token(BufferGPU) ShadowShaderUniformBufferParameter_member_memory;

    static ShaderUniformBufferGPUParameter allocate(const GraphicsDevice& p_graphics_device, const VkDescriptorSetLayout p_descriptor_set_layout, const Token(BufferGPU) p_buffer_memory_token,
                                                    const BufferGPU& p_buffer_memory);

    void ShadowShaderUniformBufferParameter_method_free(const GraphicsDevice& p_graphics_device);
};

struct ShaderTextureGPUParameter
{
    VkDescriptorSet descriptor_set;
    Token(TextureGPU) texture;

    static ShaderTextureGPUParameter allocate(const GraphicsDevice& p_graphics_device, const VkDescriptorSetLayout p_descriptor_set_layout, const Token(TextureGPU) p_texture_token,
                                              const TextureGPU& p_texture);

    void free(const GraphicsDevice& p_graphics_device);
};

struct ShaderParameter
{
    enum class Type
    {
        UNKNOWN = 0,
        UNIFORM_HOST = 1,
        UNIFORM_GPU = 2,
        TEXTURE_GPU = 3
    } type;

    union
    {
        Token(ShaderUniformBufferHostParameter) uniform_host;
        Token(ShaderUniformBufferGPUParameter) uniform_gpu;
        Token(ShaderTextureGPUParameter) texture_gpu;
    };
};

struct GraphicsHeap2
{
    Pool<TextureGPU> textures_gpu;
    PoolOfVector<Token(TextureGPU)> renderpass_attachment_textures;
    Pool<GraphicsPass> graphics_pass;
    Pool<ShaderLayout> shader_layouts;
    Pool<ShaderModule> shader_modules;
    Pool<Shader> shaders;

    Pool<ShaderUniformBufferHostParameter> shader_uniform_buffer_host_parameters;
    Pool<ShaderUniformBufferGPUParameter> shader_uniform_buffer_gpu_parameters;
    Pool<ShaderTextureGPUParameter> shader_texture_gpu_parameters;

    PoolOfVector<ShaderParameter> material_parameters;

    inline static GraphicsHeap2 allocate_default()
    {
        return GraphicsHeap2{Pool<TextureGPU>::allocate(0),
                             PoolOfVector<Token(TextureGPU)>::allocate_default(),
                             Pool<GraphicsPass>::allocate(0),
                             Pool<ShaderLayout>::allocate(0),
                             Pool<ShaderModule>::allocate(0),
                             Pool<Shader>::allocate(0),
                             Pool<ShaderUniformBufferHostParameter>::allocate(0),
                             Pool<ShaderUniformBufferGPUParameter>::allocate(0),
                             Pool<ShaderTextureGPUParameter>::allocate(0),
                             PoolOfVector<ShaderParameter>::allocate_default()};
    };

    inline void free()
    {
#if GPU_DEBUG
        assert_true(!this->textures_gpu.has_allocated_elements());
        assert_true(!this->renderpass_attachment_textures.has_allocated_elements());
        assert_true(!this->graphics_pass.has_allocated_elements());
        assert_true(!this->shader_layouts.has_allocated_elements());
        assert_true(!this->shader_modules.has_allocated_elements());
        assert_true(!this->shaders.has_allocated_elements());
        assert_true(!this->shader_uniform_buffer_host_parameters.has_allocated_elements());
        assert_true(!this->shader_uniform_buffer_gpu_parameters.has_allocated_elements());
        assert_true(!this->shader_texture_gpu_parameters.has_allocated_elements());
        assert_true(!this->material_parameters.has_allocated_elements());
#endif

        this->textures_gpu.free();
        this->renderpass_attachment_textures.free();
        this->graphics_pass.free();
        this->shader_layouts.free();
        this->shader_modules.free();
        this->shaders.free();
        this->shader_uniform_buffer_host_parameters.free();
        this->shader_uniform_buffer_gpu_parameters.free();
        this->shader_texture_gpu_parameters.free();
        this->material_parameters.free();
    };
};

inline ShaderLayoutParameters ShaderLayoutParameters::allocate(gc_t const p_device)
{
    ShaderLayoutParameters l_shader_layout_parameters;
    l_shader_layout_parameters.parameter_set = Binding0{l_shader_layout_parameters.create_layout(p_device, ShaderLayoutParameterType::UNIFORM_BUFFER_VERTEX, (uint32)0),
                                                        l_shader_layout_parameters.create_layout(p_device, ShaderLayoutParameterType::UNIFORM_BUFFER_VERTEX_FRAGMENT, (uint32)0),
                                                        l_shader_layout_parameters.create_layout(p_device, ShaderLayoutParameterType::TEXTURE_FRAGMENT, (uint32)0)};
    return l_shader_layout_parameters;
};

inline void ShaderLayoutParameters::free(gc_t const p_device)
{
    vkDestroyDescriptorSetLayout(p_device, this->parameter_set.uniformbuffer_vertex_layout, NULL);
    vkDestroyDescriptorSetLayout(p_device, this->parameter_set.uniformbuffer_fragment_vertex_layout, NULL);
    vkDestroyDescriptorSetLayout(p_device, this->parameter_set.texture_fragment_layout, NULL);
};

inline VkDescriptorSetLayout ShaderLayoutParameters::get_descriptorset_layout(const ShaderLayoutParameterType p_shader_layout_parameter_type) const
{
    switch (p_shader_layout_parameter_type)
    {
    case ShaderLayoutParameterType::UNIFORM_BUFFER_VERTEX:
        return this->parameter_set.uniformbuffer_vertex_layout;
    case ShaderLayoutParameterType::UNIFORM_BUFFER_VERTEX_FRAGMENT:
        return this->parameter_set.uniformbuffer_fragment_vertex_layout;
    case ShaderLayoutParameterType::TEXTURE_FRAGMENT:
        return this->parameter_set.texture_fragment_layout;
    default:
        abort();
    }
};

inline VkDescriptorSetLayout ShaderLayoutParameters::create_layout(gc_t const p_device, const ShaderLayoutParameterType p_shader_layout_parameter_type, const uint32 p_shader_binding)
{
    VkDescriptorType l_descriptor_type;
    VkShaderStageFlags l_shader_stage;
    switch (p_shader_layout_parameter_type)
    {
    case ShaderLayoutParameterType::UNIFORM_BUFFER_VERTEX:
        l_descriptor_type = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        l_shader_stage = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;
        break;
    case ShaderLayoutParameterType::UNIFORM_BUFFER_VERTEX_FRAGMENT:
        l_descriptor_type = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        l_shader_stage = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT | VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
        break;
    case ShaderLayoutParameterType::TEXTURE_FRAGMENT:
        l_descriptor_type = VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        l_shader_stage = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
        break;
    default:
        abort();
    }

    VkDescriptorSetLayoutBinding l_binding = this->create_binding(p_shader_binding, l_descriptor_type, l_shader_stage);

    VkDescriptorSetLayoutCreateInfo l_descriptorset_layot_create{};
    l_descriptorset_layot_create.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    l_descriptorset_layot_create.bindingCount = 1;
    l_descriptorset_layot_create.pBindings = &l_binding;

    VkDescriptorSetLayout l_layout;
    vk_handle_result(vkCreateDescriptorSetLayout(p_device, &l_descriptorset_layot_create, NULL, &l_layout));
    return l_layout;
};

inline VkDescriptorSetLayoutBinding ShaderLayoutParameters::create_binding(const uint32 p_shader_binding, const VkDescriptorType p_descriptor_type, const VkShaderStageFlags p_shader_stage)
{
    VkDescriptorSetLayoutBinding l_camera_matrices_layout_binding{};
    l_camera_matrices_layout_binding.binding = p_shader_binding;
    l_camera_matrices_layout_binding.descriptorCount = 1;
    l_camera_matrices_layout_binding.descriptorType = p_descriptor_type;
    l_camera_matrices_layout_binding.stageFlags = p_shader_stage;
    l_camera_matrices_layout_binding.pImmutableSamplers = nullptr;
    return l_camera_matrices_layout_binding;
};

inline ShaderParameterPool ShaderParameterPool::allocate(const gc_t p_device, const uimax p_max_sets)
{
    VkDescriptorPoolSize l_types{};
    l_types.descriptorCount = 4;

    VkDescriptorPoolCreateInfo l_descriptor_pool_create_info{};
    l_descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    l_descriptor_pool_create_info.poolSizeCount = 1;
    l_descriptor_pool_create_info.pPoolSizes = &l_types;
    l_descriptor_pool_create_info.flags = VkDescriptorPoolCreateFlagBits::VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    l_descriptor_pool_create_info.maxSets = (uint32_t)p_max_sets;

    ShaderParameterPool l_shader_parameter_pool;
    vk_handle_result(vkCreateDescriptorPool(p_device, &l_descriptor_pool_create_info, NULL, &l_shader_parameter_pool.descriptor_pool));
    return l_shader_parameter_pool;
};

inline void ShaderParameterPool::free(const gc_t p_device)
{
    vkDestroyDescriptorPool(p_device, this->descriptor_pool, NULL);
};

inline TextureSamplers TextureSamplers::allocate(const gc_t p_device)
{
    TextureSamplers l_samplers;

    VkSamplerCreateInfo l_sampler_create_info{};
    l_sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    l_sampler_create_info.magFilter = VkFilter::VK_FILTER_NEAREST;
    l_sampler_create_info.minFilter = VkFilter::VK_FILTER_NEAREST;
    l_sampler_create_info.addressModeU = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT;
    l_sampler_create_info.addressModeV = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT;
    l_sampler_create_info.addressModeW = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT;
    l_sampler_create_info.borderColor = VkBorderColor::VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    l_sampler_create_info.unnormalizedCoordinates = 0;
    l_sampler_create_info.compareEnable = false;
    l_sampler_create_info.compareOp = VkCompareOp::VK_COMPARE_OP_ALWAYS;
    l_sampler_create_info.mipmapMode = VkSamplerMipmapMode::VK_SAMPLER_MIPMAP_MODE_LINEAR;
    l_sampler_create_info.mipLodBias = 0.0f;
    l_sampler_create_info.maxLod = 0.0f;
    l_sampler_create_info.minLod = 0.0f;

    vk_handle_result(vkCreateSampler(p_device, &l_sampler_create_info, NULL, &l_samplers.Default));
    return l_samplers;
};

inline void TextureSamplers::free(const gc_t p_device)
{
    vkDestroySampler(p_device, this->Default, NULL);
};

inline GraphicsDevice GraphicsDevice::allocate(GPUInstance& p_instance)
{
    GraphicsDevice l_graphics_device;
    l_graphics_device.graphics_card = p_instance.graphics_card;
    l_graphics_device.device = p_instance.logical_device;

    vkGetDeviceQueue(l_graphics_device.device, p_instance.graphics_card.graphics_queue_family, 0, &l_graphics_device.graphics_queue);

    l_graphics_device.command_pool = CommandPool::allocate(l_graphics_device.device, p_instance.graphics_card.graphics_queue_family);
    l_graphics_device.command_buffer = l_graphics_device.command_pool.allocate_command_buffer(l_graphics_device.device, l_graphics_device.graphics_queue);

    l_graphics_device.shaderparameter_pool = ShaderParameterPool::allocate(l_graphics_device.device, 10000);
    l_graphics_device.shaderlayout_parameters = ShaderLayoutParameters::allocate(l_graphics_device.device);

    l_graphics_device.texture_samplers = TextureSamplers::allocate(l_graphics_device.device);

    return l_graphics_device;
};

inline void GraphicsDevice::free()
{
    this->command_pool.free_command_buffer(this->device, this->command_buffer);
    this->command_pool.free(this->device);

    this->shaderparameter_pool.free(this->device);
    this->shaderlayout_parameters.free(this->device);

    this->texture_samplers.free(this->device);
};

inline TextureGPU TextureGPU::allocate(BufferMemory& p_buffer_memory, const ImageFormat& p_image_format)
{
    TextureGPU l_texture_gpu;
    l_texture_gpu.Image = BufferAllocatorComposition::allocate_imagegpu_and_push_creation_event(p_buffer_memory.allocator, p_buffer_memory.events, p_image_format);

    ImageGPU& l_image_gpu = p_buffer_memory.allocator.gpu_images.get(l_texture_gpu.Image);

    VkImageViewCreateInfo l_imageview_create_info{};
    l_imageview_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    l_imageview_create_info.image = l_image_gpu.image;
    switch (p_image_format.imageType)
    {
    case VkImageType::VK_IMAGE_TYPE_2D:
        l_imageview_create_info.viewType = VkImageViewType::VK_IMAGE_VIEW_TYPE_2D;
        break;
    default:
        abort();
    }
    l_imageview_create_info.format = p_image_format.format;
    l_imageview_create_info.subresourceRange = VkImageSubresourceRange{p_image_format.imageAspect, 0, (uint32_t)p_image_format.mipLevels, 0, (uint32_t)p_image_format.arrayLayers};

    vk_handle_result(vkCreateImageView(p_buffer_memory.allocator.device.device, &l_imageview_create_info, NULL, &l_texture_gpu.ImageView));

    return l_texture_gpu;
};

inline void TextureGPU::free(BufferMemory& p_buffer_memory)
{
    vkDestroyImageView(p_buffer_memory.allocator.device.device, this->ImageView, NULL);
    BufferAllocatorComposition::free_image_gpu_and_remove_event_references(p_buffer_memory.allocator, p_buffer_memory.events, this->Image);
};

template <uint8 AttachmentCount> inline RenderPass RenderPass::allocate(const GraphicsDevice& p_device, const SliceN<RenderPassAttachment, AttachmentCount>& p_attachments)
{

#if GPU_DEBUG
    assert_true(AttachmentCount >= 1);
#endif

    VkAttachmentDescription l_attachments_raw[AttachmentCount];
    Slice<VkAttachmentDescription> l_attachments = Slice<VkAttachmentDescription>::build_memory_elementnb(l_attachments_raw, AttachmentCount);

    VkAttachmentReference l_color_attachments_ref_raw[AttachmentCount];
    Slice<VkAttachmentReference> l_color_attachments_ref = Slice<VkAttachmentReference>::build_memory_elementnb(l_color_attachments_ref_raw, AttachmentCount);
    uint8 l_color_attachments_ref_count = 0;

    VkAttachmentReference l_depth_attachment_reference;

    VkSubpassDescription l_subpass = VkSubpassDescription{};

    for (loop(i, 0, AttachmentCount))
    {

        VkAttachmentDescription& l_attachment_description = l_attachments.get(i);
        l_attachment_description = VkAttachmentDescription{};
        l_attachment_description.format = p_attachments.get(i).image_format.format;
        l_attachment_description.samples = p_attachments.get(i).image_format.samples;
        l_attachment_description.loadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR;
        l_attachment_description.storeOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE;
        l_attachment_description.stencilLoadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR;
        l_attachment_description.stencilStoreOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE;
        l_attachment_description.initialLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;

        if (p_attachments.get(i).type == AttachmentType::KHR)
        {
            l_attachment_description.finalLayout = VkImageLayout::VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        }
        else
        {
            l_attachment_description.finalLayout = ImageLayoutTransitionBarriers::get_imagelayout_from_imageusage(p_attachments.get(i).image_format.imageUsage);
        }

        switch (p_attachments.get(i).type)
        {
        case AttachmentType::COLOR:
        {
            VkAttachmentReference& l_attachment_reference = l_color_attachments_ref.get(l_color_attachments_ref_count);
            l_attachment_reference.attachment = (uint32)i;
            l_attachment_reference.layout = l_attachment_description.finalLayout;
            l_color_attachments_ref_count += 1;
        }
        break;
        case AttachmentType::KHR:
        {
            VkAttachmentReference& l_attachment_reference = l_color_attachments_ref.get(l_color_attachments_ref_count);
            l_attachment_reference.attachment = (uint32)i;
            l_attachment_reference.layout = ImageLayoutTransitionBarriers::get_imagelayout_from_imageusage(ImageUsageFlag::SHADER_COLOR_ATTACHMENT);
            l_color_attachments_ref_count += 1;
        }
        break;
        case AttachmentType::DEPTH:
        {
            l_depth_attachment_reference = VkAttachmentReference{};
            l_depth_attachment_reference.attachment = (uint32)i;
            l_depth_attachment_reference.layout = l_attachment_description.finalLayout;
            l_subpass.pDepthStencilAttachment = &l_depth_attachment_reference;
        }
        break;
        }
    }

    l_subpass.pipelineBindPoint = VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS;
    l_subpass.colorAttachmentCount = l_color_attachments_ref_count;
    l_subpass.pColorAttachments = l_color_attachments_ref.Begin;

    VkRenderPassCreateInfo l_renderpass_create_info{};
    l_renderpass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    l_renderpass_create_info.attachmentCount = (uint32)l_attachments.Size;
    l_renderpass_create_info.pAttachments = l_attachments.Begin;
    l_renderpass_create_info.subpassCount = 1;
    l_renderpass_create_info.pSubpasses = &l_subpass;

    RenderPass l_render_pass_object;

    vk_handle_result(vkCreateRenderPass(p_device.device, &l_renderpass_create_info, NULL, &l_render_pass_object.render_pass));
    return l_render_pass_object;
};

inline void RenderPass::free(const GraphicsDevice& p_device)
{
    vkDestroyRenderPass(p_device.device, this->render_pass, NULL);
};

template <uint32 AttachmentCount>
inline GraphicsPass GraphicsPass::allocate(const TransferDevice& p_transfer_device, const GraphicsDevice& p_graphics_device, const Token(Slice<Token(TextureGPU)>) p_allocated_attachment_textures,
                                           const SliceN<RenderPassAttachment, AttachmentCount>& p_render_pass_attachments, const VkImageView* p_attachment_image_views)
{

#if GPU_DEBUG
    v3ui l_extend = p_render_pass_attachments.get(0).image_format.extent;
    for (loop(i, 1, AttachmentCount))
    {
        assert_true(p_render_pass_attachments.get(i).image_format.extent == l_extend);
    }
#endif

    RenderPass l_render_pass = RenderPass::allocate<AttachmentCount>(p_graphics_device, p_render_pass_attachments);

    VkFramebufferCreateInfo l_framebuffer_create{};
    l_framebuffer_create.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    l_framebuffer_create.attachmentCount = AttachmentCount;
    l_framebuffer_create.pAttachments = p_attachment_image_views;
    l_framebuffer_create.layers = 1;
    l_framebuffer_create.width = p_render_pass_attachments.get(0).image_format.extent.x;
    l_framebuffer_create.height = p_render_pass_attachments.get(0).image_format.extent.y;
    l_framebuffer_create.renderPass = l_render_pass.render_pass;

    GraphicsPass l_graphics_pass;

#if GPU_DEBUG
    l_graphics_pass.attachement_layout = Span<RenderPassAttachment>::allocate(AttachmentCount);
    l_graphics_pass.attachement_layout.slice.copy_memory(0, p_render_pass_attachments.to_slice());
#endif
    vk_handle_result(vkCreateFramebuffer(p_transfer_device.device, &l_framebuffer_create, NULL, &l_graphics_pass.frame_buffer));

    l_graphics_pass.attachment_textures = p_allocated_attachment_textures;
    l_graphics_pass.render_pass = l_render_pass;

    return l_graphics_pass;
};

inline void GraphicsPass::free(const GraphicsDevice& p_graphics_device)
{
    this->render_pass.free(p_graphics_device);

    vkDestroyFramebuffer(p_graphics_device.device, this->frame_buffer, NULL);

#if GPU_DEBUG
    this->attachement_layout.free();
#endif
};

inline ShaderLayout ShaderLayout::allocate(const GraphicsDevice& p_device, const Span<ShaderLayoutParameterType>& in_shaderlayout_parameter_types,
                                           const Span<VertexInputParameter>& in_vertex_input_layout, const uimax in_vertex_element_size)
{
    VkPipelineLayoutCreateInfo l_pipeline_create_info{};
    l_pipeline_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    Span<VkDescriptorSetLayout> l_descriptor_set_layouts = Span<VkDescriptorSetLayout>::allocate(in_shaderlayout_parameter_types.Capacity);

    for (loop(i, 0, in_shaderlayout_parameter_types.Capacity))
    {
        l_descriptor_set_layouts.get(i) = p_device.shaderlayout_parameters.get_descriptorset_layout(in_shaderlayout_parameter_types.get(i));
    }

    l_pipeline_create_info.setLayoutCount = (uint32_t)l_descriptor_set_layouts.Capacity;
    l_pipeline_create_info.pSetLayouts = l_descriptor_set_layouts.Memory;

    ShaderLayout l_shader_layout;
    l_shader_layout.shader_layout_parameter_types = in_shaderlayout_parameter_types;
    l_shader_layout.vertex_input_layout = in_vertex_input_layout;
    l_shader_layout.vertex_element_size = in_vertex_element_size;
    vk_handle_result(vkCreatePipelineLayout(p_device.device, &l_pipeline_create_info, NULL, &l_shader_layout.layout));

    l_descriptor_set_layouts.free();

    return l_shader_layout;
};

inline void ShaderLayout::free(const GraphicsDevice& p_device)
{
    vkDestroyPipelineLayout(p_device.device, this->layout, NULL);
    this->shader_layout_parameter_types.free();
    this->vertex_input_layout.free();
};

inline ShaderModule ShaderModule::allocate(const GraphicsDevice& p_graphics_device, const Slice<int8>& p_shader_compiled_str)
{
    VkShaderModuleCreateInfo l_shader_module_create_info{};
    l_shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    l_shader_module_create_info.codeSize = p_shader_compiled_str.Size;
    l_shader_module_create_info.pCode = (uint32_t*)p_shader_compiled_str.Begin;

    ShaderModule l_shader_module;
    vk_handle_result(vkCreateShaderModule(p_graphics_device.device, &l_shader_module_create_info, NULL, &l_shader_module.module));
    return l_shader_module;
};

inline ShaderModule ShaderModule::build_default()
{
    return ShaderModule{NULL};
};

inline void ShaderModule::free(const GraphicsDevice& p_graphics_device)
{
    vkDestroyShaderModule(p_graphics_device.device, this->module, NULL);
};

inline Shader Shader::allocate(const GraphicsDevice& p_device, const ShaderAllocateInfo& p_shader_allocate_info)
{
    Shader l_shader;
    l_shader.layout = p_shader_allocate_info.shader_layout;

    Span<VkVertexInputAttributeDescription> l_vertex_input_attributes = Span<VkVertexInputAttributeDescription>::allocate(p_shader_allocate_info.shader_layout.vertex_input_layout.Capacity);

    VkGraphicsPipelineCreateInfo l_pipeline_graphics_create_info{};
    l_pipeline_graphics_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    l_pipeline_graphics_create_info.layout = p_shader_allocate_info.shader_layout.layout;
    l_pipeline_graphics_create_info.renderPass = p_shader_allocate_info.graphics_pass.render_pass.render_pass;

    VkPipelineInputAssemblyStateCreateInfo l_inputassembly_state{};
    l_inputassembly_state.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    l_inputassembly_state.topology = VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    l_inputassembly_state.primitiveRestartEnable = 0;

    VkPipelineRasterizationStateCreateInfo l_rasterization_state{};
    l_rasterization_state.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    l_rasterization_state.polygonMode = VkPolygonMode::VK_POLYGON_MODE_FILL;
    l_rasterization_state.cullMode = VkCullModeFlagBits::VK_CULL_MODE_BACK_BIT;
    l_rasterization_state.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    l_rasterization_state.lineWidth = 1.0f;
    l_rasterization_state.depthClampEnable = 0;
    l_rasterization_state.rasterizerDiscardEnable = 0;
    l_rasterization_state.depthClampEnable = 0;

    VkPipelineColorBlendAttachmentState l_blendattachment_state{};
    l_blendattachment_state.colorWriteMask = 0xf;
    l_blendattachment_state.blendEnable = 0;
    VkPipelineColorBlendStateCreateInfo l_blendattachment_state_create{};
    l_blendattachment_state_create.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    l_blendattachment_state_create.attachmentCount = 1;
    l_blendattachment_state_create.pAttachments = &l_blendattachment_state;

    VkPipelineViewportStateCreateInfo l_viewport_state{};
    l_viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    l_viewport_state.viewportCount = 1;
    l_viewport_state.scissorCount = 1;

    VkDynamicState l_dynamicstates_enabled[2];
    l_dynamicstates_enabled[0] = VkDynamicState::VK_DYNAMIC_STATE_VIEWPORT;
    l_dynamicstates_enabled[1] = VkDynamicState::VK_DYNAMIC_STATE_SCISSOR;
    VkPipelineDynamicStateCreateInfo l_dynamicstates{};
    l_dynamicstates.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    l_dynamicstates.dynamicStateCount = 2;
    l_dynamicstates.pDynamicStates = l_dynamicstates_enabled;

    VkPipelineDepthStencilStateCreateInfo l_depthstencil_state{};
    l_depthstencil_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    if (p_shader_allocate_info.shader_configuration.ztest != ShaderConfiguration::CompareOp::Invalid)
    {
        l_depthstencil_state.depthTestEnable = 1;
        l_depthstencil_state.depthWriteEnable = p_shader_allocate_info.shader_configuration.zwrite;
        l_depthstencil_state.depthCompareOp = (VkCompareOp)p_shader_allocate_info.shader_configuration.ztest;
        l_depthstencil_state.depthBoundsTestEnable = 0;

        VkStencilOpState l_back{};
        l_back.compareOp = VkCompareOp::VK_COMPARE_OP_ALWAYS;
        l_back.failOp = VkStencilOp::VK_STENCIL_OP_KEEP;
        l_back.passOp = VkStencilOp::VK_STENCIL_OP_KEEP;
        l_depthstencil_state.back = l_back;
        l_depthstencil_state.front = l_back;
        l_depthstencil_state.stencilTestEnable = 0;
    }

    VkPipelineMultisampleStateCreateInfo l_multisample_state{};
    l_multisample_state.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    l_multisample_state.rasterizationSamples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;

    for (loop(i, 0, l_vertex_input_attributes.Capacity))
    {
        const ShaderLayout::VertexInputParameter& l_vertex_input_parameter = p_shader_allocate_info.shader_layout.vertex_input_layout.get(i);
        l_vertex_input_attributes.get(i) = VkVertexInputAttributeDescription{(uint32_t)i, 0, get_primitivetype_format(l_vertex_input_parameter.type), (uint32_t)l_vertex_input_parameter.offset};
    }

    VkVertexInputBindingDescription l_vertex_input_binding{0, (uint32_t)p_shader_allocate_info.shader_layout.vertex_element_size, VkVertexInputRate::VK_VERTEX_INPUT_RATE_VERTEX};

    VkPipelineVertexInputStateCreateInfo l_vertex_input_create{};
    l_vertex_input_create.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    l_vertex_input_create.vertexBindingDescriptionCount = 1;
    l_vertex_input_create.pVertexBindingDescriptions = &l_vertex_input_binding;
    l_vertex_input_create.pVertexAttributeDescriptions = l_vertex_input_attributes.Memory;
    l_vertex_input_create.vertexAttributeDescriptionCount = (uint32_t)l_vertex_input_attributes.Capacity;

    VkPipelineShaderStageCreateInfo l_shader_stages[2]{};
    Slice<VkPipelineShaderStageCreateInfo> l_shader_stages_slice = Slice<VkPipelineShaderStageCreateInfo>::build(l_shader_stages, 0, 0);

    if (p_shader_allocate_info.vertex_shader.module != NULL)
    {
        l_shader_stages_slice.Size += 1;
        VkPipelineShaderStageCreateInfo& l_vertex_stage = l_shader_stages_slice.get(0);
        l_vertex_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        l_vertex_stage.stage = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;
        l_vertex_stage.module = p_shader_allocate_info.vertex_shader.module;
        l_vertex_stage.pName = "main";
    }

    if (p_shader_allocate_info.fragment_shader.module != NULL)
    {
        l_shader_stages_slice.Size += 1;
        VkPipelineShaderStageCreateInfo& l_fragment_stage = l_shader_stages_slice.get(1);
        l_fragment_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        l_fragment_stage.stage = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
        l_fragment_stage.module = p_shader_allocate_info.fragment_shader.module;
        l_fragment_stage.pName = "main";
    };

    l_pipeline_graphics_create_info.stageCount = (uint32_t)l_shader_stages_slice.Size;
    l_pipeline_graphics_create_info.pStages = l_shader_stages;

    l_pipeline_graphics_create_info.pVertexInputState = &l_vertex_input_create;
    l_pipeline_graphics_create_info.pInputAssemblyState = &l_inputassembly_state;
    l_pipeline_graphics_create_info.pRasterizationState = &l_rasterization_state;
    l_pipeline_graphics_create_info.pColorBlendState = &l_blendattachment_state_create;
    l_pipeline_graphics_create_info.pMultisampleState = &l_multisample_state;
    l_pipeline_graphics_create_info.pViewportState = &l_viewport_state;

    if (p_shader_allocate_info.shader_configuration.ztest != ShaderConfiguration::CompareOp::Invalid)
    {
        l_pipeline_graphics_create_info.pDepthStencilState = &l_depthstencil_state;
    }
    l_pipeline_graphics_create_info.pDynamicState = &l_dynamicstates;

    vk_handle_result(vkCreateGraphicsPipelines(p_device.device, VkPipelineCache{}, 1, &l_pipeline_graphics_create_info, NULL, &l_shader.shader));

    l_vertex_input_attributes.free();

    return l_shader;
};

inline void Shader::free(const GraphicsDevice& p_device)
{
    vkDestroyPipeline(p_device.device, this->shader, NULL);
};

inline VkFormat Shader::get_primitivetype_format(const PrimitiveSerializedTypes::Type p_primitive_type)
{
    switch (p_primitive_type)
    {
    case PrimitiveSerializedTypes::Type::FLOAT32:
        return VkFormat::VK_FORMAT_R32_SFLOAT;
    case PrimitiveSerializedTypes::Type::FLOAT32_2:
        return VkFormat::VK_FORMAT_R32G32_SFLOAT;
    case PrimitiveSerializedTypes::Type::FLOAT32_3:
        return VkFormat::VK_FORMAT_R32G32B32_SFLOAT;
    case PrimitiveSerializedTypes::Type::FLOAT32_4:
        return VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT;
    default:
        abort();
    }
};

template <class ShadowShaderUniformBufferParameter_t(_), class ShadowBuffer_t(_)>
inline ShadowShaderUniformBufferParameter_t(_) ShadowShaderUniformBufferParameter::allocate(const GraphicsDevice& p_graphics_device, const VkDescriptorSetLayout p_descriptor_set_layout,
                                                                                            const Token(ShadowBuffer_t(_)) p_buffer_memory_token, const ShadowBuffer_t(_) & p_buffer_memory)
{
    ShadowShaderUniformBufferParameter_t(_) l_shader_unifor_buffer_parameter;
    VkDescriptorSetAllocateInfo l_allocate_info{};
    l_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    l_allocate_info.descriptorPool = p_graphics_device.shaderparameter_pool.descriptor_pool;
    l_allocate_info.descriptorSetCount = 1;
    l_allocate_info.pSetLayouts = &p_descriptor_set_layout;

    l_shader_unifor_buffer_parameter.ShadowShaderUniformBufferParameter_member_memory = p_buffer_memory_token;
    vk_handle_result(vkAllocateDescriptorSets(p_graphics_device.device, &l_allocate_info, &l_shader_unifor_buffer_parameter.ShadowShaderUniformBufferParameter_member_descriptor_set));

    VkDescriptorBufferInfo l_descriptor_buffer_info;
    l_descriptor_buffer_info.buffer = p_buffer_memory.ShadowBuffer_member_buffer;
    l_descriptor_buffer_info.offset = 0;
    l_descriptor_buffer_info.range = p_buffer_memory.ShadowBuffer_member_size;

    VkWriteDescriptorSet l_write_descriptor_set{};
    l_write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    l_write_descriptor_set.dstSet = l_shader_unifor_buffer_parameter.ShadowShaderUniformBufferParameter_member_descriptor_set;
    l_write_descriptor_set.descriptorCount = 1;
    l_write_descriptor_set.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    l_write_descriptor_set.pBufferInfo = &l_descriptor_buffer_info;

    vkUpdateDescriptorSets(p_graphics_device.device, 1, &l_write_descriptor_set, 0, NULL);

    return l_shader_unifor_buffer_parameter;
};

inline ShaderUniformBufferHostParameter ShaderUniformBufferHostParameter::allocate(const GraphicsDevice& p_graphics_device, const VkDescriptorSetLayout p_descriptor_set_layout,
                                                                                   const Token(BufferHost) p_buffer_memory_token, const BufferHost& p_buffer_memory)
{
    return ShadowShaderUniformBufferParameter::allocate<ShaderUniformBufferHostParameter>(p_graphics_device, p_descriptor_set_layout, p_buffer_memory_token, p_buffer_memory);
};

inline void ShaderUniformBufferHostParameter::ShadowShaderUniformBufferParameter_method_free(const GraphicsDevice& p_graphics_device)
{
    vk_handle_result(vkFreeDescriptorSets(p_graphics_device.device, p_graphics_device.shaderparameter_pool.descriptor_pool, 1, &this->descriptor_set));
};

inline ShaderUniformBufferGPUParameter ShaderUniformBufferGPUParameter::allocate(const GraphicsDevice& p_graphics_device, const VkDescriptorSetLayout p_descriptor_set_layout,
                                                                                 const Token(BufferGPU) p_buffer_memory_token, const BufferGPU& p_buffer_memory)
{
    return ShadowShaderUniformBufferParameter::allocate<ShaderUniformBufferGPUParameter>(p_graphics_device, p_descriptor_set_layout, p_buffer_memory_token, p_buffer_memory);
};

inline void ShaderUniformBufferGPUParameter::ShadowShaderUniformBufferParameter_method_free(const GraphicsDevice& p_graphics_device)
{
    vk_handle_result(vkFreeDescriptorSets(p_graphics_device.device, p_graphics_device.shaderparameter_pool.descriptor_pool, 1, &this->descriptor_set));
};

inline ShaderTextureGPUParameter ShaderTextureGPUParameter::allocate(const GraphicsDevice& p_graphics_device, const VkDescriptorSetLayout p_descriptor_set_layout,
                                                                     const Token(TextureGPU) p_texture_token, const TextureGPU& p_texture)
{
    ShaderTextureGPUParameter l_shader_texture_gpu_parameter;
    VkDescriptorSetAllocateInfo l_allocate_info{};
    l_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    l_allocate_info.descriptorPool = p_graphics_device.shaderparameter_pool.descriptor_pool;
    l_allocate_info.descriptorSetCount = 1;
    l_allocate_info.pSetLayouts = &p_descriptor_set_layout;

    l_shader_texture_gpu_parameter.texture = p_texture_token;
    vk_handle_result(vkAllocateDescriptorSets(p_graphics_device.device, &l_allocate_info, &l_shader_texture_gpu_parameter.descriptor_set));

    VkDescriptorImageInfo l_descriptor_image_info;
    l_descriptor_image_info.imageView = p_texture.ImageView;
    l_descriptor_image_info.imageLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    l_descriptor_image_info.sampler = p_graphics_device.texture_samplers.Default;

    VkWriteDescriptorSet l_write_descriptor_set{};
    l_write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    l_write_descriptor_set.dstSet = l_shader_texture_gpu_parameter.descriptor_set;
    l_write_descriptor_set.descriptorCount = 1;
    l_write_descriptor_set.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    l_write_descriptor_set.pImageInfo = &l_descriptor_image_info;

    vkUpdateDescriptorSets(p_graphics_device.device, 1, &l_write_descriptor_set, 0, NULL);

    return l_shader_texture_gpu_parameter;
};

inline void ShaderTextureGPUParameter::free(const GraphicsDevice& p_graphics_device)
{
    vk_handle_result(vkFreeDescriptorSets(p_graphics_device.device, p_graphics_device.shaderparameter_pool.descriptor_pool, 1, &this->descriptor_set));
};

struct GraphicsAllocator2
{
    GraphicsDevice graphics_device;
    GraphicsHeap2 heap;

    inline static GraphicsAllocator2 allocate_default(GPUInstance& p_gpu_instance)
    {
        return GraphicsAllocator2{GraphicsDevice::allocate(p_gpu_instance), GraphicsHeap2::allocate_default()};
    };

    inline void free()
    {
        this->heap.free();
        this->graphics_device.free();
    };

    inline Token(TextureGPU) allocate_texturegpu(const TransferDevice& p_transfer_device, const Token(ImageGPU) p_image_gpu_token, const ImageGPU& p_image_gpu, const ImageFormat& p_image_format)
    {
        TextureGPU l_texture_gpu;
        l_texture_gpu.Image = p_image_gpu_token;

        VkImageViewCreateInfo l_imageview_create_info{};
        l_imageview_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        l_imageview_create_info.image = p_image_gpu.image;
        switch (p_image_format.imageType)
        {
        case VkImageType::VK_IMAGE_TYPE_2D:
            l_imageview_create_info.viewType = VkImageViewType::VK_IMAGE_VIEW_TYPE_2D;
            break;
        default:
            abort();
        }
        l_imageview_create_info.format = p_image_format.format;
        l_imageview_create_info.subresourceRange = VkImageSubresourceRange{p_image_format.imageAspect, 0, (uint32_t)p_image_format.mipLevels, 0, (uint32_t)p_image_format.arrayLayers};

        vk_handle_result(vkCreateImageView(p_transfer_device.device, &l_imageview_create_info, NULL, &l_texture_gpu.ImageView));

        return this->heap.textures_gpu.alloc_element(l_texture_gpu);
    };

    inline void free_texturegpu(const TransferDevice& p_transfer_device, const Token(TextureGPU) p_texture_gpu)
    {
        TextureGPU& l_texture_gpu = this->heap.textures_gpu.get(p_texture_gpu);
        vkDestroyImageView(p_transfer_device.device, l_texture_gpu.ImageView, NULL);

        this->heap.textures_gpu.release_element(p_texture_gpu);
    };

    template <uint8 AttachmentCount>
    inline Token(GraphicsPass) allocate_graphicspass(const TransferDevice& p_transfer_device, const Token(ImageGPU) p_attachment_image_tokens[AttachmentCount],
                                                     const ImageGPU p_attachment_images[AttachmentCount], const SliceN<RenderPassAttachment, AttachmentCount>& p_attachments)
    {
        return this->heap.graphics_pass.alloc_element(_allocate_graphics_pass<AttachmentCount>(p_transfer_device, p_attachment_image_tokens, p_attachment_images, p_attachments));
    };

    inline void free_graphicspass(const TransferDevice& p_transfer_device, const Token(GraphicsPass) p_graphics_pass)
    {
        GraphicsPass& l_graphics_pass = this->heap.graphics_pass.get(p_graphics_pass);

        Slice<Token(TextureGPU)> l_attached_textures = this->heap.renderpass_attachment_textures.get_vector(l_graphics_pass.attachment_textures);
        for (loop(i, 0, l_attached_textures.Size))
        {
            this->free_texturegpu(p_transfer_device, l_attached_textures.get(i));
        }
        this->heap.renderpass_attachment_textures.release_vector(l_graphics_pass.attachment_textures);

        l_graphics_pass.free(this->graphics_device);

        this->heap.graphics_pass.release_element(p_graphics_pass);
    };

    inline Token(ShaderLayout)
        allocate_shader_layout(Span<ShaderLayoutParameterType>& in_shaderlayout_parameter_types, Span<ShaderLayout::VertexInputParameter>& in_vertex_input_layout, const uimax in_vertex_element_size)
    {
        return this->heap.shader_layouts.alloc_element(
            ShaderLayout::allocate(this->graphics_device, in_shaderlayout_parameter_types.move_to_value(), in_vertex_input_layout.move_to_value(), in_vertex_element_size));
    };

    inline void free_shader_layout(const Token(ShaderLayout) p_shader_layout)
    {
        ShaderLayout& l_shader_layout = this->heap.shader_layouts.get(p_shader_layout);
        l_shader_layout.free(this->graphics_device);
        this->heap.shader_layouts.release_element(p_shader_layout);
    };

    inline Token(ShaderModule) allocate_shader_module(const Slice<int8>& p_shader_compiled_str)
    {
        return this->heap.shader_modules.alloc_element(ShaderModule::allocate(this->graphics_device, p_shader_compiled_str));
    };

    inline void free_shader_module(const Token(ShaderModule) p_shader_module)
    {
        ShaderModule& l_shader_module = this->heap.shader_modules.get(p_shader_module);
        l_shader_module.free(this->graphics_device);
        this->heap.shader_modules.release_element(p_shader_module);
    };

    inline Token(Shader) allocate_shader(const ShaderAllocateInfo& p_shader_allocate_info)
    {
        return this->heap.shaders.alloc_element(Shader::allocate(this->graphics_device, p_shader_allocate_info));
    };

    inline void free_shader(const Token(Shader) p_shader)
    {
        Shader& l_shader = this->heap.shaders.get(p_shader);
        l_shader.free(this->graphics_device);
        this->heap.shaders.release_element(p_shader);
    };

    inline Token(ShaderUniformBufferHostParameter)
        allocate_shaderuniformbufferhost_parameter(const ShaderLayoutParameterType p_shaderlayout_parameter_type, const Token(BufferHost) p_memory_token, const BufferHost& p_memory)
    {
        ShaderUniformBufferHostParameter l_shader_uniform_buffer_parameter = ShaderUniformBufferHostParameter::allocate(
            this->graphics_device, this->graphics_device.shaderlayout_parameters.get_descriptorset_layout(p_shaderlayout_parameter_type), p_memory_token, p_memory);

        return this->heap.shader_uniform_buffer_host_parameters.alloc_element(l_shader_uniform_buffer_parameter);
    };

    inline void free_shaderuniformbufferhost_parameter(const Token(ShaderUniformBufferHostParameter) p_parameter)
    {
        ShaderUniformBufferHostParameter& l_parameter = this->heap.shader_uniform_buffer_host_parameters.get(p_parameter);
        l_parameter.free(this->graphics_device);
        this->heap.shader_uniform_buffer_host_parameters.release_element(p_parameter);
    };

    inline Token(ShaderUniformBufferGPUParameter)
        allocate_shaderuniformbuffergpu_parameter(const ShaderLayoutParameterType p_shaderlayout_parameter_type, const Token(BufferGPU) p_memory_token, const BufferGPU& p_memory)
    {
        ShaderUniformBufferGPUParameter l_shader_uniform_buffer_paramter = ShaderUniformBufferGPUParameter::allocate(
            this->graphics_device, this->graphics_device.shaderlayout_parameters.get_descriptorset_layout(p_shaderlayout_parameter_type), p_memory_token, p_memory);

        return this->heap.shader_uniform_buffer_gpu_parameters.alloc_element(l_shader_uniform_buffer_paramter);
    };

    inline void free_shaderuniformbuffergpu_parameter(const Token(ShaderUniformBufferGPUParameter) p_parameter)
    {
        ShaderUniformBufferGPUParameter& l_parameter = this->heap.shader_uniform_buffer_gpu_parameters.get(p_parameter);
        l_parameter.free(this->graphics_device);
        this->heap.shader_uniform_buffer_gpu_parameters.release_element(p_parameter);
    };

    inline Token(ShaderTextureGPUParameter)
        allocate_shadertexturegpu_parameter(const ShaderLayoutParameterType p_shaderlayout_parameter_type, const Token(TextureGPU) p_texture_token, const TextureGPU& p_texture)
    {
        ShaderTextureGPUParameter l_shader_texture_gpu_parameter = ShaderTextureGPUParameter::allocate(
            this->graphics_device, this->graphics_device.shaderlayout_parameters.get_descriptorset_layout(p_shaderlayout_parameter_type), p_texture_token, p_texture);

        return this->heap.shader_texture_gpu_parameters.alloc_element(l_shader_texture_gpu_parameter);
    };

    inline void free_shadertexturegpu_parameter(const TransferDevice& p_transfer_device, const Token(ShaderTextureGPUParameter) p_parameter_token, ShaderTextureGPUParameter& p_parameter)
    {
        p_parameter.free(this->graphics_device);
        this->heap.shader_texture_gpu_parameters.release_element(p_parameter_token);
    };

    inline void free_shadertexturegpu_parameter_with_texture(const TransferDevice& p_transfer_device, const Token(ShaderTextureGPUParameter) p_parameter_token, ShaderTextureGPUParameter& p_parameter)
    {
        this->free_texturegpu(p_transfer_device, p_parameter.texture);
        this->free_shadertexturegpu_parameter(p_transfer_device, p_parameter_token, p_parameter);
    };

    inline Token(Slice<ShaderParameter>) allocate_material_parameters()
    {
        return this->heap.material_parameters.alloc_vector();
    };

    inline void free_material_parameters(const Token(Slice<ShaderParameter>) p_material_parameters)
    {
        this->heap.material_parameters.release_vector(p_material_parameters);
    };

  private:
    template <uint8 AttachmentCount>
    inline GraphicsPass _allocate_graphics_pass(const TransferDevice& p_transfer_device, const Token(ImageGPU) p_attachment_image_tokens[AttachmentCount],
                                                const ImageGPU p_attachment_images[AttachmentCount], const SliceN<RenderPassAttachment, AttachmentCount>& p_attachments)
    {
        TextureGPU l_attachment_textures[AttachmentCount];
        ImageView_t l_attachment_image_views[AttachmentCount];
        Token(TextureGPU) l_tokenized_textures[AttachmentCount];
        for (loop(i, 0, AttachmentCount))
        {
            l_tokenized_textures[i] = this->allocate_texturegpu(p_transfer_device, p_attachment_image_tokens[i], p_attachment_images[i], p_attachments.get(i).image_format);
            l_attachment_textures[i] = this->heap.textures_gpu.get(l_tokenized_textures[i]);
            l_attachment_image_views[i] = l_attachment_textures[i].ImageView;
        };

        return GraphicsPass::allocate<AttachmentCount>(
            p_transfer_device, this->graphics_device,
            this->heap.renderpass_attachment_textures.alloc_vector_with_values(Slice<Token(TextureGPU)>::build_memory_elementnb(l_tokenized_textures, AttachmentCount)), p_attachments,
            l_attachment_image_views);
    };
};

/*
    Any allocation that allocate ressources with the BufferAllocator and GraphicsAllocator2
*/
struct GraphicsAllocatorComposition
{

    template <uint8 AttachmentCount>
    inline static Token(GraphicsPass)
        allocate_graphicspass_with_associatedimages(BufferMemory& p_buffer_memory, GraphicsAllocator2& p_graphics_allocator, const SliceN<RenderPassAttachment, AttachmentCount>& p_attachments)
    {
        Token(ImageGPU) l_tokenized_images[AttachmentCount];
        ImageGPU l_images[AttachmentCount];

        for (loop(i, 0, AttachmentCount))
        {
            l_tokenized_images[i] = BufferAllocatorComposition::allocate_imagegpu_and_push_creation_event(p_buffer_memory.allocator, p_buffer_memory.events, p_attachments.get(i).image_format);
            l_images[i] = p_buffer_memory.allocator.gpu_images.get(l_tokenized_images[i]);
        }

        return p_graphics_allocator.allocate_graphicspass<AttachmentCount>(p_buffer_memory.allocator.device, l_tokenized_images, l_images, p_attachments);
    };

    inline static void free_graphicspass_with_associatedimages(BufferMemory& p_buffer_memory, GraphicsAllocator2& p_graphics_allocator, const Token(GraphicsPass) p_graphics_pass)
    {
        GraphicsPass& l_graphics_pass = p_graphics_allocator.heap.graphics_pass.get(p_graphics_pass);
        Slice<Token(TextureGPU)> l_attachment_textures = p_graphics_allocator.heap.renderpass_attachment_textures.get_vector(l_graphics_pass.attachment_textures);
        for (loop(i, 0, l_attachment_textures.Size))
        {
            BufferAllocatorComposition::free_image_gpu_and_remove_event_references(p_buffer_memory.allocator, p_buffer_memory.events,
                                                                                   p_graphics_allocator.heap.textures_gpu.get(l_attachment_textures.get(i)).Image);
        }

        p_graphics_allocator.free_graphicspass(p_buffer_memory.allocator.device, p_graphics_pass);
    };

    inline static Token(TextureGPU) allocate_texturegpu_with_imagegpu(BufferMemory& p_buffer_memory, GraphicsAllocator2& p_graphics_allocator, const ImageFormat& p_image_format)
    {
        Token(ImageGPU) l_image_gpu = BufferAllocatorComposition::allocate_imagegpu_and_push_creation_event(p_buffer_memory.allocator, p_buffer_memory.events, p_image_format);
        return p_graphics_allocator.allocate_texturegpu(p_buffer_memory.allocator.device, l_image_gpu, p_buffer_memory.allocator.gpu_images.get(l_image_gpu), p_image_format);
    };

    inline static void free_texturegpu_with_imagegpu(BufferMemory& p_buffer_memory, GraphicsAllocator2& p_graphics_allocator, const Token(TextureGPU) p_texture_gpu)
    {
        TextureGPU& p_texture = p_graphics_allocator.heap.textures_gpu.get(p_texture_gpu);
        BufferAllocatorComposition::free_image_gpu_and_remove_event_references(p_buffer_memory.allocator, p_buffer_memory.events, p_texture.Image);
        p_graphics_allocator.free_texturegpu(p_buffer_memory.allocator.device, p_texture_gpu);
    };

    inline static void free_shaderparameter_uniformbufferhost_with_buffer(BufferMemory& p_buffer_memory, GraphicsAllocator2& p_graphics_allocator,
                                                                          const Token(ShaderUniformBufferHostParameter) p_parameter)
    {
        ShaderUniformBufferHostParameter& l_parameter = p_graphics_allocator.heap.shader_uniform_buffer_host_parameters.get(p_parameter);
        BufferAllocatorComposition::free_buffer_host_and_remove_event_references(p_buffer_memory.allocator, p_buffer_memory.events, l_parameter.memory);
        p_graphics_allocator.free_shaderuniformbufferhost_parameter(p_parameter);
    };
};

struct GraphicsPassReader
{
    inline static Token(BufferHost)
        read_graphics_pass_attachment_to_bufferhost(BufferMemory& p_buffer_memory, GraphicsAllocator2& p_graphics_allocator, const GraphicsPass& p_graphics_pass, const uimax p_index)
    {
        TextureGPU& l_attachment = p_graphics_allocator.heap.textures_gpu.get(p_graphics_allocator.heap.renderpass_attachment_textures.get_vector(p_graphics_pass.attachment_textures).get(p_index));
        return BufferReadWrite::read_from_imagegpu_to_buffer(p_buffer_memory.allocator, p_buffer_memory.events, l_attachment.Image, p_buffer_memory.allocator.gpu_images.get(l_attachment.Image));
    };
    inline static Token(BufferHost) read_graphics_pass_attachment_to_bufferhost_with_imageformat(BufferMemory& p_buffer_memory, GraphicsAllocator2& p_graphics_allocator,
                                                                                                 const GraphicsPass& p_graphics_pass, const uimax p_index, ImageFormat* out_image_format)
    {
        TextureGPU& l_attachment = p_graphics_allocator.heap.textures_gpu.get(p_graphics_allocator.heap.renderpass_attachment_textures.get_vector(p_graphics_pass.attachment_textures).get(p_index));
        *out_image_format = p_buffer_memory.allocator.gpu_images.get(l_attachment.Image).format;
        return BufferReadWrite::read_from_imagegpu_to_buffer(p_buffer_memory.allocator, p_buffer_memory.events, l_attachment.Image, p_buffer_memory.allocator.gpu_images.get(l_attachment.Image));
    };
};
}; // namespace v2
