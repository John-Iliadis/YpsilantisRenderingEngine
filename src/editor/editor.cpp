//
// Created by Gianni on 4/02/2025.
//

#include "editor.hpp"

Editor::Editor(Renderer& renderer)
    : mRenderer(renderer)
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
    , mCopyFlag(CopyFlags::None)
    , mCopiedNodeID()
{
}

Editor::~Editor() = default;

void Editor::update(float dt)
{
    mDt = dt;

    ImGui::ShowDemoWindow();

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

    if (mShowDebugPanel)
        debugPanel();

    if (mShowConsole)
        console();

    if (mShowViewport)
        viewPort();
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
                importModel();

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

void Editor::inspectorPanel()
{
    ImGui::Begin("Inspector", &mShowInspectorPanel, ImGuiWindowFlags_NoScrollbar);

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
    auto model = mRenderer.mModels.at(modelID);

    ImGui::Text("Asset Type: Model");
    ImGui::Text("Name: %s", model->name.c_str());
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

// todo: improve drag drop area
void Editor::sceneGraphPanel()
{
    ImGui::Begin("Scene Graph", &mShowSceneGraph);

    sceneGraphPopup();

    for (auto child : mSceneGraph.mRoot.children())
    {
        sceneNodeRecursive(child);
    }

    ImGui::Dummy(ImGui::GetContentRegionAvail());
    sceneNodeDragDropTarget(&mSceneGraph.mRoot);

    ImGui::End();

    mSceneGraph.updateTransforms();
}

// todo: fix bugs
// todo fix bug: copied node gets deleted and then getting pasted
// todo: test
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
        uint32_t instanceID = mesh.mesh.addInstance({}, {});

        graphNode = new MeshNode(NodeType::Mesh, sceneNode.name, sceneNode.transformation, parent,
                                 model.id, meshID, instanceID);
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
    if (auto copiedNode = mSceneGraph.searchNode(mCopiedNodeID))
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
        if (ImGui::MenuItem("Import Model##assetPanel"))
            importModel();

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

            transferNode->orphan();
            transferNode->markDirty();

            node->addChild(transferNode);
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

void Editor::modelDragDropTarget()
{
    if (ImGui::BeginDragDropTarget())
    {
        checkPayloadType("Model");

        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Model"))
        {
            uuid32_t modelID = *(uuid32_t*)payload->Data;
            Model& model = *mRenderer.mModels.at(modelID);
            createModelGraph(model);
        }

        ImGui::EndDragDropTarget();
    }
}

void Editor::importModel()
{
    std::filesystem::path path = fileDialog("All Files");

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
            uint32_t instanceID = mRenderer.mModels.at(modelID)->getMesh(meshID).mesh.addInstance({}, {});
            newNode = new MeshNode(meshNode->type(),
                                   meshNode->name(),
                                   meshNode->localTransform(),
                                   meshNode->parent(),
                                   modelID,
                                   meshID,
                                   instanceID);
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
