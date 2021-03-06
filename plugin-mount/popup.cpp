/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXDE-Qt - a lightweight, Qt based, desktop toolset
 * http://razor-qt.org
 *
 * Copyright: 2011-2013 Razor team
 * Authors:
 *   Petr Vanek <petr@scribus.info>
 *   Alexander Sokoloff <sokoloff.a@gmail.com>
 *
 * This program or library is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 *
 * END_COMMON_COPYRIGHT_HEADER */

#include "popup.h"

#include <QDesktopWidget>
#include <QVBoxLayout>
#include <Solid/StorageAccess>
#include <Solid/StorageDrive>
#include <Solid/DeviceNotifier>

// Paulo: I'm not sure what this is for
static bool hasRemovableParent(Solid::Device device)
{
    // qDebug() << "acess:" << device.udi();
    for ( ; !device.udi().isEmpty(); device = device.parent())
    {
        Solid::StorageDrive* drive = device.as<Solid::StorageDrive>();
        if (drive && drive->isRemovable())
        {
            // qDebug() << "removable parent drive:" << device.udi();
            return true;
        }
    }
    return false;
}

Popup::Popup(QWidget* parent):
    QDialog(parent,  Qt::Dialog | Qt::WindowStaysOnTopHint | Qt::CustomizeWindowHint | Qt::Popup | Qt::X11BypassWindowManagerHint),
    mPlaceholder(nullptr),
    mDisplayCount(0)
{
    setObjectName("LxQtMountPopup");
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setLayout(new QVBoxLayout(this));
    layout()->setMargin(0);

    setAttribute(Qt::WA_AlwaysShowToolTips);

    mPlaceholder = new QLabel(tr("No devices are available"), this);
    mPlaceholder->setObjectName("NoDiskLabel");
    mPlaceholder->hide();
    layout()->addWidget(mPlaceholder);

    for (Solid::Device device : Solid::Device::listFromType(Solid::DeviceInterface::StorageAccess))
        if (hasRemovableParent(device))
            addItem(device);

    connect(Solid::DeviceNotifier::instance(), &Solid::DeviceNotifier::deviceAdded,
            this, &Popup::onDeviceAdded);
    connect(Solid::DeviceNotifier::instance(), &Solid::DeviceNotifier::deviceRemoved,
            this, &Popup::onDeviceRemoved);
}

void Popup::showHide()
{
    setVisible(isHidden());
}

void Popup::onDeviceAdded(QString const & udi)
{
    Solid::Device device(udi);
    if (device.is<Solid::StorageAccess>() && hasRemovableParent(device))
        addItem(device);
}

void Popup::onDeviceRemoved(QString const & udi)
{
    MenuDiskItem* item = nullptr;
    const int size = layout()->count() - 1;
    for (int i = size; 0 <= i; --i)
    {
        QWidget *w = layout()->itemAt(i)->widget();
        if (w == mPlaceholder)
            continue;

        MenuDiskItem *it = static_cast<MenuDiskItem *>(w);
        if (udi == it->deviceUdi())
        {
            item = it;
            break;
        }
    }

    if (item != nullptr)
    {
        layout()->removeWidget(item);
        item->deleteLater();

        --mDisplayCount;
        if (mDisplayCount == 0)
            mPlaceholder->show();
    }
}

void Popup::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    realign();
}

void Popup::showEvent(QShowEvent *event)
{
    mPlaceholder->setVisible(mDisplayCount == 0);
    realign();
    setFocus();
    activateWindow();
    QWidget::showEvent(event);
    emit visibilityChanged(true);
}

void Popup::hideEvent(QHideEvent *event)
{
    QWidget::hideEvent(event);
    emit visibilityChanged(false);
}

void Popup::addItem(Solid::Device device)
{
    MenuDiskItem *item = new MenuDiskItem(device, this);
    connect(item, &MenuDiskItem::invalid, this, &Popup::onDeviceRemoved);
    item->setVisible(true);
    layout()->addWidget(item);

    mDisplayCount++;
    if (mDisplayCount != 0)
        mPlaceholder->hide();

    if (isVisible())
        realign();
}

void Popup::realign()
{
    adjustSize();
}
