    TargetLock findBestTarget(bool keep_existing_lock) {
        const std::vector<TargetEntity> entities = buildTargetEntities();

        if (!keep_existing_lock) {
            resetTargetLock();
        }

        std::vector<TargetEntity> targetable_entities;
        targetable_entities.reserve(entities.size());
        for (const TargetEntity& entity : entities) {
            if (isFallenTargetRemnant(entity)) {
                continue;
            }
            targetable_entities.push_back(entity);
        }
        last_targetable_detection_count = static_cast<int>(targetable_entities.size());

        const bool use_lock_continuity = keep_existing_lock
            && target_lock_initialized
            && cfg.auto_target_part
            && cfg.target_entity_priority == "head";

        if (use_lock_continuity) {
            TargetLock locked = findExistingLockedTarget(targetable_entities);
            if (locked.valid) {
                rememberTargetLock(locked);
                return locked;
            }
            ++target_lock_missed_frames;
            if (target_lock_missed_frames <= 3) {
                return TargetLock{};
            }
            resetTargetLock();
        }

        auto select_closest_entity = [&]() {
            TargetLock best;
            const TargetEntity* best_entity = nullptr;
            double min_entity_dist = cfg.max_lock_distance_pixels;
            for (const TargetEntity& entity : targetable_entities) {
                if (entity.bounds.width <= 0 || entity.bounds.height <= 0) {
                    continue;
                }
                const cv::Point entity_center = BoxCenter(entity.bounds);
                const double dist = std::hypot(
                    static_cast<double>(entity_center.x - active_crosshair.x),
                    static_cast<double>(entity_center.y - active_crosshair.y));
                if (dist < min_entity_dist) {
                    min_entity_dist = dist;
                    best_entity = &entity;
                }
            }
            if (best_entity) {
                best = makeTargetLock(*best_entity);
            }
            return best;
        };

        auto select_head_first_target = [&]() {
            TargetLock best;
            double min_head_dist = cfg.max_lock_distance_pixels;
            for (const TargetEntity& entity : targetable_entities) {
                if (!entity.head) {
                    continue;
                }
                const cv::Point head_center = BoxCenter(entity.head->box);
                const double dist = std::hypot(
                    static_cast<double>(head_center.x - active_crosshair.x),
                    static_cast<double>(head_center.y - active_crosshair.y));
                if (dist < min_head_dist) {
                    min_head_dist = dist;
                    best = makeSelectedPartLock(entity, entity.head);
                }
            }
            if (!best.valid) {
                best = select_closest_entity();
            }
            return best;
        };

        TargetLock best_target = (cfg.auto_target_part && cfg.target_entity_priority == "head")
            ? select_head_first_target()
            : select_closest_entity();
        if (keep_existing_lock && best_target.valid) {
            rememberTargetLock(best_target);
        }
        return best_target;
    }
