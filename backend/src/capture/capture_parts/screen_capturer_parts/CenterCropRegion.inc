static cv::Rect CenterCropRegion(int screen_width, int screen_height, int crop_size) {
    return cv::Rect(
        (screen_width - crop_size) / 2,
        (screen_height - crop_size) / 2,
        crop_size,
        crop_size);
}
