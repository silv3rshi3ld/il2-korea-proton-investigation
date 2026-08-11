/*
 * D21: reproduce IL-2's cross-workgroup R32_UINT storage-texel-buffer atomic.
 *
 * This is a standalone Vulkan diagnostic. It does not access the game, Steam,
 * Proton prefixes, or captured resources.
 */

#include <vulkan/vulkan.h>

#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ACTIVE_WIDTH 80u
#define ACTIVE_HEIGHT 34u
#define ACTIVE_COUNT (ACTIVE_WIDTH * ACTIVE_HEIGHT)
#define GROUP_SIZE_X 8u
#define GROUP_SIZE_Y 8u
#define DISPATCH_X 10u
#define DISPATCH_Y 5u
#define LIVE_COUNTER_SIZE 87040u
#define SENTINEL UINT32_MAX

static uint32_t validation_errors;
static uint32_t validation_warnings;

struct options
{
    int32_t device_index;
    bool list_only;
    bool validation;
    const char *texel_spv;
    const char *ssbo_spv;
    const char *exact_spv;
    const char *exact_r16_spv;
    const char *exact_r16_ssbo_spv;
    bool format_ab;
};

struct context
{
    VkPhysicalDevice physical_device;
    VkDevice device;
    VkQueue queue;
    uint32_t queue_family;
    VkPhysicalDeviceMemoryProperties memory_properties;
};

struct host_buffer
{
    VkBuffer buffer;
    VkDeviceMemory memory;
    void *mapping;
    VkDeviceSize allocation_size;
    VkDeviceSize logical_size;
    bool coherent;
};

struct device_image
{
    VkImage image;
    VkDeviceMemory memory;
    VkImageView view;
};

enum atomic_variant
{
    ATOMIC_TEXEL,
    ATOMIC_SSBO,
};

struct atomic_metrics
{
    uint32_t final_counter;
    uint32_t unique;
    uint32_t missing;
    uint32_t duplicates;
    uint32_t out_of_range;
    uint32_t sentinel;
    uint32_t groups_containing_zero;
    uint32_t groups_min_zero;
    bool globally_correct;
};

static const char *vk_result_name(VkResult result)
{
    switch (result)
    {
        case VK_SUCCESS: return "VK_SUCCESS";
        case VK_NOT_READY: return "VK_NOT_READY";
        case VK_TIMEOUT: return "VK_TIMEOUT";
        case VK_EVENT_SET: return "VK_EVENT_SET";
        case VK_EVENT_RESET: return "VK_EVENT_RESET";
        case VK_INCOMPLETE: return "VK_INCOMPLETE";
        case VK_ERROR_OUT_OF_HOST_MEMORY: return "VK_ERROR_OUT_OF_HOST_MEMORY";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
        case VK_ERROR_INITIALIZATION_FAILED: return "VK_ERROR_INITIALIZATION_FAILED";
        case VK_ERROR_DEVICE_LOST: return "VK_ERROR_DEVICE_LOST";
        case VK_ERROR_MEMORY_MAP_FAILED: return "VK_ERROR_MEMORY_MAP_FAILED";
        case VK_ERROR_LAYER_NOT_PRESENT: return "VK_ERROR_LAYER_NOT_PRESENT";
        case VK_ERROR_EXTENSION_NOT_PRESENT: return "VK_ERROR_EXTENSION_NOT_PRESENT";
        case VK_ERROR_FEATURE_NOT_PRESENT: return "VK_ERROR_FEATURE_NOT_PRESENT";
        case VK_ERROR_INCOMPATIBLE_DRIVER: return "VK_ERROR_INCOMPATIBLE_DRIVER";
        case VK_ERROR_TOO_MANY_OBJECTS: return "VK_ERROR_TOO_MANY_OBJECTS";
        case VK_ERROR_FORMAT_NOT_SUPPORTED: return "VK_ERROR_FORMAT_NOT_SUPPORTED";
        default: return "VK_RESULT_UNKNOWN";
    }
}

static bool check_vk(VkResult result, const char *expression, const char *file, int line)
{
    if (result == VK_SUCCESS)
        return true;

    fprintf(stderr, "%s:%d: %s failed: %s (%d)\n",
            file, line, expression, vk_result_name(result), result);
    return false;
}

#define VK_CHECK_RETURN(expression) \
    do { if (!check_vk((expression), #expression, __FILE__, __LINE__)) return false; } while (0)

#define VK_CHECK_GOTO(expression) \
    do { if (!check_vk((expression), #expression, __FILE__, __LINE__)) goto cleanup; } while (0)

static VKAPI_ATTR VkBool32 VKAPI_CALL validation_callback(
        VkDebugUtilsMessageSeverityFlagBitsEXT severity,
        VkDebugUtilsMessageTypeFlagsEXT types,
        const VkDebugUtilsMessengerCallbackDataEXT *data,
        void *user_data)
{
    const char *level;

    (void)types;
    (void)user_data;

    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        ++validation_errors;
        level = "ERROR";
    }
    else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        ++validation_warnings;
        level = "WARNING";
    }
    else
    {
        level = "INFO";
    }

    fprintf(stderr, "VULKAN_VALIDATION_%s: %s\n", level,
            data && data->pMessage ? data->pMessage : "(no message)");
    return VK_FALSE;
}

static bool has_instance_layer(const char *name)
{
    VkLayerProperties *properties = NULL;
    uint32_t count = 0;
    bool found = false;

    if (vkEnumerateInstanceLayerProperties(&count, NULL) != VK_SUCCESS || !count)
        return false;
    properties = calloc(count, sizeof(*properties));
    if (!properties)
        return false;
    if (vkEnumerateInstanceLayerProperties(&count, properties) != VK_SUCCESS)
        goto done;

    for (uint32_t i = 0; i < count; ++i)
    {
        if (!strcmp(properties[i].layerName, name))
        {
            found = true;
            break;
        }
    }

done:
    free(properties);
    return found;
}

static bool has_instance_extension(const char *name)
{
    VkExtensionProperties *properties = NULL;
    uint32_t count = 0;
    bool found = false;

    if (vkEnumerateInstanceExtensionProperties(NULL, &count, NULL) != VK_SUCCESS || !count)
        return false;
    properties = calloc(count, sizeof(*properties));
    if (!properties)
        return false;
    if (vkEnumerateInstanceExtensionProperties(NULL, &count, properties) != VK_SUCCESS)
        goto done;

    for (uint32_t i = 0; i < count; ++i)
    {
        if (!strcmp(properties[i].extensionName, name))
        {
            found = true;
            break;
        }
    }

done:
    free(properties);
    return found;
}

static bool create_instance(bool enable_validation, VkInstance *instance,
        VkDebugUtilsMessengerEXT *messenger)
{
    static const char *validation_layer = "VK_LAYER_KHRONOS_validation";
    static const char *debug_extension = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    VkDebugUtilsMessengerCreateInfoEXT debug_info = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = validation_callback,
    };
    VkApplicationInfo application_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "IL-2 D21 Vulkan atomic probe",
        .applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
        .pEngineName = "none",
        .engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
        .apiVersion = VK_API_VERSION_1_2,
    };
    VkInstanceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &application_info,
    };
    PFN_vkCreateDebugUtilsMessengerEXT create_messenger;

    *instance = VK_NULL_HANDLE;
    *messenger = VK_NULL_HANDLE;

    if (enable_validation)
    {
        if (!has_instance_layer(validation_layer))
        {
            fprintf(stderr, "Required validation layer is not installed: %s\n", validation_layer);
            return false;
        }
        if (!has_instance_extension(debug_extension))
        {
            fprintf(stderr, "Required debug extension is not installed: %s\n", debug_extension);
            return false;
        }
        create_info.enabledLayerCount = 1;
        create_info.ppEnabledLayerNames = &validation_layer;
        create_info.enabledExtensionCount = 1;
        create_info.ppEnabledExtensionNames = &debug_extension;
        create_info.pNext = &debug_info;
    }

    VK_CHECK_RETURN(vkCreateInstance(&create_info, NULL, instance));

    if (!enable_validation)
        return true;

    create_messenger = (PFN_vkCreateDebugUtilsMessengerEXT)
            vkGetInstanceProcAddr(*instance, "vkCreateDebugUtilsMessengerEXT");
    if (!create_messenger ||
            !check_vk(create_messenger(*instance, &debug_info, NULL, messenger),
                    "vkCreateDebugUtilsMessengerEXT", __FILE__, __LINE__))
    {
        vkDestroyInstance(*instance, NULL);
        *instance = VK_NULL_HANDLE;
        return false;
    }

    return true;
}

static void destroy_instance(VkInstance instance, VkDebugUtilsMessengerEXT messenger)
{
    PFN_vkDestroyDebugUtilsMessengerEXT destroy_messenger;

    if (messenger)
    {
        destroy_messenger = (PFN_vkDestroyDebugUtilsMessengerEXT)
                vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (destroy_messenger)
            destroy_messenger(instance, messenger, NULL);
    }
    if (instance)
        vkDestroyInstance(instance, NULL);
}

static const char *device_type_name(VkPhysicalDeviceType type)
{
    switch (type)
    {
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: return "integrated";
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: return "discrete";
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: return "virtual";
        case VK_PHYSICAL_DEVICE_TYPE_CPU: return "cpu";
        default: return "other";
    }
}

static void print_device(uint32_t index, VkPhysicalDevice device)
{
    VkPhysicalDeviceProperties properties;

    vkGetPhysicalDeviceProperties(device, &properties);
    printf("D21_DEVICE index=%u type=%s vendor=0x%04x device=0x%04x "
           "api=%u.%u.%u driver=%u.%u.%u name=\"%s\"\n",
            index, device_type_name(properties.deviceType), properties.vendorID,
            properties.deviceID, VK_API_VERSION_MAJOR(properties.apiVersion),
            VK_API_VERSION_MINOR(properties.apiVersion),
            VK_API_VERSION_PATCH(properties.apiVersion),
            VK_API_VERSION_MAJOR(properties.driverVersion),
            VK_API_VERSION_MINOR(properties.driverVersion),
            VK_API_VERSION_PATCH(properties.driverVersion), properties.deviceName);
}

static bool enumerate_devices(VkInstance instance, VkPhysicalDevice **devices,
        uint32_t *count)
{
    *devices = NULL;
    *count = 0;
    VK_CHECK_RETURN(vkEnumeratePhysicalDevices(instance, count, NULL));
    if (!*count)
    {
        fprintf(stderr, "No Vulkan physical devices found.\n");
        return false;
    }
    *devices = calloc(*count, sizeof(**devices));
    if (!*devices)
    {
        fprintf(stderr, "Out of memory while enumerating Vulkan devices.\n");
        return false;
    }
    if (!check_vk(vkEnumeratePhysicalDevices(instance, count, *devices),
            "vkEnumeratePhysicalDevices", __FILE__, __LINE__))
    {
        free(*devices);
        *devices = NULL;
        return false;
    }
    return true;
}

static bool has_device_extension(VkPhysicalDevice device, const char *name)
{
    VkExtensionProperties *properties = NULL;
    uint32_t count = 0;
    bool found = false;

    if (vkEnumerateDeviceExtensionProperties(device, NULL, &count, NULL) != VK_SUCCESS ||
            !count)
        return false;
    properties = calloc(count, sizeof(*properties));
    if (!properties)
        return false;
    if (vkEnumerateDeviceExtensionProperties(device, NULL, &count, properties) != VK_SUCCESS)
        goto done;
    for (uint32_t i = 0; i < count; ++i)
    {
        if (!strcmp(properties[i].extensionName, name))
        {
            found = true;
            break;
        }
    }

done:
    free(properties);
    return found;
}

static int32_t choose_default_device(const VkPhysicalDevice *devices, uint32_t count)
{
    VkPhysicalDeviceProperties properties;

    for (uint32_t i = 0; i < count; ++i)
    {
        vkGetPhysicalDeviceProperties(devices[i], &properties);
        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            return (int32_t)i;
    }
    return 0;
}

static bool create_device(VkPhysicalDevice physical_device, bool enable_exact,
        struct context *context)
{
    VkQueueFamilyProperties *queue_properties = NULL;
    uint32_t queue_count = 0;
    float queue_priority = 1.0f;
    VkDeviceQueueCreateInfo queue_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueCount = 1,
        .pQueuePriorities = &queue_priority,
    };
    VkDeviceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queue_info,
    };
    VkPhysicalDeviceFeatures enabled_features = {0};
    VkPhysicalDeviceFeatures2 available_features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
    };
    VkPhysicalDeviceDescriptorIndexingFeatures available_indexing = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES,
    };
    VkPhysicalDeviceMutableDescriptorTypeFeaturesEXT available_mutable = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MUTABLE_DESCRIPTOR_TYPE_FEATURES_EXT,
    };
    VkPhysicalDeviceBufferDeviceAddressFeatures available_address = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES,
    };
    VkPhysicalDeviceDescriptorBufferFeaturesEXT available_descriptor_buffer = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_FEATURES_EXT,
    };
    VkPhysicalDeviceDescriptorIndexingFeatures enabled_indexing = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES,
    };
    VkPhysicalDeviceMutableDescriptorTypeFeaturesEXT enabled_mutable = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MUTABLE_DESCRIPTOR_TYPE_FEATURES_EXT,
    };
    VkPhysicalDeviceBufferDeviceAddressFeatures enabled_address = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES,
    };
    VkPhysicalDeviceDescriptorBufferFeaturesEXT enabled_descriptor_buffer = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_FEATURES_EXT,
    };
    const char *exact_extensions[3] = {
        VK_EXT_MUTABLE_DESCRIPTOR_TYPE_EXTENSION_NAME,
        VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME,
        VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
    };
    bool found = false;

    memset(context, 0, sizeof(*context));
    context->physical_device = physical_device;

    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_count, NULL);
    if (!queue_count)
    {
        fprintf(stderr, "Selected Vulkan device exposes no queue families.\n");
        return false;
    }
    queue_properties = calloc(queue_count, sizeof(*queue_properties));
    if (!queue_properties)
        return false;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_count, queue_properties);

    for (uint32_t i = 0; i < queue_count; ++i)
    {
        if (queue_properties[i].queueCount &&
                (queue_properties[i].queueFlags & VK_QUEUE_COMPUTE_BIT))
        {
            context->queue_family = i;
            found = true;
            break;
        }
    }
    free(queue_properties);
    if (!found)
    {
        fprintf(stderr, "Selected Vulkan device exposes no compute queue.\n");
        return false;
    }

    queue_info.queueFamilyIndex = context->queue_family;
    if (enable_exact)
    {
        if (!has_device_extension(physical_device, exact_extensions[0]) ||
                !has_device_extension(physical_device, exact_extensions[1]) ||
                !has_device_extension(physical_device, exact_extensions[2]))
        {
            fprintf(stderr, "D22 requires mutable descriptors and descriptor buffers.\n");
            return false;
        }
        available_features.pNext = &available_indexing;
        available_indexing.pNext = &available_mutable;
        available_mutable.pNext = &available_address;
        available_address.pNext = &available_descriptor_buffer;
        vkGetPhysicalDeviceFeatures2(physical_device, &available_features);
        if (!available_features.features.shaderStorageImageReadWithoutFormat ||
                !available_features.features.shaderStorageImageWriteWithoutFormat ||
                !available_features.features.shaderStorageBufferArrayDynamicIndexing ||
                !available_indexing.runtimeDescriptorArray ||
                !available_mutable.mutableDescriptorType ||
                !available_address.bufferDeviceAddress ||
                !available_descriptor_buffer.descriptorBuffer)
        {
            fprintf(stderr, "D22 required descriptor/image features are unavailable.\n");
            return false;
        }
        enabled_features.shaderStorageImageReadWithoutFormat = VK_TRUE;
        enabled_features.shaderStorageImageWriteWithoutFormat = VK_TRUE;
        enabled_features.shaderStorageBufferArrayDynamicIndexing = VK_TRUE;
        enabled_indexing.runtimeDescriptorArray = VK_TRUE;
        enabled_indexing.pNext = &enabled_mutable;
        enabled_mutable.mutableDescriptorType = VK_TRUE;
        enabled_mutable.pNext = &enabled_address;
        enabled_address.bufferDeviceAddress = VK_TRUE;
        enabled_address.pNext = &enabled_descriptor_buffer;
        enabled_descriptor_buffer.descriptorBuffer = VK_TRUE;
        create_info.pEnabledFeatures = &enabled_features;
        create_info.pNext = &enabled_indexing;
        create_info.enabledExtensionCount = 3;
        create_info.ppEnabledExtensionNames = exact_extensions;
    }
    VK_CHECK_RETURN(vkCreateDevice(physical_device, &create_info, NULL, &context->device));
    vkGetDeviceQueue(context->device, context->queue_family, 0, &context->queue);
    vkGetPhysicalDeviceMemoryProperties(physical_device, &context->memory_properties);
    return true;
}

