/*
 * Cantata
 *
 * Copyright (c) 2011-2012 Craig Drummond <craig.p.drummond@gmail.com>
 *
 * ----
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "application.h"
#ifdef ENABLE_KDE_SUPPORT
#include <KDE/KCmdLineArgs>
#include <KDE/KStartupInfo>
#include <KDE/KMessageBox>
#else
#include <QtGui/QIcon>
#include <QtCore/QUrl>
#endif
#include "icon.h"
#include "utils.h"
#include "config.h"
#include "mainwindow.h"

#ifdef ENABLE_KDE_SUPPORT
#ifdef Q_WS_X11
Application::Application(Display *display, Qt::HANDLE visual, Qt::HANDLE colormap)
    : KUniqueApplication(display, visual, colormap)
    , w(0)
{
}
#endif

Application::Application()
    : KUniqueApplication()
    , w(0)
{
}

Application::~Application() {
}

int Application::newInstance() {
    if (w) {
        if (!w->isVisible()) {
            w->showNormal();
        }
    } else {
        if (0==Utils::getAudioGroupId() && KMessageBox::Cancel==KMessageBox::warningContinueCancel(0,
                i18n("You are not currently a member of the \"audio\" group. "
                        "Cantata will function better (saving of album covers, lyrics, etc. with the correct permissions) if you "
                        "(or your administrator) add yourself to this group.\n\n"
                        "Note, that if you do add yourself you will need to logout and back in for this to take effect.\n\n"
                        "Select \"Continue\" to start Cantata as is."),
                i18n("Audio Group"), KStandardGuiItem::cont(), KStandardGuiItem::cancel(), "audioGroupWarning")) {
            QApplication::exit(0);
        }
        w=new MainWindow();
    }

    #ifdef TAGLIB_FOUND
    KCmdLineArgs *args(KCmdLineArgs::parsedArgs());
    QList<QUrl> urls;
    for (int i = 0; i < args->count(); ++i) {
        urls.append(QUrl(args->url(i)));
    }
    if (!urls.isEmpty()) {
        w->load(urls);
    }
    #endif
    KStartupInfo::appStarted(startupId());
    return 0;
}
#else // ENABLE_KDE_SUPPORT
Application::Application(int &argc, char **argv)
    #ifdef CANTATA_ANDROID
    : QApplication(argc, argv)
    #else
    : QtSingleApplication(argc, argv)
    #endif
{
    #if defined TAGLIB_FOUND && !defined CANTATA_ANDROID
    connect(this, SIGNAL(messageReceived(const QString &)), SLOT(message(const QString &)));
    #endif
}

Application::~Application()
{
}

#ifdef CANTATA_ANDROID
static void setPal(QPalette &pal, QPalette::ColorGroup cg, const QStringList &parts)
{
    for(int i=0; i<parts.length(); ++i) {
        pal.setColor(cg, (QPalette::ColorRole)i, QColor(parts.at(i)));
    }
}
#endif

bool Application::start()
{
    #ifdef CANTATA_ANDROID
    QPalette pal;
    setPal(pal, QPalette::Active, QString("#ffffff, #000000, #dbdbdb, #b2b2b2, #949494, #6c6c6c, #ffffff, #ffffff, #ffffff, #222222, #000000, #b2b2b2, #00316e, #ffffff, #80b5ff, #c080ff, #0c0c0c, #000000, #000000, #ffffff").split(", ", QString::SkipEmptyParts));
    setPal(pal, QPalette::Disabled, QString("#ffffff, #000000, #dbdbdb, #b2b2b2, #949494, #6c6c6c, #ffffff, #ffffff, #ffffff, #222222, #000000, #b2b2b2, #001b3d, #ffffff, #80b5ff, #c080ff, #0c0c0c, #000000, #000000, #ffffff").split(", ", QString::SkipEmptyParts));
    setPal(pal, QPalette::Inactive, QString("#ffffff, #000000, #dbdbdb, #b2b2b2, #949494, #6c6c6c, #ffffff, #ffffff, #ffffff, #222222, #000000, #b2b2b2, #00316e, #ffffff, #80b5ff, #c080ff, #0c0c0c, #000000, #000000, #ffffff").split(", ", QString::SkipEmptyParts));
    setPalette(pal);
    #else
    if (isRunning()) {
        #ifdef TAGLIB_FOUND
        QStringList args(arguments());
        if (args.count()>1) {
            args.takeAt(0);
            sendMessage(args.join("\n"));
        }
        #endif
        return false;
    }

    Icon::setupIconTheme();
    #endif
    return true;
}

#if defined TAGLIB_FOUND
void Application::loadFiles()
{
    QStringList args(arguments());
    if (args.count()>1) {
        args.takeAt(0);
        load(args);
    }
}

void Application::message(const QString &msg)
{
    load(msg.split("\n"));
}

void Application::load(const QStringList &files)
{
    if (files.isEmpty()) {
        return;
    }

    QList<QUrl> urls;
    foreach (const QString &f, files) {
        urls.append(QUrl(f));
    }
    if (!urls.isEmpty()) {
        qobject_cast<MainWindow *>(activationWindow())->load(urls);
    }
}
#endif // TAGLIB_FOUND

#endif
