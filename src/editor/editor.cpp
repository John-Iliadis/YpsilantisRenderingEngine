//
// Created by Gianni on 4/02/2025.
//

#include "editor.hpp"

Editor::Editor(std::shared_ptr<Renderer> renderer, std::shared_ptr<ResourceManager> resourceManager)
    : mRenderer(std::move(renderer))
    , mResourceManager(std::move(resourceManager))
    , mCamera({}, 30.f, 1920.f, 1080.f)
    , mSelectedObjectID()
    , mShowViewport(true)
    , mShowAssetPanel(true)
    , mShowSceneGraph(true)
    , mShowCameraPanel(false)
    , mShowInspectorPanel(true)
    , mShowRendererPanel(false)
    , mShowConsole(true)
    , mShowDebugPanel(true)
{
}

Editor::~Editor() = default;

void Editor::update(float dt)
{
    mDt = dt;

    ImGui::ShowDemoWindow();

    mainMenuBar();

    if (mShowAssetPanel)
        assetPanel();

    if (mShowSceneGraph)
        sceneGraphPanel();

    if (mShowCameraPanel)
        cameraPanel();

    if (mShowInspectorPanel)
        inspectorPanel();

    if (mShowRendererPanel)
        rendererPanel();

    if (mShowDebugPanel)
        debugPanel();

    if (mShowConsole)
        console();

    if (mShowViewport)
        viewPort();
}