static bool choose_memory_type(const struct context *context, uint32_t type_bits,
        uint32_t *type_index, bool *coherent)
{
    for (uint32_t i = 0; i < context->memory_properties.memoryTypeCount; ++i)
    {
        VkMemoryPropertyFlags flags = context->memory_properties.memoryTypes[i].propertyFlags;
        if ((type_bits & (1u << i)) &&
                (flags & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) ==
                (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
        {
            *type_index = i;
            *coherent = true;
            return true;
        }
    }

    for (uint32_t i = 0; i < context->memory_properties.memoryTypeCount; ++i)
    {
        VkMemoryPropertyFlags flags = context->memory_properties.memoryTypes[i].propertyFlags;
        if ((type_bits & (1u << i)) && (flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
        {
            *type_index = i;
            *coherent = false;
            return true;
        }
    }

    return false;
}

static bool choose_device_memory_type(const struct context *context, uint32_t type_bits,
        uint32_t *type_index)
{
    for (uint32_t i = 0; i < context->memory_properties.memoryTypeCount; ++i)
    {
        VkMemoryPropertyFlags flags = context->memory_properties.memoryTypes[i].propertyFlags;
        if ((type_bits & (1u << i)) && (flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
        {
            *type_index = i;
            return true;
        }
    }
    for (uint32_t i = 0; i < context->memory_properties.memoryTypeCount; ++i)
    {
        if (type_bits & (1u << i))
        {
            *type_index = i;
            return true;
        }
    }
    return false;
}

static void destroy_host_buffer(const struct context *context, struct host_buffer *buffer)
{
    if (buffer->mapping)
        vkUnmapMemory(context->device, buffer->memory);
    if (buffer->buffer)
        vkDestroyBuffer(context->device, buffer->buffer, NULL);
    if (buffer->memory)
        vkFreeMemory(context->device, buffer->memory, NULL);
    memset(buffer, 0, sizeof(*buffer));
}

static bool create_host_buffer_extended(const struct context *context, VkDeviceSize size,
        VkBufferUsageFlags usage, bool device_address, struct host_buffer *buffer)
{
    VkMemoryAllocateFlagsInfo allocation_flags = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
        .flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT,
    };
    VkBufferCreateInfo buffer_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage | (device_address ? VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT : 0),
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    VkMemoryRequirements requirements;
    VkMemoryAllocateInfo allocation_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    };
    uint32_t type_index;

    memset(buffer, 0, sizeof(*buffer));
    buffer->logical_size = size;

    if (!check_vk(vkCreateBuffer(context->device, &buffer_info, NULL, &buffer->buffer),
            "vkCreateBuffer", __FILE__, __LINE__))
        goto fail;
    vkGetBufferMemoryRequirements(context->device, buffer->buffer, &requirements);
    if (!choose_memory_type(context, requirements.memoryTypeBits, &type_index,
            &buffer->coherent))
    {
        fprintf(stderr, "No host-visible memory type supports the test buffer.\n");
        goto fail;
    }

    allocation_info.allocationSize = requirements.size;
    allocation_info.memoryTypeIndex = type_index;
    if (device_address)
        allocation_info.pNext = &allocation_flags;
    buffer->allocation_size = requirements.size;
    if (!check_vk(vkAllocateMemory(context->device, &allocation_info, NULL, &buffer->memory),
            "vkAllocateMemory", __FILE__, __LINE__))
        goto fail;
    if (!check_vk(vkBindBufferMemory(context->device, buffer->buffer, buffer->memory, 0),
            "vkBindBufferMemory", __FILE__, __LINE__))
        goto fail;
    if (!check_vk(vkMapMemory(context->device, buffer->memory, 0, VK_WHOLE_SIZE, 0,
            &buffer->mapping), "vkMapMemory", __FILE__, __LINE__))
        goto fail;

    return true;

fail:
    destroy_host_buffer(context, buffer);
    return false;
}

static bool create_host_buffer(const struct context *context, VkDeviceSize size,
        VkBufferUsageFlags usage, struct host_buffer *buffer)
{
    return create_host_buffer_extended(context, size, usage, false, buffer);
}

static bool create_host_address_buffer(const struct context *context, VkDeviceSize size,
        VkBufferUsageFlags usage, struct host_buffer *buffer)
{
    return create_host_buffer_extended(context, size, usage, true, buffer);
}

static VkDeviceAddress get_buffer_address(const struct context *context, VkBuffer buffer)
{
    VkBufferDeviceAddressInfo info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .buffer = buffer,
    };
    return vkGetBufferDeviceAddress(context->device, &info);
}

static bool flush_host_buffer(const struct context *context,
        const struct host_buffer *buffer)
{
    VkMappedMemoryRange range = {
        .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
        .memory = buffer->memory,
        .offset = 0,
        .size = VK_WHOLE_SIZE,
    };

    if (buffer->coherent)
        return true;
    VK_CHECK_RETURN(vkFlushMappedMemoryRanges(context->device, 1, &range));
    return true;
}

static bool invalidate_host_buffer(const struct context *context,
        const struct host_buffer *buffer)
{
    VkMappedMemoryRange range = {
        .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
        .memory = buffer->memory,
        .offset = 0,
        .size = VK_WHOLE_SIZE,
    };

    if (buffer->coherent)
        return true;
    VK_CHECK_RETURN(vkInvalidateMappedMemoryRanges(context->device, 1, &range));
    return true;
}

static void destroy_device_image(const struct context *context, struct device_image *image)
{
    if (image->view)
        vkDestroyImageView(context->device, image->view, NULL);
    if (image->image)
        vkDestroyImage(context->device, image->image, NULL);
    if (image->memory)
        vkFreeMemory(context->device, image->memory, NULL);
    memset(image, 0, sizeof(*image));
}

static bool create_light_grid_image(const struct context *context,
        struct device_image *image)
{
    VkImageCreateInfo image_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_3D,
        .format = VK_FORMAT_R32_UINT,
        .extent = {ACTIVE_WIDTH, ACTIVE_HEIGHT, 2},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    VkMemoryRequirements requirements;
    VkMemoryAllocateInfo allocation_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    };
    VkImageViewCreateInfo view_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .viewType = VK_IMAGE_VIEW_TYPE_3D,
        .format = VK_FORMAT_R32_UINT,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };
    uint32_t memory_type;

    memset(image, 0, sizeof(*image));
    if (!check_vk(vkCreateImage(context->device, &image_info, NULL, &image->image),
            "vkCreateImage", __FILE__, __LINE__))
        goto fail;
    vkGetImageMemoryRequirements(context->device, image->image, &requirements);
    if (!choose_device_memory_type(context, requirements.memoryTypeBits, &memory_type))
    {
        fprintf(stderr, "No memory type supports the D22 light-grid image.\n");
        goto fail;
    }
    allocation_info.allocationSize = requirements.size;
    allocation_info.memoryTypeIndex = memory_type;
    if (!check_vk(vkAllocateMemory(context->device, &allocation_info, NULL, &image->memory),
            "vkAllocateMemory(image)", __FILE__, __LINE__))
        goto fail;
    if (!check_vk(vkBindImageMemory(context->device, image->image, image->memory, 0),
            "vkBindImageMemory", __FILE__, __LINE__))
        goto fail;
    view_info.image = image->image;
    if (!check_vk(vkCreateImageView(context->device, &view_info, NULL, &image->view),
            "vkCreateImageView", __FILE__, __LINE__))
        goto fail;
    return true;

fail:
    destroy_device_image(context, image);
    return false;
}

static uint32_t *load_spirv(const char *path, size_t *size)
{
    FILE *file = NULL;
    uint32_t *data = NULL;
    long file_size;

    *size = 0;
    file = fopen(path, "rb");
    if (!file)
    {
        fprintf(stderr, "Cannot open SPIR-V file %s: %s\n", path, strerror(errno));
        return NULL;
    }
    if (fseek(file, 0, SEEK_END) || (file_size = ftell(file)) < 0 ||
            fseek(file, 0, SEEK_SET))
    {
        fprintf(stderr, "Cannot determine SPIR-V file size: %s\n", path);
        goto done;
    }
    if (!file_size || (file_size % 4))
    {
        fprintf(stderr, "Invalid SPIR-V byte size %ld: %s\n", file_size, path);
        goto done;
    }
    data = malloc((size_t)file_size);
    if (!data)
        goto done;
    if (fread(data, 1, (size_t)file_size, file) != (size_t)file_size)
    {
        fprintf(stderr, "Cannot read complete SPIR-V file: %s\n", path);
        free(data);
        data = NULL;
        goto done;
    }
    if (data[0] != 0x07230203u)
    {
        fprintf(stderr, "Invalid SPIR-V magic: %s\n", path);
        free(data);
        data = NULL;
        goto done;
    }
    *size = (size_t)file_size;

done:
    fclose(file);
    return data;
}

static bool collect_atomic_metrics(const struct host_buffer *counter,
        const struct host_buffer *output, struct atomic_metrics *metrics)
{
    const uint32_t *values = output->mapping;
    uint32_t *seen = calloc(ACTIVE_COUNT, sizeof(*seen));

    memset(metrics, 0, sizeof(*metrics));
    metrics->final_counter = *(const uint32_t *)counter->mapping;

    if (!seen)
    {
        fprintf(stderr, "Out of memory while checking atomic output.\n");
        return false;
    }

    for (uint32_t i = 0; i < ACTIVE_COUNT; ++i)
    {
        uint32_t value = values[i];
        if (value == SENTINEL)
        {
            ++metrics->sentinel;
            continue;
        }
        if (value >= ACTIVE_COUNT)
        {
            ++metrics->out_of_range;
            continue;
        }
        if (seen[value]++)
            ++metrics->duplicates;
        else
            ++metrics->unique;
    }
    for (uint32_t i = 0; i < ACTIVE_COUNT; ++i)
    {
        if (!seen[i])
            ++metrics->missing;
    }

    for (uint32_t group_y = 0; group_y < DISPATCH_Y; ++group_y)
    {
        for (uint32_t group_x = 0; group_x < DISPATCH_X; ++group_x)
        {
            uint32_t minimum = UINT32_MAX;
            bool has_zero = false;

            for (uint32_t local_y = 0; local_y < GROUP_SIZE_Y; ++local_y)
            {
                uint32_t y = group_y * GROUP_SIZE_Y + local_y;
                if (y >= ACTIVE_HEIGHT)
                    continue;
                for (uint32_t local_x = 0; local_x < GROUP_SIZE_X; ++local_x)
                {
                    uint32_t x = group_x * GROUP_SIZE_X + local_x;
                    uint32_t value;
                    if (x >= ACTIVE_WIDTH)
                        continue;
                    value = values[y * ACTIVE_WIDTH + x];
                    if (value < minimum)
                        minimum = value;
                    if (value == 0)
                        has_zero = true;
                }
            }

            if (has_zero)
                ++metrics->groups_containing_zero;
            if (minimum == 0)
                ++metrics->groups_min_zero;
        }
    }

    metrics->globally_correct = metrics->final_counter == ACTIVE_COUNT &&
            metrics->unique == ACTIVE_COUNT && !metrics->missing &&
            !metrics->duplicates && !metrics->out_of_range && !metrics->sentinel &&
            metrics->groups_containing_zero == 1 && metrics->groups_min_zero == 1;

    free(seen);
    return true;
}

static bool analyze_result(enum atomic_variant variant, const struct host_buffer *counter,
        const struct host_buffer *output, uint32_t validation_errors_before)
{
    const char *name = variant == ATOMIC_TEXEL ? "texel" : "ssbo";
    struct atomic_metrics metrics;
    bool pass;

    if (!collect_atomic_metrics(counter, output, &metrics))
        return false;

    pass = metrics.globally_correct && validation_errors == validation_errors_before;

    printf("D21_RESULT variant=%s status=%s counter=%u expected=%u unique=%u "
           "missing=%u duplicates=%u out_of_range=%u sentinel=%u "
           "groups_containing_zero=%u groups_min_zero=%u validation_errors=%u\n",
            name, pass ? "PASS" : "FAIL", metrics.final_counter, ACTIVE_COUNT,
            metrics.unique, metrics.missing, metrics.duplicates, metrics.out_of_range,
            metrics.sentinel, metrics.groups_containing_zero, metrics.groups_min_zero,
            validation_errors - validation_errors_before);

    return pass;
}

static bool run_variant(const struct context *context, enum atomic_variant variant,
        const char *spv_path)
{
    const char *name = variant == ATOMIC_TEXEL ? "texel" : "ssbo";
    const VkDescriptorType counter_descriptor = variant == ATOMIC_TEXEL ?
            VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    struct host_buffer counter = {0}, output = {0};
    VkBufferView counter_view = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptor_set_layout = VK_NULL_HANDLE;
    VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;
    VkDescriptorSet descriptor_set = VK_NULL_HANDLE;
    VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
    VkShaderModule shader_module = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkCommandPool command_pool = VK_NULL_HANDLE;
    VkCommandBuffer command_buffer = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;
    uint32_t *spirv = NULL;
    size_t spirv_size = 0;
    bool success = false;
    uint32_t errors_before = validation_errors;
    VkBufferUsageFlags counter_usage = variant == ATOMIC_TEXEL ?
            VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT : VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    VkBufferCreateInfo unused_buffer_info;
    VkBufferViewCreateInfo view_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO,
        .format = VK_FORMAT_R32_UINT,
        .offset = 0,
        .range = sizeof(uint32_t),
    };
    VkDescriptorSetLayoutBinding bindings[2] = {
        {
            .binding = 0,
            .descriptorType = counter_descriptor,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        },
        {
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        },
    };
    VkDescriptorSetLayoutCreateInfo set_layout_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 2,
        .pBindings = bindings,
    };
    VkDescriptorPoolSize pool_sizes[2] = {
        {.type = counter_descriptor, .descriptorCount = 1},
        {.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = 1},
    };
    VkDescriptorPoolCreateInfo pool_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = 1,
        .poolSizeCount = variant == ATOMIC_TEXEL ? 2u : 1u,
        .pPoolSizes = pool_sizes,
    };
    VkDescriptorSetAllocateInfo set_allocate_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorSetCount = 1,
    };
    VkDescriptorBufferInfo counter_buffer_info = {
        .offset = 0,
        .range = sizeof(uint32_t),
    };
    VkDescriptorBufferInfo output_buffer_info = {
        .offset = 0,
        .range = ACTIVE_COUNT * sizeof(uint32_t),
    };
    VkWriteDescriptorSet writes[2] = {
        {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstBinding = 0,
            .descriptorCount = 1,
            .descriptorType = counter_descriptor,
        },
        {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstBinding = 1,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pBufferInfo = &output_buffer_info,
        },
    };
    VkPipelineLayoutCreateInfo pipeline_layout_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
    };
    VkShaderModuleCreateInfo shader_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    };
    VkPipelineShaderStageCreateInfo stage_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_COMPUTE_BIT,
        .pName = "main",
    };
    VkComputePipelineCreateInfo pipeline_info = {
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .stage = stage_info,
    };
    VkCommandPoolCreateInfo command_pool_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = context->queue_family,
    };
    VkCommandBufferAllocateInfo command_allocate_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    VkBufferMemoryBarrier barriers[2] = {
        {
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_HOST_READ_BIT,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .offset = 0,
            .size = VK_WHOLE_SIZE,
        },
        {
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_HOST_READ_BIT,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .offset = 0,
            .size = VK_WHOLE_SIZE,
        },
    };
    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &command_buffer,
    };
    VkFenceCreateInfo fence_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    };

    (void)unused_buffer_info;
    printf("D21_BEGIN variant=%s shader=%s\n", name, spv_path);

    if (!create_host_buffer(context, sizeof(uint32_t), counter_usage, &counter) ||
            !create_host_buffer(context, ACTIVE_COUNT * sizeof(uint32_t),
                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, &output))
        goto cleanup;

    *(uint32_t *)counter.mapping = 0;
    memset(output.mapping, 0xff, (size_t)output.logical_size);
    if (!flush_host_buffer(context, &counter) || !flush_host_buffer(context, &output))
        goto cleanup;

    if (variant == ATOMIC_TEXEL)
    {
        view_info.buffer = counter.buffer;
        VK_CHECK_GOTO(vkCreateBufferView(context->device, &view_info, NULL, &counter_view));
    }

    VK_CHECK_GOTO(vkCreateDescriptorSetLayout(context->device, &set_layout_info, NULL,
            &descriptor_set_layout));

    if (variant == ATOMIC_SSBO)
        pool_sizes[0].descriptorCount = 2;
    VK_CHECK_GOTO(vkCreateDescriptorPool(context->device, &pool_info, NULL, &descriptor_pool));

    set_allocate_info.descriptorPool = descriptor_pool;
    set_allocate_info.pSetLayouts = &descriptor_set_layout;
    VK_CHECK_GOTO(vkAllocateDescriptorSets(context->device, &set_allocate_info, &descriptor_set));

    counter_buffer_info.buffer = counter.buffer;
    output_buffer_info.buffer = output.buffer;
    writes[0].dstSet = descriptor_set;
    writes[1].dstSet = descriptor_set;
    if (variant == ATOMIC_TEXEL)
        writes[0].pTexelBufferView = &counter_view;
    else
        writes[0].pBufferInfo = &counter_buffer_info;
    vkUpdateDescriptorSets(context->device, 2, writes, 0, NULL);

    pipeline_layout_info.pSetLayouts = &descriptor_set_layout;
    VK_CHECK_GOTO(vkCreatePipelineLayout(context->device, &pipeline_layout_info, NULL,
            &pipeline_layout));

    spirv = load_spirv(spv_path, &spirv_size);
    if (!spirv)
        goto cleanup;
    shader_info.codeSize = spirv_size;
    shader_info.pCode = spirv;
    VK_CHECK_GOTO(vkCreateShaderModule(context->device, &shader_info, NULL, &shader_module));

    stage_info.module = shader_module;
    pipeline_info.stage = stage_info;
    pipeline_info.layout = pipeline_layout;
    VK_CHECK_GOTO(vkCreateComputePipelines(context->device, VK_NULL_HANDLE, 1,
            &pipeline_info, NULL, &pipeline));

    VK_CHECK_GOTO(vkCreateCommandPool(context->device, &command_pool_info, NULL,
            &command_pool));
    command_allocate_info.commandPool = command_pool;
    VK_CHECK_GOTO(vkAllocateCommandBuffers(context->device, &command_allocate_info,
            &command_buffer));
    VK_CHECK_GOTO(vkBeginCommandBuffer(command_buffer, &begin_info));
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE,
            pipeline_layout, 0, 1, &descriptor_set, 0, NULL);
    vkCmdDispatch(command_buffer, DISPATCH_X, DISPATCH_Y, 1);
    barriers[0].buffer = counter.buffer;
    barriers[1].buffer = output.buffer;
    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_PIPELINE_STAGE_HOST_BIT, 0, 0, NULL, 2, barriers, 0, NULL);
    VK_CHECK_GOTO(vkEndCommandBuffer(command_buffer));

    VK_CHECK_GOTO(vkCreateFence(context->device, &fence_info, NULL, &fence));
    VK_CHECK_GOTO(vkQueueSubmit(context->queue, 1, &submit_info, fence));
    VK_CHECK_GOTO(vkWaitForFences(context->device, 1, &fence, VK_TRUE, UINT64_MAX));

    if (!invalidate_host_buffer(context, &counter) ||
            !invalidate_host_buffer(context, &output))
        goto cleanup;
    success = analyze_result(variant, &counter, &output, errors_before);

