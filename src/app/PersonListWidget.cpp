/**
 * PersonListWidget implementation.
 */

#include "PersonListWidget.h"
#include <QVBoxLayout>
#include <QLabel>

namespace facefling {

PersonListWidget::PersonListWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

void PersonListWidget::setupUi()
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    
    auto *label = new QLabel(tr("People"), this);
    label->setStyleSheet("font-weight: bold; padding: 8px;");
    layout->addWidget(label);
    
    m_listWidget = new QListWidget(this);
    layout->addWidget(m_listWidget);
    
    connect(m_listWidget, &QListWidget::itemClicked,
            this, &PersonListWidget::onItemClicked);
}

void PersonListWidget::refresh()
{
    // TODO: Load persons from database
}

void PersonListWidget::clear()
{
    m_listWidget->clear();
}

void PersonListWidget::onItemClicked(QListWidgetItem *item)
{
    if (item) {
        int64_t personId = item->data(Qt::UserRole).toLongLong();
        emit personSelected(personId);
    }
}

} // namespace facefling
