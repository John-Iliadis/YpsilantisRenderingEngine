//
// Created by Gianni on 4/02/2025.
//

#include "editor.hpp"

Editor::Editor(Renderer& renderer, SaveData& saveData)
    : mRenderer(renderer)
    , mSaveData(saveData)
    , mSceneGraph(mRenderer.mSceneGraph)
    , mSelectedObjectID()
    , mFPS()
    , mFrametimeMs()
    , mCopyFlag(CopyFlags::None)
    , mCopiedNodeID()
{
}

Editor::~Editor()
{
    mSaveData["viewport"]["width"] = static_cast<uint32_t>(mViewportSize.x);
    mSaveData["viewport"]["height"] = static_cast<uint32_t>(mViewportSize.y);
}

void Editor::update(float dt)
{
    mDt = dt;

    ImGui::ShowDemoWindow();

    countFPS();
    updateFrametimeStats();
    keyboardShortcuts();

    imguiEvents();
    assetPanel();
    viewPort();
    sceneGraphPanel();
    cameraPanel();
    inspectorPanel();
    rendererPanel();
    debugPanel();

    if (mShowSSAOOutputTexture)
        ssaoTextureDebugWin();

    sharedPopups();
}

void Editor::imguiEvents()
{
    if (ImGui::IsKeyPressed(ImGuiKey_Delete))
        deleteSelectedObject();
}

void Editor::cameraPanel()
{
    ImGui::Begin("Camera", nullptr);

    ImGui::SeparatorText("Settings");
    ImGui::SliderFloat("Field of View##Cam", mRenderer.mCamera.fov(), 1.f, 145.f, "%0.f");

    ImGui::DragFloat("Near Plane",
                     mRenderer.mCamera.nearPlane(),
                     0.001f, 0.001f, *mRenderer.mCamera.farPlane(),
                     "%.3f", ImGuiSliderFlags_AlwaysClamp);

    ImGui::DragFloat("Far Plane",
                     mRenderer.mCamera.farPlane(),
                     0.01f, *mRenderer.mCamera.nearPlane(), 10000.f,
                     "%.2f", ImGuiSliderFlags_AlwaysClamp);

    ImGui::SeparatorText("Camera Mode");

    Camera::State cameraState = mRenderer.mCamera.state();

    if (ImGui::RadioButton("View", cameraState == Camera::State::VIEW_MODE))
        mRenderer.mCamera.setState(Camera::State::VIEW_MODE);

    ImGui::SameLine();
    helpMarker("Press V to select mode\n"
               "Hold left click to pan\n"
               "Hold right click to rotate\n"
               "Use the scroll wheel to zoom");

    if (ImGui::RadioButton("Edit", cameraState == Camera::State::EDITOR_MODE))
        mRenderer.mCamera.setState(Camera::State::EDITOR_MODE);

    ImGui::SameLine();
    helpMarker("Press Tab to select mode\n"
               "Hold right click to rotate\n"
               "Use the scroll wheel to zoom");

    if (ImGui::RadioButton("First Person", cameraState == Camera::State::FIRST_PERSON))
        mRenderer.mCamera.setState(Camera::State::FIRST_PERSON);

    ImGui::SameLine();
    helpMarker("Press F to select\n"
               "Use WASD to move\n"
               "Hold left click to rotate\n");

    ImGui::Separator();

    cameraState = mRenderer.mCamera.state();

    ImGui::DragFloat("Rotate Sensitivity", mRenderer.mCamera.rotateSensitivity(), 0.1f, -FLT_MAX, FLT_MAX, "%.0f", ImGuiSliderFlags_AlwaysClamp);

    if (cameraState == Camera::State::VIEW_MODE || cameraState == Camera::State::EDITOR_MODE)
        ImGui::DragFloat("Zoom Offset", mRenderer.mCamera.zScrollOffset(), 0.01f, 0.f, FLT_MAX, "%.2f", ImGuiSliderFlags_AlwaysClamp);

    if (cameraState == Camera::State::VIEW_MODE)
        ImGui::DragFloat("Pan Speed", mRenderer.mCamera.panSpeed(), 0.1f, 0.f, FLT_MAX, "%.0f", ImGuiSliderFlags_AlwaysClamp);

    if (cameraState == Camera::State::FIRST_PERSON)
        ImGui::DragFloat("Fly Speed", mRenderer.mCamera.flySpeed(), 0.1f, 0.01f, FLT_MAX, "%.0f", ImGuiSliderFlags_AlwaysClamp);

    ImGui::End();
}

void Editor::assetPanel()
{
    ImGui::Begin("Assets", nullptr);

    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        ImGui::OpenPopup("assetPanelPopup");

    for (const auto& [modelID, model] : mRenderer.mModels)
    {
        ImGui::Selectable(model->name.c_str(), mSelectedObjectID == modelID);
        lastItemClicked(modelID);
        modelDragDropSource();
    }

    assetPanelPopup();

    if (emptySpaceClicked())
        deselectAll();

    ImGui::End();
}

void Editor::sceneGraphPanel()
{
    ImGui::Begin("Scene Graph", nullptr);

    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        ImGui::OpenPopup("sceneGraphPopup");

    for (auto child : mSceneGraph->topLevelNodes())
        sceneNodeRecursive(child);

    bool nodeHovered = ImGui::IsItemHovered();
    ImGui::Dummy(ImVec2(0, 50.f));
    sceneNodeDragDropTargetWholeWindow(nodeHovered);

    sceneGraphPopup();

    if (emptySpaceClicked())
        deselectAll();

    ImGui::End();
}

