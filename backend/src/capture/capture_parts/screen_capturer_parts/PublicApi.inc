    ScreenCapturer(int crop_width, int crop_height) {
        std::cout << "--- Initializing D3D for screen capture... ---" << std::endl;
        HRESULT hr;

        hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&pFactory);
        if (FAILED(hr)) throw std::runtime_error("Failed to create DXGI Factory.");

        DXGI_OUTPUT_DESC outputDesc{};
        SelectPrimaryOutput(outputDesc);
        width = outputDesc.DesktopCoordinates.right - outputDesc.DesktopCoordinates.left;
        height = outputDesc.DesktopCoordinates.bottom - outputDesc.DesktopCoordinates.top;

        if (FAILED(D3D11CreateDevice(pAdapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &pDevice, nullptr, &pContext))) {
            throw std::runtime_error("Failed to create D3D11 device.");
        }
        if (FAILED(pOutput->QueryInterface(__uuidof(IDXGIOutput1), (void**)&pOutput1))) {
            throw std::runtime_error("Failed to query IDXGIOutput1.");
        }
        if (FAILED(pOutput1->DuplicateOutput(pDevice, &pDuplicator))) {
            throw std::runtime_error("Failed to create output duplication.");
        }

        D3D11_TEXTURE2D_DESC desc;
        desc.Width = crop_width;
        desc.Height = crop_height;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Usage = D3D11_USAGE_STAGING;
        desc.BindFlags = 0;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        desc.MiscFlags = 0;

        hr = pDevice->CreateTexture2D(&desc, NULL, &m_pStagingTexture);
        if (FAILED(hr)) {
            throw std::runtime_error("Failed to create staging texture for cropping.");
        }
        std::cout << "--- Screen capture initialized successfully (" << width << "x" << height
            << "), desktop_rect=(" << outputDesc.DesktopCoordinates.left << ","
            << outputDesc.DesktopCoordinates.top << ")-("
            << outputDesc.DesktopCoordinates.right << ","
            << outputDesc.DesktopCoordinates.bottom << ") ---" << std::endl;
    }

    ~ScreenCapturer() {
        std::cout << "--- Cleaning up D3D resources... ---" << std::endl;
        SafeRelease(&m_pStagingTexture);
        SafeRelease(&pDuplicator);
        SafeRelease(&pOutput1);
        SafeRelease(&pOutput);
        SafeRelease(&pAdapter);
        SafeRelease(&pFactory);
        SafeRelease(&pContext);
        SafeRelease(&pDevice);
    }

    ScreenCapturer(const ScreenCapturer&) = delete;
    ScreenCapturer& operator=(const ScreenCapturer&) = delete;

    bool CaptureFrame(cv::Mat& frame, const cv::Rect& crop_region) {
        IDXGIResource* pDesktopResource = nullptr;
        DXGI_OUTDUPL_FRAME_INFO frameInfo;
        HRESULT hr = pDuplicator->AcquireNextFrame(16, &frameInfo, &pDesktopResource);

        if (hr == DXGI_ERROR_WAIT_TIMEOUT) return false;
        if (FAILED(hr)) {
            std::cerr << "AcquireNextFrame failed. HRESULT: 0x" << std::hex << hr << std::endl;
            return false;
        }

        ID3D11Texture2D* pAcquiredDesktopImage = nullptr;
        hr = pDesktopResource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&pAcquiredDesktopImage);
        SafeRelease(&pDesktopResource);
        if (FAILED(hr)) {
            pDuplicator->ReleaseFrame();
            return false;
        }

        D3D11_BOX sourceRegion;
        sourceRegion.left = crop_region.x;
        sourceRegion.right = crop_region.x + crop_region.width;
        sourceRegion.top = crop_region.y;
        sourceRegion.bottom = crop_region.y + crop_region.height;
        sourceRegion.front = 0;
        sourceRegion.back = 1;

        pContext->CopySubresourceRegion(m_pStagingTexture, 0, 0, 0, 0, pAcquiredDesktopImage, 0, &sourceRegion);

        D3D11_MAPPED_SUBRESOURCE mappedResource;
        hr = pContext->Map(m_pStagingTexture, 0, D3D11_MAP_READ, 0, &mappedResource);
        if (FAILED(hr)) {
            SafeRelease(&pAcquiredDesktopImage);
            pDuplicator->ReleaseFrame();
            return false;
        }

        cv::Mat bgra_frame(crop_region.height, crop_region.width, CV_8UC4, mappedResource.pData, mappedResource.RowPitch);
        cv::cvtColor(bgra_frame, frame, cv::COLOR_BGRA2BGR);

        pContext->Unmap(m_pStagingTexture, 0);
        SafeRelease(&pAcquiredDesktopImage);
        pDuplicator->ReleaseFrame();
        return true;
    }

    int getWidth() const { return width; }
    int getHeight() const { return height; }
