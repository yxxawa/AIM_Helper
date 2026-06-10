    ObjectDetector(const Config& cfg)
        : env(ORT_LOGGING_LEVEL_WARNING, "Realtime_YOLO_Detector"), session(nullptr) {

        std::cout << "--- Initializing ONNX Runtime and YOLO model... ---" << std::endl;
        Ort::SessionOptions session_options;
        session_options.SetIntraOpNumThreads(1);
        session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

        if (cfg.backend == "tensorrt") {
            if (!cfg.trt_cache_path.empty()) {
                std::filesystem::create_directories(cfg.trt_cache_path);
            }
            OrtTensorRTProviderOptions trt_options{};
            trt_options.device_id = 0;
            trt_options.trt_max_partition_iterations = 1000;
            trt_options.trt_min_subgraph_size = 1;
            trt_options.trt_fp16_enable = 1;
            trt_options.trt_engine_cache_enable = 1;
            trt_options.trt_engine_cache_path = cfg.trt_cache_path.c_str();
            session_options.AppendExecutionProvider_TensorRT(trt_options);

            OrtCUDAProviderOptions cuda_options{};
            cuda_options.device_id = 0;
            session_options.AppendExecutionProvider_CUDA(cuda_options);
            std::cout << "--- Backend: TensorRT with CUDA fallback ---" << std::endl;
            std::cout << "--- TensorRT cache: " << cfg.trt_cache_path << " ---" << std::endl;
        }
        else if (cfg.backend == "cuda") {
            OrtCUDAProviderOptions cuda_options{};
            cuda_options.device_id = 0;
            session_options.AppendExecutionProvider_CUDA(cuda_options);
            std::cout << "--- Backend: CUDA ---" << std::endl;
        }
        else {
            std::cout << "--- Backend: CPU ---" << std::endl;
        }

        session = Ort::Session(env, cfg.model_path.c_str(), session_options);
        Ort::AllocatorWithDefaultOptions allocator;
        auto input_name_ptr = session.GetInputNameAllocated(0, allocator);
        auto output_name_ptr = session.GetOutputNameAllocated(0, allocator);
        input_name = input_name_ptr.get();
        output_name = output_name_ptr.get();

        input_shape = session.GetInputTypeInfo(0).GetTensorTypeAndShapeInfo().GetShape();
        if (input_shape.size() != 4 || input_shape[2] <= 0 || input_shape[3] <= 0) {
            throw std::runtime_error("Unsupported YOLO input shape.");
        }
        input_height = input_shape[2];
        input_width = input_shape[3];
        canvas.create(cv::Size(static_cast<int>(input_width), static_cast<int>(input_height)), CV_8UC3);
        boxes.reserve(1024);
        confidences.reserve(1024);
        class_ids.reserve(1024);
        nms_indices.reserve(256);
        std::cout << "--- Model loaded successfully! ---" << std::endl;
    }

    ObjectDetector(const ObjectDetector&) = delete;
    ObjectDetector& operator=(const ObjectDetector&) = delete;

    void Detect(const cv::Mat& image, std::vector<Detection>& detections, const Config& cfg, TimingDetails& timings) {
        detections.clear();
        if (detections.capacity() < 128) {
            detections.reserve(128);
        }
        auto stage_start = std::chrono::high_resolution_clock::now();

        float ratio_h = static_cast<float>(input_height) / image.rows;
        float ratio_w = static_cast<float>(input_width) / image.cols;
        float ratio = std::min(ratio_h, ratio_w);
        int new_w = static_cast<int>(image.cols * ratio);
        int new_h = static_cast<int>(image.rows * ratio);

        cv::resize(image, resized_img, cv::Size(new_w, new_h));

        canvas.setTo(cv::Scalar(114, 114, 114));
        int paste_x = (static_cast<int>(input_width) - new_w) / 2;
        int paste_y = (static_cast<int>(input_height) - new_h) / 2;
        resized_img.copyTo(canvas(cv::Rect(paste_x, paste_y, new_w, new_h)));

        cv::dnn::blobFromImage(canvas, blob, 1.0 / 255.0, cv::Size(input_width, input_height), cv::Scalar(), true, false);

        auto stage_end_preprocess = std::chrono::high_resolution_clock::now();
        timings.preprocess_ms = std::chrono::duration_cast<std::chrono::microseconds>(stage_end_preprocess - stage_start).count() / 1000.0;

        const char* input_names[] = { input_name.c_str() };
        const char* output_names[] = { output_name.c_str() };
        Ort::Value input_tensor = Ort::Value::CreateTensor<float>(memory_info, blob.ptr<float>(), blob.total(), input_shape.data(), input_shape.size());

        auto output_tensors = session.Run(run_options, input_names, &input_tensor, 1, output_names, 1);
        auto stage_end_inference = std::chrono::high_resolution_clock::now();
        timings.inference_ms = std::chrono::duration_cast<std::chrono::microseconds>(stage_end_inference - stage_end_preprocess).count() / 1000.0;

        if (cfg.use_end_to_end_onnx) {
            const float* output_data = output_tensors.front().GetTensorData<float>();
            const auto& output_shape = output_tensors.front().GetTensorTypeAndShapeInfo().GetShape();
            const int num_detections = static_cast<int>(output_shape[1]);

            for (int i = 0; i < num_detections; ++i) {
                const float confidence = output_data[i * 6 + 4];
                if (confidence >= cfg.confidence_threshold) {
                    const float x1 = output_data[i * 6 + 0];
                    const float y1 = output_data[i * 6 + 1];
                    const float x2 = output_data[i * 6 + 2];
                    const float y2 = output_data[i * 6 + 3];
                    const int class_id = static_cast<int>(output_data[i * 6 + 5]);
                    int left = static_cast<int>((x1 - paste_x) / ratio);
                    int top = static_cast<int>((y1 - paste_y) / ratio);
                    int width = static_cast<int>((x2 - x1) / ratio);
                    int height = static_cast<int>((y2 - y1) / ratio);
                    detections.emplace_back(Detection{ cv::Rect(left, top, width, height), confidence, class_id });
                }
            }
        }
        else {
            const float* output_data = output_tensors.front().GetTensorData<float>();
            const auto& output_shape = output_tensors.front().GetTensorTypeAndShapeInfo().GetShape();
            cv::Mat raw_output(static_cast<int>(output_shape[1]), static_cast<int>(output_shape[2]), CV_32F, (void*)output_data);
            cv::transpose(raw_output, output_transposed);
            const cv::Mat& output_mat = output_transposed;

            boxes.clear();
            confidences.clear();
            class_ids.clear();
            if (boxes.capacity() < static_cast<size_t>(output_mat.rows)) {
                boxes.reserve(output_mat.rows);
                confidences.reserve(output_mat.rows);
                class_ids.reserve(output_mat.rows);
            }
            for (int i = 0; i < output_mat.rows; ++i) {
                cv::Mat classes_scores = output_mat.row(i).colRange(4, output_mat.cols);
                cv::Point class_id_point;
                double max_score;
                cv::minMaxLoc(classes_scores, 0, &max_score, 0, &class_id_point);
                if (max_score > cfg.confidence_threshold) {
                    confidences.push_back(static_cast<float>(max_score));
                    class_ids.push_back(class_id_point.x);
                    float cx = output_mat.at<float>(i, 0);
                    float cy = output_mat.at<float>(i, 1);
                    float w = output_mat.at<float>(i, 2);
                    float h = output_mat.at<float>(i, 3);
                    int left = static_cast<int>((cx - 0.5f * w - paste_x) / ratio);
                    int top = static_cast<int>((cy - 0.5f * h - paste_y) / ratio);
                    int width = static_cast<int>(w / ratio);
                    int height = static_cast<int>(h / ratio);
                    boxes.emplace_back(left, top, width, height);
                }
            }
            nms_indices.clear();
            cv::dnn::NMSBoxes(boxes, confidences, cfg.confidence_threshold, cfg.nms_threshold, nms_indices);
            for (int idx : nms_indices) {
                detections.emplace_back(Detection{ boxes[idx], confidences[idx], class_ids[idx] });
            }
        }
        auto stage_end_postprocess = std::chrono::high_resolution_clock::now();
        timings.postprocess_ms = std::chrono::duration_cast<std::chrono::microseconds>(stage_end_postprocess - stage_end_inference).count() / 1000.0;
    }
