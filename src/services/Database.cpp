/**
 * Database implementation using SQLite.
 */

#include "Database.h"
#include <sqlite3.h>
#include <stdexcept>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace facefling {

// Utility function to get current timestamp as ISO string
static std::string get_current_timestamp() {
    auto now = std::time(nullptr);
    std::ostringstream oss;
    oss << std::put_time(std::gmtime(&now), "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

// RAII wrapper for SQLite prepared statements
class Statement {
public:
    Statement(sqlite3* db, const std::string& sql) : m_stmt(nullptr) {
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &m_stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error("Failed to prepare statement: " + sql);
        }
    }
    
    ~Statement() {
        if (m_stmt) {
            sqlite3_finalize(m_stmt);
        }
    }
    
    sqlite3_stmt* get() { return m_stmt; }
    
    void bind_int(int index, int64_t value) {
        sqlite3_bind_int64(m_stmt, index, value);
    }
    
    void bind_double(int index, double value) {
        sqlite3_bind_double(m_stmt, index, value);
    }
    
    void bind_text(int index, const std::string& value) {
        sqlite3_bind_text(m_stmt, index, value.c_str(), -1, SQLITE_TRANSIENT);
    }
    
    void bind_blob(int index, const void* data, int size) {
        sqlite3_bind_blob(m_stmt, index, data, size, SQLITE_TRANSIENT);
    }
    
    void bind_null(int index) {
        sqlite3_bind_null(m_stmt, index);
    }
    
    bool step() {
        int rc = sqlite3_step(m_stmt);
        if (rc == SQLITE_ROW) return true;
        if (rc == SQLITE_DONE) return false;
        throw std::runtime_error("Step failed");
    }
    
    void reset() {
        sqlite3_reset(m_stmt);
    }

private:
    sqlite3_stmt* m_stmt;
};

class Database::Impl {
public:
    sqlite3* db = nullptr;
    std::string db_path;
    
    ~Impl() {
        if (db) {
            sqlite3_close(db);
        }
    }
    
    void exec(const std::string& sql) {
        char* err = nullptr;
        if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &err) != SQLITE_OK) {
            std::string msg = err ? err : "Unknown error";
            sqlite3_free(err);
            throw std::runtime_error("SQL error: " + msg);
        }
    }
    
    int64_t last_insert_rowid() {
        return sqlite3_last_insert_rowid(db);
    }
};

Database::Database(const std::string& db_path)
    : m_impl(std::make_unique<Impl>())
{
    m_impl->db_path = db_path;
    
    if (sqlite3_open(db_path.c_str(), &m_impl->db) != SQLITE_OK) {
        throw std::runtime_error("Failed to open database: " + db_path);
    }
}

Database::~Database() = default;

void Database::initialize()
{
    const char* schema = R"(
        CREATE TABLE IF NOT EXISTS photos (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            file_path TEXT UNIQUE NOT NULL,
            file_name TEXT NOT NULL,
            folder_path TEXT NOT NULL,
            width INTEGER,
            height INTEGER,
            file_size INTEGER,
            exif_date TEXT,
            scan_date TEXT NOT NULL,
            checksum TEXT
        );
        
        CREATE TABLE IF NOT EXISTS faces (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            photo_id INTEGER NOT NULL,
            bbox_x INTEGER NOT NULL,
            bbox_y INTEGER NOT NULL,
            bbox_width INTEGER NOT NULL,
            bbox_height INTEGER NOT NULL,
            embedding BLOB NOT NULL,
            cluster_id INTEGER,
            person_id INTEGER,
            confidence REAL,
            FOREIGN KEY (photo_id) REFERENCES photos(id),
            FOREIGN KEY (cluster_id) REFERENCES clusters(id),
            FOREIGN KEY (person_id) REFERENCES persons(id)
        );
        
        CREATE TABLE IF NOT EXISTS clusters (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            centroid BLOB,
            face_count INTEGER DEFAULT 0,
            created_date TEXT NOT NULL,
            person_id INTEGER,
            FOREIGN KEY (person_id) REFERENCES persons(id)
        );
        
        CREATE TABLE IF NOT EXISTS persons (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            created_date TEXT NOT NULL,
            notes TEXT
        );
        
        CREATE TABLE IF NOT EXISTS scans (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            root_path TEXT NOT NULL,
            start_date TEXT NOT NULL,
            end_date TEXT,
            status TEXT NOT NULL,
            total_files INTEGER,
            processed_files INTEGER,
            total_faces INTEGER
        );
        
        CREATE INDEX IF NOT EXISTS idx_faces_photo ON faces(photo_id);
        CREATE INDEX IF NOT EXISTS idx_faces_cluster ON faces(cluster_id);
        CREATE INDEX IF NOT EXISTS idx_faces_person ON faces(person_id);
        CREATE INDEX IF NOT EXISTS idx_photos_path ON photos(file_path);
    )";
    
    m_impl->exec(schema);
}

