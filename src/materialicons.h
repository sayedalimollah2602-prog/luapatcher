#ifndef MATERIALICONS_H
#define MATERIALICONS_H

#include <QPainter>
#include <QPainterPath>
#include <QRect>
#include <QColor>

// Material Design icon painter using QPainterPath.
// Each method draws a 24x24-unit icon scaled to fit the given rect.
class MaterialIcons {
public:
    enum Icon {
        Download,
        Build,        // Wrench
        Library,      // Books
        Refresh,
        Delete,
        Add,
        RestartAlt,
        Search,
        Gamepad,
        CheckCircle,
        Flash         // Lightning bolt
    };

    static void draw(QPainter& p, const QRectF& rect, const QColor& color, Icon icon) {
        p.save();
        p.setRenderHint(QPainter::Antialiasing);

        // Scale from 24x24 canonical size to target rect
        p.translate(rect.topLeft());
        qreal sx = rect.width() / 24.0;
        qreal sy = rect.height() / 24.0;
        p.scale(sx, sy);

        p.setPen(Qt::NoPen);
        p.setBrush(color);

        switch (icon) {
        case Download:   drawDownload(p); break;
        case Build:      drawBuild(p); break;
        case Library:    drawLibrary(p); break;
        case Refresh:    drawRefresh(p); break;
        case Delete:     drawDelete(p); break;
        case Add:        drawAdd(p); break;
        case RestartAlt: drawRestartAlt(p); break;
        case Search:     drawSearch(p); break;
        case Gamepad:    drawGamepad(p); break;
        case CheckCircle:drawCheckCircle(p); break;
        case Flash:      drawFlash(p); break;
        }

        p.restore();
    }

private:
    // Download arrow with tray
    static void drawDownload(QPainter& p) {
        QPainterPath path;
        // Arrow down
        path.moveTo(12, 2);
        path.lineTo(12, 15);
        path.moveTo(12, 15);

        // Arrow body
        QPainterPath arrow;
        arrow.addRect(11, 2, 2, 13);

        // Arrow head
        QPainterPath head;
        head.moveTo(7, 12);
        head.lineTo(12, 17);
        head.lineTo(17, 12);
        head.lineTo(15, 10);
        head.lineTo(13, 12);
        head.lineTo(13, 2);
        head.lineTo(11, 2);
        head.lineTo(11, 12);
        head.lineTo(9, 10);
        head.closeSubpath();

        // Tray
        QPainterPath tray;
        tray.moveTo(5, 18);
        tray.lineTo(5, 20);
        tray.lineTo(19, 20);
        tray.lineTo(19, 18);
        tray.lineTo(17, 18);
        tray.lineTo(17, 18);
        tray.lineTo(5, 18);
        tray.closeSubpath();

        p.drawPath(head);
        p.drawPath(tray);
    }

    // Wrench
    static void drawBuild(QPainter& p) {
        QPainterPath path;
        path.moveTo(22.7, 19);
        path.lineTo(13.6, 9.9);
        path.cubicTo(14.5, 7.6, 14, 4.9, 12.1, 3);
        path.cubicTo(10.1, 1, 7.1, 0.6, 4.7, 1.7);
        path.lineTo(9.0, 6.0);
        path.lineTo(6.0, 9.0);
        path.lineTo(1.7, 4.7);
        path.cubicTo(0.6, 7.1, 1.0, 10.1, 3.0, 12.1);
        path.cubicTo(4.9, 14, 7.6, 14.5, 9.9, 13.6);
        path.lineTo(19.0, 22.7);
        path.cubicTo(19.4, 23.1, 20.1, 23.1, 20.5, 22.7);
        path.lineTo(22.7, 20.5);
        path.cubicTo(23.1, 20.1, 23.1, 19.4, 22.7, 19);
        path.closeSubpath();
        p.drawPath(path);
    }

    // Library / books icon
    static void drawLibrary(QPainter& p) {
        // Three book spines + shelf
        p.drawRect(QRectF(4, 3, 3, 14));
        p.drawRect(QRectF(8.5, 3, 3, 14));

        QPainterPath angled;
        angled.moveTo(13.5, 17);
        angled.lineTo(16, 3);
        angled.lineTo(19, 3.7);
        angled.lineTo(16.5, 17.7);
        angled.closeSubpath();
        p.drawPath(angled);

        // Shelf
        p.drawRect(QRectF(2, 19, 20, 2));
    }

