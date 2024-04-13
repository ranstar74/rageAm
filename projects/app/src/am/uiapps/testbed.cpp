#include "testbed.h"

#include "am/asset/factory.h"
#include "am/asset/ui/assetwindowfactory.h"
#include "am/graphics/geomprimitives.h"
#include "am/graphics/image/imagecache.h"
#include "am/integration/memory/hook.h"
#include "am/ui/imglue.h"
#include "am/ui/slwidgets.h"
#include "helpers/format.h"
#include "rage/physics/bounds/optimizedbvh.h"
#include "am/graphics/scene.h"

#ifdef AM_INTEGRATED
#include "am/integration/ui/modelscene.h"
#include "am/integration/ui/starbar.h"
#include "am/integration/ui/modelinspector.h"
#include "rage/paging/builder/builder.h"
#endif

#define QUICK_ITD_PATH L"X:/am.ws/am.itd"

#ifdef AM_INTEGRATED
#define QUICK_SCENE_PATH L"X:/am.ws/airpump.idr"
#define QUICK_YDR_PATH L"X:/am.ws/airpump.ydr"
#define QUICK_YDR_PATH_HS "X:/am.ws/airpump.ydr"
#endif

void rageam::ui::TestbedApp::OnStart()
{
#ifdef AM_INTEGRATED
	// Scene::ConstructFor(L"D:/quick.ws/prop_towercrane_02a.ydr");

	// Collision tests
	Scene::DefaultSpawnPosition = Vec3V(-4.1, 76.1, 7.4);
	//Scene::DefaultSpawnPosition = Vec3V(-1234.7, -1135.6, 7);
	GetUI()->FindAppByType<integration::StarBar>()->FocusCameraOnScene = false;

	Scene::ConstructFor(L"D:/quick.ws/cols.idr");
	// Scene::ConstructFor(L"D:/quick.ws/prop_towercrane_02a.ydr");
#endif
}

