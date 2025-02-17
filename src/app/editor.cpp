//
// Created by Gianni on 4/02/2025.
//

#include "editor.hpp"

Editor::Editor(Renderer& renderer, SaveData& saveData)
    : mRenderer(renderer)
    , mSaveData(saveData)
    , mSelectedObjectID()
    , mShowViewport(true)
    , mShowAssetPanel(true)
    , mShowSceneGraph(true)
    , mShowCameraPanel(true)
    , mShowInspectorPanel(true)
    , mShowRendererPanel(true)
    , mShowDebugPanel(true)
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

    imguiEvents();
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

    if (mShowViewport)
        viewPort();

    if (mShowDebugPanel)
        debugPanel();
}

void Editor::imguiEvents()
{
    if (ImGui::IsKeyPressed(ImGuiKey_Delete))
        deleteSelectedObject();
}

void Editor::mainMenuBar()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Import glTF"))
                importModel("gltf");

            if (ImGui::MenuItem("Import glb"))
                importModel("glb");

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
            ImGui::MenuItem("Debug", nullptr, &mShowDebugPanel);

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

void Editor::cameraPanel()
{
    static constexpr ImGuiSliderFlags sliderFlags = ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_NoInput;

    ImGui::Begin("Camera", &mShowCameraPanel);

    ImGui::SeparatorText("Settings");
    ImGui::SliderFloat("Field of View", mRenderer.mCamera.fov(), 1.f, 177.f, "%0.f");

    ImGui::DragFloat("Near Plane",
                     mRenderer.mCamera.nearPlane(),
                     0.01f, 0.001f, *mRenderer.mCamera.farPlane(),
                     "%.3f", sliderFlags);

    ImGui::DragFloat("Far Plane",
                     mRenderer.mCamera.farPlane(),
                     0.1f, *mRenderer.mCamera.nearPlane(), 10000.f,
                     "%.2f", sliderFlags);

    ImGui::SeparatorText("Camera Mode");

    Camera::State cameraState = mRenderer.mCamera.state();

    if (ImGui::RadioButton("View", cameraState == Camera::State::VIEW_MODE))
        mRenderer.mCamera.setState(Camera::State::VIEW_MODE);

    ImGui::SameLine();
    if (ImGui::RadioButton("Edit", cameraState == Camera::State::EDITOR_MODE))
        mRenderer.mCamera.setState(Camera::State::EDITOR_MODE);

    ImGui::SameLine();
    if (ImGui::RadioButton("First Person", cameraState == Camera::State::FIRST_PERSON))
        mRenderer.mCamera.setState(Camera::State::FIRST_PERSON);

    ImGui::Separator();

    cameraState = mRenderer.mCamera.state();

    ImGui::DragFloat("Rotate Sensitivity", mRenderer.mCamera.rotateSensitivity(), 0.1f, -FLT_MAX, FLT_MAX, "%.0f");

    if (cameraState == Camera::State::VIEW_MODE || cameraState == Camera::State::EDITOR_MODE)
        ImGui::DragFloat("Z Scroll Offset", mRenderer.mCamera.zScrollOffset(), 0.01f, 0.f, FLT_MAX, "%.2f");

    if (cameraState == Camera::State::VIEW_MODE)
        ImGui::DragFloat("Pan Speed", mRenderer.mCamera.panSpeed(), 0.01f, 0.f, FLT_MAX, "%.2f");

    if (cameraState == Camera::State::FIRST_PERSON)
        ImGui::DragFloat("Fly Speed", mRenderer.mCamera.flySpeed(), 0.01f, 0.f, FLT_MAX, "%.1f");

    ImGui::End();
}

void Editor::inspectorPanel()
{
    ImGui::Begin("Inspector", &mShowInspectorPanel);

    if (auto objectType = UUIDRegistry::getObjectType(mSelectedObjectID))
    {
        switch (*objectType)
        {
            case ObjectType::Model: modelInspector(mSelectedObjectID); break;
            case ObjectType::Mesh: assert(false);
            case ObjectType::SceneNode: sceneNodeInspector(mSceneGraph.searchNode(mSelectedObjectID)); break;
            default: assert(false);
        }
    }

    ImGui::End();
}

