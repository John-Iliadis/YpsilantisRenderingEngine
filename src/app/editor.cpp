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

    imguiEvents();
    mainMenuBar();

    if (mShowAssetPanel)
        assetPanel();

    if (mShowViewport)
        viewPort();

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

    sharedPopups();
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
            if (ImGui::MenuItem("Import Model"))
                mModelImportPopup = true;

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
    ImGui::Begin("Camera", &mShowCameraPanel);

    ImGui::SeparatorText("Settings");
    ImGui::SliderFloat("Field of View", mRenderer.mCamera.fov(), 1.f, 145.f, "%0.f");

    ImGui::DragFloat("Near Plane",
                     mRenderer.mCamera.nearPlane(),
                     0.01f, 0.001f, *mRenderer.mCamera.farPlane(),
                     "%.3f", ImGuiSliderFlags_AlwaysClamp);

    ImGui::DragFloat("Far Plane",
                     mRenderer.mCamera.farPlane(),
                     0.1f, *mRenderer.mCamera.nearPlane(), 10000.f,
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
    helpMarker("Press E to select mode\n"
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
                    const std::string& matName = model.materialNames.at(i);

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

            if (ImGui::ColorEdit3("Emission", glm::value_ptr(mat.emissionFactor), sColorEditFlags))
                matNeedsUpdate = true;

            if (ImGui::SliderFloat("Metallic", &mat.metallicFactor, 0.f, 1.f))
                matNeedsUpdate = true;

            if (ImGui::SliderFloat("Roughness", &mat.roughnessFactor, 0.f, 1.f))
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
    MeshNode *meshNode = dynamic_cast<MeshNode*>(node);

    ImGui::Text("Asset Type: Scene Node (Mesh)");
    ImGui::Text("Name: %s", node->name().c_str());

    nodeTransform(node);

    if (ImGui::CollapsingHeader("Info##meshNodeInspector", ImGuiTreeNodeFlags_DefaultOpen))
    {
        auto &model = *mRenderer.mModels.at(meshNode->modelID().value());

        std::vector<Mesh *> meshes;
        for (uuid32_t meshID: meshNode->meshIDs())
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

        if (ImGui::DragFloat("Intensity##dirLight", &dirLight.intensity))
            modified = true;
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

        if (ImGui::DragFloat("Intensity##pointLight", &pointLight.intensity))
            modified = true;

        if (ImGui::DragFloat("Range##pointLight", &pointLight.range))
            modified = true;
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

        if (ImGui::DragFloat("Intensity##spotLight", &spotLight.intensity))
            modified = true;

        if (ImGui::DragFloat("Range##spotLight", &spotLight.range))
            modified = true;

        if (ImGui::DragFloat("Inner Angle##pointLight", &spotLight.innerAngle, 0.01f, 1.f, spotLight.outerAngle, "%.3f", ImGuiSliderFlags_AlwaysClamp))
            modified = true;

        if (ImGui::DragFloat("Outer Angle##pointLight", &spotLight.outerAngle, 0.01f, spotLight.innerAngle, 179.f, "%.3f", ImGuiSliderFlags_AlwaysClamp))
            modified = true;
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

    ImTextureID renderedImageHandle = reinterpret_cast<ImTextureID>(mRenderer.mPostProcessingDs);

    ImGui::Image(renderedImageHandle, viewportSize);

    modelDragDropTargetWholeWindow();

    if (mRenderer.mRenderAxisGizmo)
        drawAxisGizmo(viewportSize);

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

    GraphNode* node = mSceneGraph.searchNode(mSelectedObjectID);

    if (node->type() == NodeType::DirectionalLight) mRenderer.deleteDirLight(node->id());
    if (node->type() == NodeType::PointLight) mRenderer.deletePointLight(node->id());
    if (node->type() == NodeType::SpotLight) mRenderer.deleteSpotLight(node->id());

    mSceneGraph.deleteNode(mSelectedObjectID);
    mSelectedObjectID = 0;
}

void Editor::deleteSelectedModel()
{
    mRenderer.deleteModel(mSelectedObjectID);
    mSelectedObjectID = 0;
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

    ImGui::End();
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

void Editor::rendererPanel()
{
    static constexpr ImGuiColorEditFlags colorEditFlags = ImGuiColorEditFlags_DisplayRGB |
        ImGuiColorEditFlags_AlphaBar;

    ImGui::Begin("Renderer", &mShowRendererPanel);

    if (ImGui::CollapsingHeader("General", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("View Axis Gizmo", &mRenderer.mRenderAxisGizmo);
        ImGui::ColorEdit3("Clear Color", glm::value_ptr(mRenderer.mClearColor));
    }

    if (ImGui::CollapsingHeader("Grid", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Render Grid", &mRenderer.mRenderGrid);
        ImGui::Separator();

        ImGui::ColorEdit4("Thin line color", glm::value_ptr(mRenderer.mGridData.thinLineColor), colorEditFlags);
        ImGui::ColorEdit4("Thick line color", glm::value_ptr(mRenderer.mGridData.thickLineColor), colorEditFlags);
    }

    if (ImGui::CollapsingHeader("Skybox", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Render Skybox", &mRenderer.mRenderSkybox);
        ImGui::Separator();

        ImGui::Checkbox("Flip X-axis##skyboxX", reinterpret_cast<bool*>(&mRenderer.mSkyboxData.flipX));
        ImGui::Checkbox("Flip Y-axis##skyboxY", reinterpret_cast<bool*>(&mRenderer.mSkyboxData.flipY));
        ImGui::Checkbox("Flip Z-axis##skyboxZ", reinterpret_cast<bool*>(&mRenderer.mSkyboxData.flipZ));
        ImGui::Separator();

        if (ImGui::Button("Import New"))
            ImGui::OpenPopup("skyboxImportPopup");
    }

    if (ImGui::CollapsingHeader("Order Independent Transparency", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Enable", &mRenderer.mOitOn);
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

    skyboxImportPopup();

    ImGui::End();
}

void Editor::sceneGraphPanel()
{
    ImGui::Begin("Scene Graph", &mShowSceneGraph);

    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        ImGui::OpenPopup("sceneGraphPopup");

    for (auto child : mSceneGraph.mRoot.children())
        sceneNodeRecursive(child);

    bool nodeHovered = ImGui::IsItemHovered();
    ImGui::Dummy(ImVec2(0, 50.f));
    sceneNodeDragDropTargetWholeWindow(nodeHovered);

    sceneGraphPopup();

    ImGui::End();

    mSceneGraph.updateTransforms();
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
    mSceneGraph.addNode(graphNode);
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

    for (uint32_t i = 0; i < sceneNode.meshIndices.size(); ++i)
    {
        uint32_t meshIndex = sceneNode.meshIndices.at(i);
        const Mesh& mesh = model.meshes.at(meshIndex);
        meshIDs.push_back(mesh.meshID);
    }

    GraphNode* graphNode = new MeshNode(NodeType::Mesh,
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
    DirectionalLight dirLight {
        .color = glm::vec4(1.f),
        .direction = glm::vec4(1.f, -1.f, 1.f, 1.f),
        .intensity = 0.5f
    };

    GraphNode* lightNode = new GraphNode(NodeType::DirectionalLight,
                                         "Directional Light",
                                         spawnPos(),
                                         dirLight.direction,
                                         {},
                                         nullptr);

    mSceneGraph.addNode(lightNode);
    mRenderer.addDirLight(lightNode->id(), dirLight);
}

void Editor::addPointLight()
{
    PointLight pointLight {
        .color = glm::vec4(1.f),
        .position = glm::vec4(spawnPos(), 1.f),
        .intensity = 0.5f,
        .range = 10.f
    };

    GraphNode* lightNode = new GraphNode(NodeType::PointLight,
                                         "Point Light",
                                         pointLight.position,
                                         {},
                                         {},
                                         nullptr);

    mSceneGraph.addNode(lightNode);
    mRenderer.addPointLight(lightNode->id(), pointLight);
}

void Editor::addSpotLight()
{
    SpotLight spotLight {
        .color = glm::vec4(1.f),
        .position = glm::vec4(spawnPos(), 1.f),
        .direction = glm::vec4(0.f, -1.f, 0.f, 0.f),
        .intensity = 0.5f,
        .range = 10.f,
        .innerAngle = 45.f,
        .outerAngle = 60.f
    };

    GraphNode* lightNode = new GraphNode(NodeType::SpotLight,
                                         "Spot Light",
                                         spotLight.position, spotLight.direction, {},
                                         nullptr);

    mSceneGraph.addNode(lightNode);
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
    GraphNode* copiedNode = mSceneGraph.searchNode(mCopiedNodeID);

    if (copiedNode)
    {
        if (mCopyFlag == CopyFlags::Copy)
        {
            GraphNode* newNode = copyGraphNode(copiedNode);
            newNode->orphan();
            parent->addChild(newNode);
        }

        if (mCopyFlag == CopyFlags::Cut && !mSceneGraph.hasDescendant(copiedNode, parent))
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

    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        ImGui::OpenPopup("assetPanelPopup");

    for (const auto& [modelID, model] : mRenderer.mModels)
    {
        ImGui::Selectable(model->name.c_str(), mSelectedObjectID == modelID);
        lastItemClicked(modelID);
        modelDragDropSource(modelID);
    }

    assetPanelPopup();

    ImGui::End();
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
                transferNode->localT = transferNode->globalT - node->globalT;

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

                if (!mSceneGraph.hasDescendant(transferNode, &mSceneGraph.mRoot))
                {
                    transferNode->localT = transferNode->globalT - mSceneGraph.mRoot.globalT;

                    transferNode->orphan();
                    transferNode->markDirty();

                    mSceneGraph.mRoot.addChild(transferNode);
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
            mSceneGraph.addNode(createEmptyNode(nullptr, "Empty Node"));

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

void Editor::skyboxImportPopup()
{
    static ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoScrollbar;

    static const char* preview = "...";
    static std::array<std::string, 6> paths;

    static auto loadPath = [] (const char* buttonLabel, std::string& outPath) {
        if (ImGui::Button(buttonLabel))
        {
            static const char* filter = "Image files *.png *.jpeg *.jpg *.bmp\0*.jpeg;*.png;*.jpg;*.bmp;\0";
            std::filesystem::path path = fileDialog("Select Texture", filter);
            if (!path.empty()) outPath = path.string();
        }
    };

    static auto displayPath = [] (std::string& path) {
        ImGui::SameLine();
        ImGui::BeginDisabled();

        float textBoxLen = ImGui::GetContentRegionAvail().x;
        if (!path.empty()) textBoxLen = std::min(textBoxLen, ImGui::CalcTextSize(path.data()).x + 8);

        ImGui::PushItemWidth(textBoxLen);

        if (path.empty()) ImGui::InputText("##skybox-path", const_cast<char*>(preview), 3);
        else ImGui::InputText("##skybox-path", path.data(), path.size());

        ImGui::PopItemWidth();
        ImGui::EndDisabled();
    };

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();

    ImGui::SetNextWindowBgAlpha(1.0f);
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(500, 300), ImGuiCond_Appearing);
    ImGui::SetNextWindowSizeConstraints(ImVec2(250, 250), ImVec2(FLT_MAX, FLT_MAX));

    if (ImGui::BeginPopupModal("skyboxImportPopup", nullptr, windowFlags))
    {
        ImGui::PushStyleVar(ImGuiStyleVar_SeparatorTextAlign, ImVec2(0.5f, 0.5f));
        ImGui::SeparatorText("Import Skybox Textures");
        ImGui::PopStyleVar();
        ImGui::Separator();

        loadPath("Add X+", paths.at(0));
        displayPath(paths.at(0));

        loadPath("Add X-", paths.at(1));
        displayPath(paths.at(1));

        loadPath("Add Y+", paths.at(2));
        displayPath(paths.at(2));

        loadPath("Add Y-", paths.at(3));
        displayPath(paths.at(3));

        loadPath("Add Z+", paths.at(4));
        displayPath(paths.at(4));

        loadPath("Add Z-", paths.at(5));
        displayPath(paths.at(5));

        ImGui::Separator();
        helpMarker("The coordinate system is right handed with the positive Y-axis pointing up.");

        float padding = 10.0f;  // Padding from the edge
        float buttonWidth = 80.0f;
        float spacing = 10.0f;  // Space between buttons
        float totalWidth = (buttonWidth * 2) + spacing;

        // Align to bottom-right
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - totalWidth - padding);
        ImGui::SetCursorPosY(ImGui::GetWindowHeight() - ImGui::GetFrameHeight() - padding);

        if (ImGui::Button("Cancel", ImVec2(buttonWidth, 0)))
        {
            paths.fill({});
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        ImGui::BeginDisabled(std::any_of(paths.begin(), paths.end(), [] (const auto& s) {return s.empty();}));
        if (ImGui::Button("OK", ImVec2(buttonWidth, 0)))
        {
            mRenderer.importSkybox(paths);
            paths.fill({});
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndDisabled();

        ImGui::EndPopup();
    }
}

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

void Editor::drawAxisGizmo(ImVec2 viewportSize)
{
    ImVec2 windowCoords = ImGui::GetWindowPos();
    float gizmoSize = 50.f;
    float paddingX = 70.f;
    float paddingY = 88.f;
    float gizmoLeft = windowCoords.x + viewportSize.x - gizmoSize - paddingX;
    float gizmoTop = windowCoords.y + paddingY;
    ImOGuizmo::SetRect(gizmoLeft, gizmoTop, gizmoSize);

    glm::mat4 view = mRenderer.mCamera.view();
    static glm::mat4 proj = glm::perspective(glm::radians(30.f), 1.5f, 0.01f, 100.f);
    ImOGuizmo::DrawGizmo(glm::value_ptr(view), glm::value_ptr(proj));
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

            MeshNode* meshNode = dynamic_cast<MeshNode*>(node);

            uuid32_t modelID = meshNode->modelID().value();
            std::vector<uuid32_t> meshIDs = meshNode->meshIDs();

            newNode = new MeshNode(meshNode->type(),
                                   meshNode->name(),
                                   node->localT,
                                   node->localR,
                                   node->localS,
                                   meshNode->parent(),
                                   modelID, meshIDs);

            for (uuid32_t meshID : meshIDs)
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
        accumulatedTime = 0.f;
    }
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

glm::vec3 Editor::spawnPos()
{
    return mRenderer.mCamera.position() - mRenderer.mCamera.front() * 10.f;
}
