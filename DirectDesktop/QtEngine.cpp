#ifdef ENABLE_QT

#include "QtEngine.h"
#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <windows.h>

static HWND g_hWorkerW = nullptr;

static BOOL CALLBACK EnumWindowsProcQt(HWND hwnd, LPARAM lParam) {
    WCHAR className[64];
    HWND hWndProgman = FindWindowW(L"Progman", L"Program Manager");
    if (GetClassNameW(hwnd, className, sizeof(className) / 2)) {
        if (wcscmp(className, L"WorkerW") == 0) {
            DWORD pid = 0, pid2 = 0;
            GetWindowThreadProcessId(hWndProgman, &pid);
            GetWindowThreadProcessId(hwnd, &pid2);
            if (pid == pid2) {
                RECT dimensions;
                GetWindowRect(hwnd, &dimensions);
                if (dimensions.right >= GetSystemMetrics(SM_CXSCREEN) && 
                    dimensions.bottom >= GetSystemMetrics(SM_CYSCREEN)) {
                    *(HWND*)lParam = hwnd;
                    return FALSE;
                }
            }
        }
    }
    return TRUE;
}

static HWND GetDesktopWorkerW_Qt() {
    HWND progman = FindWindowW(L"Progman", nullptr);
    // 0x052C with 0x0D, 1 is what DirectUI uses
    SendMessageTimeoutW(progman, 0x052C, 0x0D, 1, SMTO_NORMAL, 1000, nullptr);
    
    HWND workerw = nullptr;
    EnumWindows(EnumWindowsProcQt, (LPARAM)&workerw);
    
    if (workerw) return workerw;
    return progman;
}

int RunQtEngine(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QQmlApplicationEngine engine;

    const QUrl url(QStringLiteral("qrc:/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
            
        QQuickWindow *window = qobject_cast<QQuickWindow*>(obj);
        if (window) {
            HWND hwnd = (HWND)window->winId();
            HWND parent = GetDesktopWorkerW_Qt();
            
            SetParent(hwnd, parent);
            SetWindowPos(hwnd, nullptr, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), SWP_SHOWWINDOW);
        }
    }, Qt::QueuedConnection);
    
    engine.load(url);
    return app.exec();
}

#endif
