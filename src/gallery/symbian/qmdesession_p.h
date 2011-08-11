/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDocGallery module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#ifndef QMDESESSION_P_H
#define QMDESESSION_P_H

#include "qgallery_p.h"

#include "qgalleryqueryrequest.h"
#include "qmdegalleryresultset_p.h"

#include <QObject>
#include <QEventLoop>
#include <mdesession.h>
#include <mdequery.h>
#include <e32std.h>

QT_BEGIN_NAMESPACE

class QGalleryAbstractResponse;

class QMdeSession : public QObject, public MMdESessionObserver
{
    Q_OBJECT
public:

    QMdeSession(QObject *parent = 0);
    virtual ~QMdeSession();

public: // From MMdESessionObserver
    /**
     * For checking MdE initialization status
     * @param aSession  MdE Session which was opened
     * @param aError  Error code from the opening
     */
    void HandleSessionOpened( CMdESession &aSession, TInt aError );

    /**
     * For checking MdE session errors
     * @param aSession  MdE Session which was opened
     * @param aError  Error which has occurred
     */
    void HandleSessionError( CMdESession &aSession, TInt aError );

    CMdENamespaceDef& GetDefaultNamespaceDefL();

    CMdEObject* GetFullObjectL( const unsigned int id );

    void CommitObjectL( CMdEObject& object );

    CMdEObjectQuery* NewObjectQueryL(MMdEQueryObserver *observer,
        QGalleryQueryRequest *request,
        int &error);

    CMdEObjectQuery *NewObjectQuery(
            CMdENamespaceDef &namespaceDef,
            CMdEObjectDef &objectDef,
            MMdEQueryObserver *observer = 0);

    int RemoveObject( const unsigned int itemId );

    void AddItemAddedObserverL( MMdEObjectObserver &observer, CMdELogicCondition &condition );
    void AddItemChangedObserverL( MMdEObjectObserver &observer, RArray<TItemId> &idArray );
    void RemoveObjectObserver( MMdEObjectObserver &observer );

    void AddTrackedResultSet( QMDEGalleryResultSet* aResultSet );
    void RemoveTrackedResultSet( QMDEGalleryResultSet* aResultSet );

private:

    QEventLoop m_eventLoop;
    CMdESession *m_cmdeSession;

    RPointerArray<QMDEGalleryResultSet> m_resultSetList;
};
QT_END_NAMESPACE

#endif // QMDESESSION_H