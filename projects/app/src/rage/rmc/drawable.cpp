#include "drawable.h"

#include "rage/system/memcontainer.h"

void rage::rmcDrawable::CopyShaderGroupToContainer(const grmShaderGroup* from)
{
	// Shader container memory block consists of:
	// - grmShaderGroup
	// - grmShaderGroup::m_Shaders::m_Elements (shader pointer array)
	// - grmShader (array)
	// We also align atArray pointers size to 16 because container
	// uses minimum align of 16, so we have to account that padding in

	u32 shaderCount = from->GetShaderCount();
	u32 containerSize =
		sizeof(grmShaderGroup) +
		ALIGN_16(sizeof(pVoid) * shaderCount) +
		sizeof(grmShader) * shaderCount;

	sysMemContainerData containerData;
	containerData.Block = pgRscCompiler::GetVirtualAllocator()->Allocate(containerSize);
	containerData.Size = containerSize;
	sysMemContainer container(containerData);

	// Steps are:
	// - Bind container, allocate atArray with grmShader* 's but not anything else!
	// - Copy everything else (shader vars, embed dictionary) using regular allocator
	container.Bind();
	grmShaderGroup* containerGroup = new grmShaderGroup();
	from->PreAllocateContainer(containerGroup);
	container.UnBind();
	from->CopyToContainer(containerGroup);

	containerGroup->SetContainerBlockSize(container.GetOffset());
	m_ShaderGroup = containerGroup;

	if (pgRscCompiler::GetCurrent())
	{
		m_ShaderGroup.AddCompilerRef();
		//m_ShaderGroup->Snapshot();
	}
}

void rage::rmcDrawable::DeleteShaderGroupContainer()
{
	sysMemContainerData sgBlock;
	sgBlock.Block = m_ShaderGroup.Get();
	sgBlock.Size = m_ShaderGroup->GetContainerBlockSize();
	sysMemContainer container(sgBlock);

	container.Bind();
	m_ShaderGroup.Delete();
	container.UnBind();

	container.FreeHeap();

	m_ShaderGroup.SuppressDelete();
}

rage::rmcDrawable::rmcDrawable()
{
	m_ShaderGroup = new grmShaderGroup();

	// By default shader group container contains only itself...
	m_ShaderGroup->SetContainerBlockSize(sizeof grmShaderGroup);

	m_Unknown98 = 0;
	m_UnknownA0 = 0;
	m_UnknownA8 = 0;
}

// ReSharper disable once CppPossiblyUninitializedMember
rage::rmcDrawable::rmcDrawable(const datResource& rsc)
{
	ComputeBucketMask();
	SortForTessellation();
}

rage::rmcDrawable::rmcDrawable(const rmcDrawable& other) : pgBase(other), m_LodGroup(other.m_LodGroup)
{
	m_SkeletonData = other.m_SkeletonData;
	m_JointData = other.m_JointData;

	m_Unknown98 = other.m_Unknown98;
	m_UnknownA0 = other.m_UnknownA0;
	m_UnknownA8 = other.m_UnknownA8;

	CopyShaderGroupToContainer(other.m_ShaderGroup.Get());
}

rage::rmcDrawable::~rmcDrawable()
{
	DeleteShaderGroupContainer();
}

void rage::rmcDrawable::ComputeTessellationForShader(u16 shaderIndex)
{
	grmShader* shader = m_ShaderGroup->GetShader(shaderIndex);

	// Tessellation is ON if 'usetessellation' shader param value is 1.0
	fxHandle_t useTessellationHandle = shader->GetEffect()->LookupVarByHashKey(atStringHash("useTessellation"));
	bool useTessellation = shader->GetVar(useTessellationHandle)->GetValue<float>() == 1.0f;

	// Update draw bucket in shader
	shader->SetTessellated(useTessellation);

	// Sort geometries in models where at least one geometry uses this material
	for (int i = 0; i < LOD_COUNT; i++)
	{
		rmcLod* lod = m_LodGroup.GetLod(i);
		for (auto& model : lod->GetModels())
		{
			// Look up if any geometry in model uses selected material
			bool modelAffected = false;
			for (u16 k = 0; k < model->GetGeometryCount(); k++)
			{
				if (model->GetMaterialIndex(k) == shaderIndex)
				{
					modelAffected = true;
					break;
				}
			}

			if (modelAffected)
			{
				model->RecomputeTessellationRenderFlag();
				model->SortForTessellation(m_ShaderGroup.Get());
			}
		}
	}

	// Now we have to recompute bucket mask because tessellation affects it
	m_LodGroup.ComputeBucketMask(m_ShaderGroup.Get());
}

void rage::rmcDrawable::ComputeBucketMask()
{
	m_LodGroup.ComputeBucketMask(m_ShaderGroup.Get());
}

void rage::rmcDrawable::SortForTessellation() const
{
	m_LodGroup.SortForTessellation(m_ShaderGroup.Get());
}

void rage::rmcDrawable::Delete()
{
	delete this;
}

void rage::rmcDrawable::Draw(const Mat34V& mtx, grcRenderMask mask, eDrawableLod lod)
{
	static auto rmcLodGroup_DrawSingle = gmAddress::Scan("48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 41 54 41 55 41 56 41 57 48 83 EC 30 48 63 B4")
		.ToFunc<void(rmcLodGroup*, grmShaderGroup*, const Mat34V&, grcRenderMask, eDrawableLod)>();
	rmcLodGroup_DrawSingle(&m_LodGroup, m_ShaderGroup.Get(), mtx, mask, lod);
	//m_LodGroup.DrawSingle(m_ShaderGroup.Get(), mtx, mask, lod);
}

void rage::rmcDrawable::DrawSkinned(const Mat34V& mtx, u64 mtxSet, grcRenderMask mask, eDrawableLod lod)
{
	struct grmMatrixSet
	{
		u32 dword0;
		u64 qword8;
		u8 byte10;
		u8 m_MatrixCount;
		char pad[8];
		u64 m_Matrices;
	};
	grmMatrixSet* set = (grmMatrixSet*)mtxSet;

	static auto rmcLodGroup_DrawMulti = gmAddress::Scan("48 8B C4 48 89 58 08 48 89 68 10 48 89 70 20 4C 89 40 18 57 41 54 41 55 41 56 41 57 48 83 EC 70 48")
		.ToFunc<void(rmcLodGroup*, grmShaderGroup*, const Mat34V&, u64, grcRenderMask, eDrawableLod)>();
	rmcLodGroup_DrawMulti(&m_LodGroup, m_ShaderGroup.Get(), mtx, mtxSet, mask, lod);
}

u32 rage::rmcDrawable::GetBucketMask(int lod)
{
	return m_LodGroup.GetBucketMask(lod);
}
