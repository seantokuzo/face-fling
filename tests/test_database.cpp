/**
 * Database unit tests.
 * Tests SQLite persistence layer operations.
 */

#include <gtest/gtest.h>
#include "services/Database.h"
#include "models/Photo.h"
#include "models/Face.h"
#include "models/Cluster.h"
#include "models/Person.h"
#include <filesystem>
#include <cstring>

namespace fs = std::filesystem;
using namespace facefling;

class DatabaseTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temp database
        db_path = fs::temp_directory_path() / "facefling_test.db";
        fs::remove(db_path);  // Clean up any previous test
        
        db = std::make_unique<Database>(db_path.string());
        db->initialize();
    }
    
    void TearDown() override {
        db.reset();
        fs::remove(db_path);
    }
    
    // Helper to create a test photo
    Photo make_photo(const std::string& path) {
        Photo p;
        p.file_path = path;
        p.file_name = fs::path(path).filename().string();
        p.folder_path = fs::path(path).parent_path().string();
        p.width = 1920;
        p.height = 1080;
        p.file_size = 1024000;
        p.scan_date = "2026-02-22T10:00:00Z";
        return p;
    }
    
    // Helper to create a test face
    Face make_face(int64_t photo_id, int x = 100, int y = 100) {
        Face f;
        f.photo_id = photo_id;
        f.bbox.x = x;
        f.bbox.y = y;
        f.bbox.width = 80;
        f.bbox.height = 80;
        f.confidence = 0.95f;
        
        // Create 128-dim embedding
        f.embedding.resize(128);
        for (int i = 0; i < 128; ++i) {
            f.embedding[i] = static_cast<float>(x + i) * 0.01f;
        }
        
        return f;
    }
    
    // Helper to create a test person
    Person make_person(const std::string& name) {
        Person p;
        p.name = name;
        p.created_date = "2026-02-22T10:00:00Z";
        return p;
    }
    
    fs::path db_path;
    std::unique_ptr<Database> db;
};

// =============================================================================
// Photo Tests
// =============================================================================

TEST_F(DatabaseTest, InsertAndGetPhoto) {
    auto photo = make_photo("/photos/test.jpg");
    
    int64_t id = db->insert_photo(photo);
    EXPECT_GT(id, 0);
    
    auto retrieved = db->get_photo(id);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->file_path, photo.file_path);
    EXPECT_EQ(retrieved->file_name, photo.file_name);
    EXPECT_EQ(retrieved->width, photo.width);
    EXPECT_EQ(retrieved->height, photo.height);
}

TEST_F(DatabaseTest, GetPhotoByPath) {
    auto photo = make_photo("/photos/unique_path.jpg");
    db->insert_photo(photo);
    
    auto retrieved = db->get_photo_by_path("/photos/unique_path.jpg");
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->file_name, "unique_path.jpg");
}

TEST_F(DatabaseTest, GetPhotoByPath_NotFound) {
    auto retrieved = db->get_photo_by_path("/nonexistent.jpg");
    EXPECT_FALSE(retrieved.has_value());
}

TEST_F(DatabaseTest, UniquePhotoPaths) {
    auto photo = make_photo("/photos/duplicate.jpg");
    db->insert_photo(photo);
    
    // Same path should fail
    EXPECT_THROW(db->insert_photo(photo), std::runtime_error);
}

// =============================================================================
// Face Tests
// =============================================================================

TEST_F(DatabaseTest, InsertAndGetFace) {
    // Need a photo first
    auto photo = make_photo("/photos/face_test.jpg");
    int64_t photo_id = db->insert_photo(photo);
    
    auto face = make_face(photo_id);
    int64_t face_id = db->insert_face(face);
    EXPECT_GT(face_id, 0);
    
    auto retrieved = db->get_face(face_id);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->photo_id, photo_id);
    EXPECT_EQ(retrieved->bbox.x, face.bbox.x);
    EXPECT_EQ(retrieved->bbox.width, face.bbox.width);
    EXPECT_FLOAT_EQ(retrieved->confidence, face.confidence);
}

