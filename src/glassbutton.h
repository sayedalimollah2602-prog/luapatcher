#ifndef GLASSBUTTON_H
#define GLASSBUTTON_H

#include <QPushButton>
#include <QGraphicsOpacityEffect>

class GlassButton : public QPushButton {
    Q_OBJECT

public:
    GlassButton(const QString& iconChar, const QString& title, 
                const QString& description, const QString& accentColor,
                QWidget* parent = nullptr);
    
    void setDescription(const QString& desc);
    void setEnabled(bool enabled);
    void setActive(bool active);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QString m_iconChar;
    QString m_titleText;
    QString m_descText;
    QString m_accentColor;
    bool m_isActive = false;
    QGraphicsOpacityEffect* m_opacityEffect;
};

#endif // GLASSBUTTON_H