void Editor::rendererPanel()
{
    static constexpr ImGuiColorEditFlags colorEditFlags = ImGuiColorEditFlags_DisplayRGB |
                                                          ImGuiColorEditFlags_AlphaBar;

    ImGui::Begin("Renderer", nullptr);

    if (ImGui::CollapsingHeader("General", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("View Gizmo Controls", &mViewGizmoControls);
        ImGui::Checkbox("View Axis Gizmo", &mViewAxisGizmo);
        ImGui::ColorEdit3("Clear Color", glm::value_ptr(mRenderer.mClearColor));
    }

    if (ImGui::CollapsingHeader("Grid", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Render Grid", &mRenderer.mRenderGrid);
        ImGui::Separator();

        ImGui::ColorEdit4("Thin line color", glm::value_ptr(mRenderer.mGridData.thinLineColor), colorEditFlags);
        ImGui::ColorEdit4("Thick line color", glm::value_ptr(mRenderer.mGridData.thickLineColor), colorEditFlags);
    }

    if (ImGui::CollapsingHeader("Image Based Lighting", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Render Skybox##IBL", &mRenderer.mRenderSkybox);
        ImGui::Checkbox("Enable IBL Lighting", &mRenderer.mEnableIblLighting);

        ImGui::SliderFloat("Skybox Field of View", &mRenderer.mSkyboxFov, 1.f, 145.f, "%0.f");

        if (ImGui::Button("Import New"))
        {
            std::filesystem::path path = fileDialog("Select HDR environment map", "HDR files *.hdr\0*.hdr;\0");
            if (!path.empty())
            {
                mRenderer.importEnvMap(path.string());
            }
        }
    }

    if (ImGui::CollapsingHeader("HDR", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Enable##HDR", &mRenderer.mHDROn);
        ImGui::Separator();

        if (ImGui::BeginCombo("Tonemapping", toStr(mRenderer.mTonemap)))
        {
            if (ImGui::Selectable("Reinhard", mRenderer.mTonemap == Tonemap::Reinhard))
                mRenderer.mTonemap = Tonemap::Reinhard;
            if (ImGui::Selectable("Reinhard Extended", mRenderer.mTonemap == Tonemap::ReinhardExtended))
                mRenderer.mTonemap = Tonemap::ReinhardExtended;
            if (ImGui::Selectable("Narkowicz Aces", mRenderer.mTonemap == Tonemap::NarkowiczAces))
                mRenderer.mTonemap = Tonemap::NarkowiczAces;
            if (ImGui::Selectable("Hill Aces", mRenderer.mTonemap == Tonemap::HillAces))
                mRenderer.mTonemap = Tonemap::HillAces;

            ImGui::EndCombo();
        }

        ImGui::DragFloat("Exposure", &mRenderer.mExposure, 0.001f, 0.f, FLT_MAX, "%.3f", ImGuiSliderFlags_AlwaysClamp);

        if (mRenderer.mTonemap == Tonemap::ReinhardExtended)
        {
            ImGui::DragFloat("Max White", &mRenderer.mMaxWhite, 0.01f, 0.f, FLT_MAX, "%.2f", ImGuiSliderFlags_AlwaysClamp);
        }
    }

    if (ImGui::CollapsingHeader("SSAO", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Enable##ssao", &mRenderer.mSsaoOn);
        ImGui::Separator();

        std::string prev = std::to_string(mRenderer.mSsaoKernel.size());
        if (ImGui::BeginCombo("Sample Count##ssao", prev.data()))
        {
            for (uint32_t i = 8; i <= 128; i *= 2)
            {
                std::string label = std::to_string(i);
                bool selected = i == mRenderer.mSsaoKernel.size();
                if (ImGui::Selectable(label.data(), selected) && !selected)
                {
                    mRenderer.createSsaoKernel(i);
                    mRenderer.updateSsaoKernelSSBO();
                }
            }

            ImGui::EndCombo();
        }

        ImGui::DragFloat("Radius##ssao", &mRenderer.mSsaoRadius, 0.001f, 0.f, FLT_MAX, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        ImGui::DragFloat("Intensity##ssao", &mRenderer.mSsaoIntensity, 0.001f, 0.f, FLT_MAX, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        ImGui::DragFloat("Bias##ssao", &mRenderer.mSsaoDepthBias, 0.0001f, 0.f, FLT_MAX, "%.4f", ImGuiSliderFlags_AlwaysClamp);
    }

    if (ImGui::CollapsingHeader("Bloom", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Enable##Bloom", &mRenderer.mBloomOn);
        ImGui::Separator();

        ImGui::DragFloat("Intensity##Bloom",
                         &mRenderer.mBloomStrength,
                         0.01f,
                         0.f, FLT_MAX,
                         "%.2f",
                         ImGuiSliderFlags_AlwaysClamp);

        ImGui::DragFloat("Brightness Threshold",
                         &mRenderer.mBrightnessThreshold,
                         0.001f,
                         0.f, FLT_MAX,
                         "%.3f",
                         ImGuiSliderFlags_AlwaysClamp);

        ImGui::DragFloat("Filter Radius",
                         &mRenderer.mFilterRadius,
                         0.0001f,
                         0.f, FLT_MAX,
                         "%.4f",
                         ImGuiSliderFlags_AlwaysClamp);
    }

    if (ImGui::CollapsingHeader("Order Independent Transparency", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Enable##OIT", &mRenderer.mOitOn);
        ImGui::Separator();

        std::string val = std::to_string(mRenderer.mOitLinkedListLength) + "x";
        if (ImGui::BeginCombo("Memory Multiplier", val.data(), ImGuiComboFlags_WidthFitPreview))
        {
            for (uint32_t i = 1; i <= 64; i <<= 1)
            {
                std::string preview = std::to_string(i) + "x";
                if (ImGui::Selectable(preview.data(), mRenderer.mOitLinkedListLength == i))
                {
                    if (i == mRenderer.mOitLinkedListLength)
                        continue;

                    mRenderer.mOitLinkedListLength = i;
                    mRenderer.createOitBuffers();
                    mRenderer.updateOitResourcesDs();
                }
            }

            ImGui::EndCombo();
        }

        ImGui::SameLine();
        uint32_t memUsage = mRenderer.mWidth * mRenderer.mHeight * sizeof(TransparentNode) * mRenderer.mOitLinkedListLength / 1000000;
        std::string mem = std::format("Higher memory = more transparent fragments\nMemory usage: {} MB", memUsage);
        helpMarker(mem.data());
    }

//    skyboxImportPopup();

    ImGui::End();
}

void Editor::inspectorPanel()
{
    ImGui::Begin("Inspector", nullptr);

    if (auto objectType = UUIDRegistry::getObjectType(mSelectedObjectID))
    {
        switch (*objectType)
        {
            case ObjectType::Model: modelInspector(mSelectedObjectID); break;
            case ObjectType::Mesh: assert(false);
            case ObjectType::SceneNode: sceneNodeInspector(mSceneGraph->searchNode(mSelectedObjectID)); break;
            default: assert(false);
        }
    }

    ImGui::End();
}

void Editor::viewPort()
{
    static constexpr ImVec2 sInitialViewportSize {
        InitialViewportWidth,
        InitialViewportHeight
    };

    static constexpr ImGuiWindowFlags sWindowFlags {
        ImGuiWindowFlags_NoScrollbar
    };

    ImGui::SetNextWindowSize(sInitialViewportSize, ImGuiCond_FirstUseEver);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
    ImGui::Begin("Scene", nullptr, sWindowFlags);
    ImGui::PopStyleVar(2);

    modelDragDropTargetWholeWindow();

    ImVec2 viewportSize = ImGui::GetContentRegionAvail();

    viewportSize.x = glm::clamp(viewportSize.x, 1.f, FLT_MAX);
    viewportSize.y = glm::clamp(viewportSize.y, 1.f, FLT_MAX);

    if (mViewportSize != viewportSize && !ImGui::IsMouseDown(ImGuiMouseButton_Left))
    {
        mViewportSize = viewportSize;
        mRenderer.resize(viewportSize.x, viewportSize.y);
    }

    ImGui::Image(reinterpret_cast<ImTextureID>(mRenderer.mColor8UDs), viewportSize);

    bool usingGizmo = transformGizmo();

    if (mViewGizmoControls)
        gizmoOptions();

    if (mViewAxisGizmo)
        drawAxisGizmo();

    if (!usingGizmo)
        mRenderer.mCamera.update(mDt);

    ImGui::End();
}

void Editor::debugPanel()
{
    ImGui::Begin("Debug", nullptr);

    ImGui::Text("GPU: %s", mRenderer.mRenderDevice.getDeviceProperties().deviceName);
    ImGui::Separator();

    ImGui::Text("Window size: %lux%lu px",
                static_cast<uint32_t>(ImGui::GetIO().DisplaySize.x),
                static_cast<uint32_t>(ImGui::GetIO().DisplaySize.y));
    ImGui::Text("Rendering viewport size: %lux%lu px",
                static_cast<uint32_t>(mViewportSize.x),
                static_cast<uint32_t>(mViewportSize.y));
    ImGui::Separator();

    plotPerformanceGraphs();

    ImGui::Separator();

    ImGui::Checkbox("Debug normals", &mRenderer.mDebugNormals);
    ImGui::Checkbox("Show SSAO output texture", &mShowSSAOOutputTexture);

    ImGui::End();
}

void Editor::ssaoTextureDebugWin()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
    ImGui::Begin("SSAO Output Texture", &mShowSSAOOutputTexture, ImGuiWindowFlags_NoScrollbar);
    ImGui::PopStyleVar(2);

    ImTextureID textureId = static_cast<ImTextureID>(mRenderer.mSsaoBlurTextureDs);
    ImGui::Image(textureId, mViewportSize);

    ImGui::End();
}

void Editor::sceneNodeRecursive(GraphNode *node)
{
    static std::unordered_map<GraphNode*, bool> sOpenedState;
    if (!sOpenedState.contains(node))
        sOpenedState.emplace(node, false);

    ImGuiTreeNodeFlags treeNodeFlags {
        ImGuiTreeNodeFlags_OpenOnArrow |
        ImGuiTreeNodeFlags_OpenOnDoubleClick |
        ImGuiTreeNodeFlags_SpanFullWidth |
        ImGuiTreeNodeFlags_FramePadding
    };

    if (mSelectedObjectID == node->id()) treeNodeFlags |= ImGuiTreeNodeFlags_Selected;
    if (node->children().empty()) treeNodeFlags |= ImGuiTreeNodeFlags_Leaf;
    if (sOpenedState.at(node)) treeNodeFlags |= ImGuiTreeNodeFlags_DefaultOpen;

    bool nodeOpen = ImGui::TreeNodeEx((void*)(intptr_t)node, treeNodeFlags, node->name().c_str());

    sOpenedState.at(node) = nodeOpen;

    lastItemClicked(node->id());

    sceneNodeDragDropSource(node);
    sceneNodeDragDropTarget(node);

    if (nodeOpen)
    {
        for (auto child : node->children())
            sceneNodeRecursive(child);

        ImGui::TreePop();
    }
}

void Editor::createModelGraph(Model &model)
{
    GraphNode* graphNode = createModelGraphRecursive(model, model.root, nullptr);
    graphNode->localT = spawnPos();
    mSceneGraph->addNode(graphNode);
}

GraphNode *Editor::createModelGraphRecursive(Model &model, const SceneNode &sceneNode, GraphNode *parent)
{
    GraphNode* graphNode;

    if (!sceneNode.meshIndices.empty())
        graphNode = createMeshNode(model, sceneNode, parent);
    else
    {
        graphNode = new GraphNode(NodeType::Empty,
                                  sceneNode.name,
                                  sceneNode.translation,
                                  sceneNode.rotation,
                                  sceneNode.scale,
                                  parent,
                                  model.id);
    }

    for (const auto& child : sceneNode.children)
        graphNode->addChild(createModelGraphRecursive(model, child, graphNode));

    return graphNode;
}

GraphNode *Editor::createEmptyNode(GraphNode *parent, const std::string& name)
{
    return new GraphNode(NodeType::Empty, name, spawnPos(), {}, glm::vec3(1.f), parent);
}

GraphNode *Editor::createMeshNode(Model &model, const SceneNode &sceneNode, GraphNode* parent)
{
    std::vector<uuid32_t> meshIDs;

    for (unsigned int meshIndex : sceneNode.meshIndices)
    {
        const Mesh& mesh = model.meshes.at(meshIndex);
        meshIDs.push_back(mesh.meshID);
    }

    GraphNode* graphNode = new GraphNode(NodeType::Mesh,
                                        sceneNode.name,
                                        sceneNode.translation,
                                        sceneNode.rotation,
                                        sceneNode.scale,
                                        parent,
                                        model.id,
                                        meshIDs);

    for (uuid32_t meshID : meshIDs)
        model.getMesh(meshID)->mesh.addInstance(graphNode->id());

    return graphNode;
}

void Editor::addDirLight()
{
    constexpr glm::vec3 initialOrientation(-50.f, 30.f, -20.f);

    DirectionalLight dirLight {
        .color = glm::vec4(1.f),
        .direction = glm::vec4(calcLightDir(initialOrientation), 0.f),
        .intensity = 1.f
    };

    GraphNode* lightNode = new GraphNode(NodeType::DirectionalLight,
                                         "Directional Light",
                                         spawnPos(),
                                         initialOrientation,
                                         glm::vec3(1.f),
                                         nullptr);

    mSceneGraph->addNode(lightNode);
    mRenderer.addDirLight(lightNode->id(), dirLight);
}

void Editor::addPointLight()
{
    PointLight pointLight {
        .color = glm::vec4(1.f),
        .position = glm::vec4(spawnPos(), 1.f),
        .intensity = 1.f,
        .range = 10.f
    };

    GraphNode* lightNode = new GraphNode(NodeType::PointLight,
                                         "Point Light",
                                         pointLight.position,
                                         {},
                                         glm::vec3(1.f),
                                         nullptr);

    mSceneGraph->addNode(lightNode);
    mRenderer.addPointLight(lightNode->id(), pointLight);
}

void Editor::addSpotLight()
{
    constexpr glm::vec3 initialOrientation(0.f, 0.f, 0.f);

    SpotLight spotLight {
        .color = glm::vec4(1.f),
        .position = glm::vec4(spawnPos(), 1.f),
        .direction = glm::vec4(calcLightDir(initialOrientation), 0.f),
        .intensity = 1.f,
        .range = 10.f,
        .innerAngle = 45.f,
        .outerAngle = 60.f
    };

    GraphNode* lightNode = new GraphNode(NodeType::SpotLight,
                                         "Spot Light",
                                         spotLight.position,
                                         initialOrientation,
                                         glm::vec3(1.f),
                                         nullptr);

    mSceneGraph->addNode(lightNode);
    mRenderer.addSpotLight(lightNode->id(), spotLight);
}

void Editor::copyNode(uuid32_t nodeID)
{
    mCopyFlag = CopyFlags::Copy;
    mCopiedNodeID = nodeID;
}

void Editor::cutNode(uuid32_t nodeID)
{
    mCopyFlag = CopyFlags::Cut;
    mCopiedNodeID = nodeID;
}

void Editor::pasteNode(GraphNode *parent)
{
    GraphNode* copiedNode = mSceneGraph->searchNode(mCopiedNodeID);

    if (copiedNode)
    {
        if (mCopyFlag == CopyFlags::Copy)
        {
            GraphNode* newNode = copyGraphNode(copiedNode);
            newNode->orphan();
            parent->addChild(newNode);
        }

        if (mCopyFlag == CopyFlags::Cut && !mSceneGraph->hasDescendant(copiedNode, parent))
        {
            copiedNode->orphan();
            parent->addChild(copiedNode);
        }
    }

    mCopyFlag = CopyFlags::None;
    mCopiedNodeID = 0;
}

void Editor::duplicateNode(GraphNode *node)
{
    GraphNode* copyNode = copyGraphNode(node);
    copyNode->parent()->addChild(copyNode);
}

void Editor::modelInspector(uuid32_t modelID)
{
    auto& model = *mRenderer.mModels.at(modelID);

    ImGui::Text("Asset Type: Model");
    ImGui::Text("Name: %s", model.name.c_str());

    if (ImGui::CollapsingHeader("Info", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("Mesh Count: %zu", model.meshes.size());
        ImGui::Text("Material Count: %zu", model.materials.size());
        ImGui::Text("Texture Count: %zu", model.textures.size());
    }

    if (ImGui::CollapsingHeader("Model Settings", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::SeparatorText("Face Culling");
        if (ImGui::BeginCombo("Cull Mode", toStr(model.cullMode)))
        {
            if (ImGui::Selectable("None", !strcmp("None", toStr(model.cullMode))))
                model.cullMode = VK_CULL_MODE_NONE;

            if (ImGui::Selectable("Back", !strcmp("Back", toStr(model.cullMode))))
                model.cullMode = VK_CULL_MODE_BACK_BIT;

            if (ImGui::Selectable("Front", !strcmp("Front", toStr(model.cullMode))))
                model.cullMode = VK_CULL_MODE_FRONT_BIT;

            if (ImGui::Selectable("Front and Back", !strcmp("Front and Back", toStr(model.cullMode))))
                model.cullMode = VK_CULL_MODE_FRONT_AND_BACK;

            ImGui::EndCombo();
        }

        if (ImGui::BeginCombo("Front Face", toStr(model.frontFace)))
        {
            if (ImGui::Selectable("Clockwise", !strcmp("Clockwise", toStr(model.frontFace))))
                model.frontFace = VK_FRONT_FACE_CLOCKWISE;

            if (ImGui::Selectable("Counter Clockwise", !strcmp("Counter Clockwise", toStr(model.frontFace))))
                model.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

            ImGui::EndCombo();
        }

        ImGui::Spacing();
    }

    if (!model.materials.empty())
    {
        static std::unordered_map<uuid32_t, uint32_t> sSelectedMatIndex;
        if (!sSelectedMatIndex.contains(modelID))
            sSelectedMatIndex[modelID] = 0;

        if (ImGui::CollapsingHeader("Material Preview", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Spacing();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            if (ImGui::BeginCombo("##MaterialPreviewCombo", model.materialNames.at(sSelectedMatIndex.at(modelID)).c_str()))
            {
                for (uint32_t i = 0; i < model.materials.size(); ++i)
                {
                    std::string matName = std::format("{}##{}", model.materialNames.at(i), i);

                    if (ImGui::Selectable(matName.c_str(), i == sSelectedMatIndex.at(modelID)))
                        sSelectedMatIndex.at(modelID) = i;
                }

                ImGui::EndCombo();
            }

            Material& mat = model.materials.at(sSelectedMatIndex.at(modelID));
            bool matNeedsUpdate = false;

            if (hasTextures(mat))
            {
                ImGui::SeparatorText("Material Textures");
                if (mat.baseColorTexIndex != -1) matTexInspector("Base Color", model.textures.at(mat.baseColorTexIndex));
                if (mat.metallicTexIndex != -1) matTexInspector("Metallic", model.textures.at(mat.metallicTexIndex));
                if (mat.roughnessTexIndex != -1) matTexInspector("Roughness", model.textures.at(mat.roughnessTexIndex));
                if (mat.normalTexIndex != -1) matTexInspector("Normal", model.textures.at(mat.normalTexIndex));
                if (mat.aoTexIndex != -1) matTexInspector("AO", model.textures.at(mat.aoTexIndex));
                if (mat.emissionTexIndex != -1) matTexInspector("Emission", model.textures.at(mat.emissionTexIndex));
            }

            ImGui::SeparatorText("Material Factors");

            static constexpr ImGuiColorEditFlags sColorEditFlags = ImGuiColorEditFlags_DisplayRGB |
                ImGuiColorEditFlags_AlphaBar;

            if (ImGui::ColorEdit4("Base Color", glm::value_ptr(mat.baseColorFactor), sColorEditFlags))
                matNeedsUpdate = true;

            if (ImGui::ColorEdit3("Emission Color", glm::value_ptr(mat.emissionColor), sColorEditFlags))
                matNeedsUpdate = true;

            if (ImGui::SliderFloat("Metallic", &mat.metallicFactor, 0.f, 1.f))
                matNeedsUpdate = true;

            if (ImGui::SliderFloat("Roughness", &mat.roughnessFactor, 0.f, 1.f))
                matNeedsUpdate = true;

            if (ImGui::DragFloat("Emission Factor", &mat.emissionFactor, 0.01f, 0.f, FLT_MAX, "%.2f", ImGuiSliderFlags_AlwaysClamp))
                matNeedsUpdate = true;

            ImGui::SeparatorText("Texture Coordinates");

            if (ImGui::DragFloat2("Tiling", glm::value_ptr(mat.tiling), 0.01f))
                matNeedsUpdate = true;

            if (ImGui::DragFloat2("Offset", glm::value_ptr(mat.offset), 0.01f))
                matNeedsUpdate = true;

            if (matNeedsUpdate)
                model.updateMaterial(sSelectedMatIndex.at(modelID));
        }
    }
}

void Editor::matTexInspector(const char* label, const Texture& texture)
{
    static constexpr ImVec2 sImageSize = ImVec2(30.f, 30.f);
    static constexpr ImVec2 sTooltipImageSize = ImVec2(250.f, 250.f);
    static constexpr ImVec2 sImageButtonFramePadding = ImVec2(2.f, 2.f);
    static const ImVec2 sTextSize = ImGui::CalcTextSize("H");

    ImTextureID texHandle = reinterpret_cast<ImTextureID>(texture.descriptorSet);

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, sImageButtonFramePadding);
    ImGui::ImageButton(texHandle, sImageSize);
    ImGui::PopStyleVar();

    if (ImGui::BeginItemTooltip())
    {
        ImGui::Text(texture.name.c_str());
        ImGui::Separator();

        ImGui::Text("Size: %lux%lu", texture.vulkanTexture.width, texture.vulkanTexture.height);
        ImGui::Text("Wrap U: %s", toStr(texture.vulkanTexture.vulkanSampler.wrapS));
        ImGui::Text("Wrap V: %s", toStr(texture.vulkanTexture.vulkanSampler.wrapT));
        ImGui::Text("Mag filter: %s", toStr(texture.vulkanTexture.vulkanSampler.magFilter));
        ImGui::Text("Min filter: %s", toStr(texture.vulkanTexture.vulkanSampler.minFilter));
        ImGui::Separator();

        float textureWidth = static_cast<float>(texture.vulkanTexture.width);
        float textureHeight = static_cast<float>(texture.vulkanTexture.height);
        float aspectRatio = (textureWidth > 0) ? (textureHeight / textureWidth) : 1.0f;
        float tooltipImageWidth = glm::max(250.f, ImGui::CalcTextSize(texture.name.c_str()).x);
        float tooltipImageHeight = tooltipImageWidth * aspectRatio;

        ImGui::Image(texHandle, ImVec2(tooltipImageWidth, tooltipImageHeight));
        ImGui::EndTooltip();
    }

    ImGui::SameLine();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.f);
    ImGui::Text("%s: %s", label, texture.name.c_str());
}

void Editor::sceneNodeInspector(GraphNode *node)
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

void Editor::emptyNodeInspector(GraphNode *node)
{
    ImGui::Text("Asset Type: Scene Node (Empty)");
    ImGui::Text("Name: %s", node->name().c_str());

    nodeTransform(node);

    if (node->modelID().has_value())
    {
        if (ImGui::CollapsingHeader("Info##emptyNodeInspector", ImGuiTreeNodeFlags_DefaultOpen))
        {
            const auto& model = *mRenderer.mModels.at(*node->modelID());
            ImGui::Text("Associated Model: %s", model.name.c_str());
        }
    }
}

void Editor::meshNodeInspector(GraphNode *node)
{
    ImGui::Text("Asset Type: Scene Node (Mesh)");
    ImGui::Text("Name: %s", node->name().c_str());

    nodeTransform(node);

    if (ImGui::CollapsingHeader("Info##meshNodeInspector", ImGuiTreeNodeFlags_DefaultOpen))
    {
        auto &model = *mRenderer.mModels.at(node->modelID().value());

        std::vector<Mesh *> meshes;
        for (uuid32_t meshID: node->meshIDs())
            meshes.push_back(model.getMesh(meshID));

        ImGui::Text("Associated Model: %s", model.name.c_str());

        if (meshes.size() == 1)
        {
            Mesh &mesh = *meshes.front();
            int32_t materialIndex = mesh.materialIndex;

            ImGui::Text("Mesh: %s", mesh.name.c_str());
            ImGui::Text("Material: %s", mesh.materialIndex == -1 ? "None"
                                                                 : model.materialNames.at(materialIndex).c_str());
        }
        else
        {
            for (uint32_t i = 0; i < meshes.size(); ++i)
            {
                Mesh &mesh = *meshes.at(i);
                uint32_t materialIndex = mesh.materialIndex;

                ImGui::Text("Mesh %d: %s", i, mesh.name.c_str());
                ImGui::Text("Material %d: %s", i, mesh.materialIndex == -1 ? "None"
                                                                           : model.materialNames.at(
                        materialIndex).c_str());
                if (i != meshes.size() - 1)
                    ImGui::Spacing();
            }
        }
    }
}

void Editor::dirLightInspector(GraphNode *node)
{
    DirectionalLight& dirLight = mRenderer.getDirLight(node->id());

    ImGui::Text("Asset Type: Scene Node (Directional Light)");
    ImGui::Text("Name: %s", node->name().data());
    ImGui::Separator();

    bool modified = false;

    if (nodeTransform(node))
        modified = true;

    if (ImGui::CollapsingHeader("Emission##dirLight", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (ImGui::ColorEdit3("Color##dirLight", glm::value_ptr(dirLight.color), ImGuiColorEditFlags_DisplayRGB))
            modified = true;

        if (ImGui::DragFloat("Intensity##dirLight", &dirLight.intensity, 0.01f, 0.f, FLT_MAX, "%.2f", ImGuiSliderFlags_AlwaysClamp))
            modified = true;
    }

    if (ImGui::CollapsingHeader("Shadows##dirLight", ImGuiTreeNodeFlags_DefaultOpen))
    {
        bool modifiedShadowMapResolution = false;

        DirShadowData& options = mRenderer.getDirShadowData(node->id());
        DirShadowMap& shadowMap = mRenderer.getDirShadowMap(node->id());

        std::string prevCascade = std::format("{}x", options.cascadeCount);
        if (ImGui::BeginCombo("Cascade Count", prevCascade.data()))
        {
            for (uint32_t i = 3; i <= 9; ++i)
            {
                std::string selectablePrev = std::format("{}x", i);
                if (ImGui::Selectable(selectablePrev.data(), i == options.cascadeCount) &&
                    options.cascadeCount != i)
                {
                    options.cascadeCount = i;
                    modifiedShadowMapResolution = true;
                }
            }

            ImGui::EndCombo();
        }

        const char* shadowTypePreview = toStr(options.shadowType);
        if (ImGui::BeginCombo("Shadow Type##dirLight", shadowTypePreview))
        {
            if (ImGui::Selectable("No Shadow", options.shadowType == ShadowType::NoShadow))
                options.shadowType = ShadowType::NoShadow;

            if (ImGui::Selectable("Hard Shadow", options.shadowType == ShadowType::HardShadow))
                options.shadowType = ShadowType::HardShadow;


            if (ImGui::Selectable("Soft Shadow", options.shadowType == ShadowType::SoftShadow))
                options.shadowType = ShadowType::SoftShadow;

            ImGui::EndCombo();
        }

        if (options.shadowType != ShadowType::NoShadow)
        {
            std::string resolution = std::format("{}px", std::to_string(options.resolution));
            if (ImGui::BeginCombo("Resolution##dirLight", resolution.data()))
            {
                for (uint32_t i = 256; i <= 4096; i *= 2)
                {
                    std::string res = std::format("{}px", std::to_string(i));
                    if (ImGui::Selectable(res.data(), i == options.resolution) &&
                        i != options.resolution)
                    {
                        options.resolution = i;
                        modifiedShadowMapResolution = true;
                    }
                }

                ImGui::EndCombo();
            }

            ImGui::SliderFloat("Strength##dirLight", &options.strength, 0.f, 1.f);
            ImGui::DragFloat("Bias Slope##dirLight", &options.biasSlope, 0.000001f, 0.f, FLT_MAX, "%.6f", ImGuiSliderFlags_AlwaysClamp);
            ImGui::DragFloat("Bias Constant##dirLight", &options.biasConstant, 0.000001f, 0.f, FLT_MAX, "%.6f", ImGuiSliderFlags_AlwaysClamp);
            ImGui::DragFloat("Depth Scalar##dirLight", &options.zScalar, 0.1f, 0.f, FLT_MAX, "%.1f", ImGuiSliderFlags_AlwaysClamp);

            if (options.shadowType == ShadowType::SoftShadow)
            {
                ImGui::SliderInt("PCF Range (Smoothing)##dirLight", &options.pcfRange, 1, 10);
            }
        }

        if (modifiedShadowMapResolution) mRenderer.updateDirShadowMapImage(node->id());
    }

    if (modified) mRenderer.updateDirLight(node->id());
}

void Editor::pointLightInspector(GraphNode *node)
{
    PointLight& pointLight = mRenderer.getPointLight(node->id());

    ImGui::Text("Asset Type: Scene Node (Point Light)");
    ImGui::Text("Name: %s", node->name().data());
    ImGui::Separator();

    bool modified = false;

    if (nodeTransform(node))
        modified = true;

    if (ImGui::CollapsingHeader("Emission##pointLight", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (ImGui::ColorEdit3("Color##pointLight", glm::value_ptr(pointLight.color), ImGuiColorEditFlags_DisplayRGB))
            modified = true;

        if (ImGui::DragFloat("Intensity##pointLight", &pointLight.intensity, 0.01f, 0.f, FLT_MAX, "%.2f", ImGuiSliderFlags_AlwaysClamp))
            modified = true;

        if (ImGui::DragFloat("Range##pointLight", &pointLight.range, 0.01f, 0.01f, FLT_MAX, "%.2f", ImGuiSliderFlags_AlwaysClamp))
            modified = true;
    }

    if (ImGui::CollapsingHeader("Shadows##pointLight", ImGuiTreeNodeFlags_DefaultOpen))
    {
        bool modifiedShadowData = false;
        bool modifiedShadowMapResolution = false;

        PointShadowData& options = mRenderer.getPointShadowData(node->id());
        PointShadowMap& shadowMap = mRenderer.getPointShadowMap(node->id());

        const char* shadowTypePreview = toStr(options.shadowType);
        if (ImGui::BeginCombo("Shadow Type##pointLight", shadowTypePreview))
        {
            if (ImGui::Selectable("No Shadow", options.shadowType == ShadowType::NoShadow))
            {
                options.shadowType = ShadowType::NoShadow;
                modifiedShadowData = true;
            }

            if (ImGui::Selectable("Hard Shadow", options.shadowType == ShadowType::HardShadow))
            {
                options.shadowType = ShadowType::HardShadow;
                modifiedShadowData = true;
            }

            if (ImGui::Selectable("Soft Shadow", options.shadowType == ShadowType::SoftShadow))
            {
                options.shadowType = ShadowType::SoftShadow;
                modifiedShadowData = true;
            }

            ImGui::EndCombo();
        }

        if (options.shadowType != ShadowType::NoShadow)
        {
            std::string resolution = std::format("{}px", std::to_string(options.resolution));
            if (ImGui::BeginCombo("Resolution##pointLight", resolution.data()))
            {
                for (uint32_t i = 256; i <= 4096; i *= 2)
                {
                    std::string res = std::format("{}px", std::to_string(i));
                    if (ImGui::Selectable(res.data(), i == options.resolution) &&
                        i != options.resolution)
                    {
                        options.resolution = i;
                        modifiedShadowData = true;
                        modifiedShadowMapResolution = true;
                    }
                }

                ImGui::EndCombo();
            }

            if (ImGui::SliderFloat("Strength##pointLight", &options.strength, 0.f, 1.f))
                modifiedShadowData = true;

            if (ImGui::DragFloat("Bias Slope##pointLight", &options.biasSlope, 0.000001f, 0.f, FLT_MAX, "%.6f", ImGuiSliderFlags_AlwaysClamp))
                modifiedShadowData = true;

            if (ImGui::DragFloat("Bias Constant##pointLight", &options.biasConstant, 0.000001f, 0.f, FLT_MAX, "%.6f", ImGuiSliderFlags_AlwaysClamp))
                modifiedShadowData = true;

            if (options.shadowType == ShadowType::SoftShadow)
            {
                if (ImGui::DragFloat("PCF Radius##pointLight", &options.pcfRadius, 0.0001f, 0.f, FLT_MAX, "%.4f", ImGuiSliderFlags_AlwaysClamp))
                    modifiedShadowData = true;
            }
        }

        if (modifiedShadowData) mRenderer.updatePointShadowMapData(node->id());
        if (modifiedShadowMapResolution) mRenderer.updatePointShadowMapImage(node->id());
    }

    if (modified) mRenderer.updatePointLight(node->id());
}

void Editor::spotLightInspector(GraphNode *node)
{
    SpotLight& spotLight = mRenderer.getSpotLight(node->id());

    ImGui::Text("Asset Type: Scene Node (Spot Light)");
    ImGui::Text("Name: %s", node->name().data());

    bool modified = false;

    if (nodeTransform(node))
        modified = true;

    if (ImGui::CollapsingHeader("Emission##spotLight", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (ImGui::ColorEdit3("Color##spotLight", glm::value_ptr(spotLight.color), ImGuiColorEditFlags_DisplayRGB))
            modified = true;

        if (ImGui::DragFloat("Intensity##spotLight", &spotLight.intensity, 0.01f, 0.f, FLT_MAX, "%.2f", ImGuiSliderFlags_AlwaysClamp))
            modified = true;

        if (ImGui::DragFloat("Range##spotLight", &spotLight.range, 0.01f, 0.01f, FLT_MAX, "%.2f", ImGuiSliderFlags_AlwaysClamp))
            modified = true;

        if (ImGui::DragFloat("Inner Angle##pointLight", &spotLight.innerAngle, 0.01f, 1.f, spotLight.outerAngle, "%.2f", ImGuiSliderFlags_AlwaysClamp))
            modified = true;

        if (ImGui::DragFloat("Outer Angle##pointLight", &spotLight.outerAngle, 0.01f, spotLight.innerAngle, 179.f, "%.2f", ImGuiSliderFlags_AlwaysClamp))
            modified = true;
    }

    if (ImGui::CollapsingHeader("Shadows##spotLight", ImGuiTreeNodeFlags_DefaultOpen))
    {
        bool modifiedShadowData = false;
        bool modifiedShadowMapResolution = false;

        SpotShadowData& options = mRenderer.getSpotShadowData(node->id());
        SpotShadowMap& shadowMap = mRenderer.getSpotShadowMap(node->id());

        const char* shadowTypePreview = toStr(options.shadowType);
        if (ImGui::BeginCombo("Shadow Type##spotLight", shadowTypePreview))
        {
            if (ImGui::Selectable("No Shadow", options.shadowType == ShadowType::NoShadow))
            {
                options.shadowType = ShadowType::NoShadow;
                modifiedShadowData = true;
            }

            if (ImGui::Selectable("Hard Shadow", options.shadowType == ShadowType::HardShadow))
            {
                options.shadowType = ShadowType::HardShadow;
                modifiedShadowData = true;
            }

            if (ImGui::Selectable("Soft Shadow", options.shadowType == ShadowType::SoftShadow))
            {
                options.shadowType = ShadowType::SoftShadow;
                modifiedShadowData = true;
            }

            ImGui::EndCombo();
        }

        if (options.shadowType != ShadowType::NoShadow)
        {
            std::string resolution = std::format("{}px", std::to_string(options.resolution));
            if (ImGui::BeginCombo("Resolution##spotLight", resolution.data()))
            {
                for (uint32_t i = 256; i <= 4096; i *= 2)
                {
                    std::string res = std::format("{}px", std::to_string(i));
                    if (ImGui::Selectable(res.data(), i == options.resolution) &&
                        i != options.resolution)
                    {
                        options.resolution = i;
                        modifiedShadowData = true;
                        modifiedShadowMapResolution = true;
                    }
                }

                ImGui::EndCombo();
            }

            if (ImGui::SliderFloat("Strength##spotLight", &options.strength, 0.f, 1.f))
                modifiedShadowData = true;

            if (ImGui::DragFloat("Bias Slope##spotLight", &options.biasSlope, 0.000001f, 0.f, FLT_MAX, "%.6f", ImGuiSliderFlags_AlwaysClamp))
                modifiedShadowData = true;

            if (ImGui::DragFloat("Bias Constant##spotLight", &options.biasConstant, 0.000001f, 0.f, FLT_MAX, "%.6f", ImGuiSliderFlags_AlwaysClamp))
                modifiedShadowData = true;

            if (options.shadowType == ShadowType::SoftShadow)
            {
                if (ImGui::SliderInt("PCF Range (Smoothing)##spotLight", &options.pcfRange, 1, 10))
                    modifiedShadowData = true;
            }
        }
        
        if (modifiedShadowData) mRenderer.updateSpotShadowMapData(node->id());
        if (modifiedShadowMapResolution) mRenderer.updateSpotShadowMapImage(node->id());
    }

    if (modified) mRenderer.updateSpotLight(node->id());
}

bool Editor::nodeTransform(GraphNode *node)
{
    bool modified = false;

    if (ImGui::CollapsingHeader("Transformation", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::SeparatorText("Translation");

        if (ImGui::DragFloat("X##Translation", &node->localT.x, 0.001f)) modified = true;
        if (ImGui::DragFloat("Y##Translation", &node->localT.y, 0.001f)) modified = true;
        if (ImGui::DragFloat("Z##Translation", &node->localT.z, 0.001f)) modified = true;

        ImGui::SeparatorText("Rotation");

        if (ImGui::DragFloat("X##Rotation", &node->localR.x, 0.1f, 0.f, 0.f, "%.1f")) modified = true;
        if (ImGui::DragFloat("Y##Rotation", &node->localR.y, 0.1f, 0.f, 0.f, "%.1f")) modified = true;
        if (ImGui::DragFloat("Z##Rotation", &node->localR.z, 0.1f, 0.f, 0.f, "%.1f")) modified = true;

        ImGui::SeparatorText("Scale");

        if (ImGui::DragFloat("X##Scale", &node->localS.x, 0.001f, 0.001f, 0.f, "%.3f", ImGuiSliderFlags_AlwaysClamp)) modified = true;
        if (ImGui::DragFloat("Y##Scale", &node->localS.y, 0.001f, 0.001f, 0.f, "%.3f", ImGuiSliderFlags_AlwaysClamp)) modified = true;
        if (ImGui::DragFloat("Z##Scale", &node->localS.z, 0.001f, 0.001f, 0.f, "%.3f", ImGuiSliderFlags_AlwaysClamp)) modified = true;
    }

    if (modified) node->markDirty();

    return modified;
}

void Editor::deleteSelectedObject()
{
    if (mSelectedObjectID)
    {
        if (isModelSelected())
            deleteSelectedModel();

        if (isNodeSelected())
            deleteSelectedNode();
    }
}

void Editor::deleteSelectedNode()
{
    GraphNode* node = mSceneGraph->searchNode(mSelectedObjectID);
    deleteNode(node);
}

void Editor::deleteSelectedModel()
{
    std::vector<GraphNode*> nodes(1, mSceneGraph->root());

    while (!nodes.empty())
    {
        GraphNode* node = nodes.back();
        nodes.pop_back();

        if (auto modelID = node->modelID())
            if (*modelID == mSelectedObjectID)
                deleteNode(node);

        for (auto child : node->children())
            nodes.push_back(child);
    }

    mRenderer.mModels.erase(mSelectedObjectID);
    mSelectedObjectID = 0;
}

void Editor::deleteNode(GraphNode *node)
{
    if (mSelectedObjectID == mCopiedNodeID)
    {
        mCopyFlag = CopyFlags::None;
        mCopiedNodeID = 0;
    }

    if (node->id() == mSelectedObjectID)
        mSelectedObjectID = 0;

    if (node->type() == NodeType::DirectionalLight) mRenderer.deleteDirLight(node->id());
    if (node->type() == NodeType::PointLight) mRenderer.deletePointLight(node->id());
    if (node->type() == NodeType::SpotLight) mRenderer.deleteSpotLight(node->id());

    if (node->type() == NodeType::Mesh)
    {
        for (uuid32_t meshID : node->meshIDs())
        {
            Message msg = Message::RemoveMeshInstance(meshID, node->id());
            SNS::publishMessage(Topic::Type::SceneGraph, msg);
        }
    }

    for (size_t i = node->children().size(); i > 0; --i)
        deleteNode(node->children().at(i - 1));

    node->orphan();
    delete node;
}

void Editor::sceneNodeDragDropSource(GraphNode *node)
{
    if (ImGui::BeginDragDropSource())
    {
        ImGui::SetDragDropPayload("SceneNode", &node, sizeof(GraphNode*));
        ImGui::Text(std::format("{} (Scene Node)", node->name()).c_str());
        ImGui::EndDragDropSource();
    }
}

void Editor::sceneNodeDragDropTarget(GraphNode *node)
{
    if (ImGui::BeginDragDropTarget())
    {
        checkPayloadType("SceneNode");

        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SceneNode"))
        {
            GraphNode* transferNode = *(GraphNode**)payload->Data;

            if (!mSceneGraph->hasDescendant(transferNode, node))
            {
                glm::mat4 parentInverseMat = glm::inverse(node->globalTransform());
                glm::mat4 childCoordSystem = parentInverseMat * transferNode->globalTransform();

                ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(childCoordSystem),
                                                      glm::value_ptr(transferNode->localT),
                                                      glm::value_ptr(transferNode->localR),
                                                      glm::value_ptr(transferNode->localS));

                transferNode->orphan();
                transferNode->markDirty();

                node->addChild(transferNode);
            }
        }

        ImGui::EndDragDropTarget();
    }
}

void Editor::sceneNodeDragDropTargetWholeWindow(bool nodeHovered)
{
    if (nodeHovered)
        return;

    ImRect rect = ImGui::GetCurrentWindow()->InnerRect;

    if (ImGui::BeginDragDropTargetCustom(rect, ImGui::GetID("NodeDragDropTargetWin")))
    {
        checkPayloadType("SceneNode");

        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SceneNode", ImGuiDragDropFlags_AcceptNoDrawDefaultRect))
        {
            if (payload->IsDelivery())
            {
                GraphNode* transferNode = *(GraphNode**)payload->Data;

                ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(transferNode->globalTransform()),
                                                      glm::value_ptr(transferNode->localT),
                                                      glm::value_ptr(transferNode->localR),
                                                      glm::value_ptr(transferNode->localS));

                transferNode->orphan();
                transferNode->markDirty();

                mSceneGraph->addNode(transferNode);
            }
        }

        ImGui::EndDragDropTarget();
    }
}

void Editor::modelDragDropSource()
{
    if (ImGui::BeginDragDropSource())
    {
        ImGui::SetDragDropPayload("Model", &mSelectedObjectID, sizeof(uuid32_t));
        ImGui::Text("%s (Model)", mRenderer.mModels.at(mSelectedObjectID)->name.c_str());
        ImGui::EndDragDropSource();
    }
}

void Editor::modelDragDropTargetWholeWindow()
{
    ImRect rect = ImGui::GetCurrentWindow()->InnerRect;

    if (ImGui::BeginDragDropTargetCustom(rect, ImGui::GetID("modelDragDropTargetWholeWindow")))
    {
        checkPayloadType("Model");

        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Model", ImGuiDragDropFlags_AcceptNoDrawDefaultRect))
        {
            if (payload->IsDelivery())
            {
                uuid32_t modelID = *(uuid32_t*)payload->Data;
                Model& model = *mRenderer.mModels.at(modelID);
                createModelGraph(model);
            }
        }

        ImGui::EndDragDropTarget();
    }
}

void Editor::sharedPopups()
{
    static auto openPopup = [] (bool& open, const char* func) {
        if (open)
        {
            ImGui::OpenPopup(func);
            open = false;
        }
    };

    openPopup(mModelImportPopup, "modelImportPopup");

    modelImportPopup();
}

void Editor::assetPanelPopup()
{
    if (ImGui::BeginPopup("assetPanelPopup"))
    {
        if (ImGui::MenuItem("Import Model##assetPanel"))
            mModelImportPopup = true;

        ImGui::Separator();

        bool modelSelected = isModelSelected();

        if (ImGui::MenuItem("Delete##assetPanel", nullptr, false, modelSelected))
            deleteSelectedModel();

        ImGui::MenuItem("Rename##assetPanel", nullptr, false, modelSelected);
        if (ImGui::IsItemClicked())
        {
            ImGui::OpenPopup("RenameDialog##assetPanel");
            resetBuffer();
        }

        if (ImGui::BeginPopup("RenameDialog##assetPanel"))
        {
            std::optional<std::string> newName = renameDialog();

            ImGui::EndPopup();

            if (newName)
            {
                Model& model = *mRenderer.mModels.at(mSelectedObjectID);
                model.name = newName.value();
                ImGui::CloseCurrentPopup();
            }
        }

        ImGui::EndPopup();
    }
}

void Editor::sceneGraphPopup()
{
    if (ImGui::BeginPopup("sceneGraphPopup"))
    {
        bool nodeSelected = isNodeSelected();

        if (ImGui::MenuItem("Cut##sceneGraph", nullptr, false, nodeSelected))
            cutNode(mSelectedObjectID);

        if (ImGui::MenuItem("Copy##sceneGraph", nullptr, false, nodeSelected))
            copyNode(mSelectedObjectID);

        if (ImGui::MenuItem("Paste##sceneGraph", nullptr, false, mCopyFlag != CopyFlags::None))
            pasteNode(mSceneGraph->root());

        bool enablePasteAsChild = (mCopyFlag != CopyFlags::None) && nodeSelected && (mSelectedObjectID != mCopiedNodeID);
        if (ImGui::MenuItem("Paste as Child##sceneGraph", nullptr, false, enablePasteAsChild))
            pasteNode(mSceneGraph->searchNode(mSelectedObjectID));

        ImGui::Separator();

        ImGui::MenuItem("Rename##sceneGraph", nullptr, false, nodeSelected);
        if (ImGui::IsItemClicked())
        {
            ImGui::OpenPopup("RenameDialog##sceneGraph");
            resetBuffer();
        }

        if (ImGui::MenuItem("Duplicate##sceneGraph", nullptr, false, nodeSelected))
            duplicateNode(mSceneGraph->searchNode(mSelectedObjectID));

        if (ImGui::MenuItem("Delete##sceneGraph", nullptr, false, nodeSelected))
            deleteSelectedNode();

        ImGui::Separator();

        if (ImGui::MenuItem("Create Empty##sceneGraph"))
            mSceneGraph->addNode(createEmptyNode(nullptr, "Empty Node"));

        ImGui::MenuItem("Create from Model##sceneGraph", nullptr, false, !mRenderer.mModels.empty());
        if (ImGui::IsItemClicked())
            ImGui::OpenPopup("SelectModel##sceneGraph");

        if (ImGui::BeginMenu("Create Light"))
        {
            if (ImGui::MenuItem("Directional Light##sceneGraph"))
                addDirLight();

            if (ImGui::MenuItem("Point Light##sceneGraph"))
                addPointLight();

            if (ImGui::MenuItem("Spot Light##sceneGraph"))
                addSpotLight();

            ImGui::EndMenu();
        }

        if (ImGui::BeginPopup("RenameDialog##sceneGraph"))
        {
            std::optional<std::string> newName = renameDialog();

            ImGui::EndPopup();

            if (newName)
            {
                GraphNode& node = *mSceneGraph->searchNode(mSelectedObjectID);
                node.setName(newName.value());
                ImGui::CloseCurrentPopup();
            }
        }

        if (ImGui::BeginPopup("SelectModel##sceneGraph"))
        {
            std::optional<uuid32_t> modelID = selectModel();

            ImGui::EndPopup();

            if (modelID.has_value())
            {
                auto& model = *mRenderer.mModels.at(*modelID);
                createModelGraph(model);
                ImGui::CloseCurrentPopup();
            }
        }

        ImGui::EndPopup();
    }
}

//void Editor::skyboxImportPopup()
//{
//    static ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar |
//        ImGuiWindowFlags_NoSavedSettings |
//        ImGuiWindowFlags_NoScrollbar;
//
//    static const char* preview = "...";
//    static std::array<std::string, 6> paths;
//
//    static auto loadPath = [] (const char* buttonLabel, std::string& outPath) {
//        if (ImGui::Button(buttonLabel))
//        {
//            static const char* filter = "Image files *.png *.jpeg *.jpg *.bmp\0*.jpeg;*.png;*.jpg;*.bmp;\0";
//            std::filesystem::path path = fileDialog("Select Texture", filter);
//            if (!path.empty()) outPath = path.string();
//        }
//    };
//
//    static auto displayPath = [] (std::string& path) {
//        ImGui::SameLine();
//        ImGui::BeginDisabled();
//
//        float textBoxLen = ImGui::GetContentRegionAvail().x;
//        if (!path.empty()) textBoxLen = std::min(textBoxLen, ImGui::CalcTextSize(path.data()).x + 8);
//
//        ImGui::PushItemWidth(textBoxLen);
//
//        if (path.empty()) ImGui::InputText("##skybox-path", const_cast<char*>(preview), 3);
//        else ImGui::InputText("##skybox-path", path.data(), path.size());
//
//        ImGui::PopItemWidth();
//        ImGui::EndDisabled();
//    };
//
//    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
//
//    ImGui::SetNextWindowBgAlpha(1.0f);
//    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
//    ImGui::SetNextWindowSize(ImVec2(500, 300), ImGuiCond_Appearing);
//    ImGui::SetNextWindowSizeConstraints(ImVec2(250, 250), ImVec2(FLT_MAX, FLT_MAX));
//
//    if (ImGui::BeginPopupModal("skyboxImportPopup", nullptr, windowFlags))
//    {
//        ImGui::PushStyleVar(ImGuiStyleVar_SeparatorTextAlign, ImVec2(0.5f, 0.5f));
//        ImGui::SeparatorText("Import Skybox Textures");
//        ImGui::PopStyleVar();
//        ImGui::Separator();
//
//        loadPath("Add X+", paths.at(0));
//        displayPath(paths.at(0));
//
//        loadPath("Add X-", paths.at(1));
//        displayPath(paths.at(1));
//
//        loadPath("Add Y+", paths.at(2));
//        displayPath(paths.at(2));
//
//        loadPath("Add Y-", paths.at(3));
//        displayPath(paths.at(3));
//
//        loadPath("Add Z+", paths.at(4));
//        displayPath(paths.at(4));
//
//        loadPath("Add Z-", paths.at(5));
//        displayPath(paths.at(5));
//
//        ImGui::Separator();
//        helpMarker("The coordinate system is right handed with the positive Y-axis pointing up.");
//
//        float padding = 10.0f;  // Padding from the edge
//        float buttonWidth = 80.0f;
//        float spacing = 10.0f;  // Space between buttons
//        float totalWidth = (buttonWidth * 2) + spacing;
//
//        // Align to bottom-right
//        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - totalWidth - padding);
//        ImGui::SetCursorPosY(ImGui::GetWindowHeight() - ImGui::GetFrameHeight() - padding);
//
//        if (ImGui::Button("Cancel", ImVec2(buttonWidth, 0)))
//        {
//            paths.fill({});
//            ImGui::CloseCurrentPopup();
//        }
//
//        ImGui::SameLine();
//
//        ImGui::BeginDisabled(std::any_of(paths.begin(), paths.end(), [] (const auto& s) {return s.empty();}));
//        if (ImGui::Button("OK", ImVec2(buttonWidth, 0)))
//        {
//            mRenderer.importSkybox(paths);
//            paths.fill({});
//            ImGui::CloseCurrentPopup();
//        }
//        ImGui::EndDisabled();
//
//        ImGui::EndPopup();
//    }
//}

void Editor::modelImportPopup()
{
    static ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoScrollbar;

    static const char* preview = "...";
    static ModelImportData importData {
        .normalize = false,
        .flipUVs = true,
    };

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();

    ImGui::SetNextWindowBgAlpha(1.0f);
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(500, 300), ImGuiCond_Appearing);
    ImGui::SetNextWindowSizeConstraints(ImVec2(250, 180), ImVec2(FLT_MAX, FLT_MAX));

    if (ImGui::BeginPopupModal("modelImportPopup", nullptr, windowFlags))
    {
        ImGui::PushStyleVar(ImGuiStyleVar_SeparatorTextAlign, ImVec2(0.5f, 0.5f));
        ImGui::SeparatorText("Import Settings");
        ImGui::PopStyleVar();

        ImGui::Separator();

        if (ImGui::Button("Select"))
        {
            auto path = selectModelFileDialog();
            if (!path.empty()) importData.path = path.string();
        }

        ImGui::SameLine();
        ImGui::BeginDisabled();

        float textboxLen = ImGui::GetContentRegionAvail().x;
        if (!importData.path.empty())textboxLen = std::min(textboxLen, ImGui::CalcTextSize(importData.path.data()).x + 8);

        ImGui::PushItemWidth(textboxLen);

        if (importData.path.empty()) ImGui::InputText("##Model-path", const_cast<char*>(preview), 3);
        else ImGui::InputText("##Model-path", importData.path.data(), importData.path.size());

        ImGui::PopItemWidth();
        ImGui::EndDisabled();

        ImGui::Separator();
        ImGui::Checkbox("Normalize to unit scale", &importData.normalize);
        ImGui::SameLine();
        helpMarker("Scales the model so its coordinates fit within unit space.");

        ImGui::Checkbox("Flip texture coordinates", &importData.flipUVs);
        ImGui::SameLine();
        helpMarker("Flips Y texture coordinate on load.");

        ImGui::Separator();

        float padding = 10.0f;
        float buttonWidth = 80.0f;
        float spacing = 10.0f;
        float totalWidth = (buttonWidth * 2) + spacing;

        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - totalWidth - padding);
        ImGui::SetCursorPosY(ImGui::GetWindowHeight() - ImGui::GetFrameHeight() - padding);

        if (ImGui::Button("Cancel", ImVec2(buttonWidth, 0)))
        {
            importData = {.flipUVs = true};
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        ImGui::BeginDisabled(importData.path.empty());
        if (ImGui::Button("OK", ImVec2(buttonWidth, 0)))
        {
            importModel(importData);
            importData = {.flipUVs = true};
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndDisabled();

        ImGui::EndPopup();
    }
}

bool Editor::transformGizmo()
{
    if (!isNodeSelected())
        return false;

    ImGuiWindow* win = ImGui::GetCurrentWindow();
    ImVec2 windowPos = ImGui::GetWindowPos();
    ImVec2 windowSize = ImGui::GetWindowSize();
    float titleBarHeight = win->TitleBarHeight();

    GraphNode* node = mSceneGraph->searchNode(mSelectedObjectID);

    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetGizmoSizeClipSpace(0.2f);
    ImGuizmo::SetDrawlist();

    ImGuizmo::SetRect(windowPos.x,
                      windowPos.y + titleBarHeight,
                      windowSize.x,
                      windowSize.y - titleBarHeight);

    glm::mat4 globalMatrix = node->globalTransform();

    glm::mat4 proj = mRenderer.mCamera.projection();
    proj[1][1] *= -1.f;
    if (ImGuizmo::Manipulate(glm::value_ptr(mRenderer.mCamera.view()),
                             glm::value_ptr(proj),
                             mGizmoOp, mGizmoMode,
                             glm::value_ptr(globalMatrix)))
    {
        glm::mat4 parentGlobal = node->parent()->globalTransform();
        glm::mat4 local = glm::inverse(parentGlobal) * globalMatrix;

        glm::vec3 translation;
        glm::vec3 rotation;
        glm::vec3 scale;

        ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(local),
                                              glm::value_ptr(translation),
                                              glm::value_ptr(rotation),
                                              glm::value_ptr(scale));

        switch (mGizmoOp)
        {
            case ImGuizmo::TRANSLATE: node->localT = translation; break;
            case ImGuizmo::ROTATE: node->localR = rotation; break;
            case ImGuizmo::SCALE: node->localS = scale; break;
            default: assert(false);
        }

        node->markDirty();
    }

    return ImGuizmo::IsUsing();
}

void Editor::drawAxisGizmo()
{
    ImGuiWindow* win = ImGui::GetCurrentWindow();
    ImVec2 windowCoords = ImGui::GetWindowPos() + ImVec2(0, win->TitleBarHeight());

    const float gizmoHalfSize = 50.f;
    const float padding = 15.f;
    float gizmoLeft = windowCoords.x + mViewportSize.x - gizmoHalfSize * 2 - padding;
    float gizmoTop = windowCoords.y + gizmoHalfSize + padding;
    ImOGuizmo::SetRect(gizmoLeft, gizmoTop, gizmoHalfSize);

    glm::mat4 view = mRenderer.mCamera.view();
    static glm::mat4 proj = glm::perspective(glm::radians(30.f), 1.5f, 0.01f, 100.f);
    ImOGuizmo::DrawGizmo(glm::value_ptr(view), glm::value_ptr(proj));
}

void Editor::gizmoOptions()
{
    float titleBarHeight = ImGui::GetCurrentWindow()->TitleBarHeight();

    static constexpr ImVec2 sIconSize(40.f, 40.f);
    static constexpr ImVec2 sWindowPadding(6.f, 6.f);
    static constexpr float sIconVertSpacing = 4.f;

    // gizmo op
    ImVec2 transformIconWindowPos(10.f, 10.f + titleBarHeight);
    ImVec2 transformIconWindowSize {
        sIconSize.x + sWindowPadding.x * 2.f,
        sIconSize.y * 3.f + sWindowPadding.y * 2.f + 2.f
    };

    ImGui::SetCursorPos(transformIconWindowPos);
    ImGui::SetNextWindowBgAlpha(1.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, sWindowPadding);
    ImGui::BeginChild("Gizmo Op", transformIconWindowSize, true, ImGuiWindowFlags_NoDecoration);
    ImGui::PopStyleVar(2);

    gizmoOpIcon(reinterpret_cast<ImTextureID>(mRenderer.mTranslateIconDs),
                ImGuizmo::OPERATION::TRANSLATE,
                "Translation");

    gizmoOpIcon(reinterpret_cast<ImTextureID>(mRenderer.mRotateIconDs),
                ImGuizmo::OPERATION::ROTATE,
                "Rotate");

    gizmoOpIcon(reinterpret_cast<ImTextureID>(mRenderer.mScaleIconDs),
                ImGuizmo::OPERATION::SCALE,
                "Scale");

    ImGui::EndChild();

    // Gizmo mode
    ImVec2 modeWindowPos(10.f, 10.f + titleBarHeight + transformIconWindowSize.y + 10.f);
    ImVec2 modeWindowSize {
        transformIconWindowSize.x,
        sIconSize.y * 2.f + sWindowPadding.y + 6.f
    };

    ImGui::SetCursorPos(modeWindowPos);
    ImGui::SetNextWindowBgAlpha(1.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, sWindowPadding);
    ImGui::BeginChild("GizmoMode", modeWindowSize, true, ImGuiWindowFlags_NoDecoration);
    ImGui::PopStyleVar(2);

    gizmoModeIcon(reinterpret_cast<ImTextureID>(mRenderer.mGlobalIconDs), ImGuizmo::MODE::WORLD, "Global");
    gizmoModeIcon(reinterpret_cast<ImTextureID>(mRenderer.mLocalIconDs), ImGuizmo::MODE::LOCAL, "Local");

    ImGui::EndChild();
}

void Editor::gizmoOpIcon(ImTextureID iconDs, ImGuizmo::OPERATION op, const char *tooltip)
{
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.f);

    bool selected = mGizmoOp == op;

    if (selected)
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered]);

    if (ImGui::ImageButton(iconDs, ImVec2(32, 32)))
        mGizmoOp = op;

    ImGui::PopStyleVar();

    if (selected)
        ImGui::PopStyleColor();

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_NoSharedDelay))
        ImGui::SetTooltip(tooltip);
}

void Editor::gizmoModeIcon(ImTextureID iconDs, ImGuizmo::MODE mode, const char *tooltip)
{
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.f);

    bool selected = mGizmoMode == mode;

    if (selected)
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered]);

    if (ImGui::ImageButton(iconDs, ImVec2(32, 32)))
        mGizmoMode = mode;

    ImGui::PopStyleVar();

    if (selected)
        ImGui::PopStyleColor();

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_NoSharedDelay))
        ImGui::SetTooltip(tooltip);
}

