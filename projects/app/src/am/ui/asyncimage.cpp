#include "asyncimage.h"
#include "am/graphics/image/bc.h"

rageam::ui::AsyncImage::~AsyncImage()
{
	if (m_LoadTask)
	{
		m_LoadTask->Wait();
	}
}

bool rageam::ui::AsyncImage::Load(const file::WPath& path, int maxResolution)
{
	graphics::ImagePtr image;

	// ICOs are small and fast to load
	if (graphics::ImageFactory::GetImageKindFromPath(path) != graphics::ImageKind_ICO)
	{
		// First load the metadata (so while image is still loading we at least can provide exact size)
		image = graphics::ImageFactory::LoadFromPath(path, true);
		if (!image)
			return false;

		m_ImageInfo = image->GetInfo();
	}

	int oldPreferred = graphics::ImageFactory::tl_ImagePreferredIcoResolution;
	graphics::ImageFactory::tl_ImagePreferredIcoResolution = maxResolution;
	graphics::CompressedImageInfo encodedInfo;
	// Now we can do expensive loading
	{
		graphics::ImageCompressorOptions compOptions = {};
		compOptions.Format = graphics::BlockFormat_None; // RGBA
		compOptions.MaxResolution = maxResolution;
		// We don't generate mip maps right now because CreateDX11Resource might convert BC to RGBA
		// for non-power-of-two size and pad+generate mips
		compOptions.GenerateMipMaps = false;
		compOptions.PadToPowerOfTwo = false;
		// We use compressor with RGBA because .DDS loading is extremely fast
		image = graphics::ImageFactory::LoadFromPathAndCompress(path, compOptions, &encodedInfo);
	}
	graphics::ImageFactory::tl_ImagePreferredIcoResolution = oldPreferred;

	if (!image)
		return false;

	Vec2S uv2;

	amComPtr<ID3D11ShaderResourceView> view;
	graphics::ImageDX11ResourceOptions resourceOptions = {};
	resourceOptions.CreateMips = true;
	resourceOptions.PadToPowerOfTwo = true;
	if (!image->CreateDX11Resource(view, resourceOptions, &uv2))
		return false;

	std::unique_lock lock(m_LoadMutex);
	m_ImageView = std::move(view);
	m_ImageUV2 = ImVec2(uv2.X, uv2.Y);
	m_ImageInfo = image->GetInfo();

	return true;
}

void rageam::ui::AsyncImage::LoadAsync(const file::WPath& path, int maxResolution)
{
	m_LoadTask = BackgroundWorker::Run([=, this]
		{
			return Load(path, maxResolution);
		}, L"UI Image %ls", path.GetCStr());
}

void rageam::ui::AsyncImage::Render(float width, float height) const
{
	// To prevent layout issues while it's loading, maybe replace it on some loading indicator?
	if (IsLoading() || !m_ImageView)
	{
		ImGui::Dummy(ImVec2(width, height));
		return;
	}

	ImGui::Image(ImTextureID(m_ImageView.Get()), ImVec2(width, height), ImVec2(0, 0), m_ImageUV2);
}

void rageam::ui::AsyncImage::Render(int maxSize) const
{
	if (IsLoading())
		return;

	int width, height;
	graphics::ImageFitInRect(m_ImageInfo.Width, m_ImageInfo.Height, maxSize, width, height);

	Render(static_cast<float>(width), static_cast<float>(height));
}

ImTextureID rageam::ui::AsyncImage::GetID() const
{
	if (IsLoading())
		return ImTextureID(nullptr);
	return ImTextureID(m_ImageView.Get());
}