cleanup:
    if (context->device)
        vkDeviceWaitIdle(context->device);
    if (fence)
        vkDestroyFence(context->device, fence, NULL);
    if (command_pool)
        vkDestroyCommandPool(context->device, command_pool, NULL);
    if (pipeline)
        vkDestroyPipeline(context->device, pipeline, NULL);
    if (shader_module)
        vkDestroyShaderModule(context->device, shader_module, NULL);
    if (pipeline_layout)
        vkDestroyPipelineLayout(context->device, pipeline_layout, NULL);
    if (descriptor_pool)
        vkDestroyDescriptorPool(context->device, descriptor_pool, NULL);
    if (descriptor_set_layout)
        vkDestroyDescriptorSetLayout(context->device, descriptor_set_layout, NULL);
    if (counter_view)
        vkDestroyBufferView(context->device, counter_view, NULL);
    destroy_host_buffer(context, &output);
    destroy_host_buffer(context, &counter);
    free(spirv);
    return success;
}

static void print_counter_prefix(const char *run, const struct host_buffer *counter)
{
    const uint8_t *bytes = counter->mapping;
    size_t count = counter->logical_size < 16u ? (size_t)counter->logical_size : 16u;
    size_t nonzero_after_word = 0;
    size_t first_nonzero_after_word = SIZE_MAX;
    uint32_t u32_0 = 0;
    uint16_t u16_0 = 0, u16_1 = 0;

    if (counter->logical_size >= sizeof(u32_0))
    {
        memcpy(&u32_0, bytes, sizeof(u32_0));
        memcpy(&u16_0, bytes, sizeof(u16_0));
        memcpy(&u16_1, bytes + sizeof(u16_0), sizeof(u16_1));
    }
    for (size_t i = sizeof(u32_0); i < (size_t)counter->logical_size; ++i)
    {
        if (bytes[i])
        {
            if (first_nonzero_after_word == SIZE_MAX)
                first_nonzero_after_word = i;
            ++nonzero_after_word;
        }
    }

    printf("D50_RAW run=%s u32_0=%u u16_0=%u u16_1=%u bytes=", run,
            u32_0, u16_0, u16_1);
    for (size_t i = 0; i < count; ++i)
        printf("%02x", bytes[i]);
    printf(" nonzero_bytes_after_word=%zu first_nonzero_after_word=",
            nonzero_after_word);
    if (first_nonzero_after_word == SIZE_MAX)
        printf("none\n");
    else
        printf("%zu\n", first_nonzero_after_word);
}

