/**
 * @file vulkan_compat.h
 * @brief Vulkan compatibility definitions for newer extensions
 * 
 * This header provides forward declarations and placeholder definitions
 * for Vulkan extensions that may not be available in the system headers.
 */

#ifndef VULKAN_COMPAT_H
#define VULKAN_COMPAT_H

#include <vulkan/vulkan.h>

// Define missing Vulkan extension structures if they don't exist
#ifndef VK_EXT_INDIRECT_EXECUTION_SET_SHADER
#define VK_EXT_INDIRECT_EXECUTION_SET_SHADER 1
#define VK_STRUCTURE_TYPE_WRITE_INDIRECT_EXECUTION_SET_SHADER_EXT ((VkStructureType)1000689000)
#define VK_STRUCTURE_TYPE_WRITE_INDIRECT_EXECUTION_SET_PIPELINE_EXT ((VkStructureType)1000689001)

typedef struct VkWriteIndirectExecutionSetShaderEXT {
    VkStructureType sType;
    const void* pNext;
} VkWriteIndirectExecutionSetShaderEXT;

typedef struct VkWriteIndirectExecutionSetPipelineEXT {
    VkStructureType sType;
    const void* pNext;
} VkWriteIndirectExecutionSetPipelineEXT;

typedef VkFlags VkIndirectExecutionSetEXT;
#endif

// Additional stub definitions for missing types used in template specializations
#ifndef VK_ARM_TENSOR_TYPES
#define VK_ARM_TENSOR_TYPES 1

typedef struct VkWriteDescriptorSetTensorARM { VkStructureType sType; } VkWriteDescriptorSetTensorARM;
typedef struct VkBindTensorMemoryInfoARM { VkStructureType sType; } VkBindTensorMemoryInfoARM;
typedef struct VkCopyTensorInfoARM { VkStructureType sType; } VkCopyTensorInfoARM;
typedef struct VkDescriptorGetTensorInfoARM { VkStructureType sType; } VkDescriptorGetTensorInfoARM;
typedef struct VkDeviceTensorMemoryRequirementsARM { VkStructureType sType; } VkDeviceTensorMemoryRequirementsARM;
typedef struct VkExternalMemoryTensorCreateInfoARM { VkStructureType sType; } VkExternalMemoryTensorCreateInfoARM;
typedef struct VkExternalTensorPropertiesARM { VkStructureType sType; } VkExternalTensorPropertiesARM;
typedef struct VkFrameBoundaryTensorsARM { VkStructureType sType; } VkFrameBoundaryTensorsARM;
typedef struct VkMemoryDedicatedAllocateInfoTensorARM { VkStructureType sType; } VkMemoryDedicatedAllocateInfoTensorARM;

#define VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_TENSOR_ARM ((VkStructureType)1000687000)
#define VK_STRUCTURE_TYPE_BIND_TENSOR_MEMORY_INFO_ARM ((VkStructureType)1000687001)
#define VK_STRUCTURE_TYPE_COPY_TENSOR_INFO_ARM ((VkStructureType)1000687002)
#define VK_STRUCTURE_TYPE_DESCRIPTOR_GET_TENSOR_INFO_ARM ((VkStructureType)1000687003)
#define VK_STRUCTURE_TYPE_DEVICE_TENSOR_MEMORY_REQUIREMENTS_ARM ((VkStructureType)1000687004)
#define VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_TENSOR_CREATE_INFO_ARM ((VkStructureType)1000687005)
#define VK_STRUCTURE_TYPE_EXTERNAL_TENSOR_PROPERTIES_ARM ((VkStructureType)1000687006)
#define VK_STRUCTURE_TYPE_FRAME_BOUNDARY_TENSORS_ARM ((VkStructureType)1000687007)
#define VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO_TENSOR_ARM ((VkStructureType)1000687008)
#endif

#ifndef VK_NV_PARTITIONED_ACCELERATION_STRUCTURE
#define VK_NV_PARTITIONED_ACCELERATION_STRUCTURE 1
typedef struct VkWriteDescriptorSetPartitionedAccelerationStructureNV { VkStructureType sType; } VkWriteDescriptorSetPartitionedAccelerationStructureNV;
typedef struct VkBuildPartitionedAccelerationStructureInfoNV { VkStructureType sType; } VkBuildPartitionedAccelerationStructureInfoNV;
#define VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_PARTITIONED_ACCELERATION_STRUCTURE_NV ((VkStructureType)1000680000)
#define VK_STRUCTURE_TYPE_BUILD_PARTITIONED_ACCELERATION_STRUCTURE_INFO_NV ((VkStructureType)1000680001)
#endif

#ifndef VK_KHR_VIDEO_FORMATS
#define VK_KHR_VIDEO_FORMATS 1
typedef struct VkVideoFormatQuantizationMapPropertiesKHR { VkStructureType sType; } VkVideoFormatQuantizationMapPropertiesKHR;
typedef struct VkVideoReferenceIntraRefreshInfoKHR { VkStructureType sType; } VkVideoReferenceIntraRefreshInfoKHR;
typedef struct VkBindVideoSessionMemoryInfoKHR { VkStructureType sType; } VkBindVideoSessionMemoryInfoKHR;
#define VK_STRUCTURE_TYPE_VIDEO_FORMAT_QUANTIZATION_MAP_PROPERTIES_KHR ((VkStructureType)1000308000)
#define VK_STRUCTURE_TYPE_VIDEO_REFERENCE_INTRA_REFRESH_INFO_KHR ((VkStructureType)1000308001)
#define VK_STRUCTURE_TYPE_BIND_VIDEO_SESSION_MEMORY_INFO_KHR ((VkStructureType)1000023000)
#endif

#ifndef VK_NV_COMPUTE_PIPELINE_INDIRECT_BUFFER
#define VK_NV_COMPUTE_PIPELINE_INDIRECT_BUFFER 1
typedef struct VkComputePipelineIndirectBufferInfoNV { VkStructureType sType; } VkComputePipelineIndirectBufferInfoNV;
#define VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_INDIRECT_BUFFER_INFO_NV ((VkStructureType)1000687000)
#endif

#endif // VULKAN_COMPAT_H

// Additional Video format types
typedef struct VkVideoFormatH265QuantizationMapPropertiesKHR { VkStructureType sType; } VkVideoFormatH265QuantizationMapPropertiesKHR;
typedef struct VkVideoFormatQuantizationMapPropertiesKHR { VkStructureType sType; } VkVideoFormatQuantizationMapPropertiesKHR;
#define VK_STRUCTURE_TYPE_VIDEO_FORMAT_H265_QUANTIZATION_MAP_PROPERTIES_KHR ((VkStructureType)1000308010)


// Additional ExternalComputeQueue types  
#ifndef VK_NV_EXTERNAL_COMPUTE_QUEUE
#define VK_NV_EXTERNAL_COMPUTE_QUEUE 1
typedef VkFlags VkExternalComputeQueueNV;
#endif

