/**
 * FaceGridWidget implementation.
 */

#include "FaceGridWidget.h"
#include "FaceThumbnailWidget.h"
#include "../services/Database.h"
#include "../models/Cluster.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QStandardPaths>
#include <QScrollBar>

namespace facefling {

FaceGridWidget::FaceGridWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

void FaceGridWidget::setupUi()
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setStyleSheet("QScrollArea { border: none; }");
    
    m_gridContainer = new QWidget(this);
    m_gridContainer->setStyleSheet("background-color: white;");
    m_scrollArea->setWidget(m_gridContainer);
    
    auto *containerLayout = new QVBoxLayout(m_gridContainer);
    containerLayout->setContentsMargins(16, 16, 16, 16);
    containerLayout->setSpacing(24);
    
    // Placeholder message
    m_placeholder = new QLabel(tr("Open a folder to start scanning for faces"), m_gridContainer);
    m_placeholder->setAlignment(Qt::AlignCenter);
    m_placeholder->setStyleSheet("color: #888; font-size: 16px; padding: 40px;");
    containerLayout->addWidget(m_placeholder);
    containerLayout->addStretch();
    
    layout->addWidget(m_scrollArea);
}

void FaceGridWidget::setDatabase(std::shared_ptr<IDatabase> database)
{
    m_database = database;
}

void FaceGridWidget::showAllClusters()
{
    clearGrid();
    
    if (!m_database) {
        m_placeholder->setText(tr("Database not initialized"));
        m_placeholder->setVisible(true);
        return;
    }
    
    auto clusters = m_database->get_all_clusters();
    
    if (clusters.empty()) {
        m_placeholder->setText(tr("No faces found yet.\nOpen a folder to start scanning."));
        m_placeholder->setVisible(true);
        return;
    }
    
    m_placeholder->setVisible(false);
    
    // Get stats for all clusters
    auto *layout = qobject_cast<QVBoxLayout*>(m_gridContainer->layout());
    
    for (const auto& cluster : clusters) {
        // Get faces for this cluster
        auto faces = m_database->get_faces_for_cluster(cluster.id);
        if (faces.empty()) continue;
        
        // Create cluster section
        auto *sectionWidget = new QWidget(m_gridContainer);
        auto *sectionLayout = new QVBoxLayout(sectionWidget);
        sectionLayout->setContentsMargins(0, 0, 0, 0);
        sectionLayout->setSpacing(8);
        
        // Cluster header
        QString headerText;
        if (cluster.person_id.has_value()) {
            auto person = m_database->get_person(cluster.person_id.value());
            if (person) {
                headerText = QString::fromStdString(person->name);
            }
        }
        if (headerText.isEmpty()) {
            headerText = tr("Unknown Person");
        }
        headerText += QString(" (%1 faces)").arg(faces.size());
        
        auto *headerLabel = new QLabel(headerText, sectionWidget);
        headerLabel->setStyleSheet(
            "font-size: 14px; font-weight: bold; color: #333; padding: 4px 0;"
        );
        headerLabel->setCursor(Qt::PointingHandCursor);
        
        // Make header clickable to select cluster
        int64_t clusterId = cluster.id;
        connect(headerLabel, &QLabel::linkActivated, this, [this, clusterId]() {
            emit clusterSelected(clusterId);
        });
        
        sectionLayout->addWidget(headerLabel);
        
        // Face thumbnails in a flow layout
        auto *facesWidget = new QWidget(sectionWidget);
        auto *facesLayout = new QHBoxLayout(facesWidget);
        facesLayout->setContentsMargins(0, 0, 0, 0);
        facesLayout->setSpacing(8);
        facesLayout->setAlignment(Qt::AlignLeft);
        
        // Show first N faces (limit for performance)
        int maxFaces = 8;
        int count = 0;
        for (const auto& face : faces) {
            if (count++ >= maxFaces) break;
            
            auto *thumbnail = new FaceThumbnailWidget(face.id, facesWidget);
            thumbnail->setThumbnailPath(getThumbnailPath(face.id));
            
            connect(thumbnail, &FaceThumbnailWidget::clicked,
                    this, &FaceGridWidget::onFaceClicked);
            connect(thumbnail, &FaceThumbnailWidget::doubleClicked,
                    this, &FaceGridWidget::onFaceDoubleClicked);
            
            facesLayout->addWidget(thumbnail);
            m_thumbnails.push_back(thumbnail);
        }
        
        // Show "+N more" if there are more faces
        if (faces.size() > static_cast<size_t>(maxFaces)) {
            auto *moreLabel = new QLabel(
                tr("+%1 more").arg(faces.size() - maxFaces), facesWidget);
            moreLabel->setStyleSheet("color: #007AFF; padding: 8px;");
            moreLabel->setCursor(Qt::PointingHandCursor);
            facesLayout->addWidget(moreLabel);
        }
        
        facesLayout->addStretch();
        sectionLayout->addWidget(facesWidget);
        
        layout->insertWidget(layout->count() - 1, sectionWidget); // Before stretch
    }
}