void Editor::keyboardShortcuts()
{
    if (ImGui::IsKeyPressed(ImGuiKey_V))
        mRenderer.mCamera.setState(Camera::State::VIEW_MODE);

    if (ImGui::IsKeyPressed(ImGuiKey_F))
        mRenderer.mCamera.setState(Camera::State::FIRST_PERSON);

    if (ImGui::IsKeyPressed(ImGuiKey_Tab))
        mRenderer.mCamera.setState(Camera::State::EDITOR_MODE);

//    if (ImGui::IsKeyPressed(ImGuiKey_W))
//        mGizmoOp = ImGuizmo::OPERATION::TRANSLATE;
//
//    if (ImGui::IsKeyPressed(ImGuiKey_E))
//        mGizmoOp = ImGuizmo::OPERATION::ROTATE;
//
//    if (ImGui::IsKeyPressed(ImGuiKey_R))
//        mGizmoOp = ImGuizmo::OPERATION::SCALE;
//
//    if (ImGui::IsKeyPressed(ImGuiKey_G))
//        mGizmoMode = ImGuizmo::MODE::WORLD;
//
//    if (ImGui::IsKeyPressed(ImGuiKey_L))
//        mGizmoMode = ImGuizmo::MODE::LOCAL;
}

void Editor::importModel(const ModelImportData &importData)
{
    mRenderer.importModel(importData);
}

