#pragma once
#include "ECS/ECS.h"
#include "Core/TimeStep.h"
#include "Physics2D/Physics2D.h"
#include "box2d/b2_world.h"
#include "Renderer/EditorCamera.h"
#include "Renderer/Renderer2D.h"
#include "Renderer/Renderer3D.h"

class Entity;

class Scene
{
public:
	Scene();
	~Scene();
	
	Entity CreateEntity(std::string name);
	void DestroyEntity(Entity entity);

	void OnRuntimeStart();
	void OnRuntimeStop();
	void Render(EditorCamera& editorCamera);

	//TODO remove editor camera from runtime and use a camera component
	void OnRuntimeUpdate(TimeStep timestep, EditorCamera& editorCamera);
	void OnEditorUpdate(TimeStep timestep, EditorCamera& editorCamera);

private:
	ECS m_Ecs;
	b2World* m_PhysicsWorld;
	Physics2D* m_Physics2D;
	Renderer2D* m_Renderer2D;
	Renderer3D* m_Renderer3D;

	friend class Entity;
	friend class SceneHierarchyPanel;
	friend class SceneSerializer;
};