    // Refresh circle arrow
    static void drawRefresh(QPainter& p) {
        p.setBrush(Qt::NoBrush);
        QPen pen(p.brush().color(), 2.2);
        pen.setCapStyle(Qt::RoundCap);
        p.setPen(pen);

        // Arc
        p.drawArc(QRectF(4, 4, 16, 16), 90 * 16, -270 * 16);

        // Arrow head
        p.setPen(Qt::NoPen);
        p.setBrush(pen.color());
        QPainterPath arrow;
        arrow.moveTo(20, 8);
        arrow.lineTo(20, 3);
        arrow.lineTo(15, 8);
        arrow.closeSubpath();
        p.drawPath(arrow);
    }

    // Delete / trash
    static void drawDelete(QPainter& p) {
        // Lid
        p.drawRect(QRectF(5, 4, 14, 2));
        // Handle
        p.drawRect(QRectF(9, 2, 6, 2));
        // Body
        QPainterPath body;
        body.moveTo(6, 7);
        body.lineTo(7, 21);
        body.lineTo(17, 21);
        body.lineTo(18, 7);
        body.closeSubpath();
        p.drawPath(body);
    }

    // Plus / Add
    static void drawAdd(QPainter& p) {
        p.drawRect(QRectF(11, 5, 2, 14));
        p.drawRect(QRectF(5, 11, 14, 2));
    }

    // Restart
    static void drawRestartAlt(QPainter& p) {
        // Similar to refresh but with double arrows
        p.setBrush(Qt::NoBrush);
        QPen pen(p.brush().color(), 2.2);
        pen.setCapStyle(Qt::RoundCap);
        p.setPen(pen);

        p.drawArc(QRectF(4, 4, 16, 16), 90 * 16, -270 * 16);

        p.setPen(Qt::NoPen);
        p.setBrush(pen.color());

        // Arrow
        QPainterPath arrow;
        arrow.moveTo(12, 2);
        arrow.lineTo(16, 6);
        arrow.lineTo(12, 10);
        arrow.closeSubpath();
        p.drawPath(arrow);
    }

    // Search magnifier
    static void drawSearch(QPainter& p) {
        p.setBrush(Qt::NoBrush);
        QPen pen(p.brush().color(), 2.5);
        pen.setCapStyle(Qt::RoundCap);
        p.setPen(pen);

        // Circle
        p.drawEllipse(QRectF(3, 3, 13, 13));

        // Handle
        p.drawLine(QPointF(14.5, 14.5), QPointF(20.5, 20.5));
    }

    // Gamepad
    static void drawGamepad(QPainter& p) {
        QPainterPath path;
        path.moveTo(6, 9);
        path.cubicTo(1, 9, 1, 15, 2, 18);
        path.cubicTo(2.5, 19.5, 4, 19.5, 5, 18);
        path.lineTo(7, 15);
        path.lineTo(17, 15);
        path.lineTo(19, 18);
        path.cubicTo(20, 19.5, 21.5, 19.5, 22, 18);
        path.cubicTo(23, 15, 23, 9, 18, 9);
        path.closeSubpath();

        p.drawPath(path);

        // D-pad
        QColor btnColor(0, 0, 0, 100);
        p.setBrush(btnColor);
        p.drawRect(QRectF(8, 10.5, 4, 1.2));
        p.drawRect(QRectF(9.4, 9.2, 1.2, 4));

        // Buttons
        p.drawEllipse(QPointF(16, 11), 0.8, 0.8);
        p.drawEllipse(QPointF(18, 11), 0.8, 0.8);
    }

    // Check circle
    static void drawCheckCircle(QPainter& p) {
        // Circle
        p.drawEllipse(QRectF(2, 2, 20, 20));

        // Checkmark in white
        QPen pen(QColor(0, 0, 0), 2.5);
        pen.setCapStyle(Qt::RoundCap);
        pen.setJoinStyle(Qt::RoundJoin);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);

        QPainterPath check;
        check.moveTo(7, 12);
        check.lineTo(10.5, 15.5);
        check.lineTo(17, 8.5);
        p.drawPath(check);
    }

    // Flash / Lightning bolt
    static void drawFlash(QPainter& p) {
        QPainterPath path;
        path.moveTo(13, 2);
        path.lineTo(6, 13);
        path.lineTo(11, 13);
        path.lineTo(11, 22);
        path.lineTo(18, 11);
        path.lineTo(13, 11);
        path.closeSubpath();
        p.drawPath(path);
    }
};

#endif // MATERIALICONS_H