// ============================================================================
// Photo operations
// ============================================================================

int64_t Database::insert_photo(const Photo& photo) {
    Statement stmt(m_impl->db, R"(
        INSERT INTO photos (file_path, file_name, folder_path, width, height, file_size, exif_date, scan_date, checksum)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
    )");
    
    stmt.bind_text(1, photo.file_path);
    stmt.bind_text(2, photo.file_name);
    stmt.bind_text(3, photo.folder_path);
    stmt.bind_int(4, photo.width);
    stmt.bind_int(5, photo.height);
    stmt.bind_int(6, photo.file_size);
    
    if (photo.exif_date.has_value()) {
        stmt.bind_text(7, photo.exif_date.value());
    } else {
        stmt.bind_null(7);
    }
    
    stmt.bind_text(8, photo.scan_date.empty() ? get_current_timestamp() : photo.scan_date);
    stmt.bind_text(9, photo.checksum);
    
    stmt.step();
    return m_impl->last_insert_rowid();
}

std::optional<Photo> Database::get_photo(int64_t id) {
    Statement stmt(m_impl->db, "SELECT * FROM photos WHERE id = ?");
    stmt.bind_int(1, id);
    
    if (!stmt.step()) {
        return std::nullopt;
    }
    
    Photo photo;
    photo.id = sqlite3_column_int64(stmt.get(), 0);
    photo.file_path = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 1));
    photo.file_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 2));
    photo.folder_path = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 3));
    photo.width = sqlite3_column_int(stmt.get(), 4);
    photo.height = sqlite3_column_int(stmt.get(), 5);
    photo.file_size = sqlite3_column_int64(stmt.get(), 6);
    
    if (sqlite3_column_type(stmt.get(), 7) != SQLITE_NULL) {
        photo.exif_date = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 7));
    }
    
    if (sqlite3_column_text(stmt.get(), 8)) {
        photo.scan_date = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 8));
    }
    if (sqlite3_column_text(stmt.get(), 9)) {
        photo.checksum = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 9));
    }
    
    return photo;
}

std::optional<Photo> Database::get_photo_by_path(const std::string& path) {
    Statement stmt(m_impl->db, "SELECT * FROM photos WHERE file_path = ?");
    stmt.bind_text(1, path);
    
    if (!stmt.step()) {
        return std::nullopt;
    }
    
    Photo photo;
    photo.id = sqlite3_column_int64(stmt.get(), 0);
    photo.file_path = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 1));
    photo.file_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 2));
    photo.folder_path = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 3));
    photo.width = sqlite3_column_int(stmt.get(), 4);
    photo.height = sqlite3_column_int(stmt.get(), 5);
    photo.file_size = sqlite3_column_int64(stmt.get(), 6);
    
    if (sqlite3_column_type(stmt.get(), 7) != SQLITE_NULL) {
        photo.exif_date = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 7));
    }
    
    if (sqlite3_column_text(stmt.get(), 8)) {
        photo.scan_date = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 8));
    }
    if (sqlite3_column_text(stmt.get(), 9)) {
        photo.checksum = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 9));
    }
    
    return photo;
}

