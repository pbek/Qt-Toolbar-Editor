/**

@author Mattia Basaglia

@section License

    Copyright (C) 2013-2014 Mattia Basaglia

    This software is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This software is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Color Widgets.  If not, see <http://www.gnu.org/licenses/>.

*/
#include "toolbar_editor.hpp"
#include <QMenu>
#include <QToolBar>
#include <QDebug>
#include <QMessageBox>
#include <utils/gui.h>
#include <QRegularExpression>
#include <QRegularExpressionMatch>

Q_DECLARE_METATYPE(QMenu*)

Q_DECLARE_METATYPE(QToolBar*)

Q_DECLARE_METATYPE(QAction*)

const QString Toolbar_Editor::customToolbarNamePrefix = QString(
        "custom_toolbar_");

Toolbar_Editor::Toolbar_Editor(QWidget *parent) :
        QWidget(parent), target(NULL) {
    setupUi(this);
    _customToolbarRemovalOnly = false;

            foreach(QPushButton *b,
                    findChildren<QPushButton *>())b->setSizePolicy(
                    QSizePolicy::Expanding, QSizePolicy::Fixed);
}

void Toolbar_Editor::setTargetWindow(QMainWindow *w) {
    target = w;
    updateBars();
}

QSize Toolbar_Editor::sizeHint() const {
    return QSize(462, 220);
}

void Toolbar_Editor::apply() const {
    if (!target)
        return;

    QList<QToolBar *> new_toolbars;

    for (QMap<QString,
            QList<QAction *> >::const_iterator i = toolbar_items.begin(),
                 e = toolbar_items.end(); i != e; ++i) {
        QToolBar *newtb = target->findChild<QToolBar *>(i.key());

        if (!newtb) {
            newtb = new QToolBar(i.key(), target);
            newtb->setObjectName(i.key());
            new_toolbars.push_back(newtb);
        }

        newtb->clear();
        foreach(QAction *act, i.value()) {
                newtb->insertAction(0, act);
            }

    }

    foreach(QToolBar *toolbar, target->findChildren<QToolBar *>()) {
            if (!toolbar_items.contains(toolbar->objectName())) {
                target->removeToolBar(toolbar);
            }
        }

    foreach(QToolBar *toolbar, new_toolbars) {
            target->addToolBar(Qt::TopToolBarArea, toolbar);
            toolbar->show();
        }
}

/**
 * Returns the object names of all toolbars
 *
 * @return
 */
QStringList Toolbar_Editor::toolbarObjectNames() {
    return toolbar_items.keys();
}

void Toolbar_Editor::updateBars() {
    if (!target)
        return;

    combo_menu->clear();
    list_menu->clear();
    combo_toolbar->clear();
    list_toolbar->clear();
    toolbar_items.clear();
    int i = 0;
    QVariant v(0);

    foreach(QMenu *menu, target->findChildren<QMenu *>()) {
            QString name = menu->objectName();

            combo_menu->addItem(menu->title().replace('&', ""),
                                QVariant::fromValue(menu));

            // disable the menu in the combo box
            if (_disabledMenuNames.contains(name)) {
                QModelIndex index = combo_menu->model()->index(i, 0);
                combo_menu->model()->setData(index, v, Qt::UserRole - 1);
            }

            i++;
        }

    i = 0;
    foreach(QToolBar *toolbar, target->findChildren<QToolBar *>()) {
            QString name = toolbar->objectName();

            toolbar_items[name] = toolbar->actions();
            combo_toolbar->addItem(name);

            // disable the toolbar in the combo box
            if (_disabledToolbarNames.contains(name)) {
                QModelIndex index = combo_toolbar->model()->index(i, 0);
                combo_toolbar->model()->setData(index, v, Qt::UserRole - 1);
            }

            i++;
        }
}

/**
 * Set the toolbar names that have to be disabled
 *
 * @param names
 */
void Toolbar_Editor::setDisabledToolbarNames(QStringList names) {
    _disabledToolbarNames = names;
}

/**
 * Set the menu names that have to be disabled
 *
 * @param names
 */
void Toolbar_Editor::setDisabledMenuNames(QStringList names) {
    _disabledMenuNames = names;
}

/**
 * Set the menu action names that have to be disabled
 *
 * @param names
 */
void Toolbar_Editor::setDisabledMenuActionNames(QStringList names) {
    _disabledMenuActionNames = names;
}

void Toolbar_Editor::on_combo_menu_currentIndexChanged(int index) {
    list_menu->clear();
    QMenu *menu = combo_menu->itemData(index).value<QMenu *>();
    if (!menu)
        return;

    foreach(QAction *action, menu->actions()) {
            if (!action->isVisible()) {
                continue;
            }

            QListWidgetItem *item;

            if (!action->isSeparator()) {
                item = new QListWidgetItem(action->icon(), action->iconText());
            } else {
                item = new QListWidgetItem(tr("--(separator)--"));
                item->setTextAlignment(Qt::AlignHCenter);
            }

            QString name = action->objectName();

            // disable the item if needed
            if (name.isEmpty() || _disabledMenuActionNames.contains(name)) {
                item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
            }

            item->setData(Qt::UserRole, QVariant::fromValue(action));
            list_menu->addItem(item);
        }
}

