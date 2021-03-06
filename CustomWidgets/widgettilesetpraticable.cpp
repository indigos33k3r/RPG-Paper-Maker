﻿/*
    RPG Paper Maker Copyright (C) 2017-2018 Marie Laporte

    This file is part of RPG Paper Maker.

    RPG Paper Maker is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    RPG Paper Maker is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "widgettilesetpraticable.h"
#include "rpm.h"
#include "dialogrect.h"
#include <QPainter>
#include <QtMath>

const int WidgetTilesetPraticable::OFFSET = 5;

// -------------------------------------------------------
//
//  CONSTRUCTOR / DESTRUCTOR / GET / SET
//
// -------------------------------------------------------

WidgetTilesetPraticable::WidgetTilesetPraticable(QWidget *parent) :
    QWidget(parent),
    m_squares(nullptr),
    m_selectedPoint(-1, -1),
    m_hoveredPoint(-1, -1),
    m_resizeKind(CollisionResizeKind::None),
    m_selectedCollision(nullptr),
    m_isCreating(false),
    m_zoom(1.0f),
    m_firstResize(false),
    m_picture(nullptr),
    m_lastCursorShape(Qt::ArrowCursor)
{
    this->setMouseTracking(true);
    this->setContextMenuPolicy(Qt::CustomContextMenu);
    this->setFocusPolicy(Qt::StrongFocus);
    this->setFocus();

    // Context menu connections
    m_contextMenu = ContextMenuList::createContextPraticable(this);
    connect(this, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(showContextMenu(const QPoint &)));
}

WidgetTilesetPraticable::~WidgetTilesetPraticable() {
    delete m_contextMenu;
}

void WidgetTilesetPraticable::setSquares(QHash<QPoint, CollisionSquare*>*
                                         squares)
{
    m_squares = squares;
}

// -------------------------------------------------------
//
//  INTERMEDIARY FUNCTIONS
//
// -------------------------------------------------------

void WidgetTilesetPraticable::updateImage(SystemPicture* picture,
                                          PictureKind kind)
{
    QString path = picture->getPath(kind);
    m_baseImage = (!path.isEmpty() && QFile::exists(path)) ? QImage(path) :
        QImage();
    updateImageGeneral(picture);
}

// -------------------------------------------------------

void WidgetTilesetPraticable::updateImageSpecial(QImage& editedImage,
                                                 SystemPicture* picture)
{
    m_baseImage = editedImage;
    updateImageGeneral(picture);
}

// -------------------------------------------------------

void WidgetTilesetPraticable::updateImageGeneral(SystemPicture* picture)
{
    m_picture = picture;
    m_image = m_baseImage;
    updateImageSize();
}

// -------------------------------------------------------

void WidgetTilesetPraticable::updateZoom(float zoom) {
    m_zoom = zoom;
    updateImageSize();
}

// -------------------------------------------------------

void WidgetTilesetPraticable::updateImageSize() {
    if (!m_image.isNull()) {
        m_image = m_baseImage.scaled(m_baseImage.width() * m_zoom,
                                     m_baseImage.height() * m_zoom);
    }
    this->setGeometry(0, 0, m_image.width() + 1, m_image.height() + 1);
    setFixedSize(m_image.width() + 1, m_image.height() + 1);
    this->repaint();
}

// -------------------------------------------------------

void WidgetTilesetPraticable::getMousePoint(QPoint& point, QMouseEvent *event) {
    point = event->pos();
    point.setX((int)(point.x() / getSquareProportion()));
    point.setY((int)(point.y() / getSquareProportion()));
}

// -------------------------------------------------------

void WidgetTilesetPraticable::getRectCollision(QRect& rect,
                                               const QPoint& localPoint,
                                               CollisionSquare* collision)
{
    if (collision->rect() == nullptr)
        return;

    getRect(rect, localPoint, *collision->rect());
}

// -------------------------------------------------------

void WidgetTilesetPraticable::getRect(QRect& rect, const QPoint& localPoint,
                                      QRectF& rectBefore)
{
    rect.setX(qRound(rectBefore.x() * getSquareProportion() / 100.0) +
              (localPoint.x() * getSquareProportion()));
    rect.setY(qRound(rectBefore.y() * getSquareProportion() / 100.0) +
              (localPoint.y() * getSquareProportion()));
    rect.setWidth(qRound(rectBefore.width() *
                         getSquareProportion() / 100.0));
    rect.setHeight(qRound(rectBefore.height() *
                          getSquareProportion() / 100.0));
}

// -------------------------------------------------------

void WidgetTilesetPraticable::getRectRepeatBot(QRect& rect) const {
    int h = qCeil(m_image.height() / 4.0f);
    rect.setX(0);
    rect.setY(h);
    rect.setWidth(m_image.width());
    rect.setHeight(m_image.height() - h);
}

// -------------------------------------------------------

void WidgetTilesetPraticable::getRectRepeatTop(QRect& rect) const {
    int w = qCeil(m_image.width() / ((float) RPM::get()->project()
                                     ->gameDatas()->systemDatas()
                                     ->framesAnimation()));
    rect.setX(w);
    rect.setY(0);
    rect.setWidth(m_image.width() - w);
    rect.setHeight(qCeil(m_image.height() / 4.0f));
}

// -------------------------------------------------------

void WidgetTilesetPraticable::getPointsRepeat(QHash<QPoint, CollisionSquare *>&
                                              list)
{
    m_picture->getRepeatList(m_baseImage, *m_squares, list);
}

// -------------------------------------------------------

void WidgetTilesetPraticable::getBasicRect(QRect& rect,
                                           const QPoint& localPoint)
{
    rect.setX(localPoint.x() * getSquareProportion());
    rect.setY(localPoint.y() * getSquareProportion());
    rect.setWidth(getSquareProportion());
    rect.setHeight(getSquareProportion());
}

// -------------------------------------------------------

bool WidgetTilesetPraticable::isMouseOn(QRect& rect, QPoint point,
                                        QPoint &mousePoint) const
{
    int offset = getOffset(rect);
    return mousePoint.x() >= point.x() - offset && mousePoint.x() <= point.x() +
           offset && mousePoint.y() >= point.y() - offset &&
           mousePoint.y() <= point.y() + offset;
}

// -------------------------------------------------------

bool WidgetTilesetPraticable::isMouseOnLeft(QRect& rect,
                                            QPoint& mousePoint) const
{
    int offset = getOffset(rect);
    return mousePoint.x() >= rect.x() - offset && mousePoint.x() <= rect.x() +
           offset;
}

// -------------------------------------------------------

bool WidgetTilesetPraticable::isMouseOnRight(QRect& rect,
                                             QPoint& mousePoint) const
{
    int offset = getOffset(rect);
    return mousePoint.x() >= rect.right() - offset && mousePoint.x() <=
           rect.right() + offset;
}

// -------------------------------------------------------

bool WidgetTilesetPraticable::isMouseOnTop(QRect& rect,
                                           QPoint& mousePoint) const
{
    int offset = getOffset(rect);
    return mousePoint.y() >= rect.y() - offset && mousePoint.y() <= rect.y() +
           offset;
}

// -------------------------------------------------------

bool WidgetTilesetPraticable::isMouseOnBot(QRect& rect,
                                           QPoint& mousePoint) const
{
    int offset = getOffset(rect);
    return mousePoint.y() >= rect.bottom() - offset && mousePoint.y() <=
           rect.bottom() + offset;
}

// -------------------------------------------------------

void WidgetTilesetPraticable::updateCursor(QRect& rect, QPoint& mousePoint)
{
    if (isMouseOn(rect, rect.topLeft(), mousePoint))
    {
        m_resizeKind = CollisionResizeKind::TopLeft;
        m_lastCursorShape = Qt::SizeFDiagCursor;
    }
    else if (isMouseOn(rect, rect.bottomRight(), mousePoint)) {
        m_resizeKind = CollisionResizeKind::BotRight;
        m_lastCursorShape = Qt::SizeFDiagCursor;
    }
    else if (isMouseOn(rect, rect.topRight(), mousePoint)) {
        m_resizeKind = CollisionResizeKind::TopRight;
        m_lastCursorShape = Qt::SizeBDiagCursor;
    }
    else if (isMouseOn(rect, rect.bottomLeft(), mousePoint)) {
        m_resizeKind = CollisionResizeKind::BotLeft;
        m_lastCursorShape = Qt::SizeBDiagCursor;
    }
    else if (isMouseOnLeft(rect, mousePoint)) {
        m_resizeKind = CollisionResizeKind::Left;
        m_lastCursorShape = Qt::SizeHorCursor;
    }
    else if (isMouseOnRight(rect, mousePoint)) {
        m_resizeKind = CollisionResizeKind::Right;
        m_lastCursorShape = Qt::SizeHorCursor;
    }
    else if (isMouseOnTop(rect, mousePoint)) {
        m_resizeKind = CollisionResizeKind::Top;
        m_lastCursorShape = Qt::SizeVerCursor;
    }
    else if (isMouseOnBot(rect, mousePoint)) {
        m_resizeKind = CollisionResizeKind::Bottom;
        m_lastCursorShape = Qt::SizeVerCursor;
    }
    else {
        m_resizeKind = CollisionResizeKind::None;
        m_lastCursorShape = Qt::ArrowCursor;
    }

    this->setCursor(QCursor(m_lastCursorShape));
}

// -------------------------------------------------------

void WidgetTilesetPraticable::updateRect(QRect &rect, QPoint& mousePoint,
                                         QPoint& localPoint,
                                         CollisionSquare* collision)
{
    QRect rectBasic;
    getBasicRect(rectBasic, localPoint);

    switch (m_resizeKind) {
    case CollisionResizeKind::Left:
        updateRectLeft(rect, mousePoint, rectBasic);
        break;
    case CollisionResizeKind::Right:
        updateRectRight(rect, mousePoint, rectBasic);
        break;
    case CollisionResizeKind::Top:
        updateRectTop(rect, mousePoint, rectBasic);
        break;
    case CollisionResizeKind::Bottom:
        updateRectBot(rect, mousePoint, rectBasic);
        break;
    case CollisionResizeKind::TopLeft:
        updateRectTop(rect, mousePoint, rectBasic);
        updateRectLeft(rect, mousePoint, rectBasic);
        break;
    case CollisionResizeKind::TopRight:
        updateRectTop(rect, mousePoint, rectBasic);
        updateRectRight(rect, mousePoint, rectBasic);
        break;
    case CollisionResizeKind::BotLeft:
        updateRectBot(rect, mousePoint, rectBasic);
        updateRectLeft(rect, mousePoint, rectBasic);
        break;
    case CollisionResizeKind::BotRight:
        updateRectBot(rect, mousePoint, rectBasic);
        updateRectRight(rect, mousePoint, rectBasic);
        break;
    default:
        break;
    }

    // Getting in percent
    float x = (rect.x() % (int)(getSquareProportion())) / getSquareProportion()
            * 100.0f;
    float y = (rect.y() % (int)(getSquareProportion())) / getSquareProportion()
            * 100.0f;
    float w = rect.width() / getSquareProportion() * 100.0f;
    float h = rect.height() / getSquareProportion() * 100.0f;

    // But round for adapting to the square size
    int i_x = qCeil((x / 100.0f) * RPM::get()->getSquareSize());
    int i_y = qCeil((y / 100.0f) * RPM::get()->getSquareSize());
    int i_w = qFloor((w / 100.0f) * RPM::get()->getSquareSize());
    int i_h = qFloor((h / 100.0f) * RPM::get()->getSquareSize());
    if (i_w < 1) {
        if (i_x >= RPM::get()->getSquareSize())
            i_x -= 1;
        i_w = 1;
    }
    if (i_h < 1) {
        if (i_y >= RPM::get()->getSquareSize())
            i_y -= 1;
        i_h = 1;
    }

    // Update collision rect with rounded values
    collision->rect()->setX((i_x / (float) RPM::get()->getSquareSize()) *
                            100.0f);
    collision->rect()->setY((i_y / (float) RPM::get()->getSquareSize()) *
                            100.0f);
    collision->rect()->setWidth((i_w / (float) RPM::get()->getSquareSize()) *
                                100.0f);
    collision->rect()->setHeight((i_h / (float) RPM::get()->getSquareSize()) *
                                 100.0f);

    // Update the fake rect (no rounded values)
    m_fakeRect = QRectF(x, y, w, h);
}

// -------------------------------------------------------

void WidgetTilesetPraticable::updateRectLeft(QRect &rect, QPoint& mousePoint,
                                             QRect& rectBasic)
{
    int value = getRectValue(mousePoint.x(), rectBasic.left(), rect.right());
    rect.setLeft(value);
    if (rect.width() == 1)
        rect.setLeft(rect.left() - 1);
}

// -------------------------------------------------------

void WidgetTilesetPraticable::updateRectRight(QRect &rect, QPoint& mousePoint,
                                              QRect& rectBasic)
{
    int value = getRectValue(mousePoint.x(), rect.left(), rectBasic.right());
    rect.setRight(value);
    if (rect.width() == 1)
        rect.setRight(rect.right() + 1);
}

// -------------------------------------------------------

void WidgetTilesetPraticable::updateRectTop(QRect &rect, QPoint& mousePoint,
                                            QRect& rectBasic)
{
    int value = getRectValue(mousePoint.y(), rectBasic.top(), rect.bottom());
    rect.setTop(value);
    if (rect.height() == 1)
        rect.setTop(rect.top() - 1);
}

// -------------------------------------------------------

void WidgetTilesetPraticable::updateRectBot(QRect &rect, QPoint& mousePoint,
                                            QRect& rectBasic)
{
    int value = getRectValue(mousePoint.y(), rect.top(), rectBasic.bottom());
    rect.setBottom(value);
    if (rect.height() == 1)
        rect.setBottom(rect.bottom() + 1);
}

// -------------------------------------------------------

int WidgetTilesetPraticable::getRectValue(int mousePos, int left, int right) {
    int value = mousePos;
    if (value < left)
        value = left;
    else if (value > right)
        value = right;

    return value;
}

// -------------------------------------------------------

void WidgetTilesetPraticable::drawCollision(
        QPainter& painter, const QPoint &localPoint, CollisionSquare* collision,
        const QColor &color, bool outline)
{
    QRect rect;
    QPen pen(painter.pen().color(), 2, Qt::DashLine);
    painter.setPen(pen);
    getRectCollision(rect, localPoint, collision);
    painter.fillRect(rect, color);
    if (outline)
        painter.drawRect(rect);
}

// -------------------------------------------------------

int WidgetTilesetPraticable::getOffset(QRect& rect) const {
    return (rect.width() < (OFFSET * 2) || rect.height() < (OFFSET * 2)) ?
           0 : OFFSET;
}

// -------------------------------------------------------

void WidgetTilesetPraticable::editCollision() {
    DialogRect dialog(m_selectedCollision->rect(), this);
    QRect rect;
    getRectCollision(rect, m_selectedPoint, m_selectedCollision);
    QPoint point = this->mapToGlobal(
                QPoint(rect.x() + getSquareProportion(),
                       rect.y() + getSquareProportion()));
    dialog.setGeometry(point.x(), point.y(), dialog.width(), dialog.height());
    dialog.exec();
}

// -------------------------------------------------------

void WidgetTilesetPraticable::deleteCollision() {
    if (m_selectedCollision->hasAllDirections()) {
        delete m_selectedCollision;
        m_squares->remove(m_selectedPoint);
        m_selectedCollision = nullptr;
        m_selectedPoint = QPoint(-1, -1);
    }
    else
        m_selectedCollision->setRect(nullptr);
}

// -------------------------------------------------------

float WidgetTilesetPraticable::getSquareProportion() const {
    return RPM::get()->getSquareSize() * m_zoom;
}

// -------------------------------------------------------

bool WidgetTilesetPraticable::canDraw(QPoint& mousePoint) const {
    QRect rectBot, rectTop;
    getRectRepeatBot(rectBot);
    getRectRepeatTop(rectTop);

    return (!m_picture->repeatCollisions() ||
        (!rectBot.contains(mousePoint) && !rectTop.contains(mousePoint)));
}

// -------------------------------------------------------
//
//  EVENTS
//
// -------------------------------------------------------

void WidgetTilesetPraticable::keyPressEvent(QKeyEvent *event){
    QKeySequence seq = RPM::getKeySequence(event);
    QList<QAction*> actions = m_contextMenu->actions();
    QAction* action;

    // Forcing shortcuts
    action = actions.at(0);
    if (RPM::isPressingEnter(event) && action->isEnabled()) {
        contextEdit();
        return;
    }
    action = actions.at(2);
    if (action->shortcut().matches(seq) && action->isEnabled()) {
        contextDelete();
        return;
    }
}

// -------------------------------------------------------

void WidgetTilesetPraticable::mousePressEvent(QMouseEvent *event) {
    QPoint mousePoint = event->pos();
    if (canDraw(mousePoint)) {
        QPoint point;

        getMousePoint(point, event);

        // Update collisions
        CollisionSquare* collision = m_squares->value(point);
        if (collision == nullptr) {
            collision = new CollisionSquare;
            collision->setDefaultPraticable();
            m_squares->insert(point, collision);
            m_isCreating = true;
        }
        else if (collision->rect() == nullptr) {
            collision->setDefaultPraticable();
            m_isCreating = true;
        }

        // Update selected collision
        m_selectedPoint = point;
        m_selectedCollision = collision;

        m_firstResize = true;

        this->repaint();
    }
}

// -------------------------------------------------------

void WidgetTilesetPraticable::mouseMoveEvent(QMouseEvent *event) {
    if (m_isCreating)
        return;

    QRect rect;
    QPoint mousePoint = event->pos();
    QPoint point;
    getMousePoint(point, event);

    if (canDraw(mousePoint)) {
        if (event->buttons() == Qt::MouseButton::LeftButton &&
            m_selectedCollision != nullptr)
        {
            if (m_firstResize) {
                m_fakeRect = *m_selectedCollision->rect();
            }
            getRect(rect, m_selectedPoint, m_fakeRect);
            updateRect(rect, mousePoint, m_selectedPoint, m_selectedCollision);
            this->setCursor(QCursor(m_lastCursorShape));
        }
        else {
            CollisionSquare* collision = m_squares->value(point);
            if (collision == nullptr) {
                this->setCursor(QCursor(Qt::ArrowCursor));
                m_lastCursorShape = Qt::ArrowCursor;
            }
            else {
                getRectCollision(rect, point, collision);
                updateCursor(rect, mousePoint);
            }
            m_hoveredPoint = point;
        }
    }
    else
        this->setCursor(QCursor(Qt::ForbiddenCursor));

    m_firstResize = false;

    this->repaint();
}

// -------------------------------------------------------

void WidgetTilesetPraticable::mouseReleaseEvent(QMouseEvent*) {
    m_isCreating = false;
}

// -------------------------------------------------------

void WidgetTilesetPraticable::paintEvent(QPaintEvent *){
    QPainter painter(this);

    // Draw background
    painter.fillRect(0, 0, m_image.width(), m_image.height(),
                     RPM::colorAlmostTransparent);

    // Draw image
    if (!m_image.isNull()) {
        painter.drawImage(0, 0, m_image);
    }

    if (m_squares == nullptr)
        return;

    // Draw all the collisions
    painter.setPen(RPM::get()->engineSettings()->theme() == ThemeKind::Dark ?
        RPM::colorAlmostWhite : RPM::colorFortyTransparent);
    for (QHash<QPoint, CollisionSquare*>::iterator i = m_squares->begin();
         i != m_squares->end(); i++)
    {
        drawCollision(painter, i.key(), i.value(),
                      RPM::colorFortyTransparent);
    }

    // Draw another layer for the selected collision
    CollisionSquare* collision = m_squares->value(m_selectedPoint);
    if (collision != nullptr) {
        painter.setPen(RPM::colorPurpleSelection);
        drawCollision(painter, m_selectedPoint, collision,
                      RPM::colorPurpleSelectionBackground);
    }

    // Draw hovered layer
    collision = m_squares->value(m_hoveredPoint);
    if (collision != nullptr) {
        drawCollision(painter, m_hoveredPoint, collision,
                      RPM::colorGrayHoverBackground, false);
    }

    // For repeat option :
    if (m_picture->repeatCollisions()) {
        QRect rect;
        getRectRepeatBot(rect);
        painter.fillRect(rect, RPM::colorFortyTransparent);
        getRectRepeatTop(rect);
        painter.fillRect(rect, RPM::colorFortyTransparent);
        QHash<QPoint, CollisionSquare*> list;
        getPointsRepeat(list);
        for (QHash<QPoint, CollisionSquare*>::iterator i = list.begin();
             i != list.end(); i++)
        {
            drawCollision(painter, i.key(), i.value(),
                          RPM::colorFortyTransparent, true);
        }
    }
}

// -------------------------------------------------------
//
//  CONTEXT MENU SLOTS
//
// -------------------------------------------------------

void WidgetTilesetPraticable::showContextMenu(const QPoint& p) {
    if (m_selectedCollision != nullptr &&m_selectedCollision->rect() != nullptr)
        m_contextMenu->showContextMenu(p);
}

// -------------------------------------------------------

void WidgetTilesetPraticable::contextEdit() {
    if (m_selectedCollision != nullptr &&m_selectedCollision->rect() != nullptr)
        editCollision();
}

// -------------------------------------------------------

void WidgetTilesetPraticable::contextDelete() {
    if (m_selectedCollision != nullptr &&m_selectedCollision->rect() != nullptr)
        deleteCollision();
}