std::vector<Photo> Database::get_photos_for_person(int64_t person_id) {
    Statement stmt(m_impl->db, R"(
        SELECT DISTINCT p.* FROM photos p
        INNER JOIN faces f ON f.photo_id = p.id
        WHERE f.person_id = ?
    )");
    stmt.bind_int(1, person_id);
    
    std::vector<Photo> results;
    while (stmt.step()) {
        Photo photo;
        photo.id = sqlite3_column_int64(stmt.get(), 0);
        photo.file_path = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 1));
        photo.file_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 2));
        photo.folder_path = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 3));
        photo.width = sqlite3_column_int(stmt.get(), 4);
        photo.height = sqlite3_column_int(stmt.get(), 5);
        photo.file_size = sqlite3_column_int64(stmt.get(), 6);
        results.push_back(photo);
    }
    
    return results;
}

// ============================================================================
// Face operations
// ============================================================================

int64_t Database::insert_face(const Face& face) {
    Statement stmt(m_impl->db, R"(
        INSERT INTO faces (photo_id, bbox_x, bbox_y, bbox_width, bbox_height, embedding, cluster_id, person_id, confidence)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
    )");
    
    stmt.bind_int(1, face.photo_id);
    stmt.bind_int(2, face.bbox.x);
    stmt.bind_int(3, face.bbox.y);
    stmt.bind_int(4, face.bbox.width);
    stmt.bind_int(5, face.bbox.height);
    stmt.bind_blob(6, face.embedding.data(), static_cast<int>(face.embedding.size() * sizeof(float)));
    
    if (face.cluster_id.has_value()) {
        stmt.bind_int(7, face.cluster_id.value());
    } else {
        stmt.bind_null(7);
    }
    
    if (face.person_id.has_value()) {
        stmt.bind_int(8, face.person_id.value());
    } else {
        stmt.bind_null(8);
    }
    
    stmt.bind_double(9, face.confidence);
    
    stmt.step();
    return m_impl->last_insert_rowid();
}

static Face read_face(sqlite3_stmt* stmt) {
    Face face;
    face.id = sqlite3_column_int64(stmt, 0);
    face.photo_id = sqlite3_column_int64(stmt, 1);
    face.bbox.x = sqlite3_column_int(stmt, 2);
    face.bbox.y = sqlite3_column_int(stmt, 3);
    face.bbox.width = sqlite3_column_int(stmt, 4);
    face.bbox.height = sqlite3_column_int(stmt, 5);
    
    // Read embedding blob
    const void* blob = sqlite3_column_blob(stmt, 6);
    int blob_bytes = sqlite3_column_bytes(stmt, 6);
    if (blob && blob_bytes > 0) {
        int num_floats = blob_bytes / sizeof(float);
        face.embedding.resize(num_floats);
        std::memcpy(face.embedding.data(), blob, blob_bytes);
    }
    
    if (sqlite3_column_type(stmt, 7) != SQLITE_NULL) {
        face.cluster_id = sqlite3_column_int64(stmt, 7);
    }
    
    if (sqlite3_column_type(stmt, 8) != SQLITE_NULL) {
        face.person_id = sqlite3_column_int64(stmt, 8);
    }
    
    face.confidence = static_cast<float>(sqlite3_column_double(stmt, 9));
    
    return face;
}

std::optional<Face> Database::get_face(int64_t id) {
    Statement stmt(m_impl->db, "SELECT * FROM faces WHERE id = ?");
    stmt.bind_int(1, id);
    
    if (!stmt.step()) {
        return std::nullopt;
    }
    
    return read_face(stmt.get());
}

std::vector<Face> Database::get_faces_for_photo(int64_t photo_id) {
    Statement stmt(m_impl->db, "SELECT * FROM faces WHERE photo_id = ?");
    stmt.bind_int(1, photo_id);
    
    std::vector<Face> results;
    while (stmt.step()) {
        results.push_back(read_face(stmt.get()));
    }
    return results;
}