void FaceGridWidget::showCluster(int64_t clusterId)
{
    clearGrid();
    m_selectedClusterId = clusterId;
    
    if (!m_database) return;
    
    auto faces = m_database->get_faces_for_cluster(clusterId);
    
    if (faces.empty()) {
        m_placeholder->setText(tr("No faces in this cluster"));
        m_placeholder->setVisible(true);
        return;
    }
    
    m_placeholder->setVisible(false);
    
    auto *layout = qobject_cast<QVBoxLayout*>(m_gridContainer->layout());
    
    // Cluster header
    QString headerText;
    auto cluster = m_database->get_cluster(clusterId);
    if (cluster && cluster->person_id.has_value()) {
        auto person = m_database->get_person(cluster->person_id.value());
        if (person) {
            headerText = QString::fromStdString(person->name);
        }
    }
    if (headerText.isEmpty()) {
        headerText = tr("Unknown Person");
    }
    headerText += QString(" - %1 faces").arg(faces.size());
    
    auto *headerLabel = new QLabel(headerText, m_gridContainer);
    headerLabel->setStyleSheet(
        "font-size: 18px; font-weight: bold; color: #333; padding: 8px 0;"
    );
    layout->insertWidget(0, headerLabel);
    
    // Flow layout for faces
    auto *facesWidget = new QWidget(m_gridContainer);
    auto *flowLayout = new QGridLayout(facesWidget);
    flowLayout->setContentsMargins(0, 0, 0, 0);
    flowLayout->setSpacing(8);
    
    int columns = 6;
    int row = 0, col = 0;
    
    for (const auto& face : faces) {
        auto *thumbnail = new FaceThumbnailWidget(face.id, facesWidget);
        thumbnail->setThumbnailPath(getThumbnailPath(face.id));
        
        connect(thumbnail, &FaceThumbnailWidget::clicked,
                this, &FaceGridWidget::onFaceClicked);
        connect(thumbnail, &FaceThumbnailWidget::doubleClicked,
                this, &FaceGridWidget::onFaceDoubleClicked);
        
        flowLayout->addWidget(thumbnail, row, col);
        m_thumbnails.push_back(thumbnail);
        
        col++;
        if (col >= columns) {
            col = 0;
            row++;
        }
    }
    
    layout->insertWidget(1, facesWidget);
    
    emit clusterSelected(clusterId);
}

void FaceGridWidget::showPerson(int64_t personId)
{
    clearGrid();
    
    if (!m_database) return;
    
    auto faces = m_database->get_faces_for_person(personId);
    
    if (faces.empty()) {
        m_placeholder->setText(tr("No faces for this person"));
        m_placeholder->setVisible(true);
        return;
    }
    
    m_placeholder->setVisible(false);
    
    auto *layout = qobject_cast<QVBoxLayout*>(m_gridContainer->layout());
    
    // Person header
    QString headerText;
    auto person = m_database->get_person(personId);
    if (person) {
        headerText = QString::fromStdString(person->name);
    } else {
        headerText = tr("Unknown Person");
    }
    headerText += QString(" - %1 faces").arg(faces.size());
    
    auto *headerLabel = new QLabel(headerText, m_gridContainer);
    headerLabel->setStyleSheet(
        "font-size: 18px; font-weight: bold; color: #333; padding: 8px 0;"
    );
    layout->insertWidget(0, headerLabel);
    
    // Flow layout for faces
    auto *facesWidget = new QWidget(m_gridContainer);
    auto *flowLayout = new QGridLayout(facesWidget);
    flowLayout->setContentsMargins(0, 0, 0, 0);
    flowLayout->setSpacing(8);
    
    int columns = 6;
    int row = 0, col = 0;
    
    for (const auto& face : faces) {
        auto *thumbnail = new FaceThumbnailWidget(face.id, facesWidget);
        thumbnail->setThumbnailPath(getThumbnailPath(face.id));
        
        connect(thumbnail, &FaceThumbnailWidget::clicked,
                this, &FaceGridWidget::onFaceClicked);
        connect(thumbnail, &FaceThumbnailWidget::doubleClicked,
                this, &FaceGridWidget::onFaceDoubleClicked);
        
        flowLayout->addWidget(thumbnail, row, col);
        m_thumbnails.push_back(thumbnail);
        
        col++;
        if (col >= columns) {
            col = 0;
            row++;
        }
    }
    
    layout->insertWidget(1, facesWidget);
}

void FaceGridWidget::clear()
{
    clearGrid();
}

void FaceGridWidget::clearGrid()
{
    // Remove all widgets except placeholder and stretch
    auto *layout = qobject_cast<QVBoxLayout*>(m_gridContainer->layout());
    
    // Clear thumbnail references
    m_thumbnails.clear();
    m_selectedFaces.clear();
    m_selectedClusterId = 0;
    
    // Remove all widgets except placeholder
    while (layout->count() > 2) {
        QLayoutItem *item = layout->takeAt(0);
        if (item->widget() && item->widget() != m_placeholder) {
            delete item->widget();
        }
        delete item;
    }
    
    m_placeholder->setVisible(true);
    m_placeholder->setText(tr("Open a folder to start scanning for faces"));
}

std::vector<int64_t> FaceGridWidget::selectedFaceIds() const
{
    return m_selectedFaces;
}

void FaceGridWidget::onFaceClicked(int64_t faceId)
{
    // Toggle selection
    auto it = std::find(m_selectedFaces.begin(), m_selectedFaces.end(), faceId);
    
    if (it != m_selectedFaces.end()) {
        m_selectedFaces.erase(it);
    } else {
        m_selectedFaces.push_back(faceId);
    }
    
    // Update visual state
    for (auto *thumbnail : m_thumbnails) {
        bool selected = std::find(m_selectedFaces.begin(), m_selectedFaces.end(), 
                                   thumbnail->faceId()) != m_selectedFaces.end();
        thumbnail->setSelected(selected);
    }
    
    emit faceSelected(faceId);
    emit facesSelected(m_selectedFaces);
}

void FaceGridWidget::onFaceDoubleClicked(int64_t faceId)
{
    // Get the face to find its cluster
    if (m_database) {
        auto face = m_database->get_face(faceId);
        if (face && face->cluster_id.has_value()) {
            showCluster(face->cluster_id.value());
        }
    }
}

QString FaceGridWidget::getThumbnailPath(int64_t faceId) const
{
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return QString("%1/thumbnails/face_%2.jpg").arg(dataPath).arg(faceId);
}

} // namespace facefling
