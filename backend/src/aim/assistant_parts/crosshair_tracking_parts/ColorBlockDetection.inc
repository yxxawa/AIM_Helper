    bool findCrosshairColorBlock(cv::Point2d& out_point, int& out_area, double& out_score) {
        if (captured_frame.empty() || captured_frame.channels() != 3) {
            return false;
        }

        const int tolerance = std::clamp(cfg.crosshair_color_tolerance, 0, 255);
        const cv::Scalar lower(
            std::max(0, cfg.crosshair_color_b - tolerance),
            std::max(0, cfg.crosshair_color_g - tolerance),
            std::max(0, cfg.crosshair_color_r - tolerance));
        const cv::Scalar upper(
            std::min(255, cfg.crosshair_color_b + tolerance),
            std::min(255, cfg.crosshair_color_g + tolerance),
            std::min(255, cfg.crosshair_color_r + tolerance));

        cv::inRange(captured_frame, lower, upper, crosshair_mask);
        if (cv::countNonZero(crosshair_mask) == 0) {
            return false;
        }

        cv::Mat labels;
        cv::Mat stats;
        cv::Mat centroids;
        const int component_count = cv::connectedComponentsWithStats(
            crosshair_mask,
            labels,
            stats,
            centroids,
            8,
            CV_32S);

        const int min_area = std::max(1, cfg.crosshair_min_area);
        const int max_area = std::max(min_area, cfg.crosshair_max_area);
        const cv::Point2d reference = crosshair_tracking_initialized
            ? cv::Point2d(static_cast<double>(active_crosshair.x), static_cast<double>(active_crosshair.y))
            : cv::Point2d(static_cast<double>(crop_center.x), static_cast<double>(crop_center.y));

        bool found = false;
        double best_score = std::numeric_limits<double>::max();
        cv::Point2d best_point(0.0, 0.0);
        int best_area = 0;
        for (int label = 1; label < component_count; ++label) {
            const int area = stats.at<int>(label, cv::CC_STAT_AREA);
            if (area < min_area || area > max_area) {
                continue;
            }
            const int width = stats.at<int>(label, cv::CC_STAT_WIDTH);
            const int height = stats.at<int>(label, cv::CC_STAT_HEIGHT);
            if (width <= 0 || height <= 0) {
                continue;
            }
            const double aspect = static_cast<double>(std::max(width, height)) /
                static_cast<double>(std::max(1, std::min(width, height)));
            if (aspect > 18.0) {
                continue;
            }

            const cv::Point2d center(
                centroids.at<double>(label, 0),
                centroids.at<double>(label, 1));
            const double ref_dist = std::hypot(center.x - reference.x, center.y - reference.y);
            const double center_dist = std::hypot(center.x - crop_center.x, center.y - crop_center.y);
            const double area_bonus = std::min(40.0, std::sqrt(static_cast<double>(area)) * 1.8);
            const double aspect_penalty = std::max(0.0, aspect - 1.0) * 3.0;
            const double score = ref_dist * 1.35 + center_dist * 0.25 + aspect_penalty - area_bonus;
            if (score < best_score) {
                best_score = score;
                best_point = center;
                best_area = area;
                found = true;
            }
        }

        if (!found) {
            return false;
        }
        out_point = best_point;
        out_area = best_area;
        out_score = best_score;
        return true;
    }