static bool run_minimal_format_ab(const struct context *context, const char *spv_path)
{
    static const unsigned int sequence[] = {0, 1, 0};
    static const char *run_names[] = {"r32-first", "r16", "r32-second"};
    static const VkFormat formats[] = {VK_FORMAT_R32_UINT, VK_FORMAT_R16_UINT};
    struct host_buffer counter = {0}, output = {0};
    VkBufferView counter_views[2] = {VK_NULL_HANDLE, VK_NULL_HANDLE};
    VkDescriptorSetLayout descriptor_set_layout = VK_NULL_HANDLE;
    VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;
    VkDescriptorSet descriptor_sets[2] = {VK_NULL_HANDLE, VK_NULL_HANDLE};
    VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
    VkShaderModule shader_module = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkCommandPool command_pool = VK_NULL_HANDLE;
    VkCommandBuffer command_buffer = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;
    uint32_t *spirv = NULL;
    size_t spirv_size = 0;
    bool r32_first_pass = false, r16_correct = false, r32_second_pass = false;
    bool success = false;
    VkFormatProperties format_properties[2];
    VkBufferViewCreateInfo view_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO,
        .offset = 0,
        .range = LIVE_COUNTER_SIZE,
    };
    VkDescriptorSetLayoutBinding bindings[2] = {
        {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        },
        {
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        },
    };
    VkDescriptorSetLayoutCreateInfo set_layout_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 2,
        .pBindings = bindings,
    };
    VkDescriptorPoolSize pool_sizes[2] = {
        {.type = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, .descriptorCount = 2},
        {.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = 2},
    };
    VkDescriptorPoolCreateInfo pool_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = 2,
        .poolSizeCount = 2,
        .pPoolSizes = pool_sizes,
    };
    VkDescriptorSetLayout allocated_layouts[2];
    VkDescriptorSetAllocateInfo set_allocate_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorSetCount = 2,
        .pSetLayouts = allocated_layouts,
    };
    VkDescriptorBufferInfo output_buffer_info = {
        .offset = 0,
        .range = ACTIVE_COUNT * sizeof(uint32_t),
    };
    VkWriteDescriptorSet writes[4] = {0};
    VkPipelineLayoutCreateInfo pipeline_layout_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
    };
    VkShaderModuleCreateInfo shader_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    };
    VkPipelineShaderStageCreateInfo stage_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_COMPUTE_BIT,
        .pName = "main",
    };
    VkComputePipelineCreateInfo pipeline_info = {
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
    };
    VkCommandPoolCreateInfo command_pool_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = context->queue_family,
    };
    VkCommandBufferAllocateInfo command_allocate_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    VkBufferMemoryBarrier barriers[2] = {
        {
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_HOST_READ_BIT,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .offset = 0,
            .size = VK_WHOLE_SIZE,
        },
        {
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_HOST_READ_BIT,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .offset = 0,
            .size = VK_WHOLE_SIZE,
        },
    };
    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &command_buffer,
    };
    VkFenceCreateInfo fence_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    };

    vkGetPhysicalDeviceFormatProperties(context->physical_device, formats[0],
            &format_properties[0]);
    vkGetPhysicalDeviceFormatProperties(context->physical_device, formats[1],
            &format_properties[1]);
    printf("D50_BEGIN variant=minimal-coordinate-zero-format-ab shader=%s "
           "counter_size=%u r32_atomic=%s r16_atomic=%s\n",
            spv_path, LIVE_COUNTER_SIZE,
            (format_properties[0].bufferFeatures &
                    VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_ATOMIC_BIT) ? "yes" : "no",
            (format_properties[1].bufferFeatures &
                    VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_ATOMIC_BIT) ? "yes" : "no");

    if (!create_host_buffer(context, LIVE_COUNTER_SIZE,
            VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT, &counter) ||
            !create_host_buffer(context, ACTIVE_COUNT * sizeof(uint32_t),
                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, &output))
        goto cleanup;

    view_info.buffer = counter.buffer;
    for (unsigned int i = 0; i < 2; ++i)
    {
        view_info.format = formats[i];
        VK_CHECK_GOTO(vkCreateBufferView(context->device, &view_info, NULL,
                &counter_views[i]));
    }

    VK_CHECK_GOTO(vkCreateDescriptorSetLayout(context->device, &set_layout_info, NULL,
            &descriptor_set_layout));
    VK_CHECK_GOTO(vkCreateDescriptorPool(context->device, &pool_info, NULL,
            &descriptor_pool));
    allocated_layouts[0] = descriptor_set_layout;
    allocated_layouts[1] = descriptor_set_layout;
    set_allocate_info.descriptorPool = descriptor_pool;
    VK_CHECK_GOTO(vkAllocateDescriptorSets(context->device, &set_allocate_info,
            descriptor_sets));

    output_buffer_info.buffer = output.buffer;
    for (unsigned int i = 0; i < 2; ++i)
    {
        writes[2u * i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[2u * i].dstSet = descriptor_sets[i];
        writes[2u * i].dstBinding = 0;
        writes[2u * i].descriptorCount = 1;
        writes[2u * i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
        writes[2u * i].pTexelBufferView = &counter_views[i];
        writes[2u * i + 1u].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[2u * i + 1u].dstSet = descriptor_sets[i];
        writes[2u * i + 1u].dstBinding = 1;
        writes[2u * i + 1u].descriptorCount = 1;
        writes[2u * i + 1u].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[2u * i + 1u].pBufferInfo = &output_buffer_info;
    }
    vkUpdateDescriptorSets(context->device, 4, writes, 0, NULL);

    pipeline_layout_info.pSetLayouts = &descriptor_set_layout;
    VK_CHECK_GOTO(vkCreatePipelineLayout(context->device, &pipeline_layout_info, NULL,
            &pipeline_layout));
    spirv = load_spirv(spv_path, &spirv_size);
    if (!spirv)
        goto cleanup;
    shader_info.codeSize = spirv_size;
    shader_info.pCode = spirv;
    VK_CHECK_GOTO(vkCreateShaderModule(context->device, &shader_info, NULL,
            &shader_module));
    stage_info.module = shader_module;
    pipeline_info.stage = stage_info;
    pipeline_info.layout = pipeline_layout;
    VK_CHECK_GOTO(vkCreateComputePipelines(context->device, VK_NULL_HANDLE, 1,
            &pipeline_info, NULL, &pipeline));

    VK_CHECK_GOTO(vkCreateCommandPool(context->device, &command_pool_info, NULL,
            &command_pool));
    command_allocate_info.commandPool = command_pool;
    VK_CHECK_GOTO(vkAllocateCommandBuffers(context->device, &command_allocate_info,
            &command_buffer));
    VK_CHECK_GOTO(vkCreateFence(context->device, &fence_info, NULL, &fence));
    barriers[0].buffer = counter.buffer;
    barriers[1].buffer = output.buffer;

    for (unsigned int run = 0; run < 3; ++run)
    {
        const unsigned int descriptor_index = sequence[run];
        struct atomic_metrics metrics;
        uint32_t errors_before = validation_errors;
        uint32_t warnings_before = validation_warnings;
        bool valid_run;

        if (run)
        {
            VK_CHECK_GOTO(vkResetFences(context->device, 1, &fence));
            VK_CHECK_GOTO(vkResetCommandPool(context->device, command_pool, 0));
        }
        memset(counter.mapping, 0, (size_t)counter.logical_size);
        memset(output.mapping, 0xff, (size_t)output.logical_size);
        if (!flush_host_buffer(context, &counter) || !flush_host_buffer(context, &output))
            goto cleanup;

        VK_CHECK_GOTO(vkBeginCommandBuffer(command_buffer, &begin_info));
        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
        vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                pipeline_layout, 0, 1, &descriptor_sets[descriptor_index], 0, NULL);
        vkCmdDispatch(command_buffer, DISPATCH_X, DISPATCH_Y, 1);
        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                VK_PIPELINE_STAGE_HOST_BIT, 0, 0, NULL, 2, barriers, 0, NULL);
        VK_CHECK_GOTO(vkEndCommandBuffer(command_buffer));
        VK_CHECK_GOTO(vkQueueSubmit(context->queue, 1, &submit_info, fence));
        VK_CHECK_GOTO(vkWaitForFences(context->device, 1, &fence, VK_TRUE, UINT64_MAX));

        if (!invalidate_host_buffer(context, &counter) ||
                !invalidate_host_buffer(context, &output) ||
                !collect_atomic_metrics(&counter, &output, &metrics))
            goto cleanup;
        valid_run = metrics.globally_correct && validation_errors == errors_before;
        printf("D50_RESULT run=%s view=%s output_status=%s counter=%u expected=%u "
               "unique=%u missing=%u duplicates=%u out_of_range=%u sentinel=%u "
               "groups_containing_zero=%u groups_min_zero=%u "
               "validation_errors=%u validation_warnings=%u\n",
                run_names[run], descriptor_index ? "R16_UINT" : "R32_UINT",
                metrics.globally_correct ? "GLOBAL_CORRECT" : "CORRUPT",
                metrics.final_counter, ACTIVE_COUNT, metrics.unique, metrics.missing,
                metrics.duplicates, metrics.out_of_range, metrics.sentinel,
                metrics.groups_containing_zero, metrics.groups_min_zero,
                validation_errors - errors_before, validation_warnings - warnings_before);
        print_counter_prefix(run_names[run], &counter);

        if (!run)
            r32_first_pass = valid_run;
        else if (run == 1)
            r16_correct = metrics.globally_correct;
        else
            r32_second_pass = valid_run;
    }

    success = r32_first_pass && r32_second_pass;
    printf("D50_SUMMARY r32_first=%s r16=%s r32_second=%s status=%s\n",
            r32_first_pass ? "PASS" : "FAIL",
            r16_correct ? "GLOBAL_CORRECT" : "CORRUPT",
            r32_second_pass ? "PASS" : "FAIL", success ? "EXECUTED" : "FAIL");