void Editor::modelInspector(uuid32_t modelID)
{
    auto& model = *mRenderer.mModels.at(modelID);

    ImGui::Text("Asset Type: Model");
    ImGui::Text("Name: %s", model.name.c_str());

    ImGui::SeparatorText("Info");

    ImGui::Text("Mesh Count: %zu", model.meshes.size());
    ImGui::Text("Material Count: %zu", model.materials.size());
    ImGui::Text("Texture Count: %zu", model.textures.size());

    if (!model.materials.empty())
    {
        static std::unordered_map<uuid32_t, uint32_t> sSelectedMatIndex;
        if (!sSelectedMatIndex.contains(modelID))
            sSelectedMatIndex[modelID] = 0;

        ImGui::SeparatorText("Materials");
        ImGui::BeginChild("LeftMaterialPanel", ImVec2(150.f, 0.f), ImGuiChildFlags_Border | ImGuiChildFlags_ResizeX);

        for (uint32_t i = 0; i < model.materials.size(); ++i)
        {
            const std::string& matName = model.materialNames.at(i);

            bool selected = i == sSelectedMatIndex.at(modelID);

            if (ImGui::Selectable(matName.c_str(), selected))
                sSelectedMatIndex.at(modelID) = i;
        }

        ImGui::EndChild();

        ImGui::SameLine();
        ImGui::BeginChild("RightMaterialsPanel", ImVec2(), ImGuiChildFlags_Border);

        uint32_t selectedMaterialIndex = sSelectedMatIndex.at(modelID);

        Material& mat = model.materials.at(selectedMaterialIndex);
        std::string& name = model.materialNames.at(selectedMaterialIndex);
        bool matNeedsUpdate = false;

        ImGui::SeparatorText("Material Textures");

        if (mat.baseColorTexIndex != -1) matTexInspector("Base Color", model.textures.at(mat.baseColorTexIndex));
        if (mat.metallicRoughnessTexIndex != -1) matTexInspector("Metallic Roughness", model.textures.at(mat.metallicRoughnessTexIndex));
        if (mat.normalTexIndex != -1) matTexInspector("Normal", model.textures.at(mat.normalTexIndex));
        if (mat.aoTexIndex != -1) matTexInspector("AO", model.textures.at(mat.aoTexIndex));
        if (mat.emissionTexIndex != -1) matTexInspector("Emission", model.textures.at(mat.emissionTexIndex));

        ImGui::SeparatorText("Material Factors");

        static constexpr ImGuiColorEditFlags sColorEditFlags {
            ImGuiColorEditFlags_DisplayRGB |
            ImGuiColorEditFlags_AlphaBar
        };

        if (ImGui::ColorEdit4("Base Color", glm::value_ptr(mat.baseColorFactor), sColorEditFlags))
            matNeedsUpdate = true;

        if (ImGui::ColorEdit3("Emission", glm::value_ptr(mat.emissionFactor), sColorEditFlags))
            matNeedsUpdate = true;

        if (ImGui::SliderFloat("Metallic", &mat.metallicFactor, 0.f, 1.f))
            matNeedsUpdate = true;

        if (ImGui::SliderFloat("Roughness", &mat.roughnessFactor, 0.f, 1.f))
            matNeedsUpdate = true;

        if (ImGui::SliderFloat("AO", &mat.occlusionFactor, 0.f, 1.f))
            matNeedsUpdate = true;

        ImGui::SeparatorText("Texture Coordinates");

        if (ImGui::DragFloat2("Tiling", glm::value_ptr(mat.tiling), 0.01f))
            matNeedsUpdate = true;

        if (ImGui::DragFloat2("Offset", glm::value_ptr(mat.offset), 0.01f))
            matNeedsUpdate = true;

        ImGui::EndChild();

        if (matNeedsUpdate)
            model.updateMaterial(selectedMaterialIndex);
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
        case NodeType::DirectionalLight: nullptr; break;
        case NodeType::PointLight: nullptr; break;
        case NodeType::SpotLight: nullptr; break;
        default: assert(false);
    }
}

void Editor::emptyNodeInspector(GraphNode *node)
{
    ImGui::Text("Asset Type: Empty Node");
    ImGui::Text("Name: %s", node->name().c_str());

    nodeTransform(node);

    if (node->modelID().has_value())
    {
        ImGui::SeparatorText("Info");

        const auto& model = *mRenderer.mModels.at(*node->modelID());
        ImGui::Text("Associated Model: %s", model.name.c_str());
    }
}

