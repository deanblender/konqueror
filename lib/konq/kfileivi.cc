/* This file is part of the KDE project
   Copyright (C) 1999 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "kfileivi.h"
#include "konqfileitem.h"
#include "konqdrag.h"
#include "konqiconviewwidget.h"
#include "konqoperations.h"

#include <kapp.h>
#include <kipc.h>
#undef Bool

KFileIVI::KFileIVI( KonqIconViewWidget *iconview, KonqFileItem* fileitem, int size )
    : QIconViewItem( iconview, fileitem->text(),
		     fileitem->pixmap( size, KIcon::DefaultState ) ),
  m_size(size), m_state( KIcon::DefaultState ),
    m_bDisabled( false ), m_bThumbnail( false ), m_fileitem( fileitem )
{
    setDropEnabled( S_ISDIR( m_fileitem->mode() ) );
}

void KFileIVI::setIcon( int size, int state, bool recalc, bool redraw )
{
    m_size = size;
    if ( m_bDisabled )
      m_state = KIcon::DisabledState;
    else
      m_state = state;
    QIconViewItem::setPixmap( m_fileitem->pixmap( m_size, m_state ), recalc, redraw );
}

void KFileIVI::setDisabled( bool disabled )
{
    if ( m_bDisabled != disabled && !isThumbnail() )
    {
        m_bDisabled = disabled;
        m_state = m_bDisabled ? KIcon::DisabledState : KIcon::DefaultState;
        QIconViewItem::setPixmap( m_fileitem->pixmap( m_size, m_state ), false, true );
    }
}

void KFileIVI::setThumbnailPixmap( const QPixmap & pixmap )
{
    m_bThumbnail = true;
    QIconViewItem::setPixmap( pixmap, true /* recalc */ );
}

void KFileIVI::refreshIcon( bool redraw )
{
    if ( !isThumbnail())
        QIconViewItem::setPixmap( m_fileitem->pixmap( m_size, m_state ), true, redraw );
}

bool KFileIVI::acceptDrop( const QMimeSource *mime ) const
{
    if ( mime->provides( "text/uri-list" ) ) // We're dragging URLs
    {
        if ( m_fileitem->acceptsDrops() ) // Directory, executables, ...
            return true;
        KURL::List uris;
        if ( iconView()->inherits( "KonqIconViewWidget" ) )
            // Use cache if we can
            uris = ( static_cast<KonqIconViewWidget*>(iconView()) )->dragURLs();
        else
            KonqDrag::decode( mime, uris );

        // Check if we want to drop something on itself
        // (Nothing will happen, but it's a convenient way to move icons)
        KURL::List::Iterator it = uris.begin();
        for ( ; it != uris.end() ; it++ )
        {
            if ( m_fileitem->url().cmp( *it, true /*ignore trailing slashes*/ ) )
                return true;
        }
    }
    return QIconViewItem::acceptDrop( mime );
}

void KFileIVI::setKey( const QString &key )
{
    QString theKey = key;

    QVariant sortDirProp = iconView()->property( "sortDirectoriesFirst" );

    if ( S_ISDIR( m_fileitem->mode() ) && ( !sortDirProp.isValid() || ( sortDirProp.type() == QVariant::Bool && sortDirProp.toBool() ) ) )
      theKey.prepend( iconView()->sortDirection() ? '0' : '1' );
    else
      theKey.prepend( iconView()->sortDirection() ? '1' : '0' );

    QIconViewItem::setKey( theKey );
}

void KFileIVI::dropped( QDropEvent *e, const QValueList<QIconDragItem> & )
{
  KonqOperations::doDrop( item(), e, iconView() );
}

void KFileIVI::returnPressed()
{
    m_fileitem->run();
}

void KFileIVI::paintItem( QPainter *p, const QColorGroup &cg )
{
    QColorGroup c( cg );
    c.setColor( QColorGroup::Text, static_cast<KonqIconViewWidget*>(iconView())->itemColor() );
    if ( m_fileitem->isLink() )
    {
        QFont f( p->font() );
        f.setItalic( TRUE );
        p->setFont( f );
    }
    QIconViewItem::paintItem( p, c );
}