cleanup:
    if (context->device)
        vkDeviceWaitIdle(context->device);
    if (fence)
        vkDestroyFence(context->device, fence, NULL);
    if (command_pool)
        vkDestroyCommandPool(context->device, command_pool, NULL);
    if (pipeline)
        vkDestroyPipeline(context->device, pipeline, NULL);
    if (shader_module)
        vkDestroyShaderModule(context->device, shader_module, NULL);
    if (pipeline_layout)
        vkDestroyPipelineLayout(context->device, pipeline_layout, NULL);
    if (descriptor_pool)
        vkDestroyDescriptorPool(context->device, descriptor_pool, NULL);
    if (descriptor_set_layout)
        vkDestroyDescriptorSetLayout(context->device, descriptor_set_layout, NULL);
    for (unsigned int i = 0; i < 2; ++i)
    {
        if (counter_views[i])
            vkDestroyBufferView(context->device, counter_views[i], NULL);
    }
    destroy_host_buffer(context, &output);
    destroy_host_buffer(context, &counter);
    free(spirv);
    return success;
}

static uint32_t d22_input_count(uint32_t x, uint32_t y)
{
    return 1u + ((x + 3u * y) % 5u);
}

static bool analyze_exact_result(const struct host_buffer *counter,
        const struct host_buffer *staging, uint32_t expected_total,
        uint32_t validation_errors_before, bool descriptor_buffer_backend,
        VkFormat counter_format, VkDeviceSize counter_size,
        bool counter_as_storage_buffer)
{
    const uint32_t *grid = staging->mapping;
    const uint32_t final_counter = *(const uint32_t *)counter->mapping;
    uint32_t *coverage = calloc(expected_total, sizeof(*coverage));
    uint32_t count_mismatch = 0, out_of_range = 0, overlap = 0, missing = 0;
    uint32_t layer1_mismatch = 0, groups_containing_zero = 0;
    size_t counter_tail_nonzero = 0;
    bool correct;

    if (!coverage)
    {
        fprintf(stderr, "Out of memory while checking D22 output.\n");
        return false;
    }

    for (uint32_t y = 0; y < ACTIVE_HEIGHT; ++y)
    {
        for (uint32_t x = 0; x < ACTIVE_WIDTH; ++x)
        {
            uint32_t index = y * ACTIVE_WIDTH + x;
            uint32_t packed = grid[index];
            uint32_t count = packed & 1023u;
            uint32_t start = packed >> 10;
            uint32_t expected_count = d22_input_count(x, y);

            if (count != expected_count)
                ++count_mismatch;
            if ((grid[ACTIVE_COUNT + index] >> 10) != start ||
                    (grid[ACTIVE_COUNT + index] & 1023u))
                ++layer1_mismatch;
            if (start > expected_total || count > expected_total - start)
            {
                ++out_of_range;
                continue;
            }
            for (uint32_t i = 0; i < count; ++i)
            {
                if (coverage[start + i]++)
                    ++overlap;
            }
        }
    }

    for (uint32_t i = 0; i < expected_total; ++i)
    {
        if (!coverage[i])
            ++missing;
    }

    for (size_t i = sizeof(uint32_t); i < (size_t)counter->logical_size; ++i)
    {
        if (((const uint8_t *)counter->mapping)[i])
            ++counter_tail_nonzero;
    }

    for (uint32_t group_y = 0; group_y < DISPATCH_Y; ++group_y)
    {
        for (uint32_t group_x = 0; group_x < DISPATCH_X; ++group_x)
        {
            bool has_zero = false;
            for (uint32_t local_y = 0; local_y < GROUP_SIZE_Y; ++local_y)
            {
                uint32_t y = group_y * GROUP_SIZE_Y + local_y;
                if (y >= ACTIVE_HEIGHT)
                    continue;
                for (uint32_t local_x = 0; local_x < GROUP_SIZE_X; ++local_x)
                {
                    uint32_t x = group_x * GROUP_SIZE_X + local_x;
                    uint32_t start;
                    if (x >= ACTIVE_WIDTH)
                        continue;
                    start = grid[y * ACTIVE_WIDTH + x] >> 10;
                    if (!start)
                        has_zero = true;
                }
            }
            if (has_zero)
                ++groups_containing_zero;
        }
    }

    correct = final_counter == expected_total && !count_mismatch && !out_of_range &&
            !overlap && !missing && !layer1_mismatch && !counter_tail_nonzero &&
            groups_containing_zero == 1 &&
            ((counter_format == VK_FORMAT_R16_UINT && !counter_as_storage_buffer) ||
                    validation_errors == validation_errors_before);

    if (counter_format == VK_FORMAT_R16_UINT && !counter_as_storage_buffer)
    {
        printf("D23_RESULT variant=exact-game-spv-r16-live-view backend=%s "
               "output_status=%s counter=%u expected=%u "
               "count_mismatch=%u out_of_range=%u overlap=%u missing=%u "
               "layer1_mismatch=%u groups_containing_zero=%u "
               "counter_tail_nonzero=%zu validation_errors=%u\n",
                descriptor_buffer_backend ? "descriptor-buffer" : "mutable-descriptor-set",
                correct ? "GLOBAL_CORRECT" : "CORRUPT", final_counter, expected_total,
                count_mismatch, out_of_range, overlap, missing, layer1_mismatch,
                groups_containing_zero, counter_tail_nonzero,
                validation_errors - validation_errors_before);
    }
    else if (counter_as_storage_buffer)
    {
        printf("D24_RESULT variant=exact-game-spv-r16-uav-ssbo-atomic backend=%s "
               "status=%s counter=%u expected=%u "
               "count_mismatch=%u out_of_range=%u overlap=%u missing=%u "
               "layer1_mismatch=%u groups_containing_zero=%u "
               "counter_tail_nonzero=%zu validation_errors=%u\n",
                descriptor_buffer_backend ? "descriptor-buffer" : "mutable-descriptor-set",
                correct ? "PASS" : "FAIL", final_counter, expected_total,
                count_mismatch, out_of_range, overlap, missing, layer1_mismatch,
                groups_containing_zero, counter_tail_nonzero,
                validation_errors - validation_errors_before);
    }
    else if (counter_size == LIVE_COUNTER_SIZE)
    {
        printf("D51_RESULT variant=exact-game-spv-r32-live-sized-alias backend=%s "
               "status=%s counter=%u expected=%u "
               "count_mismatch=%u out_of_range=%u overlap=%u missing=%u "
               "layer1_mismatch=%u groups_containing_zero=%u "
               "counter_tail_nonzero=%zu validation_errors=%u\n",
                descriptor_buffer_backend ? "descriptor-buffer" : "mutable-descriptor-set",
                correct ? "PASS" : "FAIL", final_counter, expected_total,
                count_mismatch, out_of_range, overlap, missing, layer1_mismatch,
                groups_containing_zero, counter_tail_nonzero,
                validation_errors - validation_errors_before);
    }
    else
    {
        printf("D22_RESULT variant=exact-game-spv backend=%s status=%s "
               "counter=%u expected=%u "
               "count_mismatch=%u out_of_range=%u overlap=%u missing=%u "
               "layer1_mismatch=%u groups_containing_zero=%u "
               "counter_tail_nonzero=%zu validation_errors=%u\n",
                descriptor_buffer_backend ? "descriptor-buffer" : "mutable-descriptor-set",
                correct ? "PASS" : "FAIL", final_counter, expected_total, count_mismatch,
                out_of_range, overlap, missing, layer1_mismatch, groups_containing_zero,
                counter_tail_nonzero,
                validation_errors - validation_errors_before);
    }

    free(coverage);
    /* D23 deliberately binds an invalid R16_UINT view to an R32ui atomic shader.
     * Reaching analysis is a successful experiment regardless of the observed data. */
    return (counter_format == VK_FORMAT_R16_UINT && !counter_as_storage_buffer) || correct;
}

