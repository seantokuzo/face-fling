/**
 * Clustering algorithm unit tests.
 * Tests embedding distance calculations and clustering logic.
 */

#include <gtest/gtest.h>
#include <vector>
#include <cmath>
#include <cstdlib>

// Type alias matching the project
using FaceEmbedding = std::vector<float>;

// Local implementation of embedding distance for testing (avoids dlib dependency)
static float embedding_distance(const FaceEmbedding& a, const FaceEmbedding& b) {
    if (a.size() != 128 || b.size() != 128) {
        throw std::invalid_argument("Embeddings must be 128-dimensional");
    }
    
    float sum = 0.0f;
    for (size_t i = 0; i < 128; ++i) {
        float diff = a[i] - b[i];
        sum += diff * diff;
    }
    return std::sqrt(sum);
}

static bool is_same_person(const FaceEmbedding& a, const FaceEmbedding& b, float threshold = 0.6f) {
    return embedding_distance(a, b) < threshold;
}

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
    
    // Compute centroid of embeddings
    FaceEmbedding compute_centroid(const std::vector<FaceEmbedding>& embeddings) {
        if (embeddings.empty()) return {};
        
        FaceEmbedding centroid(128, 0.0f);
        for (const auto& emb : embeddings) {
            for (size_t i = 0; i < 128; ++i) {
                centroid[i] += emb[i];
            }
        }
        
        float n = static_cast<float>(embeddings.size());
        for (size_t i = 0; i < 128; ++i) {
            centroid[i] /= n;
        }
        
        return centroid;
    }
};

TEST_F(ClusteringTest, EmbeddingDistance_Identical) {
    auto emb = make_embedding(1.0f);
    float dist = embedding_distance(emb, emb);
    EXPECT_FLOAT_EQ(dist, 0.0f);
}

TEST_F(ClusteringTest, EmbeddingDistance_Different) {
    auto emb1 = make_embedding(0.0f);
    auto emb2 = make_embedding(1.0f);
    float dist = embedding_distance(emb1, emb2);
    EXPECT_GT(dist, 0.0f);
}

TEST_F(ClusteringTest, EmbeddingDistance_Symmetry) {
    auto emb1 = make_embedding(0.5f);
    auto emb2 = make_embedding(1.5f);
    float dist1 = embedding_distance(emb1, emb2);
    float dist2 = embedding_distance(emb2, emb1);
    EXPECT_FLOAT_EQ(dist1, dist2);
}

TEST_F(ClusteringTest, EmbeddingDistance_TriangleInequality) {
    auto emb1 = make_embedding(0.0f);
    auto emb2 = make_embedding(0.5f);
    auto emb3 = make_embedding(1.0f);
    
    float d12 = embedding_distance(emb1, emb2);
    float d23 = embedding_distance(emb2, emb3);
    float d13 = embedding_distance(emb1, emb3);
    
    // Triangle inequality: d13 <= d12 + d23
    EXPECT_LE(d13, d12 + d23 + 0.0001f);  // Small epsilon for floating point
}

TEST_F(ClusteringTest, IsSamePerson_Similar) {
    auto emb1 = make_embedding(1.0f);
    auto emb2 = add_noise(emb1, 0.1f);  // Small noise
    
    float dist = embedding_distance(emb1, emb2);
    // With small noise, should be close
    EXPECT_LT(dist, 0.6f);
    EXPECT_TRUE(is_same_person(emb1, emb2, 0.6f));
}

TEST_F(ClusteringTest, IsSamePerson_Different) {
    auto emb1 = make_embedding(0.0f);
    auto emb2 = make_embedding(5.0f);  // Very different
    
    EXPECT_FALSE(is_same_person(emb1, emb2, 0.6f));
}

TEST_F(ClusteringTest, ThresholdAffectsClustering) {
    auto emb1 = make_embedding(0.0f);
    auto emb2 = add_noise(emb1, 0.3f);
    
    float dist = embedding_distance(emb1, emb2);
    
    // Should cluster together with high threshold
    EXPECT_TRUE(is_same_person(emb1, emb2, dist + 0.1f));
    
    // Should NOT cluster with low threshold
    EXPECT_FALSE(is_same_person(emb1, emb2, dist - 0.1f));
}

TEST_F(ClusteringTest, CentroidComputation_SingleEmbedding) {
    auto emb = make_embedding(1.0f);
    auto centroid = compute_centroid({emb});
    
    // Centroid of single embedding should equal that embedding
    float dist = embedding_distance(centroid, emb);
    EXPECT_FLOAT_EQ(dist, 0.0f);
}

TEST_F(ClusteringTest, CentroidComputation_MultipleEmbeddings) {
    auto emb1 = make_embedding(0.0f);
    auto emb2 = make_embedding(2.0f);
    auto centroid = compute_centroid({emb1, emb2});
    
    // Centroid should be midpoint
    auto expected = make_embedding(1.0f);
    float dist = embedding_distance(centroid, expected);
    EXPECT_LT(dist, 0.0001f);
}

TEST_F(ClusteringTest, CentroidIsCloserToAllPoints) {
    auto emb1 = make_embedding(0.0f);
    auto emb2 = make_embedding(1.0f);
    auto emb3 = make_embedding(2.0f);
    
    auto centroid = compute_centroid({emb1, emb2, emb3});
    
    // Centroid should be closer to all points than extreme points are to each other
    float d1c = embedding_distance(emb1, centroid);
    float d3c = embedding_distance(emb3, centroid);
    float d13 = embedding_distance(emb1, emb3);
    
    EXPECT_LT(d1c, d13);
    EXPECT_LT(d3c, d13);
}

TEST_F(ClusteringTest, InvalidEmbeddingSize) {
    FaceEmbedding invalid(64);  // Wrong size
    auto valid = make_embedding(1.0f);
    
    EXPECT_THROW(embedding_distance(invalid, valid), std::invalid_argument);
    EXPECT_THROW(embedding_distance(valid, invalid), std::invalid_argument);
}
