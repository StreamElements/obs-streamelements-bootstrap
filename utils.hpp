#pragma once

#include <obs-frontend-api.h>
#include <util/platform.h>
#include <util/threading.h>

#include <QMainWindow>
#include <QStatusBar>
#include <QMessageBox>
#include <QThread>
#include <QTimer>

#include <functional>

inline static void QtExecSync(void(*func)(void*), void* const data)
{
	if (QThread::currentThread() == qApp->thread()) {
		func(data);
	}
	else {
		os_event_t* completeEvent;

		os_event_init(&completeEvent, OS_EVENT_TYPE_AUTO);

		QTimer *t = new QTimer();
		t->moveToThread(qApp->thread());
		t->setSingleShot(true);
		QObject::connect(t, &QTimer::timeout, [=]() {
			t->deleteLater();

			func(data);

			os_event_signal(completeEvent);
		});
		QMetaObject::invokeMethod(t, "start", Qt::QueuedConnection, Q_ARG(int, 0));

		QApplication::sendPostedEvents();

		os_event_wait(completeEvent);
		os_event_destroy(completeEvent);
	}
}

inline static void QtExecSync(std::function<void()> func)
{
	if (QThread::currentThread() == qApp->thread()) {
		func();
	} else {
		os_event_t *completeEvent;

		os_event_init(&completeEvent, OS_EVENT_TYPE_AUTO);

		QTimer *t = new QTimer();
		t->moveToThread(qApp->thread());
		t->setSingleShot(true);
		QObject::connect(t, &QTimer::timeout, [=]() {
			t->deleteLater();

			func();

			os_event_signal(completeEvent);
		});
		QMetaObject::invokeMethod(t, "start", Qt::QueuedConnection,
					  Q_ARG(int, 0));

		QApplication::sendPostedEvents();

		os_event_wait(completeEvent);
		os_event_destroy(completeEvent);
	}
}

inline static void QtPostTask(void (*func)(void *), void *const data)
{
	QTimer *t = new QTimer();
	t->moveToThread(qApp->thread());
	t->setSingleShot(true);
	QObject::connect(t, &QTimer::timeout, [=]() {
		t->deleteLater();

		func(data);
	});
	QMetaObject::invokeMethod(t, "start", Qt::QueuedConnection,
				  Q_ARG(int, 0));
}

inline static void QtPostTask(std::function<void()> func)
{
	QTimer *t = new QTimer();
	t->moveToThread(qApp->thread());
	t->setSingleShot(true);
	QObject::connect(t, &QTimer::timeout, [=]() {
		t->deleteLater();

		func();
	});
	QMetaObject::invokeMethod(t, "start", Qt::QueuedConnection,
				  Q_ARG(int, 0));
}

inline static void ShowStatusBarMessage(std::string msg, bool showMsgBox, int timeout = 3000)
{
    blog(LOG_INFO, "obs-streamelements: ShowStatusBarMessage: %s", msg.c_str());
    
	struct local_context {
		std::string text;
		bool showMsgBox;
		int timeout;
	};

	local_context* context = new local_context();
	context->text = msg;;
	context->showMsgBox = showMsgBox;
	context->timeout = timeout;

	QtPostTask([](void* data) {
		local_context* context = (local_context*)data;

		QMainWindow* mainWindow = (QMainWindow*)obs_frontend_get_main_window();
		mainWindow->statusBar()->showMessage(("SE.Live: " + context->text).c_str(), context->timeout);

		if (context->showMsgBox) {
			QMessageBox msgBox;
			msgBox.setWindowTitle("StreamElements: The Ultimate Streamer Platform"),
			msgBox.setText(QString(context->text.c_str()));
			msgBox.exec();
		}

		delete context;
	}, context);
}