static bool run_exact_variant(const struct context *context, const char *spv_path,
        bool descriptor_buffer_backend, VkFormat counter_format,
        VkDeviceSize counter_size,
        bool counter_as_storage_buffer)
{
    const VkDeviceSize grid_size = ACTIVE_COUNT * 2u * sizeof(uint32_t);
    const VkDeviceSize cbuffer_size = 4096u * 4u * sizeof(float);
    const bool r32_live_sized = counter_format == VK_FORMAT_R32_UINT &&
            !counter_as_storage_buffer && counter_size == LIVE_COUNTER_SIZE;
    const char *experiment = counter_as_storage_buffer ? "D24" :
            (counter_format == VK_FORMAT_R16_UINT ? "D23" :
                    (r32_live_sized ? "D51" : "D22"));
    struct host_buffer counter = {0}, staging = {0}, cbuffer = {0};
    struct host_buffer descriptor_buffers[2] = {{0}, {0}};
    struct device_image grid_image = {0};
    VkBufferView counter_view = VK_NULL_HANDLE;
    VkDescriptorSetLayout set_layouts[3] = {VK_NULL_HANDLE, VK_NULL_HANDLE,
            VK_NULL_HANDLE};
    VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;
    VkDescriptorSet descriptor_sets[2] = {VK_NULL_HANDLE, VK_NULL_HANDLE};
    VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
    VkShaderModule shader_module = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkCommandPool command_pool = VK_NULL_HANDLE;
    VkCommandBuffer command_buffer = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;
    uint32_t *spirv = NULL;
    size_t spirv_size = 0;
    uint32_t expected_total = 0;
    uint32_t errors_before = validation_errors;
    bool success = false;
    PFN_vkGetDescriptorSetLayoutSizeEXT get_layout_size = NULL;
    PFN_vkGetDescriptorSetLayoutBindingOffsetEXT get_binding_offset = NULL;
    PFN_vkGetDescriptorEXT get_descriptor = NULL;
    PFN_vkCmdBindDescriptorBuffersEXT cmd_bind_descriptor_buffers = NULL;
    PFN_vkCmdSetDescriptorBufferOffsetsEXT cmd_set_descriptor_offsets = NULL;
    VkPhysicalDeviceDescriptorBufferPropertiesEXT descriptor_properties = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_PROPERTIES_EXT,
    };
    VkPhysicalDeviceProperties2 properties2 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
        .pNext = &descriptor_properties,
    };
    bool (*create_bound_buffer)(const struct context *, VkDeviceSize,
            VkBufferUsageFlags, struct host_buffer *) = descriptor_buffer_backend ?
            create_host_address_buffer : create_host_buffer;
    VkBufferViewCreateInfo counter_view_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO,
        .format = counter_format,
        .offset = 0,
        .range = counter_size,
    };
    VkDescriptorSetLayoutCreateInfo empty_layout_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    };
    VkDescriptorType mutable_types[3] = {
        VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
    };
    VkMutableDescriptorTypeListEXT mutable_list = {
        .descriptorTypeCount = 3,
        .pDescriptorTypes = mutable_types,
    };
    VkMutableDescriptorTypeCreateInfoEXT mutable_layout_info = {
        .sType = VK_STRUCTURE_TYPE_MUTABLE_DESCRIPTOR_TYPE_CREATE_INFO_EXT,
        .mutableDescriptorTypeListCount = 1,
        .pMutableDescriptorTypeLists = &mutable_list,
    };
    VkDescriptorSetLayoutBinding mutable_binding = {
        .binding = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_MUTABLE_EXT,
        .descriptorCount = 2,
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
    };
    VkDescriptorSetLayoutCreateInfo mutable_set_layout_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = &mutable_layout_info,
        .bindingCount = 1,
        .pBindings = &mutable_binding,
    };
    VkDescriptorSetLayoutBinding uniform_binding = {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
    };
    VkDescriptorSetLayoutCreateInfo uniform_set_layout_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1,
        .pBindings = &uniform_binding,
    };
    VkDescriptorPoolSize pool_sizes[2] = {
        {.type = VK_DESCRIPTOR_TYPE_MUTABLE_EXT, .descriptorCount = 2},
        {.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = 1},
    };
    VkMutableDescriptorTypeListEXT pool_mutable_lists[2] = {
        {
            .descriptorTypeCount = 3,
            .pDescriptorTypes = mutable_types,
        },
        {0},
    };
    VkMutableDescriptorTypeCreateInfoEXT pool_mutable_info = {
        .sType = VK_STRUCTURE_TYPE_MUTABLE_DESCRIPTOR_TYPE_CREATE_INFO_EXT,
        .mutableDescriptorTypeListCount = 2,
        .pMutableDescriptorTypeLists = pool_mutable_lists,
    };
    VkDescriptorPoolCreateInfo pool_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = &pool_mutable_info,
        .maxSets = 2,
        .poolSizeCount = 2,
        .pPoolSizes = pool_sizes,
    };
    VkDescriptorSetLayout allocated_layouts[2];
    VkDescriptorSetAllocateInfo allocate_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorSetCount = 2,
        .pSetLayouts = allocated_layouts,
    };
    VkDescriptorImageInfo image_descriptor = {
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
    };
    VkDescriptorBufferInfo counter_descriptor = {
        .offset = 0,
        .range = counter_size,
    };
    VkDescriptorBufferInfo cbuffer_descriptor = {
        .offset = 0,
        .range = cbuffer_size,
    };
    VkWriteDescriptorSet writes[3] = {
        {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstBinding = 1,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .pImageInfo = &image_descriptor,
        },
        {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstBinding = 1,
            .dstArrayElement = 1,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
            .pTexelBufferView = &counter_view,
        },
        {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstBinding = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo = &cbuffer_descriptor,
        },
    };
    VkPushConstantRange push_range = {
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        .offset = 0,
        .size = 2u * sizeof(uint32_t),
    };
    VkPipelineLayoutCreateInfo pipeline_layout_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 3,
        .pSetLayouts = set_layouts,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &push_range,
    };
    VkShaderModuleCreateInfo shader_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    };
    VkPipelineShaderStageCreateInfo stage_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_COMPUTE_BIT,
        .pName = "main",
    };
    VkComputePipelineCreateInfo pipeline_info = {
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
    };
    VkCommandPoolCreateInfo command_pool_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = context->queue_family,
    };
    VkCommandBufferAllocateInfo command_allocate_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    VkBufferImageCopy copy_region = {
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
        .imageOffset = {0, 0, 0},
        .imageExtent = {ACTIVE_WIDTH, ACTIVE_HEIGHT, 2},
    };
    VkImageMemoryBarrier image_barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = VK_NULL_HANDLE,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };
    VkBufferMemoryBarrier buffer_barrier = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .offset = 0,
        .size = VK_WHOLE_SIZE,
    };
    uint32_t push_constants[2] = {0, 0};
    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &command_buffer,
    };
    VkFenceCreateInfo fence_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    };

    printf("%s_BEGIN variant=exact-game-spv%s shader=%s backend=%s "
           "counter_format=%s counter_range=%" PRIu64 "\n",
            experiment,
            counter_as_storage_buffer ? "-r16-uav-ssbo-atomic" :
                    (counter_format == VK_FORMAT_R16_UINT ? "-r16-live-view" :
                            (r32_live_sized ? "-r32-live-sized-alias" : "")),
            spv_path,
            descriptor_buffer_backend ? "descriptor-buffer" : "mutable-descriptor-set",
            counter_format == VK_FORMAT_R16_UINT ? "R16_UINT" : "R32_UINT",
            (uint64_t)counter_size);

    if (!create_bound_buffer(context, counter_size,
            VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT |
                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, &counter) ||
            !create_host_buffer(context, grid_size,
                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                    &staging) ||
            !create_bound_buffer(context, cbuffer_size,
                    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, &cbuffer) ||
            !create_light_grid_image(context, &grid_image))
        goto cleanup;

    memset(counter.mapping, 0, (size_t)counter_size);
    memset(staging.mapping, 0, (size_t)grid_size);
    memset(cbuffer.mapping, 0, (size_t)cbuffer_size);
    for (uint32_t y = 0; y < ACTIVE_HEIGHT; ++y)
    {
        for (uint32_t x = 0; x < ACTIVE_WIDTH; ++x)
        {
            uint32_t count = d22_input_count(x, y);
            ((uint32_t *)staging.mapping)[y * ACTIVE_WIDTH + x] = count;
            expected_total += count;
        }
    }
    ((float *)cbuffer.mapping)[17u * 4u + 0u] = (float)ACTIVE_WIDTH;
    ((float *)cbuffer.mapping)[17u * 4u + 1u] = (float)ACTIVE_HEIGHT;
    ((float *)cbuffer.mapping)[24u * 4u + 2u] = 43520.0f;
    if (!flush_host_buffer(context, &counter) ||
            !flush_host_buffer(context, &staging) ||
            !flush_host_buffer(context, &cbuffer))
        goto cleanup;

    if (descriptor_buffer_backend)
    {
        empty_layout_info.flags =
                VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;
        mutable_set_layout_info.flags =
                VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;
        uniform_set_layout_info.flags =
                VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;
    }
    VK_CHECK_GOTO(vkCreateDescriptorSetLayout(context->device, &empty_layout_info, NULL,
            &set_layouts[0]));
    VK_CHECK_GOTO(vkCreateDescriptorSetLayout(context->device, &mutable_set_layout_info,
            NULL, &set_layouts[1]));
    VK_CHECK_GOTO(vkCreateDescriptorSetLayout(context->device, &uniform_set_layout_info,
            NULL, &set_layouts[2]));
    image_descriptor.imageView = grid_image.view;
    counter_descriptor.buffer = counter.buffer;
    cbuffer_descriptor.buffer = cbuffer.buffer;

    if (!descriptor_buffer_backend)
    {
        if (!counter_as_storage_buffer)
        {
            counter_view_info.buffer = counter.buffer;
            VK_CHECK_GOTO(vkCreateBufferView(context->device, &counter_view_info, NULL,
                    &counter_view));
        }
        VK_CHECK_GOTO(vkCreateDescriptorPool(context->device, &pool_info, NULL,
                &descriptor_pool));
        allocated_layouts[0] = set_layouts[1];
        allocated_layouts[1] = set_layouts[2];
        allocate_info.descriptorPool = descriptor_pool;
        VK_CHECK_GOTO(vkAllocateDescriptorSets(context->device, &allocate_info,
                descriptor_sets));
        writes[0].dstSet = descriptor_sets[0];
        writes[1].dstSet = descriptor_sets[0];
        writes[2].dstSet = descriptor_sets[1];
        if (counter_as_storage_buffer)
        {
            writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            writes[1].pTexelBufferView = NULL;
            writes[1].pBufferInfo = &counter_descriptor;
        }
        vkUpdateDescriptorSets(context->device, 3, writes, 0, NULL);
    }
    else
    {
        VkDeviceSize layout_sizes[2] = {0, 0};
        VkDeviceSize binding_offsets[2] = {0, 0};
        VkDeviceSize mutable_stride;
        VkDeviceAddress counter_address = get_buffer_address(context, counter.buffer);
        VkDeviceAddress cbuffer_address = get_buffer_address(context, cbuffer.buffer);
        VkDescriptorAddressInfoEXT counter_address_info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT,
            .address = counter_address,
            .range = counter_size,
            .format = counter_as_storage_buffer ? VK_FORMAT_UNDEFINED : counter_format,
        };
        VkDescriptorAddressInfoEXT cbuffer_address_info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT,
            .address = cbuffer_address,
            .range = cbuffer_size,
            .format = VK_FORMAT_UNDEFINED,
        };
        VkDescriptorGetInfoEXT descriptor_info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
        };

        get_layout_size = (PFN_vkGetDescriptorSetLayoutSizeEXT)
                vkGetDeviceProcAddr(context->device, "vkGetDescriptorSetLayoutSizeEXT");
        get_binding_offset = (PFN_vkGetDescriptorSetLayoutBindingOffsetEXT)
                vkGetDeviceProcAddr(context->device,
                        "vkGetDescriptorSetLayoutBindingOffsetEXT");
        get_descriptor = (PFN_vkGetDescriptorEXT)
                vkGetDeviceProcAddr(context->device, "vkGetDescriptorEXT");
        cmd_bind_descriptor_buffers = (PFN_vkCmdBindDescriptorBuffersEXT)
                vkGetDeviceProcAddr(context->device, "vkCmdBindDescriptorBuffersEXT");
        cmd_set_descriptor_offsets = (PFN_vkCmdSetDescriptorBufferOffsetsEXT)
                vkGetDeviceProcAddr(context->device,
                        "vkCmdSetDescriptorBufferOffsetsEXT");
        if (!get_layout_size || !get_binding_offset || !get_descriptor ||
                !cmd_bind_descriptor_buffers || !cmd_set_descriptor_offsets)
        {
            fprintf(stderr, "%s descriptor-buffer entry points are unavailable.\n",
                    experiment);
            goto cleanup;
        }

        vkGetPhysicalDeviceProperties2(context->physical_device, &properties2);
        get_layout_size(context->device, set_layouts[1], &layout_sizes[0]);
        get_layout_size(context->device, set_layouts[2], &layout_sizes[1]);
        get_binding_offset(context->device, set_layouts[1], 1, &binding_offsets[0]);
        get_binding_offset(context->device, set_layouts[2], 0, &binding_offsets[1]);
        mutable_stride = descriptor_properties.storageImageDescriptorSize >
                descriptor_properties.storageTexelBufferDescriptorSize ?
                descriptor_properties.storageImageDescriptorSize :
                descriptor_properties.storageTexelBufferDescriptorSize;
        if (!counter_address || !cbuffer_address || !layout_sizes[0] ||
                !layout_sizes[1] ||
                binding_offsets[0] + 2u * mutable_stride > layout_sizes[0] ||
                binding_offsets[1] + descriptor_properties.uniformBufferDescriptorSize >
                        layout_sizes[1])
        {
            fprintf(stderr, "%s invalid descriptor-buffer addresses or layout sizes.\n",
                    experiment);
            goto cleanup;
        }
        if (!create_host_address_buffer(context, layout_sizes[0],
                VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT,
                &descriptor_buffers[0]) ||
                !create_host_address_buffer(context, layout_sizes[1],
                        VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT,
                        &descriptor_buffers[1]))
            goto cleanup;
        memset(descriptor_buffers[0].mapping, 0, (size_t)layout_sizes[0]);
        memset(descriptor_buffers[1].mapping, 0, (size_t)layout_sizes[1]);

        descriptor_info.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        descriptor_info.data.pStorageImage = &image_descriptor;
        get_descriptor(context->device, &descriptor_info,
                descriptor_properties.storageImageDescriptorSize,
                (uint8_t *)descriptor_buffers[0].mapping + binding_offsets[0]);
        descriptor_info.type = counter_as_storage_buffer ?
                VK_DESCRIPTOR_TYPE_STORAGE_BUFFER :
                VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
        if (counter_as_storage_buffer)
            descriptor_info.data.pStorageBuffer = &counter_address_info;
        else
            descriptor_info.data.pStorageTexelBuffer = &counter_address_info;
        get_descriptor(context->device, &descriptor_info,
                counter_as_storage_buffer ?
                        descriptor_properties.storageBufferDescriptorSize :
                        descriptor_properties.storageTexelBufferDescriptorSize,
                (uint8_t *)descriptor_buffers[0].mapping + binding_offsets[0] +
                        mutable_stride);
        descriptor_info.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptor_info.data.pUniformBuffer = &cbuffer_address_info;
        get_descriptor(context->device, &descriptor_info,
                descriptor_properties.uniformBufferDescriptorSize,
                (uint8_t *)descriptor_buffers[1].mapping + binding_offsets[1]);
        if (!flush_host_buffer(context, &descriptor_buffers[0]) ||
                !flush_host_buffer(context, &descriptor_buffers[1]))
            goto cleanup;
        printf("%s_DESCRIPTOR_LAYOUT set1_size=%" PRIu64 " set1_offset=%" PRIu64
               " mutable_stride=%" PRIu64 " set2_size=%" PRIu64
               " set2_offset=%" PRIu64 " alignment=%" PRIu64 "\n",
                experiment,
                (uint64_t)layout_sizes[0], (uint64_t)binding_offsets[0],
                (uint64_t)mutable_stride, (uint64_t)layout_sizes[1],
                (uint64_t)binding_offsets[1],
                (uint64_t)descriptor_properties.descriptorBufferOffsetAlignment);
    }

    VK_CHECK_GOTO(vkCreatePipelineLayout(context->device, &pipeline_layout_info, NULL,
            &pipeline_layout));
    spirv = load_spirv(spv_path, &spirv_size);
    if (!spirv)
        goto cleanup;
    shader_info.codeSize = spirv_size;
    shader_info.pCode = spirv;
    VK_CHECK_GOTO(vkCreateShaderModule(context->device, &shader_info, NULL, &shader_module));
    stage_info.module = shader_module;
    pipeline_info.stage = stage_info;
    pipeline_info.layout = pipeline_layout;
    if (descriptor_buffer_backend)
        pipeline_info.flags = VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;
    VK_CHECK_GOTO(vkCreateComputePipelines(context->device, VK_NULL_HANDLE, 1,
            &pipeline_info, NULL, &pipeline));

    VK_CHECK_GOTO(vkCreateCommandPool(context->device, &command_pool_info, NULL,
            &command_pool));
    command_allocate_info.commandPool = command_pool;
    VK_CHECK_GOTO(vkAllocateCommandBuffers(context->device, &command_allocate_info,
            &command_buffer));
    VK_CHECK_GOTO(vkBeginCommandBuffer(command_buffer, &begin_info));

    image_barrier.image = grid_image.image;
    image_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    image_barrier.srcAccessMask = 0;
    image_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &image_barrier);
    vkCmdCopyBufferToImage(command_buffer, staging.buffer, grid_image.image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);

    image_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    image_barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    image_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    image_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, NULL, 0, NULL, 1,
            &image_barrier);
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
    if (!descriptor_buffer_backend)
    {
        vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                pipeline_layout, 1, 2, descriptor_sets, 0, NULL);
    }
    else
    {
        VkDescriptorBufferBindingInfoEXT binding_infos[2] = {
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT,
                .address = get_buffer_address(context, descriptor_buffers[0].buffer),
                .usage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT,
            },
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT,
                .address = get_buffer_address(context, descriptor_buffers[1].buffer),
                .usage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT,
            },
        };
        uint32_t buffer_indices[2] = {0, 1};
        VkDeviceSize offsets[2] = {0, 0};

        cmd_bind_descriptor_buffers(command_buffer, 2, binding_infos);
        cmd_set_descriptor_offsets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                pipeline_layout, 1, 2, buffer_indices, offsets);
    }
    vkCmdPushConstants(command_buffer, pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT,
            0, sizeof(push_constants), push_constants);
    vkCmdDispatch(command_buffer, DISPATCH_X, DISPATCH_Y, 1);

    image_barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
    image_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    image_barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    image_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1,
            &image_barrier);
    buffer_barrier.buffer = staging.buffer;
    buffer_barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    buffer_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 1, &buffer_barrier, 0, NULL);
    vkCmdCopyImageToBuffer(command_buffer, grid_image.image,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, staging.buffer, 1, &copy_region);
    buffer_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    buffer_barrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_HOST_BIT, 0, 0, NULL, 1, &buffer_barrier, 0, NULL);
    buffer_barrier.buffer = counter.buffer;
    buffer_barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    buffer_barrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_PIPELINE_STAGE_HOST_BIT, 0, 0, NULL, 1, &buffer_barrier, 0, NULL);
    VK_CHECK_GOTO(vkEndCommandBuffer(command_buffer));

    VK_CHECK_GOTO(vkCreateFence(context->device, &fence_info, NULL, &fence));
    VK_CHECK_GOTO(vkQueueSubmit(context->queue, 1, &submit_info, fence));
    VK_CHECK_GOTO(vkWaitForFences(context->device, 1, &fence, VK_TRUE, UINT64_MAX));
    if (!invalidate_host_buffer(context, &counter) ||
            !invalidate_host_buffer(context, &staging))
        goto cleanup;
    success = analyze_exact_result(&counter, &staging, expected_total, errors_before,
            descriptor_buffer_backend, counter_format, counter_size,
            counter_as_storage_buffer);

