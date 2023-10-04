#include "scene.h"

#include "scene_gl.h"
#include "am/file/pathutils.h"
#include "rage/crypto/joaat.h"

bool rageam::graphics::SceneGeometry::HasSkin() const
{
	return m_Mesh->HasSkin();
}

rageam::graphics::SceneNode* rageam::graphics::SceneGeometry::GetParentNode() const
{
	return m_Mesh->GetParentNode();
}

bool rageam::graphics::SceneMesh::HasSkin() const
{
	return m_Parent->HasSkin();
}

void rageam::graphics::SceneNode::ComputeMatrices()
{
	m_LocalMatrix = rage::Mat44V::Transform(GetScale(), GetRotation(), GetTranslation());

	rage::Mat44V world = GetLocalTransform();
	SceneNode* parent = GetParent();
	while (parent)
	{
		world = parent->GetLocalTransform() * world;
		parent = parent->GetParent();
	}
	m_WorldMatrix = rage::Mat44V(world);
}

bool rageam::graphics::SceneNode::HasTransformedChild() const
{
	SceneNode* child = GetFirstChild();
	while (child)
	{
		if (child->HasTransform())
			return true;

		// Scan child tree
		SceneNode* childChild = child->GetFirstChild();
		if (childChild && childChild->HasTransformedChild())
			return true;

		child = child->GetNextSibling();
	}
	return false;
}

void rageam::graphics::Scene::FindSkinnedNodes()
{
	m_HasSkinning = false;
	for (u16 i = 0; i < GetNodeCount(); i++)
	{
		if (GetNode(i)->HasSkin())
		{
			m_HasSkinning = true;
			return;
		}
	}
}

void rageam::graphics::Scene::FindTransformedModels()
{
	m_HasTransform = false;
	for (u16 i = 0; i < GetNodeCount(); i++)
	{
		if (GetNode(i)->HasTransform())
		{
			m_HasTransform = true;
			return;
		}
	}
}

void rageam::graphics::Scene::FindNeedDefaultMaterial()
{
	m_NeedDefaultMaterial = false;
	for (u16 i = 0; i < GetNodeCount(); i++)
	{
		SceneNode* node = GetNode(i);
		SceneMesh* mesh = node->GetMesh();
		if (!mesh)
			continue;

		for (u16 k = 0; k < mesh->GetGeometriesCount(); k++)
		{
			SceneGeometry* geometry = mesh->GetGeometry(k);
			if (geometry->GetMaterialIndex() == DEFAULT_MATERIAL)
			{
				m_NeedDefaultMaterial = true;
				return;
			}
		}
	}
}

void rageam::graphics::Scene::ScanForMultipleRootBones()
{
	SceneNode* root = GetFirstNode();
	m_HasMultipleRootNodes = root && root->GetNextSibling() != nullptr;
}

void rageam::graphics::Scene::ComputeNodeMatrices() const
{
	for (u16 i = 0; i < GetNodeCount(); i++)
	{
		SceneNode* node = GetNode(i);
		node->ComputeMatrices();
	}
}

void rageam::graphics::Scene::Init()
{
	FindTransformedModels();
	FindNeedDefaultMaterial();
	ScanForMultipleRootBones();
	ComputeNodeMatrices();
}

amPtr<rageam::graphics::Scene> rageam::graphics::SceneFactory::LoadFrom(ConstWString path)
{
	ConstWString extension = file::GetExtension(path);

	Scene* scene = nullptr;

	switch (rage::joaat(extension))
	{
	case rage::joaat("gltf"):
	case rage::joaat("glb"):
		scene = new SceneGl();
		break;
	}

	if (!AM_VERIFY(scene != nullptr, "SceneFactory::LoadFrom() -> File %ls is not supported", extension))
		return nullptr;

	if (!scene->Load(path))
		return nullptr;

	wchar_t nameBuffer[64];
	file::GetFileNameWithoutExtension(nameBuffer, sizeof nameBuffer, path);
	ConstString sceneName = String::ToAnsiTemp(nameBuffer);

	scene->Init();
	scene->SetName(sceneName);

	return amPtr<Scene>(scene);
}

bool rageam::graphics::SceneFactory::IsSupportedFormat(ConstWString extension)
{
	// TODO: Use map with create delegate instead, so we can just check if it's in map
	switch (rage::joaat(extension))
	{
	case rage::joaat("gltf"):
	case rage::joaat("glb"):
		return true;
	}
	return false;
}
