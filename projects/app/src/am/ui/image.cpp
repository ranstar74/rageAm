#include "image.h"

rageam::graphics::ImageDX11ResourceOptions rageam::ui::ImImage::GetResourceOptions(int maxResolution) const
{
	graphics::ImageDX11ResourceOptions resourceOptions = {};
	resourceOptions.MaxResolution = maxResolution;
	resourceOptions.PadToPowerOfTwo = true;
	// We don't need mip maps for SVG because it's rasterized to render size
	resourceOptions.CreateMips = !m_IsSvg;
	resourceOptions.MipFilter = graphics::ResizeFilter_Triangle;
	return resourceOptions;
}

rageam::ui::ImImage::ImageLayer& rageam::ui::ImImage::FindBestMatchLayer(List<ImageLayer>& layers, int width, int height) const
{
	int bestFitIndex = graphics::ImageFindBestResolutionMatch(
		layers.GetSize(), width, height, [this, &layers](int i, int& outW, int& outH)
		{
			const graphics::ImageInfo& info = layers[i].ImageInfo;
			outW = info.Width;
			outH = info.Height;
		});
	return layers[bestFitIndex];
}

rageam::ui::ImImage::~ImImage()
{
	if (m_LoadTask)
	{
		m_LoadTask->Wait();
	}
}

ImTextureID rageam::ui::ImImage::GetID() const
{
	std::unique_lock lock(m_Mutex);
	return ImTextureID(m_LargestLayer.ImageView.Get());
}

bool rageam::ui::ImImage::LoadInternal(const file::WPath& path, int maxResolution)
{
	m_LayersPending.Clear();
	m_LoadedPath = path;

	List<graphics::ImagePtr> images;
	images.Reserve(1);

	graphics::ImageFileKind imageKind = graphics::ImageFactory::GetImageKindFromPath(path);

	// ICOs are small and fast to load, for PNGs we first load meta info
	if (imageKind == graphics::ImageKind_ICO)
	{
		if (!graphics::ImageFactory::LoadIco(path, images))
			return false;

		if (!images.Any())
		{
			AM_ERRF(L"AsyncImage::Load() -> No icons loaded from '%ls'", path.GetCStr());
			return false;
		}
	}
	else if (imageKind == graphics::ImageKind_SVG)
	{
		m_IsSvg = true;
		// Nothing to rasterize yet, first call
		if (m_LastRenderWidth == 0 || m_LastRenderHeight == 0)
			return true;

		graphics::ImagePtr image = graphics::ImageFactory::LoadSvg(path, m_LastRenderWidth, m_LastRenderHeight);
		if (!image)
			return false;

		images.Emplace(std::move(image));
	}
	else
	{
		// First load the metadata (so while image is still loading we at least can provide exact size)
		graphics::ImagePtr metaImage = graphics::ImageFactory::LoadFromPath(path, true);
		if (!metaImage)
			return false;

		m_LargestLayer.ImageInfo = metaImage->GetInfo();

		graphics::ImagePtr image = graphics::ImageFactory::LoadFromPath(path);
		if (!image)
			return false;

		images.Emplace(std::move(image));
	}

	graphics::ImageDX11ResourceOptions resourceOptions = GetResourceOptions(maxResolution);
	for (const graphics::ImagePtr& image : images)
	{
		Vec2S uv2;
		amComPtr<ID3D11ShaderResourceView> view;
		if (!image->CreateDX11Resource(view, resourceOptions, &uv2))
			return false;

		std::unique_lock lock(m_Mutex);
		ImageLayer& layer = m_LayersPending.Construct();
		layer.ImageView = std::move(view);
		layer.ImageUV2 = ImVec2(uv2.X, uv2.Y);
		layer.ImageInfo = image->GetInfo();
	}

	int maxRes = graphics::IMAGE_MAX_RESOLUTION;
	m_LargestLayer = FindBestMatchLayer(m_LayersPending, maxRes, maxRes);
	m_HasPending = true;

	return true;
}

void rageam::ui::ImImage::Load(const file::WPath& path, int maxResolution)
{
	m_LoadTask = BackgroundWorker::Run([=, this]
		{
			m_IsLoading = true;
			bool success = LoadInternal(path, maxResolution);
			m_FailedToLoad = !success;
			m_IsLoading = false;
			return success;
		}, L"UI Image %ls", path.GetCStr());
}

void rageam::ui::ImImage::Set(const graphics::ImagePtr& image, int maxResolution)
{
	m_IsSvg = false;
	m_LayersPending.Clear();
	m_LargestLayer = {};
	m_LoadedPath = L"";
	graphics::ImageDX11ResourceOptions resourceOptions = GetResourceOptions(maxResolution);
	Vec2S uv2;
	amComPtr<ID3D11ShaderResourceView> view;
	if (!image->CreateDX11Resource(view, resourceOptions, &uv2))
	{
		m_FailedToLoad = true;
		return;
	}
	std::unique_lock lock(m_Mutex);
	ImageLayer& layer = m_LayersPending.Construct();
	layer.ImageView = std::move(view);
	layer.ImageUV2 = ImVec2(uv2.X, uv2.Y);
	layer.ImageInfo = image->GetInfo();
	int maxRes = graphics::IMAGE_MAX_RESOLUTION;
	m_LargestLayer = FindBestMatchLayer(m_LayersPending, maxRes, maxRes);
	m_FailedToLoad = false;
	m_HasPending = true;
}

void rageam::ui::ImImage::Render(float width, float height)
{
	int widthI = static_cast<int>(width);
	int heightI = static_cast<int>(height);

	bool needRasterizeSvg = m_IsSvg && (m_LastRenderWidth != widthI || m_LastRenderHeight != heightI);
	m_LastRenderWidth = widthI;
	m_LastRenderHeight = heightI;
	if (needRasterizeSvg)
	{
		// We don't use async loading because it causes awful looking flickering
		LoadInternal(m_LoadedPath);
	}

	// Accept loaded image...
	if (m_HasPending)
	{
		m_Layers = std::move(m_LayersPending);
		m_HasPending = false;
	}

	ID3D11ShaderResourceView* resourceView = nullptr;
	ImVec2 uv2 = { 1.0f, 1.0f };
	std::unique_lock lock(m_Mutex);
	if (m_Layers.Any())
	{
		const ImageLayer& bestLayer = FindBestMatchLayer(m_Layers, widthI, heightI);
		resourceView = bestLayer.ImageView.Get();
		uv2 = bestLayer.ImageUV2;
	}

	ImGui::Image(ImTextureID(resourceView), ImVec2(width, height), ImVec2(0, 0), uv2);
}

void rageam::ui::ImImage::Render(int maxSize)
{
	int width, height;
	graphics::ImageFitInRect(
		m_LargestLayer.ImageInfo.Width, m_LargestLayer.ImageInfo.Height, maxSize, width, height);

	Render(static_cast<float>(width), static_cast<float>(height));
}
