/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtDocGallery module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qdeclarativegallerytype.h"


#include <qgalleryresultset.h>

#include <QtCore/qcoreapplication.h>
#include <QtQml/qqmlinfo.h>
#include <QtQml/qqmlpropertymap.h>

QT_BEGIN_NAMESPACE_DOCGALLERY

QDeclarativeGalleryType::QDeclarativeGalleryType(QObject *parent)
    : QObject(parent)
    , m_metaData(0)
    , m_status(Null)
    , m_updateStatus(Incomplete)
{
    connect(&m_request, SIGNAL(stateChanged(QGalleryAbstractRequest::State)),
            this, SLOT(_q_stateChanged()));
    connect(&m_request, SIGNAL(progressChanged(int,int)), this, SIGNAL(progressChanged()));

    connect(&m_request, SIGNAL(typeChanged()),
            this, SLOT(_q_typeChanged()));
    connect(&m_request, SIGNAL(metaDataChanged(QList<int>)),
            this, SLOT(_q_metaDataChanged(QList<int>)));

    m_metaData = new QQmlPropertyMap(this);
}

QDeclarativeGalleryType::~QDeclarativeGalleryType()
{
}

qreal QDeclarativeGalleryType::progress() const
{
    const int max = m_request.maximumProgress();

    return max > 0
            ? qreal(m_request.currentProgress()) / max
            : qreal(0.0);
}

void QDeclarativeGalleryType::setPropertyNames(const QStringList &names)
{
    if (m_updateStatus == Incomplete) {
        m_request.setPropertyNames(names);

        Q_EMIT propertyNamesChanged();
    }
}

void QDeclarativeGalleryType::setAutoUpdate(bool enabled)
{
    if (m_request.autoUpdate() != enabled) {
        m_request.setAutoUpdate(enabled);

        if (enabled)
            deferredExecute();
        else if (m_status == Idle)
            m_request.cancel();

        Q_EMIT autoUpdateChanged();
    }
}

void QDeclarativeGalleryType::componentComplete()
{
    m_updateStatus = NoUpdate;

    if (!m_request.itemType().isEmpty())
        m_request.execute();
}

void QDeclarativeGalleryType::reload()
{
    if (m_updateStatus == PendingUpdate)
        m_updateStatus = CanceledUpdate;

    m_request.execute();
}

void QDeclarativeGalleryType::cancel()
{
    if (m_updateStatus == PendingUpdate)
        m_updateStatus = CanceledUpdate;

    m_request.cancel();
}

void QDeclarativeGalleryType::clear()
{
    if (m_updateStatus == PendingUpdate)
        m_updateStatus = CanceledUpdate;

    m_request.clear();
}

void QDeclarativeGalleryType::deferredExecute()
{
    if (m_updateStatus == NoUpdate) {
        m_updateStatus = PendingUpdate;

        QCoreApplication::postEvent(this, new QEvent(QEvent::UpdateRequest));
    } else if (m_updateStatus == CanceledUpdate) {
        m_updateStatus = PendingUpdate;
    }
}

bool QDeclarativeGalleryType::event(QEvent *event)
{
    if (event->type() == QEvent::UpdateRequest) {
        UpdateStatus status = m_updateStatus;
        m_updateStatus = NoUpdate;

        if (status == PendingUpdate)
            m_request.execute();

        return true;
    } else {
        return QObject::event(event);
    }
}

void QDeclarativeGalleryType::_q_stateChanged()
{
    m_status = Status(m_request.state());

    if (m_status == Error) {
        const QString message = m_request.errorString();

        if (!message.isEmpty()) {
            qmlInfo(this) << message;
        } else {
            switch (m_request.error()) {
            case QDocumentGallery::ConnectionError:
                qmlInfo(this) << tr("An error was encountered connecting to the document gallery");
                break;
            case QDocumentGallery::ItemTypeError:
                qmlInfo(this) << tr("DocumentGallery.%1 is not a supported item type")
                        .arg(m_request.itemType());
                break;
            default:
                break;
            }
        }
        Q_EMIT statusChanged();
    } else if (m_status == Idle && !m_request.autoUpdate()) {
        m_request.cancel();
    } else {
        Q_EMIT statusChanged();
    }
}

void QDeclarativeGalleryType::_q_typeChanged()
{
    if (m_request.isValid()) {
        for (QHash<int, QString>::const_iterator it = m_propertyKeys.constBegin();
                it != m_propertyKeys.constEnd();
                ++it) {
            if (m_request.propertyKey(it.value()) < 0)
                m_metaData->clear(it.value());
        }

        m_propertyKeys.clear();

        const QStringList propertyNames = m_request.propertyNames();

        for (QStringList::const_iterator it = propertyNames.begin(); it != propertyNames.end(); ++it) {
            const int key = m_request.propertyKey(*it);

            if (key >= 0) {
                m_propertyKeys.insert(key, *it);

                QVariant value = m_request.metaData(key);
                m_metaData->insert(*it, value.isNull()
                        ? QVariant(m_request.propertyType(key))
                        : value);
            }
        }
    } else {
        typedef QHash<int, QString>::const_iterator iterator;
        for (iterator it = m_propertyKeys.constBegin(); it != m_propertyKeys.constEnd(); ++it)
            m_metaData->clear(it.value());

        m_propertyKeys.clear();
    }

    Q_EMIT availableChanged();
}

