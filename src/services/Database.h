#pragma once

#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <cstdint>
#include "../models/Photo.h"
#include "../models/Face.h"
#include "../models/Cluster.h"
#include "../models/Person.h"

namespace facefling {

/**
 * Database interface for persistence.
 */
class IDatabase {
public:
    virtual ~IDatabase() = default;
    
    // Photos
    virtual int64_t insert_photo(const Photo& photo) = 0;
    virtual std::optional<Photo> get_photo(int64_t id) = 0;
    virtual std::optional<Photo> get_photo_by_path(const std::string& path) = 0;
    virtual std::vector<Photo> get_photos_for_person(int64_t person_id) = 0;
    
    // Faces
    virtual int64_t insert_face(const Face& face) = 0;
    virtual std::optional<Face> get_face(int64_t id) = 0;
    virtual std::vector<Face> get_faces_for_photo(int64_t photo_id) = 0;
    virtual std::vector<Face> get_faces_for_cluster(int64_t cluster_id) = 0;
    virtual std::vector<Face> get_faces_for_person(int64_t person_id) = 0;
    virtual std::vector<Face> get_all_faces_with_embeddings() = 0;
    virtual std::vector<Face> get_unclustered_faces() = 0;
    virtual void update_face_cluster(int64_t face_id, int64_t cluster_id) = 0;
    virtual void update_face_person(int64_t face_id, int64_t person_id) = 0;
    
    // Clusters
    virtual int64_t insert_cluster(const Cluster& cluster) = 0;
    virtual std::optional<Cluster> get_cluster(int64_t id) = 0;
    virtual std::vector<Cluster> get_all_clusters() = 0;
    virtual void update_cluster_centroid(int64_t cluster_id, const std::vector<float>& centroid) = 0;
    virtual void delete_cluster(int64_t cluster_id) = 0;
    
    // Persons
    virtual int64_t insert_person(const Person& person) = 0;
    virtual std::optional<Person> get_person(int64_t id) = 0;
    virtual std::vector<Person> get_all_persons() = 0;
    virtual void update_person(const Person& person) = 0;
    virtual void delete_person(int64_t person_id) = 0;
    
    // Transactions
    virtual void begin_transaction() = 0;
    virtual void commit() = 0;
    virtual void rollback() = 0;
};

/**
 * SQLite implementation of the database interface.
 */
class Database : public IDatabase {
public:
    explicit Database(const std::string& db_path);
    ~Database() override;
    
    // Initialize database schema
    void initialize();
    
    // IDatabase implementation
    int64_t insert_photo(const Photo& photo) override;
    std::optional<Photo> get_photo(int64_t id) override;
    std::optional<Photo> get_photo_by_path(const std::string& path) override;
    std::vector<Photo> get_photos_for_person(int64_t person_id) override;
    
    int64_t insert_face(const Face& face) override;
    std::optional<Face> get_face(int64_t id) override;
    std::vector<Face> get_faces_for_photo(int64_t photo_id) override;
    std::vector<Face> get_faces_for_cluster(int64_t cluster_id) override;
    std::vector<Face> get_faces_for_person(int64_t person_id) override;
    std::vector<Face> get_all_faces_with_embeddings() override;
    std::vector<Face> get_unclustered_faces() override;
    void update_face_cluster(int64_t face_id, int64_t cluster_id) override;
    void update_face_person(int64_t face_id, int64_t person_id) override;
    
    int64_t insert_cluster(const Cluster& cluster) override;
    std::optional<Cluster> get_cluster(int64_t id) override;
    std::vector<Cluster> get_all_clusters() override;
    void update_cluster_centroid(int64_t cluster_id, const std::vector<float>& centroid) override;
    void delete_cluster(int64_t cluster_id) override;
    
    int64_t insert_person(const Person& person) override;
    std::optional<Person> get_person(int64_t id) override;
    std::vector<Person> get_all_persons() override;
    void update_person(const Person& person) override;
    void delete_person(int64_t person_id) override;
    
    void begin_transaction() override;
    void commit() override;
    void rollback() override;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace facefling
