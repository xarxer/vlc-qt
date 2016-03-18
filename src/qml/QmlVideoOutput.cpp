/****************************************************************************
* VLC-Qt - Qt and libvlc connector library
* Copyright (C) 2016 Tadej Novak <tadej@tano.si>
*
* This library is free software: you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published
* by the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this library. If not, see <http://www.gnu.org/licenses/>.
*****************************************************************************/

#include "core/VideoFrameYUV.h"
#include "qml/QmlSource.h"
#include "qml/QmlVideoOutput.h"
#include "qml/rendering/VideoNode.h"

VlcQmlVideoOutput::VlcQmlVideoOutput()
    : _fillMode(Vlc::PreserveAspectFit),
      _source(0),
      _frameUpdated(false)
{
    setFlag(QQuickItem::ItemHasContents, true);
}

VlcQmlVideoOutput::~VlcQmlVideoOutput()
{
    setSource(0);
}

VlcQmlSource *VlcQmlVideoOutput::source() const
{
    return _source;
}

void VlcQmlVideoOutput::setSource(VlcQmlSource *source)
{
    if (source == _source)
        return;

    if (_source)
        _source->deregisterVideoOutput(this);

    _source = source;

    if (_source)
        _source->registerVideoOutput(this);

    emit sourceChanged();
}

int VlcQmlVideoOutput::fillMode() const
{
    return _fillMode;
}

void VlcQmlVideoOutput::setFillMode(int mode)
{
    if (_fillMode == mode)
        return;

    _fillMode = Vlc::FillMode(mode);

    update();

    emit fillModeChanged();
}

QSGNode *VlcQmlVideoOutput::updatePaintNode(QSGNode *oldNode,
                                            UpdatePaintNodeData *data)
{
    Q_UNUSED(data)

    VideoNode *node = static_cast<VideoNode *>(oldNode);
    if (!_frame) {
        delete node;
        return 0;
    }

    if (!node)
        node = new VideoNode;

    QRectF outRect(0, 0, width(), height());
    QRectF srcRect(0, 0, 1., 1.);

    if (fillMode() != Vlc::Stretch) {
        const uint16_t fw = _frame->width;
        const uint16_t fh = _frame->height;

        const qreal frameAspect = qreal(fw) / fh;
        const qreal itemAspect = width() / height();

        if (fillMode() == Vlc::PreserveAspectFit) {
            qreal outWidth = width();
            qreal outHeight = height();
            if (frameAspect > itemAspect)
                outHeight = outWidth / frameAspect;
            else if (frameAspect < itemAspect)
                outWidth = outHeight * frameAspect;

            outRect = QRectF((width() - outWidth) / 2, (height() - outHeight) / 2,
                             outWidth, outHeight);
        } else if (fillMode() == Vlc::PreserveAspectCrop) {
            if (frameAspect > itemAspect) {
                srcRect.setX((1. - itemAspect / frameAspect) / 2);
                srcRect.setWidth(1. - srcRect.x() - srcRect.x());
            } else if (frameAspect < itemAspect) {
                srcRect.setY((1. - frameAspect / itemAspect) / 2);
                srcRect.setHeight(1. - srcRect.y() - srcRect.y());
            }
        }
    }

    if (_frameUpdated) {
        node->setFrame(_frame);
        _frameUpdated = false;
    }
    node->setRect(outRect, srcRect);

    return node;
}

void VlcQmlVideoOutput::presentFrame(const std::shared_ptr<const VlcVideoFrameYUV> &frame)
{
    _frame = frame;
    _frameUpdated = true;
    update();
}
