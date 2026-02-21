#include "Graphics/Vulkan/VulkanPipeline.h"

#include "Graphics/Vertex.h"
#include "Graphics/Vulkan/VulkanDevice.h"
#include "Utilities/Logger.h"

namespace PE::Graphics::Vulkan {
VulkanPipeline::VulkanPipeline(VulkanPipeline &&other) noexcept
	: m_state(other.m_state), ref_device(other.ref_device), m_vkPipeline(other.m_vkPipeline), m_layout(other.m_layout) {
	other.m_vkPipeline = VK_NULL_HANDLE;
	other.m_layout	   = VK_NULL_HANDLE;
	other.ref_device   = nullptr;
	other.m_state	   = SystemState::Uninitialized;
}

VulkanPipeline &VulkanPipeline::operator=(VulkanPipeline &&other) noexcept {
	if (this != &other) {
		Shutdown();

		m_state		 = other.m_state;
		ref_device	 = other.ref_device;
		m_vkPipeline = other.m_vkPipeline;
		m_layout	 = other.m_layout;

		other.m_vkPipeline = VK_NULL_HANDLE;
		other.m_layout	   = VK_NULL_HANDLE;
		other.ref_device   = nullptr;
		other.m_state	   = SystemState::Uninitialized;
	}
	return *this;
}

ERROR_CODE VulkanPipeline::Initialize(VulkanDevice *device, const VulkanShader &shader, const VkPipelineLayout layout,
									  const VkExtent2D extent, const PipelineDescription &desc) {
	auto bindingDesc = Vertex::GetBindingDescription();
	auto attribDesc	 = Vertex::GetAttributeDescriptions();

	const std::vector<VkVertexInputBindingDescription>	 bindings = {bindingDesc};
	const std::vector<VkVertexInputAttributeDescription> attributes(attribDesc.begin(), attribDesc.end());

	return Initialize(device, shader, layout, extent, desc, bindings, attributes);
}

ERROR_CODE VulkanPipeline::Initialize(VulkanDevice *device, const VulkanShader &shader, VkPipelineLayout layout,
									  VkExtent2D extent, const PipelineDescription &desc,
									  const std::vector<VkVertexInputBindingDescription>   &bindings,
									  const std::vector<VkVertexInputAttributeDescription> &attributes) {
	PE_CHECK_STATE_INIT(m_state, "This vulkan pipeline is already initialized.");
	m_state = SystemState::Initializing;

	ref_device = device;
	m_layout   = layout;

	VkPipelineShaderStageCreateInfo vertStage{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
	vertStage.stage	 = VK_SHADER_STAGE_VERTEX_BIT;
	vertStage.module = shader.GetVertModule();
	vertStage.pName	 = "main";

	VkPipelineShaderStageCreateInfo fragStage{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
	fragStage.stage	 = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragStage.module = shader.GetFragModule();
	fragStage.pName	 = "main";

	VkPipelineShaderStageCreateInfo stages[] = {vertStage, fragStage};

	VkPipelineVertexInputStateCreateInfo vertexInput{};
	vertexInput.sType							= VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInput.vertexBindingDescriptionCount	= static_cast<uint32_t>(bindings.size());
	vertexInput.pVertexBindingDescriptions		= bindings.data();
	vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size());
	vertexInput.pVertexAttributeDescriptions	= attributes.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType					 = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology				 = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport{};
	viewport.x		  = 0.0f;
	viewport.y		  = 0.0f;
	viewport.width	  = static_cast<float>(extent.width);
	viewport.height	  = static_cast<float>(extent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset = {0, 0};
	scissor.extent = extent;

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType			= VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports	= &viewport;
	viewportState.scissorCount	= 1;
	viewportState.pScissors		= &scissor;

	std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

	if (desc.enableDepthBias) {
		dynamicStates.push_back(VK_DYNAMIC_STATE_DEPTH_BIAS);
	}

	VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
	dynamicStateInfo.sType			   = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicStateInfo.pDynamicStates	   = dynamicStates.data();

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType				   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable		   = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode			   = desc.wireframe ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth			   = 1.0f;
	rasterizer.cullMode				   = desc.cullMode;
	rasterizer.frontFace			   = VK_FRONT_FACE_CLOCKWISE;
	rasterizer.depthBiasEnable		   = desc.enableDepthBias ? VK_TRUE : VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType					= VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable	= VK_FALSE;
	multisampling.rasterizationSamples	= VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading		= 1.0f;
	multisampling.pSampleMask			= nullptr;
	multisampling.alphaToCoverageEnable = VK_FALSE;
	multisampling.alphaToOneEnable		= VK_FALSE;

	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType				   = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable	   = desc.enableDepthTest;
	depthStencil.depthWriteEnable	   = desc.enableDepthWrite;
	depthStencil.depthCompareOp		   = desc.compareOp;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.stencilTestEnable	   = VK_FALSE;

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	if (desc.enableBlend) {
		colorBlendAttachment.blendEnable		 = VK_TRUE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.colorBlendOp		 = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.alphaBlendOp		 = VK_BLEND_OP_ADD;
	} else {
		colorBlendAttachment.blendEnable = VK_FALSE;
	}

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType			  = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments	  = &colorBlendAttachment;

	VkPipelineRenderingCreateInfo pipelineRenderingInfo{};
	pipelineRenderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	if (desc.colorFormat != VK_FORMAT_UNDEFINED) {
		pipelineRenderingInfo.colorAttachmentCount	  = 1;
		pipelineRenderingInfo.pColorAttachmentFormats = &desc.colorFormat;
	} else {
		pipelineRenderingInfo.colorAttachmentCount	  = 0;
		pipelineRenderingInfo.pColorAttachmentFormats = nullptr;
	}
	pipelineRenderingInfo.depthAttachmentFormat = desc.depthFormat;

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType				 = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.pNext				 = &pipelineRenderingInfo;
	pipelineInfo.stageCount			 = 2;
	pipelineInfo.pStages			 = stages;
	pipelineInfo.pVertexInputState	 = &vertexInput;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState		 = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState	 = &multisampling;
	pipelineInfo.pDepthStencilState	 = &depthStencil;
	pipelineInfo.pColorBlendState	 = &colorBlending;
	pipelineInfo.pDynamicState		 = &dynamicStateInfo;
	pipelineInfo.renderPass			 = VK_NULL_HANDLE;
	pipelineInfo.subpass			 = 0;
	pipelineInfo.layout				 = m_layout;

	if (vkCreateGraphicsPipelines(ref_device->GetVkDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr,
								  &m_vkPipeline) != VK_SUCCESS) {
		PE_LOG_FATAL("Failed to create custom graphics pipeline!");
		return ERROR_CODE::VULKAN_PIPELINE_CREATION_FAILED;
	}

	m_state = SystemState::Running;
	return ERROR_CODE::OK;
}

void VulkanPipeline::Shutdown() {
	if (m_state == SystemState::Uninitialized || m_state == SystemState::ShuttingDown) return;
	m_state = SystemState::ShuttingDown;

	if (m_vkPipeline != VK_NULL_HANDLE) {
		vkDestroyPipeline(ref_device->GetVkDevice(), m_vkPipeline, nullptr);
		m_vkPipeline = VK_NULL_HANDLE;
	}
	m_layout = VK_NULL_HANDLE;
	m_state	 = SystemState::Uninitialized;
}

void VulkanPipeline::Bind(VkCommandBuffer cmd) {
	if (m_vkPipeline != VK_NULL_HANDLE) vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vkPipeline);
}
}  // namespace PE::Graphics::Vulkan