void Toolbar_Editor::update_list_toolbar(QString name) {
    list_toolbar->clear();

            foreach(QAction *act, toolbar_items[name]) {
            QListWidgetItem *item;
            if (!act->isSeparator()) {
                item = new QListWidgetItem(act->icon(), act->iconText());
            } else {
                item = new QListWidgetItem(tr("--(separator)--"));
                item->setTextAlignment(Qt::AlignHCenter);
            }

            item->setData(Qt::UserRole, QVariant::fromValue(act));
            list_toolbar->addItem(item);
        }

    // check if toolbar removal is allowed
    button_remove_toolbar->setEnabled(allowToolbarRemoval(name));
}

void Toolbar_Editor::on_button_up_clicked() {

    int curr_index = list_toolbar->currentRow();

    QList<QAction *> &list = toolbar_items[combo_toolbar->currentText()];
    if (curr_index < 1 || curr_index >= list.size())
        return;

    qSwap(list[curr_index], list[curr_index - 1]);

    update_list_toolbar(combo_toolbar->currentText());
    list_toolbar->setCurrentRow(curr_index - 1);
}

void Toolbar_Editor::on_button_down_clicked() {

    int curr_index = list_toolbar->currentRow();

    QList<QAction *> &list = toolbar_items[combo_toolbar->currentText()];
    if (curr_index < 0 || curr_index >= list.size() - 1)
        return;

    qSwap(list[curr_index], list[curr_index + 1]);

    update_list_toolbar(combo_toolbar->currentText());
    list_toolbar->setCurrentRow(curr_index + 1);
}

void Toolbar_Editor::on_button_insert_clicked() {
    QListWidgetItem *item = list_menu->currentItem();

    if (item == Q_NULLPTR)
        return;

    insert_action(item->data(Qt::UserRole).value<QAction *>());
}

void Toolbar_Editor::on_button_remove_clicked() {
    int to_rm = list_toolbar->currentRow();

    QList<QAction *> &list = toolbar_items[combo_toolbar->currentText()];
    if (to_rm >= 0 && to_rm < list.size()) {
        list.removeAt(to_rm);
        update_list_toolbar(combo_toolbar->currentText());
        list_toolbar->setCurrentRow(to_rm - 1);
    }
}

void Toolbar_Editor::on_button_insert_separator_clicked() {
    QAction *act = new QAction(NULL);
    act->setSeparator(true);
    insert_action(act);
}

void Toolbar_Editor::insert_action(QAction *new_action) {
    int act_before = list_toolbar->currentRow();

    if (new_action) {
        QList<QAction *> &list = toolbar_items[combo_toolbar->currentText()];
        if (act_before >= 0 && act_before < list.size()) {
            list.insert(act_before + 1, new_action);
            update_list_toolbar(combo_toolbar->currentText());
            list_toolbar->setCurrentRow(act_before + 1);
        } else {
            list.push_back(new_action);
            update_list_toolbar(combo_toolbar->currentText());
            list_toolbar->setCurrentRow(list.size() - 1);
        }

    }
}

void Toolbar_Editor::on_button_remove_toolbar_clicked() {
    QString name = combo_toolbar->currentText();

    if (!allowToolbarRemoval(name))
        return;

    // ask for permission to remove the toolbar
    if (Utils::Gui::question(
            this,
            tr("Remove current toolbar"),
            tr("Remove the current toolbar?"),
            "remove-toolbar") != QMessageBox::Yes) {
        return;
    }

    toolbar_items.remove(name);
    combo_toolbar->removeItem(combo_toolbar->currentIndex());
}

void Toolbar_Editor::on_button_add_toolbar_clicked() {
    QString name = customToolbarNamePrefix +
                   QString::number(getMaxCustomToolbarId());
    toolbar_items.insert(name, QList<QAction *>());

    combo_toolbar->addItem(name);
    combo_toolbar->setCurrentIndex(combo_toolbar->count() - 1);
}

/**
 * Returns the highest custom toolbar id
 *
 * @return
 */
int Toolbar_Editor::getMaxCustomToolbarId() {
    int maxId = 0;
    QRegularExpression regExp(
            "^" + QRegularExpression::escape(customToolbarNamePrefix) +
            "(\\d)");

    if (combo_toolbar->count() == 0) {
        return 1;
    }

    for (int i = 0; i < combo_toolbar->count(); i++) {
        QString name = combo_toolbar->itemText(i);
        QRegularExpressionMatch match = regExp.match(name);

        if (!match.hasMatch()) {
            continue;
        }

        int id = match.captured(1).toInt();

        if (id > maxId) {
            maxId = id;
        }
    }

    return maxId + 1;
}

void Toolbar_Editor::setCustomToolbarRemovalOnly(bool flag) {
    _customToolbarRemovalOnly = flag;

    // check if toolbar removal is allowed
    button_remove_toolbar->setEnabled(
            allowToolbarRemoval(combo_toolbar->currentText()));
}

bool Toolbar_Editor::customToolbarRemovalOnly() const {
    return _customToolbarRemovalOnly;
}

bool Toolbar_Editor::allowToolbarRemoval(QString name) {
    return !_customToolbarRemovalOnly ||
           name.startsWith(customToolbarNamePrefix);
}

void Toolbar_Editor::on_list_menu_doubleClicked(const QModelIndex &index) {
    Q_UNUSED(index);
    on_button_insert_clicked();
}

void Toolbar_Editor::on_list_toolbar_doubleClicked(const QModelIndex &index) {
    Q_UNUSED(index);
    on_button_remove_clicked();
}
