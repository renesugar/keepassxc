/*
 *  Copyright (C) 2012 Felix Geyer <debfx@fobos.de>
 *  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 or (at your option)
 *  version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef KEEPASSX_SETTINGSWIDGET_H
#define KEEPASSX_SETTINGSWIDGET_H

#include "gui/EditWidget.h"

namespace Ui
{
    class ApplicationSettingsWidgetGeneral;
    class ApplicationSettingsWidgetSecurity;
} // namespace Ui

class ISettingsPage
{
public:
    virtual ~ISettingsPage()
    {
    }
    virtual QString name() = 0;
    virtual QIcon icon() = 0;
    virtual QWidget* createWidget() = 0;
    virtual void loadSettings(QWidget* widget) = 0;
    virtual void saveSettings(QWidget* widget) = 0;
};

class ApplicationSettingsWidget : public EditWidget
{
    Q_OBJECT

public:
    explicit ApplicationSettingsWidget(QWidget* parent = nullptr);
    ~ApplicationSettingsWidget();
    void addSettingsPage(ISettingsPage* page);
    void loadSettings();

private slots:
    void saveSettings();
    void reject();
    void autoSaveToggled(bool checked);
    void systrayToggled(bool checked);
    void toolbarSettingsToggled(bool checked);
    void rememberDatabasesToggled(bool checked);

private:
    QWidget* const m_secWidget;
    QWidget* const m_generalWidget;
    const QScopedPointer<Ui::ApplicationSettingsWidgetSecurity> m_secUi;
    const QScopedPointer<Ui::ApplicationSettingsWidgetGeneral> m_generalUi;
    Qt::Key m_globalAutoTypeKey;
    Qt::KeyboardModifiers m_globalAutoTypeModifiers;
    class ExtraPage;
    QList<ExtraPage> m_extraPages;
};

#endif // KEEPASSX_SETTINGSWIDGET_H