void Editor::mainMenuBar()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Import Model"))
            {
                std::filesystem::path path = fileDialog("All Files");

                if (!path.empty())
                    mResourceManager->importModel(path);
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Settings"))
        {
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View"))
        {
            ImGui::MenuItem("Viewport", nullptr, &mShowViewport);
            ImGui::MenuItem("Asset Panel", nullptr, &mShowAssetPanel);
            ImGui::MenuItem("Scene Graph", nullptr, &mShowSceneGraph);
            ImGui::MenuItem("Camera Panel", nullptr, &mShowCameraPanel);
            ImGui::MenuItem("Inspector", nullptr, &mShowInspectorPanel);
            ImGui::MenuItem("Renderer", nullptr, &mShowRendererPanel);
            ImGui::MenuItem("Console", nullptr, &mShowConsole);
            ImGui::MenuItem("Debug", nullptr, &mShowDebugPanel);

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

void Editor::assetPanel()
{
    ImGui::Begin("Assets", &mShowAssetPanel);

    static int selectedIndex = 0;
    static const std::array<const char*, 3> items {{
                                                       "Models",
                                                       "Materials",
                                                       "Textures"
                                                   }};

    {
        ImGui::BeginGroup();

        for (int i = 0; i < items.size(); ++i)
        {
            const ImGuiStyle& style = ImGui::GetStyle();
            ImVec2 selectableSize = ImGui::CalcTextSize(items.at(i)) + style.FramePadding * 2.f;

            ImGui::PushStyleColor(ImGuiCol_Button, style.Colors[selectedIndex == i? ImGuiCol_ButtonHovered : ImGuiCol_Button]);

            if (ImGui::Button(items.at(i), selectableSize))
                selectedIndex = i;

            ImGui::PopStyleColor();

            if (i < items.size() - 1)
                ImGui::SameLine();
        }

        ImGui::EndGroup();
        ImGui::Separator();
    }

    if (selectedIndex == 0)
        displayModels();

    if (selectedIndex == 1)
        displayMaterials();

    if (selectedIndex == 2)
        displayTextures();

    ImGui::End();
}

void Editor::displayModels()
{
    for (const auto& [modelID, modelName] : mResourceManager->mModelNames)
    {
        if (ImGui::Selectable(modelName.c_str(), mSelectedObjectID == modelID))
        {
            mSelectedObjectID = modelID;
        }

        modelDragDropSource(modelID);
    }
}

void Editor::displayMaterials()
{
    for (const auto& [materialID, materialName] : mResourceManager->mMaterialNames)
    {
        if (ImGui::Selectable(materialName.c_str(), mSelectedObjectID == materialID))
        {
            mSelectedObjectID = materialID;
        }
    }
}

void Editor::displayTextures()
{
    for (const auto& [textureID, textureName] : mResourceManager->mTextureNames)
    {
        if (ImGui::Selectable(textureName.c_str(), mSelectedObjectID == textureID))
        {
            mSelectedObjectID = textureID;
        }

        textureDragDropSource(textureID);
    }
}

void Editor::sceneGraphPanel()
{
    ImGui::Begin("Scene Graph", &mShowSceneGraph);

    for (auto child : mSceneGraph.mRoot.children())
        sceneNodeRecursive(child);

    // todo: improve drag drop area
    ImGui::Dummy(ImGui::GetContentRegionAvail());
    sceneNodeDragDropTarget(&mSceneGraph.mRoot);

    sceneGraphPopup();

    ImGui::End();

    mSceneGraph.updateTransforms();
}

void Editor::sceneNodeRecursive(SceneNode *node)
{
    ImGuiTreeNodeFlags treeNodeFlags {
        ImGuiTreeNodeFlags_OpenOnArrow |
        ImGuiTreeNodeFlags_OpenOnDoubleClick |
        ImGuiTreeNodeFlags_SpanFullWidth |
        ImGuiTreeNodeFlags_FramePadding
    };

    if (mSelectedObjectID == node->id())
        treeNodeFlags |= ImGuiTreeNodeFlags_Selected;

    if (node->children().empty())
        treeNodeFlags |= ImGuiTreeNodeFlags_Leaf;

    bool nodeOpen = ImGui::TreeNodeEx((void*)(intptr_t)node, treeNodeFlags, node->name().c_str());

    if (ImGui::IsItemClicked(ImGuiMouseButton_Left) || ImGui::IsItemClicked(ImGuiMouseButton_Right))
        mSelectedObjectID = node->id();

    sceneNodeDragDropSource(node);
    sceneNodeDragDropTarget(node);

    if (nodeOpen)
    {
        for (auto child : node->children())
            sceneNodeRecursive(child);

        ImGui::TreePop();
    }
}

void Editor::sceneNodeDragDropSource(SceneNode *node)
{
    if (ImGui::BeginDragDropSource())
    {
        ImGui::SetDragDropPayload("SceneNode", &node, sizeof(SceneNode*));
        ImGui::Text(std::format("{} (Scene Node)", node->name()).c_str());
        ImGui::EndDragDropSource();
    }
}

void Editor::sceneNodeDragDropTarget(SceneNode *node)
{
    if (ImGui::BeginDragDropTarget())
    {
        checkPayloadType("SceneNode");

        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SceneNode"))
        {
            SceneNode* transferNode = *(SceneNode**)payload->Data;

            transferNode->orphan();
            transferNode->markDirty();

            node->addChild(transferNode);
        }

        ImGui::EndDragDropTarget();
    }
}

void Editor::cameraPanel()
{
    ImGui::Begin("Camera", &mShowCameraPanel);

    ImGui::SliderFloat("Field of View", mCamera.fov(), 1.f, 177.f, "%0.f");
    ImGui::DragFloat("Near Plane", mCamera.nearPlane(), 0.1f, 0.1f, *mCamera.farPlane(), "%.1f");
    ImGui::DragFloat("Far Plane", mCamera.farPlane(), 0.1f, *mCamera.nearPlane(), FLT_MAX, "%.1f");
    ImGui::DragFloat("Fly Speed", mCamera.flySpeed(), 1.f, 0.f, FLT_MAX, "%.0f");
    ImGui::DragFloat("Pan Speed", mCamera.panSpeed(), 1.f, 0.f, FLT_MAX, "%.0f");
    ImGui::DragFloat("Z Scroll Offset", mCamera.zScrollOffset(), 1.f, 0.f, FLT_MAX, "%.0f");
    ImGui::DragFloat("Rotate Sensitivity", mCamera.rotateSensitivity(), 1.f, 0.f, FLT_MAX, "%.0f");
    ImGui::SeparatorText("Camera Mode");

    static int selectedIndex = 0;
    static const std::array<const char*, 3> items {{
                                                       "First Person",
                                                       "View",
                                                       "Edit"
                                                   }};

    {
        ImGui::BeginGroup();

        for (int i = 0; i < items.size(); ++i)
        {
            const ImGuiStyle& style = ImGui::GetStyle();
            ImVec2 selectableSize = ImGui::CalcTextSize(items.at(i)) + style.FramePadding * 2.f;

            ImGui::PushStyleColor(ImGuiCol_Button, style.Colors[selectedIndex == i? ImGuiCol_ButtonHovered : ImGuiCol_Button]);

            if (ImGui::Button(items.at(i), selectableSize))
                selectedIndex = i;

            ImGui::PopStyleColor();

            if (i < items.size() - 1)
                ImGui::SameLine();
        }

        ImGui::EndGroup();
    }

    if (selectedIndex == 0)
        mCamera.setState(Camera::FIRST_PERSON);
    if (selectedIndex == 1)
        mCamera.setState(Camera::VIEW_MODE);
    if (selectedIndex == 2)
        mCamera.setState(Camera::EDITOR_MODE);

    ImGui::End();
}

// todo: fix mesh instance::addInstance
// todo: what if node doesn't have a material
SceneNode *Editor::createModelGraph(std::shared_ptr<Model> model, const Model::Node &modelNode, SceneNode* parent)
{
    SceneNode* sceneNode;

    if (auto meshID = modelNode.meshID)
    {
        uint32_t instanceID = mResourceManager->mMeshes.at(*meshID)->addInstance({}, {}, {});
        index_t materialIndex = 0;
        std::string matName = "";

        if (auto materialID = model->getMaterialID(modelNode))
            materialIndex = mResourceManager->getMatIndex(*materialID);

        if (auto& matNameOpt = modelNode.materialName)
            matName = *matNameOpt;

        sceneNode = new MeshNode(NodeType::Mesh,
                                 modelNode.name,
                                 modelNode.transformation,
                                 parent,
                                 *meshID,
                                 instanceID,
                                 materialIndex,
                                 matName);
    }
    else
    {
        sceneNode = new SceneNode(NodeType::Empty, modelNode.name, modelNode.transformation, parent);
    }

    for (const auto& child : modelNode.children)
        sceneNode->addChild(createModelGraph(model, child, sceneNode));

    return sceneNode;
}

void Editor::checkPayloadType(const char *type)
{
    const ImGuiPayload* payload = ImGui::GetDragDropPayload();

    if (payload && strcmp(payload->DataType, type) != 0)
        ImGui::SetMouseCursor(ImGuiMouseCursor_NotAllowed);
}

void Editor::inspectorPanel()
{
    ImGui::Begin("Inspector", &mShowInspectorPanel, ImGuiWindowFlags_NoScrollbar);

    if (auto objectType = UUIDRegistry::getObjectType(mSelectedObjectID))
    {
        switch (*objectType)
        {
            case ObjectType::Model: modelInspector(mSelectedObjectID); break;
            case ObjectType::Material: materialInspector(mSelectedObjectID); break;
            case ObjectType::Texture2D: textureInspector(mSelectedObjectID); break;
            case ObjectType::TextureCube: assert(false);
            case ObjectType::Mesh: assert(false);
            case ObjectType::SceneNode: sceneNodeInspector(mSceneGraph.searchNode(mSelectedObjectID)); break;
            default: assert(false);
        }
    }

    ImGui::End();
}

void Editor::rendererPanel()
{
    ImGui::Begin("Renderer", &mShowRendererPanel);
    ImGui::End();
}

void Editor::console()
{
    ImGui::Begin("Console", &mShowConsole);
    ImGui::End();
}

void Editor::debugPanel()
{
    ImGui::Begin("Debug", &mShowDebugPanel);
    ImGui::End();
}

void Editor::modelInspector(uuid64_t modelID)
{
    auto model = mResourceManager->getModel(modelID);

    ImGui::Text("Asset Type: Model");
    ImGui::Text("Name: %s", mResourceManager->mModelNames.at(modelID).c_str());
    ImGui::Separator();

    // todo: put this into a function
    if (ImGui::CollapsingHeader("Mapped Materials", ImGuiTreeNodeFlags_DefaultOpen))
    {
        for (const auto& [mappedMaterialName, mappedMaterialID] : model->mappedMaterials)
        {
            const char* selectedMat = mResourceManager->mMaterialNames.at(mappedMaterialID).c_str();

            if (ImGui::BeginCombo(mappedMaterialName.c_str(), selectedMat))
            {
                for (const auto& [materialID, materialName] : mResourceManager->mMaterialNames)
                {
                    bool selected = mappedMaterialID == materialID;
                    if (ImGui::Selectable(materialName.c_str(), selected))
                    {
                        index_t materialIndex = mResourceManager->getMatIndex(materialID);
                        model->remapMaterial(mappedMaterialName, materialID, materialIndex);
                    }
                }

                ImGui::EndCombo();
            }
        }
    }

    ImGui::Text("Todo: Add reset button");
}

void Editor::materialInspector(uuid64_t materialID)
{
    static constexpr ImGuiColorEditFlags sColorEditFlags {
        ImGuiColorEditFlags_DisplayRGB |
        ImGuiColorEditFlags_AlphaBar
    };

    index_t matIndex = mResourceManager->getMatIndex(materialID);
    Material& material = mResourceManager->mMaterialArray.at(matIndex);

    ImGui::Text("Asset Type: Material");
    ImGui::Text("Name: %s", mResourceManager->mMaterialNames.at(materialID).c_str());
    ImGui::Separator();

    bool matNeedsUpdate = false;

    if (ImGui::CollapsingHeader("Material Textures", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (materialTextureInspector(material.baseColorTexIndex, "Base Color"))
            matNeedsUpdate = true;
        if (materialTextureInspector(material.metallicRoughnessTexIndex, "Metallic Roughness"))
            matNeedsUpdate = true;
        if (materialTextureInspector(material.normalTexIndex, "Normal"))
            matNeedsUpdate = true;
        if (materialTextureInspector(material.aoTexIndex, "Ambient Occlusion"))
            matNeedsUpdate = true;
        if (materialTextureInspector(material.emissionTexIndex, "Emission"))
            matNeedsUpdate = true;
    }

    if (ImGui::CollapsingHeader("Factors", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (ImGui::ColorEdit4("Base Color Factor", &material.baseColorFactor[0], sColorEditFlags))
            matNeedsUpdate = true;

        if (ImGui::ColorEdit3("Emission Factor", &material.emissionFactor[0], sColorEditFlags))
            matNeedsUpdate = true;

        if (ImGui::SliderFloat("Metallic Factor", &material.metallicFactor, 0.f, 1.f))
            matNeedsUpdate = true;

        if (ImGui::SliderFloat("Roughness Factor", &material.roughnessFactor, 0.f, 1.f))
            matNeedsUpdate = true;

        if (ImGui::SliderFloat("Occlusion Factor", &material.occlusionFactor, 0.f, 1.f))
            matNeedsUpdate = true;
    }

    if (ImGui::CollapsingHeader("Texture Coordinates", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (ImGui::DragFloat2("Tiling", &material.tiling[0], 0.01f))
            matNeedsUpdate = true;

        if (ImGui::DragFloat2("Offset", &material.offset[0], 0.01f))
            matNeedsUpdate = true;
    }

    if (matNeedsUpdate)
        mResourceManager->updateMaterial(matIndex);
}

bool Editor::materialTextureInspector(index_t &textureIndex, std::string label)
{
    static constexpr ImVec2 sImageSize = ImVec2(20.f, 20.f);
    static constexpr ImVec2 sTooltipImageSize = ImVec2(250.f, 250.f);
    static constexpr ImVec2 sImageButtonFramePadding = ImVec2(2.f, 2.f);
    static const ImVec2 sTextSize = ImGui::CalcTextSize("H");

    uuid64_t textureID = mResourceManager->getTexIDFromIndex(textureIndex);
    std::shared_ptr<VulkanTexture> texture = mResourceManager->getTexture(textureID);
    VkDescriptorSet textureDescriptorSet = mResourceManager->getTextureDescriptorSet(textureID);

    bool matNeedsUpdate = false;

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, sImageButtonFramePadding);
    if (ImGui::ImageButton((ImTextureID)textureDescriptorSet, sImageSize))
        ImGui::OpenPopup(label.c_str());
    ImGui::PopStyleVar();

    if (ImGui::BeginPopup(label.c_str()))
    {
        ImGui::SeparatorText("Select Texture:");

        if (auto selectedTexID = textureCombo(textureID))
        {
            textureIndex = mResourceManager->getTextureIndex(*selectedTexID);
            matNeedsUpdate = true;
        }

        ImGui::EndPopup();
    }

    if (textureDragDropTarget(textureIndex))
        matNeedsUpdate = true;

    if (ImGui::BeginItemTooltip())
    {
        ImGui::Image((ImTextureID)textureDescriptorSet, sTooltipImageSize);
        ImGui::EndTooltip();
    }

    ImGui::SameLine();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + sImageSize.y / 2.f - sTextSize.y / 2.f + 1.f);
    ImGui::Text("%s | (%s)", label.c_str(), mResourceManager->mTextureNames.at(textureID).c_str());

    return matNeedsUpdate;
}

void Editor::modelDragDropSource(uuid64_t modelID)
{
    if (ImGui::BeginDragDropSource())
    {
        ImGui::SetDragDropPayload("Model", &modelID, sizeof(uuid64_t));
        ImGui::Text("%s (Model)", mResourceManager->mModelNames.at(modelID).c_str());
        ImGui::EndDragDropSource();
    }
}

void Editor::modelDragDropTarget()
{
    if (ImGui::BeginDragDropTarget())
    {
        checkPayloadType("Model");

        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Model"))
        {
            uuid64_t modelID = *(uuid64_t*)payload->Data;
            std::shared_ptr<Model> model = mResourceManager->getModel(modelID);
            mSceneGraph.mRoot.addChild(createModelGraph(model, model->root, &mSceneGraph.mRoot));
        }

        ImGui::EndDragDropTarget();
    }
}

void Editor::textureDragDropSource(uuid64_t textureID)
{
    if (ImGui::BeginDragDropSource())
    {
        ImGui::SetDragDropPayload("Texture", &textureID, sizeof(uuid64_t));
        ImGui::Text("%s (Texture)", mResourceManager->mTextureNames.at(textureID).c_str());
        ImGui::EndDragDropSource();
    }
}

bool Editor::textureDragDropTarget(index_t& textureIndex)
{
    if (ImGui::BeginDragDropTarget())
    {
        checkPayloadType("Texture");

        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Texture"))
        {
            uuid64_t textureID = *(uuid64_t*)payload->Data;
            textureIndex = mResourceManager->getTextureIndex(textureID);
            return true;
        }

        ImGui::EndDragDropTarget();
    }

    return false;
}

std::optional<uuid64_t> Editor::textureCombo(uuid64_t selectedTextureID)
{
    for (const auto& [textureID, textureName] : mResourceManager->mTextureNames)
    {
        bool selected = selectedTextureID == textureID;

        if (ImGui::Selectable(textureName.c_str(), selected))
            return textureID;
    }

    return std::nullopt;
}

void Editor::textureInspector(uuid64_t textureID)
{
    auto texture = mResourceManager->getTexture(textureID);
    VkDescriptorSet textureDescriptorSet = mResourceManager->getTextureDescriptorSet(textureID);

    const ImVec2 textureSize(texture->vulkanImage.width, texture->vulkanImage.height);

    ImGui::Text("Asset Type: Texture");
    ImGui::Text("Name: %s", mResourceManager->mTextureNames.at(textureID).c_str());

    ImGui::SeparatorText("Texture Info");

    ImGui::Text("Format: %s", toStr(texture->vulkanImage.format));
    ImGui::Text("Filter Mode: %s", toStr(texture->vulkanSampler.filterMode));
    ImGui::Text("Wrap Mode: %s", toStr(texture->vulkanSampler.wrapMode));
    ImGui::Text("Size: %dx%d", (int)textureSize.x, (int)textureSize.y);

    ImGui::SeparatorText("Preview");

    float windowWidth = ImGui::GetContentRegionAvail().x;
    float newHeight = (windowWidth / textureSize.x) * textureSize.y;
    float xPadding = (ImGui::GetContentRegionAvail().x - windowWidth) * 0.5f;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + xPadding);
    ImGui::Image((ImTextureID)textureDescriptorSet, ImVec2(windowWidth, newHeight));
}

void Editor::viewPort()
{
    static constexpr ImGuiWindowFlags windowFlags {
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoTitleBar
    };

    ImGui::Begin("Viewport", &mShowViewport, windowFlags);

    mCamera.update(mDt);

    ImGui::Dummy(ImGui::GetContentRegionAvail());
    modelDragDropTarget();

    ImGui::End();
}

void Editor::sceneNodeInspector(SceneNode *node)
{
    switch (node->type())
    {
        case NodeType::Empty: emptyNodeInspector(node); break;
        case NodeType::Mesh: meshNodeInspector(node); break;
        case NodeType::DirectionalLight: dirLightInspector(node); break;
        case NodeType::PointLight: pointLightInspector(node); break;
        case NodeType::SpotLight: spotLightInspector(node); break;
        default: assert(false);
    }
}

void Editor::emptyNodeInspector(SceneNode *node)
{
    ImGui::Text("Asset Type: Empty Node");
    ImGui::Text("Name: %s", node->name().c_str());

    nodeTransform(node);
}

void Editor::meshNodeInspector(SceneNode *node)
{
    MeshNode* meshNode = dynamic_cast<MeshNode*>(node);

    ImGui::Text("Asset Type: Mesh Node");
    ImGui::Text("Name: %s", node->name().c_str());

    nodeTransform(node);

    ImGui::SeparatorText("Info");

    ImGui::Text("Mesh: %s", mResourceManager->mMeshNames.at(meshNode->meshID()).c_str());

    uuid64_t matID = mResourceManager->getMatIdFromIndex(meshNode->materialIndex());
    ImGui::Text("Material: %s", mResourceManager->mMaterialNames.at(matID).c_str());
}

void Editor::dirLightInspector(SceneNode *node)
{
    DirectionalLight* dirLight = dynamic_cast<DirectionalLight*>(node);

    ImGui::Text("Asset Type: Directional Light");
    ImGui::Text("Name: %s", node->name().c_str());

    nodeTransform(dirLight);

    lightOptions(dirLight);
    lightShadowOptions(dirLight);
}

void Editor::pointLightInspector(SceneNode *node)
{
    PointLight* pointLight = dynamic_cast<PointLight*>(node);

    ImGui::Text("Asset Type: Point Light");
    ImGui::Text("Name: %s", node->name().c_str());

    nodeTransform(pointLight);

    lightOptions(pointLight);
    ImGui::DragFloat("Range##pointLight", &pointLight->range, 0.1f);
    lightShadowOptions(pointLight);
}

void Editor::spotLightInspector(SceneNode *node)
{
    SpotLight* spotLight = dynamic_cast<SpotLight*>(node);

    ImGui::Text("Asset Type: Spot Light");
    ImGui::Text("Name: %s", node->name().c_str());

    nodeTransform(spotLight);

    lightOptions(spotLight);
    ImGui::DragFloat("Range##spotLight", &spotLight->range, 0.1f);
    lightShadowOptions(spotLight);
}

void Editor::nodeTransform(SceneNode *node)
{
    ImGui::SeparatorText("Transformation");

    glm::mat4 mat = node->globalTransform();

    float translation[3];
    float rotation[3];
    float scale[3];

    ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(mat), translation, rotation, scale);

    bool modified = false;

    if (ImGui::DragFloat3("Translation", translation, 0.1f))
        modified = true;

    if (ImGui::DragFloat3("Rotation", rotation, 0.1f))
        modified = true;

    if (ImGui::DragFloat3("Scale", scale, 0.1f))
        modified = true;

    if (modified)
    {
        ImGuizmo::RecomposeMatrixFromComponents(translation, rotation, scale, glm::value_ptr(mat));
        node->setLocalTransform(mat);
    }
}

void Editor::lightOptions(LightBase *light)
{
    ImGui::SeparatorText("Lighting Options");
    ImGui::ColorEdit3("Color##light", glm::value_ptr(light->color), ImGuiColorEditFlags_DisplayRGB);
    ImGui::DragFloat("Intensity##light", &light->intensity, 0.1f, 0.f); // todo: clamp
}

void Editor::createEmptyNode()
{
    mSceneGraph.addNode(new SceneNode(NodeType::Empty, "Empty Node", glm::identity<glm::mat4>(), nullptr));
}

// todo: implement properly
void Editor::createDirectionalLight()
{
    LightSpecification lightSpecification {
        .color = glm::vec3(1.f),
        .intensity = 1.f,
        .shadowOption = ShadowOptionNoShadows,
        .shadowSoftness = ShadowSoftnessMedium,
        .shadowStrength = 1.f,
        .shadowBias = 0.04f
    };

    mSceneGraph.addNode(new DirectionalLight(lightSpecification));
}

void Editor::createSpotLight()
{
    LightSpecification lightSpecification {
        .color = glm::vec3(1.f),
        .intensity = 1.f,
        .shadowOption = ShadowOptionNoShadows,
        .shadowSoftness = ShadowSoftnessMedium,
        .shadowStrength = 1.f,
        .shadowBias = 0.04f
    };

    mSceneGraph.addNode(new SpotLight(lightSpecification));
}

void Editor::createPointLight()
{
    LightSpecification lightSpecification {
        .color = glm::vec3(1.f),
        .intensity = 1.f,
        .shadowOption = ShadowOptionNoShadows,
        .shadowSoftness = ShadowSoftnessMedium,
        .shadowStrength = 1.f,
        .shadowBias = 0.04f
    };

    mSceneGraph.addNode(new PointLight(lightSpecification));
}

void Editor::lightShadowOptions(LightBase *light)
{
    ImGui::SeparatorText("Shadow Options");

    if (ImGui::BeginCombo("Shadow Option", toStr(light->shadowOption)))
    {
        for (uint32_t i = 0; i < ShadowOptionCount; ++i)
        {
            bool selected = light->shadowOption == i;

            if (ImGui::Selectable(toStr(static_cast<ShadowOption>(i)), selected))
                light->shadowOption = static_cast<ShadowOption>(i);
        }

        ImGui::EndCombo();
    }

    if (light->shadowOption == ShadowOptionSoftShadows)
    {
        if (ImGui::BeginCombo("Shadow Softness", toStr(light->shadowSoftness)))
        {
            for (uint32_t i = 0; i < ShadowSoftnessCount; ++i)
            {
                bool selected = light->shadowSoftness == i;

                if (ImGui::Selectable(toStr(static_cast<ShadowSoftness>(i)), selected))
                    light->shadowSoftness = static_cast<ShadowSoftness>(i);
            }

            ImGui::EndCombo();
        }
    }

    if (light->shadowOption != ShadowOptionNoShadows)
    {
        ImGui::SliderFloat("Strength", &light->shadowStrength, 0.001f, 1.f);
        ImGui::SliderFloat("Bias", &light->shadowBias, 0.001f, 1.f);
    }
}

void Editor::sceneGraphPopup()
{
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        ImGui::OpenPopup("sceneGraphPopup");

    if (ImGui::BeginPopup("sceneGraphPopup"))
    {
        if (ImGui::MenuItem("Cut##sceneGraph", nullptr, false, false))
            nullptr;

        if (ImGui::MenuItem("Copy##sceneGraph", nullptr, false, false))
            nullptr;

        if (ImGui::MenuItem("Paste##sceneGraph", nullptr, false, false))
            nullptr;

        if (ImGui::MenuItem("Paste as Child##sceneGraph", nullptr, false, false))
            nullptr;

        ImGui::Separator();

        if (ImGui::MenuItem("Rename##sceneGraph", nullptr, false, false))
            nullptr;

        if (ImGui::MenuItem("Duplicate##sceneGraph", nullptr, false, false))
            nullptr;

        if (ImGui::MenuItem("Delete##sceneGraph", nullptr, false, nodeSelected()))
            deleteSelectedNode();

        ImGui::Separator();

        if (ImGui::MenuItem("Create Empty##sceneGraph"))
            createEmptyNode();

        if (ImGui::BeginMenu("Create Light"))
        {
            if (ImGui::MenuItem("Directional Light##sceneGraph"))
                createDirectionalLight();

            if (ImGui::MenuItem("Point Light##sceneGraph"))
                createPointLight();

            if (ImGui::MenuItem("Spot Light##sceneGraph"))
                createSpotLight();

            ImGui::EndMenu();
        }

        ImGui::EndPopup();
    }
}

bool Editor::nodeSelected()
{
    return UUIDRegistry::isSceneNode(mSelectedObjectID);
}

void Editor::deleteSelectedNode()
{
    mSceneGraph.deleteNode(mSelectedObjectID);
    mSelectedObjectID = 0;
}

void Editor::deleteSelectedModel()
{
    mResourceManager->deleteModel(mSelectedObjectID);
    mSelectedObjectID = 0;
}

void Editor::deleteSelectedMaterial()
{
    mResourceManager->deleteMaterial(mSelectedObjectID);
    mSelectedObjectID = 0;
}

void Editor::deleteSelectedTexture()
{
    mResourceManager->deleteTexture(mSelectedObjectID);
    mSelectedObjectID = 0;
}
