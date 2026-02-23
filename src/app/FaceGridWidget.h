#pragma once

#include <QWidget>
#include <QScrollArea>
#include <QLabel>
#include <memory>
#include <cstdint>
#include <vector>

namespace facefling {

// Forward declarations
class IDatabase;
class FaceThumbnailWidget;
struct ClusterStats;

/**
 * Grid widget displaying face thumbnails organized by cluster.
 * Can show all clusters, faces in a specific cluster, or faces for a person.
 */
class FaceGridWidget : public QWidget
{
    Q_OBJECT
    
public:
    explicit FaceGridWidget(QWidget *parent = nullptr);
    ~FaceGridWidget() override = default;
    
    void setDatabase(std::shared_ptr<IDatabase> database);
    
    void showAllClusters();
    void showCluster(int64_t clusterId);
    void showPerson(int64_t personId);
    void clear();
    
    std::vector<int64_t> selectedFaceIds() const;
    int64_t selectedClusterId() const { return m_selectedClusterId; }
    
signals:
    void clusterSelected(int64_t clusterId);
    void faceSelected(int64_t faceId);
    void facesSelected(const std::vector<int64_t> &faceIds);
    
private slots:
    void onFaceClicked(int64_t faceId);
    void onFaceDoubleClicked(int64_t faceId);
    
private:
    void setupUi();
    void clearGrid();
    void addClusterSection(const ClusterStats& stats);
    void addFaceThumbnail(int64_t faceId, const QString& thumbnailPath);
    QString getThumbnailPath(int64_t faceId) const;
    
    std::shared_ptr<IDatabase> m_database;
    QScrollArea *m_scrollArea = nullptr;
    QWidget *m_gridContainer = nullptr;
    QWidget *m_flowWidget = nullptr;
    QLabel *m_placeholder = nullptr;
    
    int64_t m_selectedClusterId = 0;
    std::vector<int64_t> m_selectedFaces;
    std::vector<FaceThumbnailWidget*> m_thumbnails;
};

} // namespace facefling
