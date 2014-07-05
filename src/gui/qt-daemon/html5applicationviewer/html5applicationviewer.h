// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef HTML5APPLICATIONVIEWER_H
#define HTML5APPLICATIONVIEWER_H

#include <QWidget>
#include <QUrl>
#include "view_iface.h"
#ifndef Q_MOC_RUN
#include "daemon_backend.h"
#endif

class QGraphicsWebView;

class Html5ApplicationViewer : public QWidget, public view::i_view
{
    Q_OBJECT

public:
    enum ScreenOrientation {
        ScreenOrientationLockPortrait,
        ScreenOrientationLockLandscape,
        ScreenOrientationAuto
    };

    explicit Html5ApplicationViewer(QWidget *parent = 0);
    virtual ~Html5ApplicationViewer();

    void loadFile(const QString &fileName);
    void loadUrl(const QUrl &url);

    // Note that this will only have an effect on Fremantle.
    void setOrientation(ScreenOrientation orientation);

    void showExpanded();

    QGraphicsWebView *webView() const;
    bool start_backend(int argc, char* argv[]);
protected:

private slots:
    bool do_close();
    bool on_request_quit();
public slots:
    void open_wallet();
    void generate_wallet();
    void close_wallet();
    QString get_version();
    QString transfer(const QString& json_transfer_object);
    void message_box(const QString& msg);
    QString request_uri(const QString& uri, const QString& params);

private:
    void closeEvent(QCloseEvent *event);


    //------- i_view ---------
    virtual bool update_daemon_status(const view::daemon_status_info& info);
    virtual bool on_backend_stopped();
    virtual bool show_msg_box(const std::string& message);
    virtual bool update_wallet_status(const view::wallet_status_info& wsi);
    virtual bool update_wallet_info(const view::wallet_info& wsi);
    virtual bool money_receive(const view::transfer_event_info& tei);
    virtual bool money_spent(const view::transfer_event_info& tei);
    virtual bool show_wallet();
    virtual bool hide_wallet();
    virtual bool switch_view(int view_no);

    //----------------------------------------------
    bool is_uri_allowed(const QString& uri);


    class Html5ApplicationViewerPrivate *m_d;
    daemon_backend m_backend;
    std::atomic<bool> m_quit_requested;
    std::atomic<bool> m_deinitialize_done;
};

#endif // HTML5APPLICATIONVIEWER_H
