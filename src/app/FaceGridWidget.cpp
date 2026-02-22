/**
 * FaceGridWidget implementation.
 */

#include "FaceGridWidget.h"
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>

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
    
    m_gridContainer = new QWidget(this);
    m_scrollArea->setWidget(m_gridContainer);
    
    layout->addWidget(m_scrollArea);
    
    // Placeholder message
    auto *placeholder = new QLabel(tr("Open a folder to start scanning for faces"), m_gridContainer);
    placeholder->setAlignment(Qt::AlignCenter);
    auto *gridLayout = new QVBoxLayout(m_gridContainer);
    gridLayout->addWidget(placeholder);
}

void FaceGridWidget::showAllClusters()
{
    // TODO: Load and display all clusters
}

void FaceGridWidget::showCluster(int64_t clusterId)
{
    Q_UNUSED(clusterId);
    // TODO: Load and display faces in cluster
}

void FaceGridWidget::showPerson(int64_t personId)
{
    Q_UNUSED(personId);
    // TODO: Load and display faces for person
}

void FaceGridWidget::clear()
{
    // TODO: Clear grid
}

} // namespace facefling