TEST_F(DatabaseTest, FaceEmbeddingPersistence) {
    auto photo = make_photo("/photos/embedding_test.jpg");
    int64_t photo_id = db->insert_photo(photo);
    
    auto face = make_face(photo_id);
    int64_t face_id = db->insert_face(face);
    
    auto retrieved = db->get_face(face_id);
    ASSERT_TRUE(retrieved.has_value());
    ASSERT_EQ(retrieved->embedding.size(), 128u);
    
    // Check embedding values match
    for (size_t i = 0; i < 128; ++i) {
        EXPECT_FLOAT_EQ(retrieved->embedding[i], face.embedding[i]);
    }
}

TEST_F(DatabaseTest, GetFacesForPhoto) {
    auto photo = make_photo("/photos/multi_face.jpg");
    int64_t photo_id = db->insert_photo(photo);
    
    // Add multiple faces
    db->insert_face(make_face(photo_id, 100, 100));
    db->insert_face(make_face(photo_id, 200, 200));
    db->insert_face(make_face(photo_id, 300, 300));
    
    auto faces = db->get_faces_for_photo(photo_id);
    EXPECT_EQ(faces.size(), 3u);
}

TEST_F(DatabaseTest, UpdateFaceCluster) {
    auto photo = make_photo("/photos/cluster_test.jpg");
    int64_t photo_id = db->insert_photo(photo);
    
    auto face = make_face(photo_id);
    int64_t face_id = db->insert_face(face);
    
    // Create a cluster
    Cluster cluster;
    cluster.face_count = 1;
    cluster.created_date = "2026-02-22T10:00:00Z";
    int64_t cluster_id = db->insert_cluster(cluster);
    
    // Assign face to cluster
    db->update_face_cluster(face_id, cluster_id);
    
    auto retrieved = db->get_face(face_id);
    ASSERT_TRUE(retrieved.has_value());
    ASSERT_TRUE(retrieved->cluster_id.has_value());
    EXPECT_EQ(retrieved->cluster_id.value(), cluster_id);
}

// =============================================================================
// Cluster Tests
// =============================================================================

TEST_F(DatabaseTest, InsertAndGetCluster) {
    Cluster cluster;
    cluster.face_count = 5;
    cluster.created_date = "2026-02-22T10:00:00Z";
    
    // Create 128-dim centroid
    cluster.centroid.resize(128);
    for (int i = 0; i < 128; ++i) {
        cluster.centroid[i] = static_cast<float>(i) * 0.01f;
    }
    
    int64_t id = db->insert_cluster(cluster);
    EXPECT_GT(id, 0);
    
    auto retrieved = db->get_cluster(id);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->face_count, cluster.face_count);
    ASSERT_EQ(retrieved->centroid.size(), 128u);
}

TEST_F(DatabaseTest, GetAllClusters) {
    for (int i = 0; i < 5; ++i) {
        Cluster c;
        c.face_count = i + 1;
        c.created_date = "2026-02-22T10:00:00Z";
        db->insert_cluster(c);
    }
    
    auto clusters = db->get_all_clusters();
    EXPECT_EQ(clusters.size(), 5u);
}

TEST_F(DatabaseTest, UpdateClusterCentroid) {
    Cluster cluster;
    cluster.face_count = 1;
    cluster.created_date = "2026-02-22T10:00:00Z";
    int64_t id = db->insert_cluster(cluster);
    
    // Update centroid
    std::vector<float> new_centroid(128, 0.5f);
    db->update_cluster_centroid(id, new_centroid);
    
    auto retrieved = db->get_cluster(id);
    ASSERT_TRUE(retrieved.has_value());
    ASSERT_EQ(retrieved->centroid.size(), 128u);
    EXPECT_FLOAT_EQ(retrieved->centroid[0], 0.5f);
}

TEST_F(DatabaseTest, DeleteCluster) {
    Cluster cluster;
    cluster.face_count = 1;
    cluster.created_date = "2026-02-22T10:00:00Z";
    int64_t id = db->insert_cluster(cluster);
    
    db->delete_cluster(id);
    
    auto retrieved = db->get_cluster(id);
    EXPECT_FALSE(retrieved.has_value());
}