cleanup:
    if (context->device)
        vkDeviceWaitIdle(context->device);
    if (fence)
        vkDestroyFence(context->device, fence, NULL);
    if (command_pool)
        vkDestroyCommandPool(context->device, command_pool, NULL);
    if (pipeline)
        vkDestroyPipeline(context->device, pipeline, NULL);
    if (shader_module)
        vkDestroyShaderModule(context->device, shader_module, NULL);
    if (pipeline_layout)
        vkDestroyPipelineLayout(context->device, pipeline_layout, NULL);
    if (descriptor_pool)
        vkDestroyDescriptorPool(context->device, descriptor_pool, NULL);
    for (uint32_t i = 0; i < 3; ++i)
    {
        if (set_layouts[i])
            vkDestroyDescriptorSetLayout(context->device, set_layouts[i], NULL);
    }
    if (counter_view)
        vkDestroyBufferView(context->device, counter_view, NULL);
    destroy_host_buffer(context, &descriptor_buffers[1]);
    destroy_host_buffer(context, &descriptor_buffers[0]);
    destroy_device_image(context, &grid_image);
    destroy_host_buffer(context, &cbuffer);
    destroy_host_buffer(context, &staging);
    destroy_host_buffer(context, &counter);
    free(spirv);
    return success;
}