void Editor::meshNodeInspector(GraphNode *node)
{
    MeshNode* meshNode = dynamic_cast<MeshNode*>(node);

    ImGui::Text("Asset Type: Mesh Node");
    ImGui::Text("Name: %s", node->name().c_str());

    nodeTransform(node);

    ImGui::SeparatorText("Info");

    auto& model = *mRenderer.mModels.at(meshNode->modelID().value());
    auto& mesh = model.getMesh(meshNode->meshID());

    ImGui::Text("Associated Model: %s", model.name.c_str());
    ImGui::Text("Mesh: %s", mesh.name.c_str());
    ImGui::Text("Material: %s", mesh.materialIndex == -1? "None" :
                            model.materialNames.at(mesh.materialIndex).c_str());
}

void Editor::nodeTransform(GraphNode *node)
{
    ImGui::SeparatorText("Transformation");

    glm::mat4 mat = node->globalTransform();

    float translation[3];
    float rotation[3];
    float scale[3];

    ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(mat), translation, rotation, scale);

    bool modified = false;

    if (ImGui::DragFloat3("Translation", translation, 0.001f))
        modified = true;

    if (ImGui::DragFloat3("Rotation", rotation, 0.001f))
        modified = true;

    if (ImGui::DragFloat3("Scale", scale, 0.001f))
        modified = true;

    if (modified)
    {
        ImGuizmo::RecomposeMatrixFromComponents(translation, rotation, scale, glm::value_ptr(mat));
        node->setLocalTransform(mat);
    }
}

void Editor::viewPort()
{
    static constexpr ImVec2 sInitialViewportSize {
        InitialViewportWidth,
        InitialViewportHeight
    };

    ImGui::SetNextWindowSize(sInitialViewportSize, ImGuiCond_FirstUseEver);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
    ImGui::Begin("Viewport", &mShowViewport, ImGuiWindowFlags_NoScrollbar);

    ImVec2 viewportSize = ImGui::GetContentRegionAvail();

    viewportSize.x = glm::clamp(viewportSize.x, 1.f, FLT_MAX);
    viewportSize.y = glm::clamp(viewportSize.y, 1.f, FLT_MAX);

    if (mViewportSize != viewportSize && !ImGui::IsMouseDown(ImGuiMouseButton_Left))
    {
        mViewportSize = viewportSize;
        mRenderer.resize(viewportSize.x, viewportSize.y);
    }

    mRenderer.mCamera.update(mDt);
    mRenderer.render();

    ImTextureID renderedImageHandle = reinterpret_cast<ImTextureID>(mRenderer.mPostProcessingDs);

    ImGui::Image(renderedImageHandle, viewportSize);

    modelDragDropTargetWholeWindow();

    ImGui::End();
    ImGui::PopStyleVar(2);
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
    if (mSelectedObjectID == mCopiedNodeID)
    {
        mCopyFlag = CopyFlags::None;
        mCopiedNodeID = 0;
    }

    mSceneGraph.deleteNode(mSelectedObjectID);
    mSelectedObjectID = 0;
}

void Editor::deleteSelectedModel()
{
    mRenderer.deleteModel(mSelectedObjectID);
    mSelectedObjectID = 0;
}