// =============================================================================
// Person Tests
// =============================================================================

TEST_F(DatabaseTest, InsertAndGetPerson) {
    auto person = make_person("John Doe");
    
    int64_t id = db->insert_person(person);
    EXPECT_GT(id, 0);
    
    auto retrieved = db->get_person(id);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->name, "John Doe");
}

TEST_F(DatabaseTest, GetAllPersons) {
    db->insert_person(make_person("Alice"));
    db->insert_person(make_person("Bob"));
    db->insert_person(make_person("Charlie"));
    
    auto persons = db->get_all_persons();
    EXPECT_EQ(persons.size(), 3u);
}

TEST_F(DatabaseTest, UpdatePerson) {
    auto person = make_person("Original Name");
    int64_t id = db->insert_person(person);
    
    person.id = id;
    person.name = "Updated Name";
    person.notes = "Added notes";
    db->update_person(person);
    
    auto retrieved = db->get_person(id);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->name, "Updated Name");
    ASSERT_TRUE(retrieved->notes.has_value());
    EXPECT_EQ(retrieved->notes.value(), "Added notes");
}

TEST_F(DatabaseTest, DeletePerson) {
    auto person = make_person("To Delete");
    int64_t id = db->insert_person(person);
    
    db->delete_person(id);
    
    auto retrieved = db->get_person(id);
    EXPECT_FALSE(retrieved.has_value());
}

// =============================================================================
// Transaction Tests
// =============================================================================

TEST_F(DatabaseTest, TransactionCommit) {
    db->begin_transaction();
    
    auto photo = make_photo("/photos/transaction_test.jpg");
    int64_t id = db->insert_photo(photo);
    
    db->commit();
    
    // Should be persisted
    auto retrieved = db->get_photo(id);
    EXPECT_TRUE(retrieved.has_value());
}

TEST_F(DatabaseTest, TransactionRollback) {
    db->begin_transaction();
    
    auto photo = make_photo("/photos/rollback_test.jpg");
    db->insert_photo(photo);
    
    db->rollback();
    
    // Should NOT be persisted
    auto retrieved = db->get_photo_by_path("/photos/rollback_test.jpg");
    EXPECT_FALSE(retrieved.has_value());
}

// =============================================================================
// Cross-Entity Tests
// =============================================================================

TEST_F(DatabaseTest, GetFacesForCluster) {
    // Create photo
    auto photo = make_photo("/photos/cluster_faces.jpg");
    int64_t photo_id = db->insert_photo(photo);
    
    // Create cluster
    Cluster cluster;
    cluster.face_count = 2;
    cluster.created_date = "2026-02-22T10:00:00Z";
    int64_t cluster_id = db->insert_cluster(cluster);
    
    // Create faces and assign to cluster
    auto face1 = make_face(photo_id, 100, 100);
    auto face2 = make_face(photo_id, 200, 200);
    int64_t fid1 = db->insert_face(face1);
    int64_t fid2 = db->insert_face(face2);
    
    db->update_face_cluster(fid1, cluster_id);
    db->update_face_cluster(fid2, cluster_id);
    
    auto faces = db->get_faces_for_cluster(cluster_id);
    EXPECT_EQ(faces.size(), 2u);
}

TEST_F(DatabaseTest, GetUnclusteredFaces) {
    auto photo = make_photo("/photos/unclustered.jpg");
    int64_t photo_id = db->insert_photo(photo);
    
    // Create cluster
    Cluster cluster;
    cluster.face_count = 1;
    cluster.created_date = "2026-02-22T10:00:00Z";
    int64_t cluster_id = db->insert_cluster(cluster);
    
    // One clustered, one unclustered
    auto clustered_face = make_face(photo_id, 100, 100);
    auto unclustered_face = make_face(photo_id, 200, 200);
    
    int64_t fid1 = db->insert_face(clustered_face);
    db->insert_face(unclustered_face);
    
    db->update_face_cluster(fid1, cluster_id);
    
    auto unclustered = db->get_unclustered_faces();
    EXPECT_EQ(unclustered.size(), 1u);
    EXPECT_FALSE(unclustered[0].cluster_id.has_value());
}
