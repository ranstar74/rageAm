#include "scene.h"

#include "scene_fbx.h"
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
	//if (HasLocalWorldMatrices())
	//	return;

	m_LocalMatrix = rage::Mat44V::Transform(GetScale(), GetRotation(), GetTranslation());

	rage::Mat44V world = GetLocalTransform();
	SceneNode* parent = GetParent();
	while (parent)
	{
		world *= parent->GetLocalTransform();
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

	// Get all light nodes
	for (u16 i = 0; i < GetNodeCount(); i++)
	{
		SceneNode* node = GetNode(i);
		if (node->HasLight())
			m_LightNodes.Add(node);
	}

	// Build material map
	u16 matCount = GetMaterialCount();
	m_NameToMaterial.InitAndAllocate(matCount, false);
	for (u16 i = 0; i < matCount; i++)
	{
		m_NameToMaterial.Insert(GetMaterial(i));
	}

	// Build name map
	u16 nodeCount = GetNodeCount();
	m_NameToNode.InitAndAllocate(nodeCount, false);
	for(u16 i = 0; i < nodeCount; i++)
	{
		m_NameToNode.Insert(GetNode(i));
	}
}

rageam::graphics::SceneNode* rageam::graphics::Scene::GetNodeByName(ConstString name) const
{
	SceneNode** ppNode = m_NameToNode.TryGetAt(SceneHashFn(name));
	if (ppNode) return *ppNode;
	return nullptr;
}

rageam::graphics::SceneMaterial* rageam::graphics::Scene::GetMaterialByName(ConstString name) const
{
	SceneMaterial** ppMat = m_NameToMaterial.TryGetAt(SceneHashFn(name));
	if (ppMat) return *ppMat;
	return nullptr;
}

void rageam::graphics::Scene::DebugPrint()
{
	AM_TRACEF("######## SCENE DEBUG INFO ########");

	auto logger = Logger::GetInstance();
	logger->GetOptions().Set(LOG_OPTION_NO_PREFIX, true);

	string indentString;
	int indent = 0;
	std::function<void(SceneNode*)> printRecurse;
	printRecurse = [&](const SceneNode* node)
		{
			if (!node)
				return;
			indentString = "";
			for (int i = 0; i < indent; i++) indentString += "    ";

			string nodeDisplay;
			nodeDisplay.AppendFormat("- %s%s <Skin:%u Mesh:%u Light:%u",
				indentString.GetCStr(), node->GetName(),
				node->HasSkin(), node->HasMesh(), node->HasLight());

			if (node->HasScale())
			{
				rage::Vec3V s = node->GetScale();
				nodeDisplay.AppendFormat(" S(%g %g %g)", s.X(), s.Y(), s.Z());
			}

			if (node->HasRotation())
			{
				rage::QuatV r = node->GetRotation();
				nodeDisplay.AppendFormat(" R(%g %g %g %g)", r.X(), r.Y(), r.Z(), r.W());
			}

			if (node->HasTranslation())
			{
				rage::Vec3V t = node->GetTranslation();
				nodeDisplay.AppendFormat(" T(%g %g %g)", t.X(), t.Y(), t.Z());
			}

			nodeDisplay += '>';

			AM_TRACEF(nodeDisplay.GetCStr());

			if (node->HasMesh())
			{
				auto mesh = node->GetMesh();
				for (u16 i = 0; i < mesh->GetGeometriesCount(); i++)
				{
					auto geom = mesh->GetGeometry(i);
					auto& bb = geom->GetAABB();
					AM_TRACEF("%s\tGeometry #%u (Verts:%u Indies:%u Mat:%u) Min(%g %g %g) Max(%g %g %g)",
						indentString.GetCStr(), i, geom->GetVertexCount(), geom->GetIndexCount(), geom->GetMaterialIndex(),
						bb.Min.X(), bb.Min.Y(), bb.Min.Z(), bb.Max.X(), bb.Max.Y(), bb.Max.Z());
				}
			}

			indent++;
			printRecurse(node->GetFirstChild());
			indent--;
			printRecurse(node->GetNextSibling());
		};

	AM_TRACEF("Scene Type: %s", GetTypeName());
	AM_TRACEF("Node Count: %u", GetNodeCount());
	AM_TRACEF("Node Hierarchy:");
	SceneNode* root = GetFirstNode();
	printRecurse(root);

	logger->GetOptions().Set(LOG_OPTION_NO_PREFIX, false);
}

amPtr<rageam::graphics::Scene> rageam::graphics::SceneFactory::LoadFrom(ConstWString path, SceneLoadOptions* options)
{
	ConstWString extension = file::GetExtension(path);

	Scene* scene = nullptr;

	switch (Hash(extension))
	{
	case Hash("gltf"):
	case Hash("glb"):
		scene = new SceneGl();
		break;
	case Hash("obj"): // UFBX also supports .obj
	case Hash("fbx"):
		scene = new SceneFbx();
		break;
	}

	if (!AM_VERIFY(scene != nullptr, "SceneFactory::LoadFrom() -> File %ls is not supported", extension))
		return nullptr;

	SceneLoadOptions dummyOptions = {};
	if (!scene->Load(path, options ? *options : dummyOptions)) // Pass either given options or dummy
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
	switch (Hash(extension))
	{
	case Hash("gltf"):
	case Hash("glb"):
	case Hash("obj"):
	case Hash("fbx"):
		return true;
	}
	return false;
}