void Editor::rendererPanel()
{
    static constexpr ImGuiColorEditFlags colorEditFlags = ImGuiColorEditFlags_DisplayRGB |
        ImGuiColorEditFlags_AlphaBar;

    ImGui::Begin("Renderer", &mShowRendererPanel);

    if (ImGui::CollapsingHeader("Grid", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Render Grid", &mRenderer.mRenderGrid);
        ImGui::Separator();

        ImGui::ColorEdit4("Thin line color", glm::value_ptr(mRenderer.mGridData.thinLineColor), colorEditFlags);
        ImGui::ColorEdit4("Thick line color", glm::value_ptr(mRenderer.mGridData.thickLineColor), colorEditFlags);
        ImGui::SliderFloat("Cell size", &mRenderer.mGridData.cellSize, 0.001f, 0.25f);
        ImGui::SliderFloat("Min pixels between cells", &mRenderer.mGridData.minPixelsBetweenCells, 1.f, 5.f);
    }

    if (ImGui::CollapsingHeader("Skybox", ImGuiTreeNodeFlags_DefaultOpen))
    {
        static bool renderSkybox;
        ImGui::Checkbox("Render Skybox", &renderSkybox);
        ImGui::Separator();

        if (ImGui::Button("Import Faces"))
        {
            ImGui::OpenPopup("Import Skybox Textures");
        }

        ImGui::SameLine();

        if (ImGui::Button("Import Equirectangular"))
        {

        }
    }

    skyboxImportPopup();

    ImGui::End();
}

void Editor::skyboxImportPopup()
{
    static ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoResize;

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();

    ImGui::SetNextWindowBgAlpha(1.0f);
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(500, 300), ImGuiCond_Appearing);

    if (ImGui::BeginPopupModal("Import Skybox Textures", nullptr, windowFlags))
    {
        ImGui::PushStyleVar(ImGuiStyleVar_SeparatorTextAlign, ImVec2(0.5f, 0.5f));
        ImGui::SeparatorText("Import Skybox Textures");
        ImGui::PopStyleVar();
        ImGui::Separator();

        char c[3];
        c[0] = '.';
        c[1] = '.';
        c[2] = '.';

        ImGui::Button("Add X+");
        ImGui::SameLine();
        ImGui::BeginDisabled();
        ImGui::PushItemWidth(-FLT_MIN);
        ImGui::InputText("##X+input", c, 3);
        ImGui::PopItemWidth();
        ImGui::EndDisabled();

        ImGui::Button("Add X-");
        ImGui::SameLine();
        ImGui::BeginDisabled();
        ImGui::InputText("##X-input", c, 3);
        ImGui::EndDisabled();

        ImGui::Button("Add Y+");
        ImGui::SameLine();
        ImGui::BeginDisabled();
        ImGui::InputText("##Y+input", c, 3);
        ImGui::EndDisabled();

        ImGui::Button("Add Y-");
        ImGui::SameLine();
        ImGui::BeginDisabled();
        ImGui::InputText("##Y-input", c, 3);
        ImGui::EndDisabled();

        ImGui::Button("Add Z+");
        ImGui::SameLine();
        ImGui::BeginDisabled();
        ImGui::InputText("##Z+input", c, 3);
        ImGui::EndDisabled();

        ImGui::Button("Add Z-");
        ImGui::SameLine();
        ImGui::BeginDisabled();
        ImGui::InputText("##Z-input", c, 3);
        ImGui::EndDisabled();
        ImGui::Separator();

        float padding = 10.0f;  // Padding from the edge
        float buttonWidth = 80.0f;
        float spacing = 10.0f;  // Space between buttons
        float totalWidth = (buttonWidth * 2) + spacing;

        // Align to bottom-right
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - totalWidth - padding);
        ImGui::SetCursorPosY(ImGui::GetWindowHeight() - ImGui::GetFrameHeight() - padding);

        if (ImGui::Button("Cancel", ImVec2(buttonWidth, 0)))
            ImGui::CloseCurrentPopup();

        ImGui::SameLine();

        if (ImGui::Button("OK", ImVec2(buttonWidth, 0)))
        {
        }

        ImGui::EndPopup();
    }
}

void Editor::debugPanel()
{
    ImGui::Begin("Debug", &mShowDebugPanel);

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

    ImGui::SeparatorText("Renderpass times");
    ImGui::Text("Clear renderpass: %.3lfms", mRenderpassTimes.clearRenderpassDurMs);
    ImGui::Text("Prepass: %.3lfms", mRenderpassTimes.prepassRenderpassDurMs);
    ImGui::Text("Skybox renderpass: %.3lfms", mRenderpassTimes.skyboxRenderpassDurMs);
    ImGui::Text("SSAO renderpass: %.3lfms", mRenderpassTimes.ssaoRenderpassDurMs);
    ImGui::Text("Shading renderpass: %.3lfms", mRenderpassTimes.shadingRenderpassDurMs);
    ImGui::Text("Grid renderpass: %.3lfms", mRenderpassTimes.gridRenderpassDurMs);
    ImGui::Text("Temp color renderpass: %.3lfms", mRenderpassTimes.tempColorTransitionDurMs);
    ImGui::Text("Post processing renderpass: %.3lfms", mRenderpassTimes.postProcessingRenderpassDurMs);
    ImGui::Separator();

    ImGui::End();
}

