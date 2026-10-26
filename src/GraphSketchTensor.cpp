// -----------------------------------------------------------------------------
// GraphSketchTensor.cpp
//
// Implements multi-layer Count-Min Sketch (CMSCU) for edge stream anomaly detection.
// -----------------------------------------------------------------------------

#include "GraphSketchTensor.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace GraphSketch {

    // Constructor
    SketchTensorManager::SketchTensorManager(int rows, int cols, float decayFactor, float stepSize)
        : r(rows), c(cols), decay(decayFactor), step(stepSize), historySketch(rows, cols, decayFactor, stepSize) {}

    // Helper: bin timestamp into a 5-minute window
    int getTimeWindow(int timestamp) {
        return timestamp / 300; // 250 best for cic-ids2012 // darpa =40 // ids-18 = 300 AND 2019 too
    }

    // ✅ Add edge to sketch (safe against default construction)
    void SketchTensorManager::addEdge(int src, int dst, int timestamp, int protoID) {
        int timeWindow = getTimeWindow(timestamp);

        auto twIt = tensorSketch.find(timeWindow);
        if (twIt == tensorSketch.end()) {
            // First time window ever seen — construct proto map with the sketch
            std::unordered_map<int, CountMinSketchCMSCU> protoMap;
            protoMap.emplace(protoID, CountMinSketchCMSCU(r, c, decay, step));
            tensorSketch.emplace(timeWindow, std::move(protoMap));
        } else {
            // Time window exists
            auto& protoMap = twIt->second;
            auto protoIt = protoMap.find(protoID);
            if (protoIt == protoMap.end()) {
                protoMap.emplace(protoID, CountMinSketchCMSCU(r, c, decay, step));
            }
            protoMap.at(protoID).addEdge(src, dst);
        }

        historySketch.addEdge(src, dst);
    }

    // Estimate anomaly score safely
    float SketchTensorManager::estimateScore(int src, int dst, int timestamp, int protoID) {
        return estimateAdaptiveScore(src, dst, timestamp, protoID);
    }

    float SketchTensorManager::estimateAdaptiveScore(int src, int dst, int timestamp, int protoID) {
        int timeWindow = getTimeWindow(timestamp);

        auto twIt = tensorSketch.find(timeWindow);
        if (twIt == tensorSketch.end()) return 0.0f;

        auto& protoMap = twIt->second;
        auto protoIt = protoMap.find(protoID);
        if (protoIt == protoMap.end()) return 0.0f;

        int currentCount = protoIt->second.estimate(src, dst);
        int prevCount = 0;
        auto prevTwIt = tensorSketch.find(timeWindow - 1);
        if (prevTwIt != tensorSketch.end()) {
            const auto& prevProtoMap = prevTwIt->second;
            auto prevProtoIt = prevProtoMap.find(protoID);
            if (prevProtoIt != prevProtoMap.end()) {
                prevCount = prevProtoIt->second.estimate(src, dst);
            }
        }

        int historicalCount = historySketch.estimate(src, dst);
        float currentCountFloat = static_cast<float>(currentCount);
        float prevCountFloat = static_cast<float>(prevCount);
        float historicalCountFloat = static_cast<float>(historicalCount);
        float burst = std::max(0.0f, currentCountFloat - prevCountFloat);
        float novelty = (prevCountFloat <= 1.0f && currentCountFloat > 1.0f) ? 1.0f : 0.0f;
        float score = 0.8f * currentCountFloat + 0.6f * burst + 0.4f * novelty + 0.12f * std::log1p(historicalCountFloat);

        return std::min(10.0f, std::max(0.0f, score));
    }

    // Apply decay to all sketches
    void SketchTensorManager::applyDecay() {
        for (auto& [t, protoMap] : tensorSketch) {
            for (auto& [p, sketch] : protoMap) {
                sketch.applyDecay();
            }
        }
        historySketch.applyDecay();
    }

    float SketchTensorManager::estimateTotal(int src, int dst, int protoID) {
        (void)protoID;
        return historySketch.estimate(src, dst);
    }

    // Remove sketches older than N time windows
    void SketchTensorManager::pruneOldWindows(int currentWindow, int keepLastN) {
        std::vector<int> toRemove;
        for (const auto& [window, _] : tensorSketch) {
            if (window < currentWindow - keepLastN) {
                toRemove.push_back(window);
            }
        }

        for (int win : toRemove) {
            tensorSketch.erase(win);
        }
    }

} // namespace GraphSketch