std::filesystem::path Editor::selectModelFileDialog()
{
    return fileDialog("Select glTF Model", "glTF Files *.gltf *.glb\0*.gltf;*.glb;\0");
}

void Editor::checkPayloadType(const char *type)
{
    const ImGuiPayload* payload = ImGui::GetDragDropPayload();

    if (payload && strcmp(payload->DataType, type) != 0)
        ImGui::SetMouseCursor(ImGuiMouseCursor_NotAllowed);
}

void Editor::resetBuffer()
{
    mBuffer.fill('\0');
}

void Editor::helpMarker(const char* desc)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::BeginItemTooltip())
    {
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

void Editor::imguiImageFitted(ImTextureID textureId, float imageWidth, float imageHeight)
{
    float width = ImGui::GetContentRegionAvail().x;

    float scale = width / imageWidth;
    float scaledHeight = imageHeight * scale;

    ImGui::Image(textureId, ImVec2(width, scaledHeight));
}

GraphNode *Editor::copyGraphNode(GraphNode *node)
{
    GraphNode* newNode;

    switch (node->type())
    {
        case NodeType::Empty:
        {
            newNode = new GraphNode(node->type(),
                                    node->name(),
                                    node->localT,
                                    node->localR,
                                    node->localS,
                                    node->parent(),
                                    node->modelID());
            break;
        }
        case NodeType::DirectionalLight:
        {
            newNode = new GraphNode(node->type(),
                                    node->name(),
                                    node->localT,
                                    node->localR,
                                    node->localS,
                                    node->parent());
            mRenderer.addDirLight(newNode->id(), mRenderer.getDirLight(node->id()));
            break;
        }
        case NodeType::SpotLight:
        {
            newNode = new GraphNode(node->type(),
                                    node->name(),
                                    node->localT,
                                    node->localR,
                                    node->localS,
                                    node->parent());
            mRenderer.addSpotLight(newNode->id(), mRenderer.getSpotLight(node->id()));
            break;
        }
        case NodeType::PointLight:
        {
            newNode = new GraphNode(node->type(),
                                    node->name(),
                                    node->localT,
                                    node->localR,
                                    node->localS,
                                    node->parent());
            mRenderer.addPointLight(newNode->id(), mRenderer.getPointLight(node->id()));
            break;
        }
        case NodeType::Mesh:
        {
            Model& model = *mRenderer.mModels.at(*node->modelID());

            newNode = new GraphNode(node->type(),
                                    node->name(),
                                    node->localT,
                                    node->localR,
                                    node->localS,
                                    node->parent(),
                                    *node->modelID(),
                                    node->meshIDs());

            for (uuid32_t meshID : node->meshIDs())
                model.getMesh(meshID)->mesh.addInstance(newNode->id());

            break;
        }
        default: assert(false);
    }

    // recursive case
    for (auto child : node->children())
        newNode->addChild(copyGraphNode(child));

    return newNode;
}

std::optional<std::string> Editor::renameDialog()
{
    std::optional<std::string> str;

    ImGui::SeparatorText("Enter Name:");

    ImGui::InputText("##renameDialogTextInput", mBuffer.data(), mBuffer.size());

    ImGui::BeginDisabled(mBuffer[0] == '\0');
    if (ImGui::Button("Submit##renameDialog") || (mBuffer[0] != '\0' && ImGui::IsKeyPressed(ImGuiKey_Enter)))
    {
        str = mBuffer.data();
        resetBuffer();
    }
    ImGui::EndDisabled();

    return str;
}

std::optional<uuid32_t> Editor::selectModel()
{
    ImGui::SeparatorText("Select Model:");

    for (const auto& [modelID, model] : mRenderer.mModels)
    {
        if (ImGui::MenuItem(model->name.c_str()))
            return modelID;
    }

    return std::nullopt;
}

glm::vec3 Editor::spawnPos()
{
    return mRenderer.mCamera.position() + mRenderer.mCamera.front() * 10.f;
}

bool Editor::lastItemClicked(uuid32_t id)
{
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left) || ImGui::IsItemClicked(ImGuiMouseButton_Right))
    {
        mSelectedObjectID = id;
        return true;
    }

    return false;
}