static bool parse_device_index(const char *text, int32_t *index)
{
    char *end;
    long value;

    errno = 0;
    value = strtol(text, &end, 10);
    if (errno || !*text || *end || value < 0 || value > INT32_MAX)
        return false;
    *index = (int32_t)value;
    return true;
}

static void usage(const char *program)
{
    fprintf(stderr,
            "Usage: %s [--validation] [--format-ab] [--device INDEX] "
            "--texel FILE --ssbo FILE "
            "[--exact FILE] [--exact-r16 FILE] [--exact-r16-ssbo FILE]\n"
            "       %s [--validation] --list\n", program, program);
}

static bool parse_options(int argc, char **argv, struct options *options)
{
    memset(options, 0, sizeof(*options));
    options->device_index = -1;

    for (int i = 1; i < argc; ++i)
    {
        if (!strcmp(argv[i], "--validation"))
        {
            options->validation = true;
        }
        else if (!strcmp(argv[i], "--list"))
        {
            options->list_only = true;
        }
        else if (!strcmp(argv[i], "--format-ab"))
        {
            options->format_ab = true;
        }
        else if (!strcmp(argv[i], "--device") && i + 1 < argc)
        {
            if (!parse_device_index(argv[++i], &options->device_index))
            {
                fprintf(stderr, "Invalid Vulkan device index: %s\n", argv[i]);
                return false;
            }
        }
        else if (!strcmp(argv[i], "--texel") && i + 1 < argc)
        {
            options->texel_spv = argv[++i];
        }
        else if (!strcmp(argv[i], "--ssbo") && i + 1 < argc)
        {
            options->ssbo_spv = argv[++i];
        }
        else if (!strcmp(argv[i], "--exact") && i + 1 < argc)
        {
            options->exact_spv = argv[++i];
        }
        else if (!strcmp(argv[i], "--exact-r16") && i + 1 < argc)
        {
            options->exact_r16_spv = argv[++i];
        }
        else if (!strcmp(argv[i], "--exact-r16-ssbo") && i + 1 < argc)
        {
            options->exact_r16_ssbo_spv = argv[++i];
        }
        else
        {
            fprintf(stderr, "Unknown or incomplete option: %s\n", argv[i]);
            return false;
        }
    }

    return options->list_only || (options->texel_spv && options->ssbo_spv);
}

int main(int argc, char **argv)
{
    struct options options;
    VkInstance instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT messenger = VK_NULL_HANDLE;
    VkPhysicalDevice *devices = NULL;
    uint32_t device_count = 0;
    struct context context = {0};
    VkPhysicalDeviceProperties selected_properties;
    VkFormatProperties format_properties;
    bool texel_pass, ssbo_pass, exact_set_pass = true, exact_buffer_pass = true;
    bool exact_r16_set_ran = true, exact_r16_buffer_ran = true;
    bool exact_r16_ssbo_set_pass = true, exact_r16_ssbo_buffer_pass = true;
    bool format_ab_pass = true;
    bool exact_r32_live_set_pass = true, exact_r32_live_buffer_pass = true;
    uint32_t non_d23_validation_errors;
    int exit_code = 1;

    if (!parse_options(argc, argv, &options))
    {
        usage(argv[0]);
        return 1;
    }
    if (!create_instance(options.validation, &instance, &messenger))
        goto cleanup;
    if (!enumerate_devices(instance, &devices, &device_count))
        goto cleanup;

    for (uint32_t i = 0; i < device_count; ++i)
        print_device(i, devices[i]);
    if (options.list_only)
    {
        exit_code = 0;
        goto cleanup;
    }

    if (options.device_index < 0)
        options.device_index = choose_default_device(devices, device_count);
    if ((uint32_t)options.device_index >= device_count)
    {
        fprintf(stderr, "Vulkan device index %d is out of range 0..%u.\n",
                options.device_index, device_count - 1);
        goto cleanup;
    }

    vkGetPhysicalDeviceProperties(devices[options.device_index], &selected_properties);
    vkGetPhysicalDeviceFormatProperties(devices[options.device_index],
            VK_FORMAT_R32_UINT, &format_properties);
    printf("D21_SELECTED index=%d queue_test=compute format_r32_uint_atomic=%s "
           "validation=%s name=\"%s\"\n",
            options.device_index,
            (format_properties.bufferFeatures &
                    VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_ATOMIC_BIT) ? "yes" : "no",
            options.validation ? "enabled" : "disabled", selected_properties.deviceName);
    if (!(format_properties.bufferFeatures &
            VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_ATOMIC_BIT))
    {
        fprintf(stderr, "Selected device does not support R32_UINT storage texel buffer atomics.\n");
        goto cleanup;
    }

    if (!create_device(devices[options.device_index],
            options.exact_spv != NULL || options.exact_r16_spv != NULL ||
                    options.exact_r16_ssbo_spv != NULL, &context))
        goto cleanup;

    texel_pass = run_variant(&context, ATOMIC_TEXEL, options.texel_spv);
    ssbo_pass = run_variant(&context, ATOMIC_SSBO, options.ssbo_spv);
    if (options.exact_spv)
    {
        exact_set_pass = run_exact_variant(&context, options.exact_spv, false,
                VK_FORMAT_R32_UINT, sizeof(uint32_t), false);
        exact_buffer_pass = run_exact_variant(&context, options.exact_spv, true,
                VK_FORMAT_R32_UINT, sizeof(uint32_t), false);
    }
    if (options.format_ab && options.exact_spv)
    {
        exact_r32_live_set_pass = run_exact_variant(&context, options.exact_spv, false,
                VK_FORMAT_R32_UINT, LIVE_COUNTER_SIZE, false);
        exact_r32_live_buffer_pass = run_exact_variant(&context, options.exact_spv, true,
                VK_FORMAT_R32_UINT, LIVE_COUNTER_SIZE, false);
    }
    non_d23_validation_errors = validation_errors;
    if (options.exact_r16_spv)
    {
        exact_r16_set_ran = run_exact_variant(&context, options.exact_r16_spv, false,
                VK_FORMAT_R16_UINT, LIVE_COUNTER_SIZE, false);
        exact_r16_buffer_ran = run_exact_variant(&context, options.exact_r16_spv, true,
                VK_FORMAT_R16_UINT, LIVE_COUNTER_SIZE, false);
    }
    if (options.exact_r16_ssbo_spv)
    {
        exact_r16_ssbo_set_pass = run_exact_variant(&context,
                options.exact_r16_ssbo_spv, false, VK_FORMAT_R16_UINT,
                LIVE_COUNTER_SIZE, true);
        exact_r16_ssbo_buffer_pass = run_exact_variant(&context,
                options.exact_r16_ssbo_spv, true, VK_FORMAT_R16_UINT,
                LIVE_COUNTER_SIZE, true);
    }
    if (options.format_ab)
    {
        format_ab_pass = run_minimal_format_ab(&context, options.texel_spv);
        printf("D50_D51_SUMMARY minimal_format_ab=%s "
               "exact_r32_live_set=%s exact_r32_live_descriptor_buffer=%s\n",
                format_ab_pass ? "EXECUTED" : "FAIL",
                options.exact_spv ?
                        (exact_r32_live_set_pass ? "PASS" : "FAIL") : "NOT_RUN",
                options.exact_spv ?
                        (exact_r32_live_buffer_pass ? "PASS" : "FAIL") : "NOT_RUN");
    }
    printf("D21_D22_D23_D24_SUMMARY device_index=%d texel=%s ssbo=%s exact_set=%s "
           "exact_descriptor_buffer=%s "
           "r16_set=%s r16_descriptor_buffer=%s "
           "r16_ssbo_set=%s r16_ssbo_descriptor_buffer=%s "
           "validation_errors_before_d23=%u validation_errors_total=%u "
           "validation_warnings=%u name=\"%s\"\n",
            options.device_index, texel_pass ? "PASS" : "FAIL",
            ssbo_pass ? "PASS" : "FAIL",
            options.exact_spv ? (exact_set_pass ? "PASS" : "FAIL") : "NOT_RUN",
            options.exact_spv ? (exact_buffer_pass ? "PASS" : "FAIL") : "NOT_RUN",
            options.exact_r16_spv ? (exact_r16_set_ran ? "EXECUTED" : "FAIL") : "NOT_RUN",
            options.exact_r16_spv ? (exact_r16_buffer_ran ? "EXECUTED" : "FAIL") : "NOT_RUN",
            options.exact_r16_ssbo_spv ?
                    (exact_r16_ssbo_set_pass ? "PASS" : "FAIL") : "NOT_RUN",
            options.exact_r16_ssbo_spv ?
                    (exact_r16_ssbo_buffer_pass ? "PASS" : "FAIL") : "NOT_RUN",
            non_d23_validation_errors, validation_errors, validation_warnings,
            selected_properties.deviceName);
    exit_code = texel_pass && ssbo_pass && exact_set_pass && exact_buffer_pass &&
            exact_r16_set_ran && exact_r16_buffer_ran &&
            exact_r16_ssbo_set_pass && exact_r16_ssbo_buffer_pass &&
            format_ab_pass && exact_r32_live_set_pass &&
            exact_r32_live_buffer_pass &&
            !non_d23_validation_errors ? 0 : 2;

cleanup:
    if (context.device)
        vkDestroyDevice(context.device, NULL);
    free(devices);
    destroy_instance(instance, messenger);
    return exit_code;
}