std::vector<Face> Database::get_faces_for_cluster(int64_t cluster_id) {
    Statement stmt(m_impl->db, "SELECT * FROM faces WHERE cluster_id = ?");
    stmt.bind_int(1, cluster_id);
    
    std::vector<Face> results;
    while (stmt.step()) {
        results.push_back(read_face(stmt.get()));
    }
    return results;
}

std::vector<Face> Database::get_faces_for_person(int64_t person_id) {
    Statement stmt(m_impl->db, "SELECT * FROM faces WHERE person_id = ?");
    stmt.bind_int(1, person_id);
    
    std::vector<Face> results;
    while (stmt.step()) {
        results.push_back(read_face(stmt.get()));
    }
    return results;
}

std::vector<Face> Database::get_all_faces_with_embeddings() {
    Statement stmt(m_impl->db, "SELECT * FROM faces WHERE embedding IS NOT NULL");
    
    std::vector<Face> results;
    while (stmt.step()) {
        results.push_back(read_face(stmt.get()));
    }
    return results;
}

std::vector<Face> Database::get_unclustered_faces() {
    Statement stmt(m_impl->db, "SELECT * FROM faces WHERE cluster_id IS NULL AND embedding IS NOT NULL");
    
    std::vector<Face> results;
    while (stmt.step()) {
        results.push_back(read_face(stmt.get()));
    }
    return results;
}

void Database::update_face_cluster(int64_t face_id, int64_t cluster_id) {
    Statement stmt(m_impl->db, "UPDATE faces SET cluster_id = ? WHERE id = ?");
    stmt.bind_int(1, cluster_id);
    stmt.bind_int(2, face_id);
    stmt.step();
}

void Database::update_face_person(int64_t face_id, int64_t person_id) {
    Statement stmt(m_impl->db, "UPDATE faces SET person_id = ? WHERE id = ?");
    stmt.bind_int(1, person_id);
    stmt.bind_int(2, face_id);
    stmt.step();
}

// ============================================================================
// Cluster operations
// ============================================================================

int64_t Database::insert_cluster(const Cluster& cluster) {
    Statement stmt(m_impl->db, R"(
        INSERT INTO clusters (centroid, face_count, created_date, person_id)
        VALUES (?, ?, ?, ?)
    )");
    
    if (!cluster.centroid.empty()) {
        stmt.bind_blob(1, cluster.centroid.data(), 
                      static_cast<int>(cluster.centroid.size() * sizeof(float)));
    } else {
        stmt.bind_null(1);
    }
    
    stmt.bind_int(2, cluster.face_count);
    stmt.bind_text(3, cluster.created_date.empty() ? get_current_timestamp() : cluster.created_date);
    
    if (cluster.person_id.has_value()) {
        stmt.bind_int(4, cluster.person_id.value());
    } else {
        stmt.bind_null(4);
    }
    
    stmt.step();
    return m_impl->last_insert_rowid();
}

static Cluster read_cluster(sqlite3_stmt* stmt) {
    Cluster cluster;
    cluster.id = sqlite3_column_int64(stmt, 0);
    
    const void* blob = sqlite3_column_blob(stmt, 1);
    int blob_bytes = sqlite3_column_bytes(stmt, 1);
    if (blob && blob_bytes > 0) {
        int num_floats = blob_bytes / sizeof(float);
        cluster.centroid.resize(num_floats);
        std::memcpy(cluster.centroid.data(), blob, blob_bytes);
    }
    
    cluster.face_count = sqlite3_column_int(stmt, 2);
    
    if (sqlite3_column_text(stmt, 3)) {
        cluster.created_date = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
    }
    
    if (sqlite3_column_type(stmt, 4) != SQLITE_NULL) {
        cluster.person_id = sqlite3_column_int64(stmt, 4);
    }
    
    return cluster;
}

std::optional<Cluster> Database::get_cluster(int64_t id) {
    Statement stmt(m_impl->db, "SELECT * FROM clusters WHERE id = ?");
    stmt.bind_int(1, id);
    
    if (!stmt.step()) {
        return std::nullopt;
    }
    
    return read_cluster(stmt.get());
}

