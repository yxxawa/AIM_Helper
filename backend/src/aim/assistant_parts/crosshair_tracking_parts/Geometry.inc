    static cv::Point2d BoxCenterPrecise(const cv::Rect& box) {
        return cv::Point2d(
            static_cast<double>(box.x) + static_cast<double>(box.width) * 0.5,
            static_cast<double>(box.y) + static_cast<double>(box.height) * 0.5);
    }

    static cv::Point BoxCenter(const cv::Rect& box) {
        const cv::Point2d center = BoxCenterPrecise(box);
        return cv::Point(
            static_cast<int>(std::lround(center.x)),
            static_cast<int>(std::lround(center.y)));
    }

    static double RectIoU(const cv::Rect& a, const cv::Rect& b) {
        const cv::Rect intersection = a & b;
        const double intersection_area = static_cast<double>(std::max(0, intersection.area()));
        const double union_area = static_cast<double>(std::max(0, a.area()) + std::max(0, b.area())) - intersection_area;
        if (union_area <= 0.0) {
            return 0.0;
        }
        return intersection_area / union_area;
    }
