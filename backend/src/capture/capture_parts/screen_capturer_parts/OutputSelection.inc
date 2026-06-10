    void SelectPrimaryOutput(DXGI_OUTPUT_DESC& selected_desc) {
        const RECT primary_rect{
            0,
            0,
            ::GetSystemMetrics(SM_CXSCREEN),
            ::GetSystemMetrics(SM_CYSCREEN)
        };
        const POINT primary_center{
            (primary_rect.right - primary_rect.left) / 2,
            (primary_rect.bottom - primary_rect.top) / 2
        };

        IDXGIAdapter1* fallback_adapter = nullptr;
        IDXGIOutput* fallback_output = nullptr;
        DXGI_OUTPUT_DESC fallback_desc{};

        for (UINT adapter_index = 0;; ++adapter_index) {
            IDXGIAdapter1* candidate_adapter = nullptr;
            HRESULT adapter_hr = pFactory->EnumAdapters1(adapter_index, &candidate_adapter);
            if (adapter_hr == DXGI_ERROR_NOT_FOUND) {
                break;
            }
            if (FAILED(adapter_hr) || !candidate_adapter) {
                continue;
            }

            bool adapter_taken = false;
            for (UINT output_index = 0;; ++output_index) {
                IDXGIOutput* candidate_output = nullptr;
                HRESULT output_hr = candidate_adapter->EnumOutputs(output_index, &candidate_output);
                if (output_hr == DXGI_ERROR_NOT_FOUND) {
                    break;
                }
                if (FAILED(output_hr) || !candidate_output) {
                    continue;
                }

                DXGI_OUTPUT_DESC desc{};
                candidate_output->GetDesc(&desc);
                const RECT& rect = desc.DesktopCoordinates;
                const bool contains_primary_center =
                    primary_center.x >= rect.left &&
                    primary_center.x < rect.right &&
                    primary_center.y >= rect.top &&
                    primary_center.y < rect.bottom;

                if (contains_primary_center) {
                    if (fallback_output) {
                        SafeRelease(&fallback_output);
                        SafeRelease(&fallback_adapter);
                    }
                    pAdapter = candidate_adapter;
                    pOutput = candidate_output;
                    selected_desc = desc;
                    adapter_taken = true;
                    std::cout << "--- Primary display selected: "
                        << (primary_rect.right - primary_rect.left) << "x"
                        << (primary_rect.bottom - primary_rect.top) << " ---" << std::endl;
                    return;
                }
                else if (!fallback_output) {
                    fallback_adapter = candidate_adapter;
                    fallback_output = candidate_output;
                    fallback_desc = desc;
                    adapter_taken = true;
                }
                else {
                    SafeRelease(&candidate_output);
                }
            }

            if (!adapter_taken) {
                SafeRelease(&candidate_adapter);
            }
        }

        if (!fallback_output || !fallback_adapter) {
            throw std::runtime_error("Failed to enumerate outputs.");
        }

        pAdapter = fallback_adapter;
        pOutput = fallback_output;
        selected_desc = fallback_desc;
        std::cout << "--- Primary display not matched; using first DXGI output. Windows primary="
            << (primary_rect.right - primary_rect.left) << "x"
            << (primary_rect.bottom - primary_rect.top) << " ---" << std::endl;
    }
