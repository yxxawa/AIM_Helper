    bool targetLockConfigCompatible(const TargetLock& candidate) const {
        if (!target_lock_initialized || !candidate.selected) {
            return false;
        }
        if (target_lock_camp_id >= 0 && ClassCampId(candidate.selected->class_id, cfg) != target_lock_camp_id) {
            return false;
        }
        if (!target_lock_part.empty() && target_lock_part != "custom" && candidate.part != target_lock_part) {
            const bool head_first = cfg.auto_target_part
                && cfg.target_entity_priority == "head";
            const bool body_first = cfg.auto_target_part
                && cfg.target_entity_priority != "head"
                && cfg.aim_part_priority == "other";
            const bool upgrade_to_head = head_first
                && target_lock_part == "other"
                && candidate.part == "head";
            const bool fallback_to_body = head_first
                && target_lock_part == "head"
                && candidate.part == "other";
            const bool upgrade_to_body = body_first
                && target_lock_part == "head"
                && candidate.part == "other";
            const bool fallback_to_head = body_first
                && target_lock_part == "other"
                && candidate.part == "head";
            if (!upgrade_to_head && !fallback_to_body && !upgrade_to_body && !fallback_to_head) {
                return false;
            }
        }
        return true;
    }

    bool targetEntityCompatible(const TargetEntity& entity) const {
        if (!target_lock_initialized) {
            return false;
        }
        if (target_lock_camp_id >= 0 && entity.camp_id != target_lock_camp_id) {
            return false;
        }

        const cv::Rect reference = (target_lock_entity_box.width > 0 && target_lock_entity_box.height > 0)
            ? target_lock_entity_box
            : target_lock_box;
        if (reference.width <= 0 || reference.height <= 0 || entity.bounds.width <= 0 || entity.bounds.height <= 0) {
            return false;
        }

        const cv::Point entity_center = BoxCenter(entity.bounds);
        const cv::Point reference_center = BoxCenter(reference);
        const double dist = std::hypot(
            static_cast<double>(entity_center.x - reference_center.x),
            static_cast<double>(entity_center.y - reference_center.y));
        const double diag = std::hypot(
            static_cast<double>(std::max(entity.bounds.width, reference.width)),
            static_cast<double>(std::max(entity.bounds.height, reference.height)));
        const double threshold = std::clamp(diag * 1.65, 28.0, 150.0);
        const double iou = RectIoU(entity.bounds, reference);
        bool plausible = dist <= threshold || iou >= 0.04;

        if (!plausible && target_lock_part == "head" && entity.body) {
            const cv::Point body_top(
                entity.body->box.x + entity.body->box.width / 2,
                entity.body->box.y);
            const double body_top_dist = std::hypot(
                static_cast<double>(body_top.x - target_lock_point.x),
                static_cast<double>(body_top.y - target_lock_point.y));
            const double body_diag = std::hypot(
                static_cast<double>(std::max(1, entity.body->box.width)),
                static_cast<double>(std::max(1, entity.body->box.height)));
            plausible = body_top_dist <= std::clamp(body_diag * 0.60, 18.0, 95.0);
        }

        return plausible;
    }

    bool isFallenTargetRemnant(const TargetEntity& entity) const {
        if (!cfg.enable_fallen_target_filter) {
            return false;
        }
        const double aspect = static_cast<double>(entity.bounds.width) / static_cast<double>(std::max(1, entity.bounds.height));
        const int crop_bottom_margin = std::max(18, cfg.crop_size / 10);
        const bool near_crop_bottom = entity.bounds.y + entity.bounds.height >= cfg.crop_size - crop_bottom_margin;
        const bool body_only = entity.body && !entity.head;

        if (!target_lock_initialized) {
            const bool extremely_flat = aspect > 1.55 && entity.bounds.height <= std::max(18, cfg.crop_size / 5);
            return body_only && extremely_flat && near_crop_bottom;
        }

        const cv::Rect reference = (target_lock_entity_box.width > 0 && target_lock_entity_box.height > 0)
            ? target_lock_entity_box
            : target_lock_box;
        if (reference.width <= 0 || reference.height <= 0 || entity.bounds.width <= 0 || entity.bounds.height <= 0) {
            return false;
        }

        const cv::Point current_center = BoxCenter(entity.bounds);
        const cv::Point reference_center = BoxCenter(reference);
        const double downward_delta = static_cast<double>(current_center.y - reference_center.y);
        const double downward_threshold = std::max(10.0, static_cast<double>(reference.height) * 0.15);
        if (downward_delta <= downward_threshold) {
            return false;
        }

        const int current_bottom = entity.bounds.y + entity.bounds.height;
        const int reference_bottom = reference.y + reference.height;
        const double bottom_delta = static_cast<double>(current_bottom - reference_bottom);
        const double bottom_threshold = std::max(8.0, static_cast<double>(reference.height) * 0.10);
        if (bottom_delta <= bottom_threshold) {
            return false;
        }

        const double height_ratio = static_cast<double>(entity.bounds.height) / static_cast<double>(std::max(1, reference.height));
        const double width_ratio = static_cast<double>(entity.bounds.width) / static_cast<double>(std::max(1, reference.width));
        const double area_ratio = static_cast<double>(std::max(1, entity.bounds.area()))
            / static_cast<double>(std::max(1, reference.area()));
        const double horizontal_delta = std::abs(current_center.x - reference_center.x);
        const double horizontal_limit = std::max(20.0, static_cast<double>(reference.width) * 0.90);
        if (horizontal_delta > horizontal_limit) {
            return false;
        }

        const bool flattened_or_shrunk =
            aspect > 1.00
            || height_ratio < 0.88
            || area_ratio < 0.86
            || width_ratio > 1.08;

        bool head_lost_or_dropped = false;
        if (target_lock_part == "head") {
            if (!entity.head) {
                head_lost_or_dropped = true;
            }
            else {
                const cv::Point current_head_center = BoxCenter(entity.head->box);
                const double head_drop = static_cast<double>(current_head_center.y - target_lock_point.y);
                const double head_drop_threshold = std::max(12.0, static_cast<double>(reference.height) * 0.18);
                head_lost_or_dropped = head_drop > head_drop_threshold;
            }
        }

        if (body_only && (flattened_or_shrunk || near_crop_bottom)) {
            return true;
        }

        if (head_lost_or_dropped && (flattened_or_shrunk || near_crop_bottom || body_only)) {
            return true;
        }

        return false;
    }

    double targetEntityLockScore(const TargetEntity& entity, const Detection* selected) const {
        const cv::Rect reference = (target_lock_entity_box.width > 0 && target_lock_entity_box.height > 0)
            ? target_lock_entity_box
            : target_lock_box;
        const cv::Point entity_center = BoxCenter(entity.bounds);
        const cv::Point reference_center = BoxCenter(reference);
        const double entity_dist = std::hypot(
            static_cast<double>(entity_center.x - reference_center.x),
            static_cast<double>(entity_center.y - reference_center.y));
        const double entity_iou = RectIoU(entity.bounds, reference);
        const double entity_size_change = std::abs(entity.bounds.area() - reference.area())
            / static_cast<double>(std::max(1, reference.area()));
        const cv::Point selected_center = BoxCenter(selected->box);
        const double selected_dist = std::hypot(
            static_cast<double>(selected_center.x - target_lock_point.x),
            static_cast<double>(selected_center.y - target_lock_point.y));

        double score = entity_dist * 1.20 + selected_dist * 0.20 + entity_size_change * 18.0 - entity_iou * 35.0;
        const bool head_first = cfg.auto_target_part
            && cfg.target_entity_priority == "head";
        const bool body_first = cfg.auto_target_part
            && cfg.target_entity_priority != "head"
            && cfg.aim_part_priority == "other";
        if (head_first) {
            if (target_lock_part == "head" && PartNameForDetection(selected) == "other") {
                score += 8.0;
            }
            else if (target_lock_part == "other" && PartNameForDetection(selected) == "head") {
                score -= 10.0;
            }
        }
        if (body_first) {
            if (target_lock_part == "other" && PartNameForDetection(selected) == "head") {
                score += 8.0;
            }
            else if (target_lock_part == "head" && PartNameForDetection(selected) == "other") {
                score -= 10.0;
            }
        }
        return score;
    }

    TargetLock findExistingLockedTarget(const std::vector<TargetEntity>& entities) const {
        TargetLock best;
        double best_score = std::numeric_limits<double>::max();
        for (const TargetEntity& entity : entities) {
            if (isFallenTargetRemnant(entity)) {
                continue;
            }
            if (!targetEntityCompatible(entity)) {
                continue;
            }
            const Detection* selected = preferredLockedPart(entity);
            if (!selected) {
                continue;
            }
            TargetLock candidate = cfg.auto_target_part
                ? makeSelectedPartLock(entity, selected)
                : makeTargetLock(entity);
            if (!candidate.valid || !targetLockConfigCompatible(candidate)) {
                continue;
            }
            const double score = targetEntityLockScore(entity, selected);
            if (score < best_score) {
                best_score = score;
                best = candidate;
            }
        }
        return best;
    }

    void resetTargetLock() {
        target_lock_initialized = false;
        target_lock_missed_frames = 0;
        target_lock_point = cv::Point();
        target_lock_box = cv::Rect();
        target_lock_entity_box = cv::Rect();
        target_lock_part.clear();
        target_lock_class_id = -1;
        target_lock_camp_id = -1;
    }

    void rememberTargetLock(const TargetLock& target) {
        if (!target.valid || !target.selected) {
            resetTargetLock();
            return;
        }
        target_lock_initialized = true;
        target_lock_missed_frames = 0;
        target_lock_point = BoxCenter(target.selected->box);
        target_lock_box = target.selected->box;
        target_lock_entity_box = (target.entity_box.width > 0 && target.entity_box.height > 0)
            ? target.entity_box
            : target.box;
        target_lock_part = target.part;
        target_lock_class_id = target.selected->class_id;
        target_lock_camp_id = ClassCampId(target.selected->class_id, cfg);
    }
