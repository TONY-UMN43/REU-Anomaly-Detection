#include <cassert>
#include <iostream>
#include "../src/GraphSketchTensor.hpp"

int main() {
    GraphSketch::SketchTensorManager manager(4, 200, 0.95f, 1.0f);

    // A historically common edge should get a lower anomaly score than a brand-new edge.
    for (int i = 0; i < 25; ++i) {
        manager.addEdge(10, 20, 100 + i, 0);
    }

    manager.addEdge(1, 2, 200, 0);
    manager.addEdge(10, 20, 200, 0);

    float new_edge_score = manager.estimateAdaptiveScore(1, 2, 200, 0);
    float common_edge_score = manager.estimateAdaptiveScore(10, 20, 200, 0);

    std::cout << "new_edge_score=" << new_edge_score << " common_edge_score=" << common_edge_score << std::endl;
    assert(new_edge_score > common_edge_score);
    return 0;
}
