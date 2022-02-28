﻿#include <QCoreApplication>
#include <QDir>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonValue>
#include <QMessageBox>
#include <QRandomGenerator>

#include <QCefContext.h>

#include "MainWindow.h"

#define URL_ROOT "http://QCefViewDoc"
#define INDEX_URL URL_ROOT "/index.html"
#define TUTORIAL_URL URL_ROOT "/tutorial.html"

MainWindow::MainWindow(QWidget* parent)
  : QMainWindow(parent)
{
  ui.setupUi(this);
  QHBoxLayout* layout = new QHBoxLayout();
  layout->setContentsMargins(2, 2, 2, 2);
  layout->setSpacing(3);

  connect(ui.btn_changeColor, &QPushButton::clicked, this, &MainWindow::onBtnChangeColorClicked);
  connect(ui.btn_callJSCode, &QPushButton::clicked, this, &MainWindow::onBtnCallJSCodeClicked);
  connect(ui.btn_newBrowser, &QPushButton::clicked, this, &MainWindow::onBtnNewBrowserClicked);
  layout->addWidget(ui.nativeContainer);

  // build the path to the web resource
  QDir dir = QCoreApplication::applicationDirPath();
#if defined(OS_MACOS)
  QString webResourceDir = /*QString("file://") +*/ QDir::toNativeSeparators(dir.filePath("../Resources/webres"));
#else
  QString webResourceDir = /*QString("file://") +*/ QDir::toNativeSeparators(dir.filePath("webres"));
#endif

  // add a local folder to URL map
  QCefContext::instance()->addLocalFolderResource(webResourceDir, URL_ROOT);

  // build settings for per QCefView
  QCefSetting setting;
  setting.setBackgroundColor(QColor::fromRgba(qRgba(250, 249, 222, 255)));

  // create the QCefView widget and add it to the layout container
  cefViewWidget = new QCefView(INDEX_URL, &setting, this);
  ui.cefContainer->layout()->addWidget(cefViewWidget);
  layout->addWidget(ui.cefContainer);

  // connect the invokeMethod to the slot
  connect(cefViewWidget, &QCefView::invokeMethod, this, &MainWindow::onInvokeMethod);

  // connect the cefQueryRequest to the slot
  connect(cefViewWidget, &QCefView::cefQueryRequest, this, &MainWindow::onQCefQueryRequest);

  connect(cefViewWidget, &QCefView::draggableRegionChanged, this, &MainWindow::onDraggableRegionChanged);

  connect(cefViewWidget, &QCefView::reportJavascriptResult, this, &MainWindow::onJavascriptResult);

  centralWidget()->setLayout(layout);
}

MainWindow::~MainWindow() {}

void
MainWindow::onDraggableRegionChanged(const QRegion& draggableRegion, const QRegion& nonDraggableRegion)
{
  draggableRegion_ = draggableRegion;
  nonDraggableRegion_ = nonDraggableRegion;
}

void
MainWindow::onInvokeMethod(int browserId, int64_t frameId, const QString& method, const QVariantList& arguments)
{
  // extract the arguments and dispatch the invocation to corresponding handler
  if (0 == method.compare("TestMethod")) {
    QString title("QCef InvokeMethod Notify");
    QString text = QString("================== Current Thread: QT_UI ==================\r\n"
                           "Frame: %1\r\n"
                           "Method: %2\r\n"
                           "Arguments:\r\n")
                     .arg(frameId)
                     .arg(method);

    for (int i = 0; i < arguments.size(); i++) {
      auto jv = QJsonValue::fromVariant(arguments[i]);

      // clang-format off
          text.append(
              QString("%1 Type:%2, Value:%3\r\n")
              .arg(i).arg(arguments[i].typeName()).arg(arguments[i].toString())
          );
      // clang-format on
    }

    auto jsonValue = QJsonDocument::fromVariant(arguments);
    auto jsonString = QString(jsonValue.toJson());
    text.append(QString("\r\nArguments List in JSON format:\r\n%1").arg(jsonString));

    QMessageBox::information(this->window(), title, text);
  } else {
  }
}

void
MainWindow::onQCefQueryRequest(int browserId, int64_t frameId, const QCefQuery& query)
{
  QString title("QCef Query Request");
  QString text = QString("Current Thread: QT_UI\r\n"
                         "Query: %1")
                   .arg(query.request());

  QMessageBox::information(this->window(), title, text);

  QString response = query.request().toUpper();
  query.setResponseResult(true, response);
  cefViewWidget->responseQCefQuery(query);
}

void
MainWindow::onJavascriptResult(int browserId, int64_t frameId, int64_t context, const QVariant& result)
{
  auto jsonValue = QJsonDocument::fromVariant(result);
  auto jsonString = QString(jsonValue.toJson());

  QString title("Javascript result notification");
  QString text = QString("Context id: %1\r\nResult in JSON format:\r\n%2").arg(context).arg(jsonString);

  QMessageBox::information(this->window(), title, text);
}

void
MainWindow::onBtnChangeColorClicked()
{
  if (cefViewWidget) {
    // create a random color
    QColor color(QRandomGenerator::global()->generate());

    // create the cef event and set the arguments
    QCefEvent event("colorChange");
    event.arguments().append(QVariant::fromValue(color.name(QColor::HexArgb)));

    // broadcast the event to all frames in all browsers created by this QCefView widget
    cefViewWidget->broadcastEvent(event);
  }
}

void
MainWindow::onBtnCallJSCodeClicked()
{
  int64_t context = 1000;
  QString code = "alert('hello QCefView'); return {k1: 'str', k2: true, k3: 100};";
  cefViewWidget->executeJavascriptWithResult(QCefView::MainFrameID, code, "", context);
}

void
MainWindow::onBtnNewBrowserClicked()
{
  QMainWindow* w = new QMainWindow(this);
  w->setAttribute(Qt::WA_DeleteOnClose);

  QCefSetting settings;
  QCefView* view = new QCefView("https://immersiveweb.dev/", &settings, w);

  w->setCentralWidget(view);
  w->resize(1024, 768);
  w->show();
}