#pragma once

// #include "Common2/common2.hpp"
#include "Math2/math.hpp"
#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include "vulkan/vulkan.h"

namespace v2
{

using gc_t = VkDevice;
using gcqueue_t = VkQueue;
using gcmemory_t = VkDeviceMemory;
using gpuinstance_t = VkInstance;
using gpuinstance_debugger_t = VkDebugUtilsMessengerEXT;
} // namespace v2

namespace v2
{
inline void _vk_handle_result(const VkResult p_result)
{
#if GPU_BOUND_TEST
    if (p_result != VK_SUCCESS)
    {
        abort();
    };
#endif
};

#if GPU_BOUND_TEST
#define vk_handle_result(Code) _vk_handle_result(Code)
#else
#define vk_handle_result(Code) Code
#endif

inline static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                           const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{

    String l_severity = String::allocate(100);

    int8 is_error = 0;

    switch (messageSeverity)
    {
    case VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        // l_severity.append(slice_int8_build_rawstr("[Verbose] - "));
        l_severity.free();
        return VK_FALSE;
        break;
    case VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        l_severity.append(slice_int8_build_rawstr("[Info] - "));
        break;
    case VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        l_severity.append(slice_int8_build_rawstr("[Warn] - "));
        break;
    case VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        l_severity.append(slice_int8_build_rawstr("[Error] - "));
        is_error = 1;
        break;
    case VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT:
        is_error = 1;
        break;
    }
    l_severity.append(slice_int8_build_rawstr("validation layer: "));
    l_severity.append(slice_int8_build_rawstr(pCallbackData->pMessage));
    l_severity.append(slice_int8_build_rawstr("\n"));
    printf(l_severity.get_memory());

    l_severity.free();

    if (is_error)
    {
        abort();
    }

    return VK_FALSE;
};

} // namespace v2

#include "./command_buffer.hpp"
#include "./instance.hpp"
#include "./memory.hpp"
#include "./graphics.hpp"
#include "./material.hpp"
#include "./graphics_binder.hpp"
#include "./present.hpp"

namespace v2
{
struct GPUContext
{
    GPUInstance instance;
    BufferMemory buffer_memory;
    GraphicsAllocator2 graphics_allocator;

    Semafore execution_semaphore;
    Semafore end_semaphore;

    inline static GPUContext allocate(const Slice<GPUExtension>& p_gpu_extensions)
    {
        GPUContext l_context;
        l_context.instance = GPUInstance::allocate(p_gpu_extensions);
        l_context.buffer_memory = BufferMemory::allocate(l_context.instance);
        l_context.graphics_allocator = GraphicsAllocator2::allocate_default(l_context.instance);
        l_context.execution_semaphore = Semafore::allocate(l_context.instance.logical_device);
        l_context.end_semaphore = Semafore::allocate(l_context.instance.logical_device);
        return l_context;
    };

    // inline static GPUContext allocate_

    inline void free()
    {
        this->execution_semaphore.free(this->instance.logical_device);
        this->end_semaphore.free(this->instance.logical_device);
        this->graphics_allocator.free();
        this->buffer_memory.free();
        this->instance.free();
    };

    inline void buffer_step_and_submit()
    {
        BufferStep::step(this->buffer_memory.allocator, this->buffer_memory.events);
        this->buffer_memory.allocator.device.command_buffer.submit_and_notity(this->execution_semaphore);
    };

    inline GraphicsBinder creates_graphics_binder()
    {
        GraphicsBinder l_binder = GraphicsBinder::build(this->buffer_memory.allocator, this->graphics_allocator);
        l_binder.start();
        return l_binder;
    };

    inline void submit_graphics_binder(GraphicsBinder& p_binder)
    {
        p_binder.end();
        p_binder.submit_after(this->execution_semaphore, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT);
    };

    inline void submit_graphics_binder_and_notity_end(GraphicsBinder& p_binder)
    {
        p_binder.end();
        p_binder.submit_after_and_notify(this->execution_semaphore, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT, this->end_semaphore);
    };

    inline void wait_for_completion()
    {
        this->graphics_allocator.graphics_device.command_buffer.wait_for_completion();
    };
};
} // namespace v2

#undef ShadowBuffer_t
#undef ShadowBuffer_member_buffer
#undef ShadowBuffer_member_size

#undef ShadowImage
#undef ShadowImage_member_image
#undef ShadowImage_member_format

#undef ShadowShaderUniformBufferParameter_t
#undef ShadowShaderUniformBufferParameter_member_descriptor_set
#undef ShadowShaderUniformBufferParameter_member_memory
#undef ShadowShaderUniformBufferParameter_method_free
#undef ShadowShaderUniformBufferParameter_methodcall_free