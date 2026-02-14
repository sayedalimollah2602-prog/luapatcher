#ifndef GLASSBUTTON_H
#define GLASSBUTTON_H

#include <QPushButton>
#include <QGraphicsOpacityEffect>
#include "materialicons.h"

class GlassButton : public QPushButton {
    Q_OBJECT

public:
    GlassButton(MaterialIcons::Icon icon, const QString& title, 
                const QString& description, const QString& accentColor,
                QWidget* parent = nullptr);

    // Legacy constructor for backward compatibility (icon string ignored, uses Flash)
    GlassButton(const QString& iconChar, const QString& title, 
                const QString& description, const QString& accentColor,
                QWidget* parent = nullptr);
    
    void setDescription(const QString& desc);
    void setEnabled(bool enabled);
    void setActive(bool active);
    void setColor(const QString& color);
    void setMaterialIcon(MaterialIcons::Icon icon);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    MaterialIcons::Icon m_icon;
    QString m_titleText;
    QString m_descText;
    QString m_accentColor;
    bool m_isActive = false;
    QGraphicsOpacityEffect* m_opacityEffect;
};

#endif // GLASSBUTTON_H