bool Editor::isNodeSelected()
{
    return UUIDRegistry::isSceneNode(mSelectedObjectID);
}

bool Editor::isModelSelected()
{
    return UUIDRegistry::isModel(mSelectedObjectID);
}

bool Editor::emptySpaceClicked()
{
    return ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup) &&
           !ImGui::IsAnyItemHovered() &&
           ImGui::IsMouseClicked(ImGuiMouseButton_Left);
}

void Editor::deselectAll()
{
    mSelectedObjectID = 0;
}

void Editor::countFPS()
{
    static float frameCount = 0;
    static float accumulatedTime = 0.f;

    ++frameCount;
    accumulatedTime += mDt;

    if (accumulatedTime >= 1.f)
    {
        mFPS = frameCount;

        frameCount = 0;
        accumulatedTime = 0.f;
    }
}

void Editor::updateFrametimeStats()
{
    static float accumulatedTime = 0.f;

    accumulatedTime += mDt;

    if (accumulatedTime > 0.3f)
    {
        mFrametimeMs = mDt * 1000.f;
        accumulatedTime = 0.f;
    }
}

void Editor::plotPerformanceGraphs()
{
    static constexpr uint32_t maxSamples = 200;
    static std::vector<float> fpsValues(maxSamples, 0.f);
    static std::vector<float> dtValues(maxSamples, 0.f);

    float dt = mDt * 1000.0f;
    float fps = 1.0f / mDt;

    if (dt > 33.f)
        debugLog(std::format("Large dt: {}ms", dt));

    if (dtValues.size() >= maxSamples)
        dtValues.erase(dtValues.begin());

    if (fpsValues.size() >= maxSamples)
        fpsValues.erase(fpsValues.begin());

    dtValues.push_back(dt);
    fpsValues.push_back(fps);

    ImGui::Text("FPS: %lu", mFPS);
    ImGui::SetNextItemWidth(-1);
    ImGui::PlotLines("##FPS", fpsValues.data(), fpsValues.size(), 0, nullptr, 0, 1000, ImVec2(0, 50));
    ImGui::Separator();

    ImGui::Text("Frame time: %.2f ms", mFrametimeMs);
    ImGui::SetNextItemWidth(-1);
    ImGui::PlotLines("##Frametime", dtValues.data(), dtValues.size(),  0, nullptr, 0, 100, ImVec2(0, 50));
}
