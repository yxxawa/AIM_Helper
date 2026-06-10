    std::string PartNameForDetection(const Detection* det) const {
        if (!det) {
            return "none";
        }
        return IsHeadClass(det->class_id, cfg) ? "head" : "other";
    }

    bool headMatchesBody(const Detection& head, const Detection& body) const {
        if (ClassCampId(head.class_id, cfg) != ClassCampId(body.class_id, cfg)) {
            return false;
        }

        const cv::Point head_center = BoxCenter(head.box);
        const int x_margin = std::max(6, body.box.width * 35 / 100);
        const int y_above = std::max(8, body.box.height * 65 / 100);
        const int y_below = std::max(8, body.box.height * 70 / 100);
        const bool x_ok = head_center.x >= body.box.x - x_margin
            && head_center.x <= body.box.x + body.box.width + x_margin;
        const bool y_ok = head_center.y >= body.box.y - y_above
            && head_center.y <= body.box.y + y_below;
        return x_ok && y_ok;
    }

    std::vector<TargetEntity> buildTargetEntities() {
        std::vector<TargetEntity> entities;
        std::vector<const Detection*> heads;
        for (const auto& det : detections) {
            if (!DetectionMatchesTargetFilters(det.class_id, cfg)) {
                continue;
            }
            if (IsHeadClass(det.class_id, cfg)) {
                heads.push_back(&det);
                continue;
            }

            TargetEntity entity;
            entity.body = &det;
            entity.bounds = det.box;
            entity.camp_id = ClassCampId(det.class_id, cfg);
            entities.push_back(entity);
        }

        for (const Detection* head : heads) {
            int best_index = -1;
            double best_score = std::numeric_limits<double>::max();
            for (size_t index = 0; index < entities.size(); ++index) {
                TargetEntity& entity = entities[index];
                if (!entity.body || entity.head || entity.camp_id != ClassCampId(head->class_id, cfg)) {
                    continue;
                }
                if (!headMatchesBody(*head, *entity.body)) {
                    continue;
                }
                const cv::Point head_center = BoxCenter(head->box);
                const cv::Point body_top(entity.body->box.x + entity.body->box.width / 2, entity.body->box.y);
                const double score = std::hypot(head_center.x - body_top.x, head_center.y - body_top.y);
                if (score < best_score) {
                    best_score = score;
                    best_index = static_cast<int>(index);
                }
            }

            if (best_index >= 0) {
                TargetEntity& entity = entities[static_cast<size_t>(best_index)];
                entity.head = head;
                entity.bounds = entity.bounds | head->box;
                continue;
            }

            TargetEntity entity;
            entity.head = head;
            entity.bounds = head->box;
            entity.camp_id = ClassCampId(head->class_id, cfg);
            entities.push_back(entity);
        }

        return entities;
    }