std::vector<Cluster> Database::get_all_clusters() {
    Statement stmt(m_impl->db, "SELECT * FROM clusters");
    
    std::vector<Cluster> results;
    while (stmt.step()) {
        results.push_back(read_cluster(stmt.get()));
    }
    return results;
}

void Database::update_cluster_centroid(int64_t cluster_id, const std::vector<float>& centroid) {
    Statement stmt(m_impl->db, "UPDATE clusters SET centroid = ? WHERE id = ?");
    stmt.bind_blob(1, centroid.data(), static_cast<int>(centroid.size() * sizeof(float)));
    stmt.bind_int(2, cluster_id);
    stmt.step();
}

void Database::delete_cluster(int64_t cluster_id) {
    // First unlink all faces from this cluster
    {
        Statement stmt(m_impl->db, "UPDATE faces SET cluster_id = NULL WHERE cluster_id = ?");
        stmt.bind_int(1, cluster_id);
        stmt.step();
    }
    
    // Then delete the cluster
    {
        Statement stmt(m_impl->db, "DELETE FROM clusters WHERE id = ?");
        stmt.bind_int(1, cluster_id);
        stmt.step();
    }
}

// ============================================================================
// Person operations
// ============================================================================

int64_t Database::insert_person(const Person& person) {
    Statement stmt(m_impl->db, R"(
        INSERT INTO persons (name, created_date, notes)
        VALUES (?, ?, ?)
    )");
    
    stmt.bind_text(1, person.name);
    stmt.bind_text(2, person.created_date.empty() ? get_current_timestamp() : person.created_date);
    
    if (person.notes.has_value()) {
        stmt.bind_text(3, person.notes.value());
    } else {
        stmt.bind_null(3);
    }
    
    stmt.step();
    return m_impl->last_insert_rowid();
}

static Person read_person(sqlite3_stmt* stmt) {
    Person person;
    person.id = sqlite3_column_int64(stmt, 0);
    person.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    
    if (sqlite3_column_text(stmt, 2)) {
        person.created_date = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
    }
    
    if (sqlite3_column_type(stmt, 3) != SQLITE_NULL) {
        person.notes = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
    }
    
    return person;
}

std::optional<Person> Database::get_person(int64_t id) {
    Statement stmt(m_impl->db, "SELECT * FROM persons WHERE id = ?");
    stmt.bind_int(1, id);
    
    if (!stmt.step()) {
        return std::nullopt;
    }
    
    return read_person(stmt.get());
}

std::vector<Person> Database::get_all_persons() {
    Statement stmt(m_impl->db, "SELECT * FROM persons");
    
    std::vector<Person> results;
    while (stmt.step()) {
        results.push_back(read_person(stmt.get()));
    }
    return results;
}

void Database::update_person(const Person& person) {
    Statement stmt(m_impl->db, "UPDATE persons SET name = ?, notes = ? WHERE id = ?");
    stmt.bind_text(1, person.name);
    
    if (person.notes.has_value()) {
        stmt.bind_text(2, person.notes.value());
    } else {
        stmt.bind_null(2);
    }
    
    stmt.bind_int(3, person.id);
    stmt.step();
}

void Database::delete_person(int64_t person_id) {
    // Unlink faces
    {
        Statement stmt(m_impl->db, "UPDATE faces SET person_id = NULL WHERE person_id = ?");
        stmt.bind_int(1, person_id);
        stmt.step();
    }
    
    // Unlink clusters
    {
        Statement stmt(m_impl->db, "UPDATE clusters SET person_id = NULL WHERE person_id = ?");
        stmt.bind_int(1, person_id);
        stmt.step();
    }
    
    // Delete person
    {
        Statement stmt(m_impl->db, "DELETE FROM persons WHERE id = ?");
        stmt.bind_int(1, person_id);
        stmt.step();
    }
}

// ============================================================================
// Transaction operations
// ============================================================================

void Database::begin_transaction() {
    m_impl->exec("BEGIN TRANSACTION");
}

void Database::commit() {
    m_impl->exec("COMMIT");
}

void Database::rollback() {
    m_impl->exec("ROLLBACK");
}

} // namespace facefling
