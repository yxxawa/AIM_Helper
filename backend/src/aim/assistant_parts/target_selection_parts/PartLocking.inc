    TargetLock makeSelectedPartLock(const TargetEntity& entity, const Detection* selected) const {
        TargetLock lock;
        lock.valid = true;
        lock.head = entity.head;
        lock.body = entity.body;
        lock.entity_box = entity.bounds;

        if (!selected) {
            lock.valid = false;
            return lock;
        }

        lock.selected = selected;
        lock.box = selected->box;
        lock.point = BoxCenter(selected->box);
        lock.measured_point = lock.point;
        lock.part = PartNameForDetection(selected);
        return lock;
    }

    TargetLock makeTargetLock(const TargetEntity& entity) const {
        TargetLock lock;
        lock.valid = true;
        lock.head = entity.head;
        lock.body = entity.body;
        lock.box = entity.bounds;
        lock.entity_box = entity.bounds;

        if (!cfg.auto_target_part) {
            lock.point = cv::Point(
                entity.bounds.x + static_cast<int>(entity.bounds.width * cfg.target_x_ratio),
                entity.bounds.y + static_cast<int>(entity.bounds.height * cfg.target_y_ratio));
            lock.measured_point = lock.point;
            lock.selected = entity.head ? entity.head : entity.body;
            lock.part = "custom";
            return lock;
        }

        if (cfg.target_entity_priority == "head") {
            return makeSelectedPartLock(entity, entity.head ? entity.head : entity.body);
        }

        const Detection* selected = nullptr;
        if (cfg.aim_part_priority == "head") {
            selected = entity.head ? entity.head : entity.body;
        }
        else if (cfg.aim_part_priority == "other") {
            selected = entity.body ? entity.body : entity.head;
        }
        else {
            double best_dist = std::numeric_limits<double>::max();
            for (const Detection* det : { entity.head, entity.body }) {
                if (!det) {
                    continue;
                }
                const cv::Point point = BoxCenter(det->box);
                const double dist = std::hypot(point.x - active_crosshair.x, point.y - active_crosshair.y);
                if (dist < best_dist) {
                    best_dist = dist;
                    selected = det;
                }
            }
        }

        return makeSelectedPartLock(entity, selected);
    }

    const Detection* preferredLockedPart(const TargetEntity& entity) const {
        if (cfg.auto_target_part && cfg.target_entity_priority == "head") {
            return entity.head ? entity.head : entity.body;
        }
        if (cfg.auto_target_part && cfg.aim_part_priority == "head") {
            return entity.head ? entity.head : entity.body;
        }
        if (cfg.auto_target_part && cfg.aim_part_priority == "other") {
            return entity.body ? entity.body : entity.head;
        }
        if (target_lock_part == "head" && entity.head) {
            return entity.head;
        }
        if (target_lock_part == "other" && entity.body) {
            return entity.body;
        }
        for (const Detection* det : { entity.head, entity.body }) {
            if (det && det->class_id == target_lock_class_id) {
                return det;
            }
        }
        return nullptr;
    }