void rageam::ui::TestbedApp::OnRender()
{
	ImGui::Begin("rageAm Testbed");

	ImGlue* ui = GetUI();
	ImGui::SetNextItemWidth(ImGui::GetFontSize() * 4.0f);
	ImGui::SliderInt("Font Size", &ui->FontSize, 8, 24);

	if (ImGui::Button("Primitive Detection"))
	{
		graphics::ScenePtr scene = graphics::SceneFactory::LoadFrom(L"C:/Users/falco/Desktop/spheres.glb");
		Timer timer = Timer::StartNew();
		u16 nodeCount = scene->GetNodeCount();
		for (u16 i = 0; i < nodeCount; i++)
		{
			graphics::SceneNode* node = scene->GetNode(i);
			graphics::SceneMesh* mesh = node->GetMesh();
			if (!mesh)
				continue;

			u16 geomCount = mesh->GetGeometriesCount();
			for (u16 k = 0; k < geomCount; k++)
			{
				graphics::SceneGeometry* geom = mesh->GetGeometry(k);
				graphics::SceneData positions;
				graphics::SceneData indices;
				geom->GetAttribute(positions, graphics::POSITION, 0);
				geom->GetIndices(indices);

				graphics::Primitive primitive;
				// MatchPrimitive(positions.GetBufferAs<Vec3S>(), geom->GetVertexCount(), indices.GetBufferAs<u16>(), geom->GetIndexCount(), primitive);

				AM_TRACEF("%s:%i -> %s", node->GetName(), k, Enum::GetName(primitive.Type));
			}
		}
		timer.Stop();

		AM_TRACEF("Match Primitive elapsed %llu micros", timer.GetElapsedMicroseconds());

		int b = 0;
	}

	if (ImGui::Button("BVH"))
	{
		rage::phBvhPrimitiveData primitives[3] = {};
		rage::phOptimizedBvh bvh;
		//bvh.SetExtents(rage::spdAABB(Vec3V(-15, 0, 0), Vec3V(0, 0, 0)));
		bvh.SetExtents(rage::spdAABB(Vec3V(0, -15, 0), Vec3V(0, 0, 0)));
		{
			for (int i = 0; i < 3; i++)
				primitives[i].PrimitiveIndex = i;

	/*		bvh.QuantizeMin(primitives[0].AABBMin, Vec3V(-10, 0, 0));
			bvh.QuantizeMax(primitives[0].AABBMax, Vec3V(-5, 0, 0));
			bvh.QuantizeMin(primitives[1].AABBMin, Vec3V(-15, 0, 0));
			bvh.QuantizeMax(primitives[1].AABBMax, Vec3V(-12, 0, 0));
			bvh.QuantizeMin(primitives[2].AABBMin, Vec3V(-2, 0, 0));
			bvh.QuantizeMax(primitives[2].AABBMax, Vec3V(-0, 0, 0));*/
			bvh.QuantizeMin(primitives[0].AABBMin, Vec3V(0, -10, 0));
			bvh.QuantizeMax(primitives[0].AABBMax, Vec3V(0, -5, 0));
			bvh.QuantizeMin(primitives[1].AABBMin, Vec3V(0, -15, 0));
			bvh.QuantizeMax(primitives[1].AABBMax, Vec3V(0, -12, 0));
			bvh.QuantizeMin(primitives[2].AABBMin, Vec3V(0, -2, 0));
			bvh.QuantizeMax(primitives[2].AABBMax, Vec3V(0, 0, 0));
		}
		bvh.BuildFromPrimitiveData(primitives, std::size(primitives), 2);
		bvh.PrintInfo();
	}

#ifdef AM_INTEGRATED
	SlGui::CategoryText("YDR Export");

	if (ImGui::Button("Compile IDR"))
	{
		auto dr = asset::AssetFactory::LoadFromPath<asset::DrawableAsset>(QUICK_SCENE_PATH);
		if (dr) dr->CompileToFile();
	}

	if (ImGui::Button("Build YDR"))
	{
		gtaDrawable* drawable = new gtaDrawable();
		//rage::pgRscBuilder::Load(&drawable, "X:/am.ws/v_ind_cfbucket.ydr", 165);
		rage::pgRscBuilder::Load(&drawable, QUICK_YDR_PATH_HS, 165);
		//delete drawable;
	}

	if (ImGui::Button("Inspect YDR"))
	{
		Scene::ConstructFor(QUICK_YDR_PATH);
	}

	SlGui::CategoryText("Model Scene");

	bool loadedScene = false;
	if (ImGui::Button("Load IDR"))
	{
		Scene::ConstructFor(QUICK_SCENE_PATH);
		loadedScene = true;
	}

	if (ImGui::Button("Load IDR + Flush Cache"))
	{
		graphics::ImageCache::GetInstance()->Clear();
		Scene::ConstructFor(QUICK_SCENE_PATH);
		loadedScene = true;
	}

	if (loadedScene)
	{
		Scene::ConstructFor(QUICK_SCENE_PATH);
	}
#endif

	if (ImGui::Button("ITD"))
	{
		using namespace asset;

		AssetPtr txd = AssetFactory::LoadFromPath(QUICK_ITD_PATH);
		AssetWindowFactory::OpenNewOrFocusExisting(txd);
	}

	if (ImGui::CollapsingHeader("ImageCache", ImGuiTreeNodeFlags_DefaultOpen))
	{
		graphics::ImageCache*     imageCache = graphics::ImageCache::GetInstance();
		graphics::ImageCacheState ics = imageCache->GetState();

		ImGui::BulletText("RAM: %s/%s", FormatSize(ics.SizeRamUsed), FormatSize(ics.SizeRamBudget));
		ImGui::BulletText("FS: %s/%s", FormatSize(ics.SizeFsUsed), FormatSize(ics.SizeFsBudget));
		ImGui::BulletText("Image counts:");
		ImGui::Indent();
		ImGui::Text("- RAM: %u", ics.ImageCountRam);
		ImGui::Text("- FS: %u", ics.ImageCountFs);
		ImGui::Text("- DX11 Views: %u", ics.DX11ViewCount);
		if (ImGui::Button("Clear"))
			imageCache->Clear();
		ImGui::Unindent();
	}

	ImGui::End();
}
