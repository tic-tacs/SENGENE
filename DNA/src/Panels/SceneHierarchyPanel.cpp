#include "SceneHierarchyPanel.h"
#include <glm/gtc/type_ptr.hpp>

#include "Renderer/ResourceManager.h"
namespace SGE
{
	SceneHierarchyPanel::SceneHierarchyPanel(const Ref<Scene> &scene)
		: m_SceneContext(scene)
	{
		m_SelectedEntity = {};
	}

	SceneHierarchyPanel::~SceneHierarchyPanel() {}

	void SceneHierarchyPanel::OnImGuiRender()
	{
		if (!m_SceneContext)
			return;

		ImGui::Begin("Scene Hierarchy");
		ImGui::SetNextItemOpen(true);
		if (ImGui::TreeNode(m_SceneContext->GetSceneName().c_str()))
		{
			ScenePopupWindows();

			// Scene Hierarchy
			if (m_align_label_with_current_x_position)
				ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing());

			auto view = m_SceneContext->Registry().view<TagComponent>();
			for (auto e : view)
				DrawEntityNode(Entity{e, m_SceneContext.get()});
			EntityPopupWindows();

			if (m_align_label_with_current_x_position)
				ImGui::Indent(ImGui::GetTreeNodeToLabelSpacing());

			ImGui::TreePop();
		}
		ImGui::End();

