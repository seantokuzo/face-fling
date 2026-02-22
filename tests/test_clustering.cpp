/**
 * Clustering algorithm unit tests.
 */

#include <gtest/gtest.h>
#include "core/Clusterer.h"
#include "services/FaceService.h"
#include <cmath>

using namespace facefling;

class ClusteringTest : public ::testing::Test {
protected:
    // Helper to create a fake embedding
    FaceEmbedding make_embedding(float base_value) {
        FaceEmbedding emb(128);
        for (size_t i = 0; i < 128; ++i) {
            emb[i] = base_value + static_cast<float>(i) * 0.01f;
        }
        return emb;
    }
    
    // Helper to add noise to an embedding
    FaceEmbedding add_noise(const FaceEmbedding& emb, float noise_level) {
        FaceEmbedding result = emb;
        for (size_t i = 0; i < 128; ++i) {
            result[i] += (static_cast<float>(rand()) / RAND_MAX - 0.5f) * noise_level;
        }
        return result;
    }
};

TEST_F(ClusteringTest, EmbeddingDistance_Identical) {
    auto emb = make_embedding(1.0f);
    float dist = FaceService::embedding_distance(emb, emb);
    EXPECT_FLOAT_EQ(dist, 0.0f);
}

TEST_F(ClusteringTest, EmbeddingDistance_Different) {
    auto emb1 = make_embedding(0.0f);
    auto emb2 = make_embedding(1.0f);
    float dist = FaceService::embedding_distance(emb1, emb2);
    EXPECT_GT(dist, 0.0f);
}

TEST_F(ClusteringTest, IsSamePerson_Similar) {
    auto emb1 = make_embedding(1.0f);
    auto emb2 = add_noise(emb1, 0.1f);  // Small noise
    
    float dist = FaceService::embedding_distance(emb1, emb2);
    // With small noise, should be close
    EXPECT_LT(dist, 0.6f);
    EXPECT_TRUE(FaceService::is_same_person(emb1, emb2, 0.6f));
}

TEST_F(ClusteringTest, IsSamePerson_Different) {
    auto emb1 = make_embedding(0.0f);
    auto emb2 = make_embedding(5.0f);  // Very different
    
    EXPECT_FALSE(FaceService::is_same_person(emb1, emb2, 0.6f));
}

TEST_F(ClusteringTest, ThresholdAffectsClustering) {
    auto emb1 = make_embedding(0.0f);
    auto emb2 = add_noise(emb1, 0.3f);
    
    float dist = FaceService::embedding_distance(emb1, emb2);
    
    // Should cluster together with high threshold
    EXPECT_TRUE(FaceService::is_same_person(emb1, emb2, dist + 0.1f));
    
    // Should NOT cluster with low threshold
    EXPECT_FALSE(FaceService::is_same_person(emb1, emb2, dist - 0.1f));
}

// TODO: Add more tests once Clusterer is implemented
// - Test cluster_all with mock database
// - Test merge operation
// - Test split operation
// - Test centroid computation