void QDeclarativeGalleryType::_q_metaDataChanged(const QList<int> &keys)
{
    typedef QList<int>::const_iterator iterator;
    for (iterator it = keys.begin(); it != keys.end(); ++it){
        QVariant value = m_request.metaData(*it);
        m_metaData->insert(m_propertyKeys.value(*it), value.isNull()
                ? QVariant(m_request.propertyType(*it))
                : value);
    }
}

/*!
    \qmltype DocumentGalleryType
    \instantiates QDeclarativeDocumentGalleryType

    \inmodule QtDocGallery
    \ingroup qml-gallery

    \brief The DocumentGalleryType element allows you to request information
    about an item type from the document gallery.

    This element is part of the \b {QtMobility.gallery 1.1} module.

    \sa DocumentGalleryModel, DocumentGalleryItem
*/

QDeclarativeDocumentGalleryType::QDeclarativeDocumentGalleryType(QObject *parent)
    : QDeclarativeGalleryType(parent)
{
}

QDeclarativeDocumentGalleryType::~QDeclarativeDocumentGalleryType()
{
}

void QDeclarativeDocumentGalleryType::classBegin()
{
    m_request.setGallery(QDeclarativeDocumentGallery::gallery(this));
}

/*!
    \qmlproperty enum DocumentGalleryType::status

    This property holds the status of a type request.  It can be one of:

    \list
    \li Null No \l itemType has been specified.
    \li Active Information about an \l itemType is being fetched from the gallery.
    \li Finished Information about an \l itemType is available.
    \li Idle Information about an \l itemType which will be automatically
    updated is available.
    \li Canceling The query was canceled but hasn't yet reached the
    canceled status.
    \li Canceled The query was canceled.
    \li Error Information about a type could not be retrieved due to an error.
    \endlist
*/

/*!
    \qmlproperty real DocumentGalleryType::progress

    This property holds the current progress of the request, from 0.0 (started)
    to 1.0 (finished).
*/

/*!
    \qmlproperty QStringList DocumentGalleryType::properties

    This property holds the type properties a request should return values for.
*/

/*!
    \qmlproperty bool DocumentGalleryType::autoUpdate

    This property holds whether a request should refresh its results
    automatically.
*/

/*!
    \qmlproperty enum DocumentGalleryType::itemType

    This property holds the item type that a request fetches information about.
    It can be one of:

    \list
    \li DocumentGallery.InvalidType
    \li DocumentGallery.File
    \li DocumentGallery.Folder
    \li DocumentGallery.Document
    \li DocumentGallery.Text
    \li DocumentGallery.Audio
    \li DocumentGallery.Image
    \li DocumentGallery.Video
    \li DocumentGallery.Playlist
    \li DocumentGallery.Artist
    \li DocumentGallery.AlbumArtist
    \li DocumentGallery.Album
    \li DocumentGallery.AudioGenre
    \li DocumentGallery.PhotoAlbum
    \endlist
*/

QDeclarativeDocumentGallery::ItemType QDeclarativeDocumentGalleryType::itemType() const
{
    return QDeclarativeDocumentGallery::itemTypeFromString(m_request.itemType());
}

void QDeclarativeDocumentGalleryType::setItemType(QDeclarativeDocumentGallery::ItemType itemType)
{
    const QString type = QDeclarativeDocumentGallery::toString(itemType);

    if (type != m_request.itemType()) {
        m_request.setItemType(type);

        if (m_updateStatus != Incomplete) {
            if (!type.isEmpty())
                m_request.execute();
            else
                m_request.clear();
        }

        Q_EMIT itemTypeChanged();
    }
}

/*!
    \qmlproperty bool DocumentGalleryType::available

    This property holds whether the meta-data of a type is available.
*/

/*!
    \qmlproperty object DocumentGalleryType::metaData

    This property holds the meta-data of an item type/
*/

/*!
    \qmlmethod DocumentGalleryType::reload()

    Re-queries the gallery.
*/

/*!
    \qmlmethod DocumentGalleryType::cancel()

    Cancels an executing request.
*/

/*!
    \qmlmethod DocumentGalleryType::clear()

    Clears the results of a request.
*/

QT_END_NAMESPACE_DOCGALLERY

#include "moc_qdeclarativegallerytype.cpp"