		if (m_SelectedEntity)
			ShowSelectedComponents();
	}

	void SceneHierarchyPanel::SetContext(const Ref<Scene> &scene)
	{
		m_SceneContext = scene;
		m_SelectedEntity = {};
	}

	void SceneHierarchyPanel::DrawEntityNode(Entity entity)
	{
		if (ImGui::GetIO().MouseClicked[1] && ImGui::IsItemHovered())
		{
			ImGui::OpenPopup("EntityPopUp");
		}
		// Disable the default "open on single-click behavior" + set Selected flag according to our selection.
		// To alter selection we use IsItemClicked() && !IsItemToggledOpen(), so clicking on an arrow doesn't alter selection.
		ImGuiTreeNodeFlags node_flags = m_TreeNodeFlags;
		auto &tag = entity.GetComponent<TagComponent>();
		const bool is_selected = (entity == m_SelectedEntity);

		if (is_selected)
			node_flags |= ImGuiTreeNodeFlags_Selected;

		bool node_open = ImGui::TreeNodeEx(tag.Tag.c_str(), node_flags, "%s", tag.Tag.c_str());
		if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
			m_SelectedEntity = entity;

		if (m_test_drag_and_drop && ImGui::BeginDragDropSource())
		{
			ImGui::SetDragDropPayload("_TREENODE", NULL, 0);
			ImGui::Text("This is a drag and drop source");
			ImGui::EndDragDropSource();
		}
		if (node_open)
		{
			// TODO: Recursive Children
			ImGui::BulletText("Child 1\n Child 2");
			ImGui::TreePop();
		}
	}

	void SceneHierarchyPanel::ShowMenu()
	{
		if (ImGui::BeginMenu("Hierarchy Layout"))
		{
			ImGui::CheckboxFlags("ImGuiTreeNodeFlags_OpenOnArrow", &m_TreeNodeFlags, ImGuiTreeNodeFlags_OpenOnArrow);
			ImGui::CheckboxFlags("ImGuiTreeNodeFlags_OpenOnDoubleClick", &m_TreeNodeFlags, ImGuiTreeNodeFlags_OpenOnDoubleClick);
			ImGui::CheckboxFlags("ImGuiTreeNodeFlags_SpanAvailWidth", &m_TreeNodeFlags, ImGuiTreeNodeFlags_SpanAvailWidth);
			ImGui::CheckboxFlags("ImGuiTreeNodeFlags_SpanFullWidth", &m_TreeNodeFlags, ImGuiTreeNodeFlags_SpanFullWidth);
			ImGui::Checkbox("Align label with current X position", &m_align_label_with_current_x_position);
			ImGui::Checkbox("Test tree node as drag source", &m_test_drag_and_drop);
			ImGui::EndMenu();
		}
	}

	void SceneHierarchyPanel::ShowSelectedComponents()
	{
		ImGui::Begin("Properties");

		// Entity components panels
		if (m_SelectedEntity.HasComponent<TagComponent>())
		{
			auto &tag = m_SelectedEntity.GetComponent<TagComponent>();

			static char buffer[TagComponent::MAX_TAG_SIZE] = "";
			strcpy(buffer, tag.Tag.c_str());

			static bool isStatic = true;
			if (ImGui::InputText("##label", buffer, 128, ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_EnterReturnsTrue))
				tag.Tag = buffer;

			ImGui::SameLine();
			ImGui::Checkbox("static", &isStatic);

			ImGui::Separator();
		}

		if (m_SelectedEntity.HasComponent<TransformComponent>())
		{
			if (ImGui::CollapsingHeader("Transform Component"))
			{
				static float rate = 0.2f;
				auto &transform = m_SelectedEntity.GetComponent<TransformComponent>();
				ImGui::DragFloat3("Position: ", glm::value_ptr(transform.Position), rate);
				ImGui::DragFloat3("Scale: ", glm::value_ptr(transform.Scale), rate);
				ImGui::DragFloat3("Rotation: ", glm::value_ptr(transform.Rotation), rate);
			}
		}

		if (m_SelectedEntity.HasComponent<MeshRendererComponent>())
		{
			if (ImGui::CollapsingHeader("Mesh Renderer"))
			{
				auto &model = m_SelectedEntity.GetComponent<MeshRendererComponent>().Model;

				ImGui::Text("Meshes : %d", model->GetNMeshes());
				ImGui::Text("Draw Calls: %d", model->GetNMaterials());

				ImGui::Separator();
				ImGui::Text("Materials: %d", model->GetNMaterials());
				ImVec2 panelSize = ImGui::GetWindowContentRegionMax();
				panelSize.x /= 2;
				panelSize.y /= 6;

				for (const auto &material : model->GetMaterials())
				{
					ImGui::Text("%s", material->Name.c_str());
					ImGui::ColorEdit3("Ambient", glm::value_ptr(material->AmbientColor), ImGuiColorEditFlags_NoInputs);
					ImGui::SameLine();
					ImGui::ColorEdit3("Diffuse", glm::value_ptr(material->DiffuseColor), ImGuiColorEditFlags_NoInputs);
					ImGui::SameLine();
					ImGui::ColorEdit3("Specular", glm::value_ptr(material->SpecularColor), ImGuiColorEditFlags_NoInputs);

					if (material->DiffuseTexture)
					{
						ImGui::Text("Diffuse Texture");
						ImGui::Image((void *)material->DiffuseTexture->GetID(), panelSize, ImVec2(0, 1), ImVec2(1, 0));
					}
					if (material->SpecularTexture)
					{
						ImGui::SameLine();
						ImGui::Text("Specular Texture");
						ImGui::Image((void *)material->SpecularTexture->GetID(), panelSize, ImVec2(0, 1), ImVec2(1, 0));
					}
				}
			}
		}

		if (m_SelectedEntity.HasComponent<SkinnedMeshRendererComponent>())
		{
			if (ImGui::CollapsingHeader("Skinned Mesh Renderer"))
			{
				auto &skinnedMeshComponent = m_SelectedEntity.GetComponent<SkinnedMeshRendererComponent>();
				auto &model = skinnedMeshComponent.AnimatedModel;

				ImGui::Text("Meshes : %d", model->GetNMeshes());
				ImGui::Text("Draw Calls: %d", model->GetNMaterials());

				ImGui::Separator();
				ImGui::Text("Materials: %d", model->GetNMaterials());
				ImGui::Checkbox("FlipUVS", &skinnedMeshComponent.FlipUVS);
				ImVec2 panelSize = ImGui::GetWindowContentRegionMax();
				panelSize.x /= 2;
				panelSize.y /= 6;

				for (const auto &material : model->GetMaterials())
				{
					ImGui::Text("%s", material->Name.c_str());
					ImGui::ColorEdit3("Ambient", glm::value_ptr(material->AmbientColor), ImGuiColorEditFlags_NoInputs);
					ImGui::SameLine();
					ImGui::ColorEdit3("Diffuse", glm::value_ptr(material->DiffuseColor), ImGuiColorEditFlags_NoInputs);
					ImGui::SameLine();
					ImGui::ColorEdit3("Specular", glm::value_ptr(material->SpecularColor), ImGuiColorEditFlags_NoInputs);

					if (material->DiffuseTexture)
					{
						ImGui::Text("Diffuse Texture");
						ImGui::Image((void *)material->DiffuseTexture->GetID(), panelSize, ImVec2(0, 1), ImVec2(1, 0));
					}
					if (material->SpecularTexture)
					{
						ImGui::SameLine();
						ImGui::Text("Specular Texture");
						ImGui::Image((void *)material->SpecularTexture->GetID(), panelSize, ImVec2(0, 1), ImVec2(1, 0));
					}
				}
			}
		}

		if (m_SelectedEntity.HasComponent<RigidBodyComponent>())
		{
			if (ImGui::CollapsingHeader("RigidBody Component"))
			{
				static float rate = 0.2f;
				auto &rb = m_SelectedEntity.GetComponent<RigidBodyComponent>();

				// TODO: Add body types to 3d Rigidbodies
				switch (rb.Body.Type)
				{
				case flg::BodyType::Static:
					ImGui::Text("Static");
					break;
				case flg::BodyType::Dynamic:
					ImGui::Text("Dynamic");
					break;
				case flg::BodyType::Kinematic:
					ImGui::Text("Kinematic");
					break;
				default:
					ImGui::Text("Uknown Body Type");
					break;
				};

				if (ImGui::RadioButton("Static", rb.Body.Type == flg::BodyType::Static))
				{
					rb.Body.Type = flg::BodyType::Static;
				}
				ImGui::SameLine();
				if (ImGui::RadioButton("Dynamic", rb.Body.Type == flg::BodyType::Dynamic))
				{
					rb.Body.Type = flg::BodyType::Dynamic;
				}
				ImGui::SameLine();
				if (ImGui::RadioButton("Kinematic", rb.Body.Type == flg::BodyType::Kinematic))
				{
					rb.Body.Type = flg::BodyType::Kinematic;
				}
				ImGui::SameLine();
				ImGui::Checkbox("UseGravity", &rb.UseGravity);
			}
		}

		if (m_SelectedEntity.HasComponent<SphereColliderComponent>())
		{
			if (ImGui::CollapsingHeader("SphereCollider Component"))
			{
				static float rate = 0.2f;
				auto &collider = m_SelectedEntity.GetComponent<SphereColliderComponent>();
				ImGui::DragFloat3("Center", &collider.sphereCollider.Center.x);
				ImGui::DragFloat("Radius", &collider.sphereCollider.Radius);
				ImGui::NewLine();
			}
		}

		if (m_SelectedEntity.HasComponent<PlaneColliderComponent>())
		{
			if (ImGui::CollapsingHeader("PlaneCollider Component"))
			{
				static float rate = 0.2f;
				auto &collider = m_SelectedEntity.GetComponent<PlaneColliderComponent>();
				ImGui::DragFloat3("Normal", &collider.planeCollider.Normal.x);
				ImGui::NewLine();
				ImGui::DragFloat3("Origin", &collider.planeCollider.Origin.x);
				ImGui::NewLine();
				ImGui::DragFloat2("Bounds", &collider.planeCollider.Bounds.x);
			}
		}

		// Add component popup
		{
			static const char *title = "+ Add New Component";
			if (ImGui::Button(title, ImVec2{ImGui::GetWindowWidth(), 0}) && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootWindow) && ImGui::IsWindowHovered(ImGuiHoveredFlags_RootWindow))
			{
				ImGui::OpenPopup("add_component_pop_up");
			}

			if (ImGui::BeginPopup("add_component_pop_up"))
			{
				if (ImGui::BeginMenu("Renderable"))
				{
					if (ImGui::MenuItem("Cube"))
					{
						if (!m_SelectedEntity.HasComponent<MeshRendererComponent>())
							m_SelectedEntity.AddComponent<MeshRendererComponent>(ResourceManager::GetModel("assets/models/cube/cube.obj"));
					}
					ImGui::EndMenu();
				}

				if (ImGui::BeginMenu("Physics"))
				{
					if (ImGui::MenuItem("RigidBody"))
					{
						if (!m_SelectedEntity.HasComponent<RigidBodyComponent>())
							m_SelectedEntity.AddComponent<SGE::RigidBodyComponent>();
					}

					if (ImGui::MenuItem("SphereCollider"))
					{
						if (!m_SelectedEntity.HasComponent<SphereColliderComponent>())
						{
							auto &collider = m_SelectedEntity.AddComponent<SphereColliderComponent>();

							// These are in terms of world positions
							collider.sphereCollider.Center = m_SelectedEntity.GetComponent<TransformComponent>().Position + collider.sphereCollider.Center;
							collider.sphereCollider.Radius = 1.0f;
						}
					}

					if (ImGui::MenuItem("PlaneCollider"))
					{
						if (!m_SelectedEntity.HasComponent<PlaneColliderComponent>())
						{
							auto &pColliderComponent = m_SelectedEntity.AddComponent<PlaneColliderComponent>();
							pColliderComponent.planeCollider.Origin = m_SelectedEntity.GetComponent<TransformComponent>().Position;
						}
					}

					ImGui::EndMenu();
				}

				ImGui::EndPopup();
			}
		}

		ImGui::End();
	}

	void SceneHierarchyPanel::ScenePopupWindows()
	{
		// Showing a menu with toggles
		if (ImGui::GetIO().MouseClicked[1] && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootWindow) && ImGui::IsWindowHovered(ImGuiHoveredFlags_RootWindow))
		{
			ImGui::OpenPopup("SceneHierarchyPopUp");
		}

		if (ImGui::BeginPopup("SceneHierarchyPopUp"))
		{
			if (ImGui::BeginMenu("Add Entity"))
			{
				if (ImGui::MenuItem("Empty Entity"))
				{
					Entity e = m_SceneContext->CreateEntity();
				}
				if (ImGui::MenuItem("Cube Entity"))
				{
					Entity e = m_SceneContext->CreateEntity();
					e.AddComponent<MeshRendererComponent>(ResourceManager::GetModel("assets/models/cube/cube.obj"));
				}
				ImGui::EndMenu();
			}

			ImGui::Separator();
			ImGui::Text("Tooltip here");
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("I am a tooltip over a popup");

			if (ImGui::Button("Stacked Popup"))
				ImGui::OpenPopup("another popup");
			if (ImGui::BeginPopup("another popup"))
			{
				if (ImGui::BeginMenu("Sub-menu"))
				{
					ImGui::MenuItem("Click me");
					if (ImGui::Button("Stacked Popup"))
						ImGui::OpenPopup("another popup");
					if (ImGui::BeginPopup("another popup"))
					{
						ImGui::Text("I am the last one here.");
						ImGui::EndPopup();
					}
					ImGui::EndMenu();
				}
				ImGui::EndPopup();
			}
			ImGui::EndPopup();
		}
	}
	void SceneHierarchyPanel::EntityPopupWindows()
	{
		if (ImGui::BeginPopup("EntityPopUp"))
		{
			if (ImGui::Button("Duplicate Entity"))
				m_SceneContext->CreateEntity(m_SelectedEntity);

			ImGui::EndPopup();
		}
	}

	void SceneHierarchyPanel::SetSelectedEntity(Entity entity)
	{
		m_SelectedEntity = entity;
	}

	Entity SceneHierarchyPanel::GetSelectedEntity()
	{
		return Entity(m_SelectedEntity, m_SceneContext.get());
	}
}