void Editor::plotPerformanceGraphs()
{
    static constexpr uint32_t maxSamples = 500;
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

void Editor::sceneGraphPanel()
{
    ImGui::Begin("Scene Graph", &mShowSceneGraph);

    sceneGraphPopup();

    for (auto child : mSceneGraph.mRoot.children())
        sceneNodeRecursive(child);

    bool nodeHovered = ImGui::IsItemHovered();
    ImGui::Dummy(ImVec2(0, 50.f));
    sceneNodeDragDropTargetWholeWindow(&mSceneGraph.mRoot, nodeHovered);

    ImGui::End();

    mSceneGraph.updateTransforms();
}

void Editor::sceneGraphPopup()
{
    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        ImGui::OpenPopup("sceneGraphPopup");

    if (ImGui::BeginPopup("sceneGraphPopup"))
    {
        bool nodeSelected = isNodeSelected();

        if (ImGui::MenuItem("Cut##sceneGraph", nullptr, false, nodeSelected))
            cutNode(mSelectedObjectID);

        if (ImGui::MenuItem("Copy##sceneGraph", nullptr, false, nodeSelected))
            copyNode(mSelectedObjectID);

        if (ImGui::MenuItem("Paste##sceneGraph", nullptr, false, mCopyFlag != CopyFlags::None))
            pasteNode(&mSceneGraph.mRoot);

        bool enablePasteAsChild = (mCopyFlag != CopyFlags::None) && nodeSelected && (mSelectedObjectID != mCopiedNodeID);
        if (ImGui::MenuItem("Paste as Child##sceneGraph", nullptr, false, enablePasteAsChild))
            pasteNode(mSceneGraph.searchNode(mSelectedObjectID));

        ImGui::Separator();

        ImGui::MenuItem("Rename##sceneGraph", nullptr, false, nodeSelected);
        if (ImGui::IsItemClicked())
        {
            ImGui::OpenPopup("RenameDialog##sceneGraph");
            resetBuffer();
        }

        if (ImGui::MenuItem("Duplicate##sceneGraph", nullptr, false, nodeSelected))
            duplicateNode(mSceneGraph.searchNode(mSelectedObjectID));

        if (ImGui::MenuItem("Delete##sceneGraph", nullptr, false, nodeSelected))
            deleteSelectedNode();

        ImGui::Separator();

        if (ImGui::MenuItem("Create Empty##sceneGraph"))
            createEmptyNode();

        ImGui::MenuItem("Create from Model##sceneGraph", nullptr, false, !mRenderer.mModels.empty());
        if (ImGui::IsItemClicked())
            ImGui::OpenPopup("SelectModel##sceneGraph");

        if (ImGui::BeginMenu("Create Light"))
        {
            if (ImGui::MenuItem("Directional Light##sceneGraph"))
                nullptr;

            if (ImGui::MenuItem("Point Light##sceneGraph"))
                nullptr;

            if (ImGui::MenuItem("Spot Light##sceneGraph"))
                nullptr;

            ImGui::EndMenu();
        }

        if (ImGui::BeginPopup("RenameDialog##sceneGraph"))
        {
            std::optional<std::string> newName = renameDialog();

            ImGui::EndPopup();

            if (newName)
            {
                GraphNode& node = *mSceneGraph.searchNode(mSelectedObjectID);
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

static GraphNode* createModelGraphImpl(Model& model, const SceneNode& sceneNode, GraphNode* parent)
{
    GraphNode* graphNode;

    if (sceneNode.meshIndex != -1)
    {
        Mesh& mesh = model.meshes.at(sceneNode.meshIndex);

        uuid32_t meshID = mesh.meshID;

        graphNode = new MeshNode(NodeType::Mesh, sceneNode.name, sceneNode.transformation, parent, model.id, meshID);

        mesh.mesh.addInstance(graphNode->id());
    }
    else
    {
        graphNode = new GraphNode(NodeType::Empty, sceneNode.name, sceneNode.transformation, parent, model.id);
    }

    for (const auto& child : sceneNode.children)
        graphNode->addChild(createModelGraphImpl(model, child, graphNode));

    return graphNode;
}

void Editor::createModelGraph(Model &model)
{
    for (const auto& sceneNode : model.scenes)
    {
        GraphNode* graphNode = createModelGraphImpl(model, sceneNode, nullptr);
        mSceneGraph.addNode(graphNode);
    }
}

void Editor::createEmptyNode()
{
    mSceneGraph.addNode(new GraphNode(NodeType::Empty, "Empty Node", glm::identity<glm::mat4>(), nullptr));
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
    GraphNode* copiedNode = mSceneGraph.searchNode(mCopiedNodeID);

    if (copiedNode && !mSceneGraph.hasDescendant(copiedNode, parent))
    {
        if (mCopyFlag == CopyFlags::Copy)
        {
            GraphNode* newNode = copyGraphNode(copiedNode);
            newNode->orphan();
            parent->addChild(newNode);
        }

        if (mCopyFlag == CopyFlags::Cut)
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

void Editor::assetPanel()
{
    ImGui::Begin("Assets", &mShowAssetPanel);

    assetPanelPopup();

    for (const auto& [modelID, model] : mRenderer.mModels)
    {
        ImGui::Selectable(model->name.c_str(), mSelectedObjectID == modelID);
        lastItemClicked(modelID);
        modelDragDropSource(modelID);
    }

    ImGui::End();
}

void Editor::assetPanelPopup()
{
    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        ImGui::OpenPopup("assetPanelPopup");

    if (ImGui::BeginPopup("assetPanelPopup"))
    {
        if (ImGui::MenuItem("Import glTF##assetPanel"))
            importModel("gltf");

        if (ImGui::MenuItem("Import glb##assetPanel"))
            importModel("glb");

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

            if (!mSceneGraph.hasDescendant(transferNode, node))
            {
                transferNode->orphan();
                transferNode->markDirty();

                node->addChild(transferNode);
            }
        }

        ImGui::EndDragDropTarget();
    }
}

void Editor::sceneNodeDragDropTargetWholeWindow(GraphNode *node, bool nodeHovered)
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

                if (!mSceneGraph.hasDescendant(transferNode, node))
                {
                    transferNode->orphan();
                    transferNode->markDirty();

                    node->addChild(transferNode);
                }
            }
        }

        ImGui::EndDragDropTarget();
    }
}

void Editor::modelDragDropSource(uuid32_t modelID)
{
    if (ImGui::BeginDragDropSource())
    {
        ImGui::SetDragDropPayload("Model", &modelID, sizeof(uuid32_t));
        ImGui::Text("%s (Model)", mRenderer.mModels.at(modelID)->name.c_str());
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

void Editor::importModel(const char* type)
{
    std::filesystem::path path;

    if (!strcmp(type, "gltf"))
        path = fileDialog("Select glTF Model", "glTF Files *.gltf\0*.gltf\0\0");
    else
        path = fileDialog("Select glb Model", "glb Files *.glb\0*.glb\0\0");

    if (!path.empty())
    {
        mRenderer.importModel(path);
    }
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

GraphNode *Editor::copyGraphNode(GraphNode *node)
{
    GraphNode* newNode;

    switch (node->type())
    {
        case NodeType::Empty:
        {
            newNode = new GraphNode(node->type(),
                                    node->name(),
                                    node->localTransform(),
                                    node->parent(),
                                    node->modelID());
            break;
        }
        case NodeType::Mesh:
        {
            MeshNode* meshNode = dynamic_cast<MeshNode*>(node);

            uuid32_t modelID = meshNode->modelID().value();
            uint32_t meshID = meshNode->meshID();

            newNode = new MeshNode(meshNode->type(),
                                   meshNode->name(),
                                   meshNode->localTransform(),
                                   meshNode->parent(),
                                   modelID, meshID);

            mRenderer.mModels.at(modelID)->getMesh(meshID).mesh.addInstance(newNode->id());
            break;
        }
        default: assert(false);
    }

    // recursive case
    for (auto child : node->children())
        newNode->addChild(copyGraphNode(child));

    return newNode;
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
        mRenderpassTimes = mRenderer.mRenderpassTimes;
        accumulatedTime = 0.f;
    }
}
