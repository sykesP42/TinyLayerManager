# TLM UI Redesign Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Redesign TLM with a modern frameless UI, 3-theme system (Dark/Light/Glass), custom title bar with theme toggle + window controls, and a preview on/off toggle.

**Architecture:** C++ ThemeManager singleton exposes Q_PROPERTY for all visual attributes. QML binds to ThemeManager properties — switching theme re-evaluates all bindings with zero code duplication. Frameless window with custom title bar for drag + controls.

**Tech Stack:** Qt6 (Quick, Core), C++17, QML 2.15, CMake 3.16+, Win32 API

---

## File Map

| Action | File | Purpose |
|--------|------|---------|
| Create | `src/ThemeManager.h` | ThemeManager class declaration |
| Create | `src/ThemeManager.cpp` | Theme values, switching logic, QSettings |
| Rewrite | `src/qml/main.qml` | Complete UI: frameless window, title bar, split layout, theme switch, preview toggle |
| Modify | `src/main.cpp` | Register ThemeManager, remove debug logger, set FramelessWindowHint |
| Modify | `src/CMakeLists.txt` | Add ThemeManager.cpp |
| Modify | `src/resources.qrc` | Remove test_minimal.qml reference if present |
| Modify | `src/WindowListModel.cpp` | Remove debug logging added during debugging |

---

### Task 1: Create ThemeManager C++ class

**Files:**
- Create: `src/ThemeManager.h`
- Create: `src/ThemeManager.cpp`

**Step 1: Create `src/ThemeManager.h`**

```cpp
#pragma once
#include <QObject>
#include <QString>
#include <QColor>
#include <QSettings>

class ThemeManager : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString theme READ theme WRITE setTheme NOTIFY themeChanged)
    Q_PROPERTY(bool previewEnabled READ previewEnabled WRITE setPreviewEnabled NOTIFY previewEnabledChanged)

    // Colors
    Q_PROPERTY(QColor bgColor READ bgColor NOTIFY themeChanged)
    Q_PROPERTY(QColor panelColor READ panelColor NOTIFY themeChanged)
    Q_PROPERTY(QColor cardColor READ cardColor NOTIFY themeChanged)
    Q_PROPERTY(QColor textColor READ textColor NOTIFY themeChanged)
    Q_PROPERTY(QColor textSecondaryColor READ textSecondaryColor NOTIFY themeChanged)
    Q_PROPERTY(QColor accentColor READ accentColor NOTIFY themeChanged)
    Q_PROPERTY(QColor accentTextColor READ accentTextColor NOTIFY themeChanged)
    Q_PROPERTY(QColor borderColor READ borderColor NOTIFY themeChanged)
    Q_PROPERTY(QColor dangerColor READ dangerColor NOTIFY themeChanged)
    Q_PROPERTY(QColor successColor READ successColor NOTIFY themeChanged)
    Q_PROPERTY(QColor titleBarColor READ titleBarColor NOTIFY themeChanged)
    Q_PROPERTY(QColor sliderTrackColor READ sliderTrackColor NOTIFY themeChanged)

    // Shape
    Q_PROPERTY(int radius READ radius NOTIFY themeChanged)

public:
    explicit ThemeManager(QObject* parent = nullptr);

    QString theme() const { return m_theme; }
    void setTheme(const QString& t);

    bool previewEnabled() const { return m_previewEnabled; }
    void setPreviewEnabled(bool v);

    QColor bgColor() const { return m_bgColor; }
    QColor panelColor() const { return m_panelColor; }
    QColor cardColor() const { return m_cardColor; }
    QColor textColor() const { return m_textColor; }
    QColor textSecondaryColor() const { return m_textSecondaryColor; }
    QColor accentColor() const { return m_accentColor; }
    QColor accentTextColor() const { return m_accentTextColor; }
    QColor borderColor() const { return m_borderColor; }
    QColor dangerColor() const { return m_dangerColor; }
    QColor successColor() const { return m_successColor; }
    QColor titleBarColor() const { return m_titleBarColor; }
    QColor sliderTrackColor() const { return m_sliderTrackColor; }
    int radius() const { return m_radius; }

signals:
    void themeChanged();
    void previewEnabledChanged();

private:
    void applyDark();
    void applyLight();
    void applyGlass();

    QString m_theme;
    bool m_previewEnabled = true;
    QSettings m_settings;

    QColor m_bgColor, m_panelColor, m_cardColor, m_textColor,
           m_textSecondaryColor, m_accentColor, m_accentTextColor,
           m_borderColor, m_dangerColor, m_successColor,
           m_titleBarColor, m_sliderTrackColor;
    int m_radius = 6;
};
```

**Step 2: Create `src/ThemeManager.cpp`**

```cpp
#include "ThemeManager.h"

ThemeManager::ThemeManager(QObject* parent)
    : QObject(parent), m_settings("TLM", "TLMWindowControl")
{
    m_theme = m_settings.value("theme", "dark").toString();
    m_previewEnabled = m_settings.value("previewEnabled", true).toBool();

    if (m_theme == "light") applyLight();
    else if (m_theme == "glass") applyGlass();
    else applyDark();
}

void ThemeManager::setTheme(const QString& t){
    if (m_theme == t) return;
    m_theme = t;
    m_settings.setValue("theme", t);

    if (t == "light") applyLight();
    else if (t == "glass") applyGlass();
    else applyDark();

    emit themeChanged();
}

void ThemeManager::setPreviewEnabled(bool v){
    if (m_previewEnabled == v) return;
    m_previewEnabled = v;
    m_settings.setValue("previewEnabled", v);
    emit previewEnabledChanged();
}

void ThemeManager::applyDark(){
    m_bgColor         = QColor("#1e1e2e");
    m_panelColor      = QColor("#181825");
    m_cardColor       = QColor("#313244");
    m_textColor       = QColor("#cdd6f4");
    m_textSecondaryColor = QColor("#6c7086");
    m_accentColor     = QColor("#89b4fa");
    m_accentTextColor = QColor("#1e1e2e");
    m_borderColor     = QColor("#45475a");
    m_dangerColor     = QColor("#f38ba8");
    m_successColor    = QColor("#a6e3a1");
    m_titleBarColor   = QColor("#181825");
    m_sliderTrackColor = QColor("#45475a");
    m_radius = 6;
}

void ThemeManager::applyLight(){
    m_bgColor         = QColor("#f5f5f5");
    m_panelColor      = QColor("#ffffff");
    m_cardColor       = QColor("#f0f0f0");
    m_textColor       = QColor("#333333");
    m_textSecondaryColor = QColor("#888888");
    m_accentColor     = QColor("#4f8cff");
    m_accentTextColor = QColor("#ffffff");
    m_borderColor     = QColor("#e0e0e0");
    m_dangerColor     = QColor("#e53935");
    m_successColor    = QColor("#43a047");
    m_titleBarColor   = QColor("#ffffff");
    m_sliderTrackColor = QColor("#d0d0d0");
    m_radius = 10;
}

void ThemeManager::applyGlass(){
    m_bgColor         = QColor(20, 20, 40, 217);     // rgba(20,20,40,0.85)
    m_panelColor      = QColor(255, 255, 255, 20);    // rgba(255,255,255,0.08)
    m_cardColor       = QColor(255, 255, 255, 31);    // rgba(255,255,255,0.12)
    m_textColor       = QColor("#ffffff");
    m_textSecondaryColor = QColor(255, 255, 255, 153); // rgba(255,255,255,0.6)
    m_accentColor     = QColor("#89b4fa");
    m_accentTextColor = QColor("#1e1e2e");
    m_borderColor     = QColor(255, 255, 255, 38);    // rgba(255,255,255,0.15)
    m_dangerColor     = QColor("#f38ba8");
    m_successColor    = QColor("#a6e3a1");
    m_titleBarColor   = QColor(20, 20, 40, 230);
    m_sliderTrackColor = QColor(255, 255, 255, 38);
    m_radius = 10;
}
```

- [ ] **Step 3: Verify compilation-ready** — no build yet, just confirm header/cpp are syntactically complete

---

### Task 2: Update main.cpp — register ThemeManager, remove debug logger

**Files:**
- Modify: `src/main.cpp`

**Step 1: Rewrite `src/main.cpp`**

Remove the file-based debug logger added during debugging. Add ThemeManager registration. Content:

```cpp
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlComponent>
#include <QQmlError>
#include <QDir>
#include "AppBridge.h"
#include "WindowListModel.h"
#include "WindowFilterProxy.h"
#include "PresetManager.h"
#include "ThemeManager.h"

int main(int argc, char** argv){
    QGuiApplication app(argc, argv);
    app.setWindowIcon(QIcon()); // no system icon — we draw our own

    QQmlApplicationEngine engine;

    AppBridge bridge;
    WindowListModel* windowModel = new WindowListModel(&app);
    PresetManager* presetManager = new PresetManager(&app);
    ThemeManager* themeManager = new ThemeManager(&app);

    engine.rootContext()->setContextProperty("AppBridge", &bridge);
    engine.rootContext()->setContextProperty("WindowListModel", windowModel);
    engine.rootContext()->setContextProperty("PresetManager", presetManager);
    engine.rootContext()->setContextProperty("Theme", themeManager);

    WindowFilterProxy* proxy = new WindowFilterProxy(&app);
    proxy->setSourceModel(windowModel);
    engine.rootContext()->setContextProperty("WindowProxyModel", proxy);

    QQmlComponent comp(&engine, QUrl(QStringLiteral("qrc:/qml/main.qml")));
    if (!comp.errors().isEmpty()) {
        for (const QQmlError &e : comp.errors()) qWarning() << e.toString();
        return -1;
    }
    if (!comp.create()) {
        for (const QQmlError &e : comp.errors()) qWarning() << e.toString();
        return -1;
    }

    return app.exec();
}
```

Note: Add `#include <QIcon>` at top.

---

### Task 3: Update CMakeLists.txt — add ThemeManager

**Files:**
- Modify: `src/CMakeLists.txt`

**Step 1: Add ThemeManager.cpp to target sources**

Current source list in `src/CMakeLists.txt`:
```
add_executable(tlmapp
    main.cpp
    AppBridge.cpp
    WindowOperator.cpp
    WindowListModel.cpp
    WindowEnumerator.cpp
    IconProvider.cpp
    WinEventMonitor.cpp
    WindowFilterProxy.cpp
    PresetManager.cpp
    resources.qrc
)
```

Add `ThemeManager.cpp` after `PresetManager.cpp`.

---

### Task 4: Remove debug logging from WindowListModel.cpp

**Files:**
- Modify: `src/WindowListModel.cpp`

**Step 1: Remove all qDebug lines added during debugging session**

In `screenshot()` method, revert the diagnostic logging back to clean code. The method should be:

```cpp
QString WindowListModel::screenshot(int row){
    if (row < 0 || row >= m_items.size()) return QString();
#ifdef _WIN32
    HWND h = (HWND) m_items[row].hwnd;
    if (!h) return QString();

    HBITMAP hbmp = WindowOperator::captureWindowBitmap(h);
    if (!hbmp) return QString();

    BITMAP bmp;
    GetObject(hbmp, sizeof(BITMAP), &bmp);
    int w = bmp.bmWidth;
    int hgt = bmp.bmHeight;
    BITMAPINFOHEADER bi;
    ZeroMemory(&bi, sizeof(bi));
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = w;
    bi.biHeight = -hgt;
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = BI_RGB;

    int imageSize = w * hgt * 4;
    std::vector<char> buf(imageSize);
    HDC hdc = GetDC(NULL);
    if (!GetDIBits(hdc, hbmp, 0, hgt, buf.data(), (BITMAPINFO*)&bi, DIB_RGB_COLORS)){
        ReleaseDC(NULL, hdc);
        DeleteObject(hbmp);
        return QString();
    }
    ReleaseDC(NULL, hdc);

    QImage img((uchar*)buf.data(), w, hgt, QImage::Format_ARGB32);
    QString base = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QDir().mkpath(base);
    QString filename = base + "/tlm_" + sanitizeFilename(m_items[row].title) + "_" + QString::number(QDateTime::currentSecsSinceEpoch()) + ".png";
    img.save(filename, "PNG");

    DeleteObject(hbmp);
    QString fileUrl = QUrl::fromLocalFile(filename).toString();
    const_cast<WindowItem&>(m_items[row]).thumbnailPath = fileUrl;
    QModelIndex idx = index(row, 0);
    emit dataChanged(idx, idx, {ThumbnailRole});
    return fileUrl;
#else
    Q_UNUSED(row);
    return QString();
#endif
}
```

Also remove debug logging in `WindowFilterProxy.cpp` `screenshot()` method — revert to clean version.

---

### Task 5: Rewrite main.qml — complete UI redesign

**Files:**
- Rewrite: `src/qml/main.qml`

This is the core task. The QML must implement:

1. **Frameless ApplicationWindow** — `flags: Qt.FramelessWindowHint | Qt.Window`
2. **Custom title bar** (38px) — drag-to-move, theme dropdown, minimize/maximize/close
3. **Split layout** — left: search + window list, right: info + controls + preview
4. **Theme binding** — all colors reference `Theme.xxx`
5. **Preview toggle** — switch in preview header, controls auto-capture behavior
6. **Window resize** — mouse handles on edges

**Full QML content:**

```qml
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

ApplicationWindow {
    id: root
    visible: true
    width: 780
    height: 500
    minimumWidth: 600
    minimumHeight: 380
    color: "transparent"
    flags: Qt.FramelessWindowHint | Qt.Window

    Component.onCompleted: WindowListModel.refresh()

    // --- Drag support ---
    property point clickPos: "0,0"

    // --- Theme dropdown state ---
    property bool themeDropdownOpen: false

    // --- Main background ---
    Rectangle {
        anchors.fill: parent
        color: Theme.bgColor
        radius: Theme.radius
        border.color: Theme.borderColor
        border.width: 1
        clip: true

        // --- Custom Title Bar ---
        Rectangle {
            id: titleBar
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 38
            color: Theme.titleBarColor
            radius: Theme.radius

            // Flat bottom corners
            Rectangle {
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                height: Theme.radius
                color: Theme.titleBarColor
            }

            MouseArea {
                anchors.fill: parent
                onPressed: clickPos = Qt.point(mouseX, mouseY)
                onPositionChanged: {
                    root.x += mouseX - clickPos.x
                    root.y += mouseY - clickPos.y
                }
                onDoubleClicked: root.visibility === Window.Maximized ? root.showNormal() : root.showMaximized()
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 14
                anchors.rightMargin: 4
                spacing: 0

                // App label
                Text {
                    text: "TLM"
                    color: Theme.textColor
                    font.pixelSize: 13
                    font.bold: true
                    font.letterSpacing: 1
                }

                Item { Layout.fillWidth: true }

                // Theme toggle button
                Item {
                    id: themeBtn
                    Layout.preferredWidth: 28
                    Layout.preferredHeight: 24

                    Rectangle {
                        anchors.fill: parent
                        radius: 5
                        color: themeBtnMouse.containsMouse ? Qt.rgba(Theme.textColor.r, Theme.textColor.g, Theme.textColor.b, 0.1) : "transparent"
                    }
                    Text {
                        anchors.centerIn: parent
                        text: Theme.theme === "dark" ? "☾" : Theme.theme === "light" ? "☀" : "✨"
                        color: Theme.textSecondaryColor
                        font.pixelSize: 14
                    }
                    MouseArea {
                        id: themeBtnMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: themeDropdownOpen = !themeDropdownOpen
                    }
                }

                // Separator
                Rectangle {
                    Layout.preferredWidth: 1
                    Layout.preferredHeight: 16
                    Layout.leftMargin: 4
                    Layout.rightMargin: 4
                    color: Theme.borderColor
                }

                // Window controls
                Repeater {
                    model: [
                        { icon: "–", action: "minimize" },
                        { icon: "□", action: "maximize" },
                        { icon: "✕", action: "close", danger: true }
                    ]
                    delegate: Item {
                        Layout.preferredWidth: 38
                        Layout.preferredHeight: 28

                        Rectangle {
                            anchors.fill: parent
                            radius: 5
                            color: {
                                if (!winCtrlMouse.containsMouse) return "transparent"
                                if (modelData.danger) return Theme.dangerColor
                                return Qt.rgba(Theme.textColor.r, Theme.textColor.g, Theme.textColor.b, 0.1)
                            }
                        }
                        Text {
                            anchors.centerIn: parent
                            text: modelData.icon
                            color: {
                                if (modelData.danger && winCtrlMouse.containsMouse) return Theme.bgColor
                                return Theme.textSecondaryColor
                            }
                            font.pixelSize: modelData.action === "maximize" ? 11 : 13
                        }
                        MouseArea {
                            id: winCtrlMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: {
                                if (modelData.action === "minimize") root.showMinimized()
                                else if (modelData.action === "maximize") root.visibility === Window.Maximized ? root.showNormal() : root.showMaximized()
                                else Qt.quit()
                            }
                        }
                    }
                }
            }
        }

        // --- Theme Dropdown ---
        Rectangle {
            id: themeDropdown
            anchors.right: parent.right
            anchors.rightMargin: 90
            anchors.top: titleBar.bottom
            anchors.topMargin: 2
            width: 120
            height: themeDropdownOpen ? themeCol.implicitHeight + 8 : 0
            radius: Theme.radius
            color: Theme.panelColor
            border.color: Theme.borderColor
            border.width: 1
            visible: height > 0
            clip: true

            Behavior on height { NumberAnimation { duration: 150; easing.type: Easing.OutCubic } }

            Column {
                id: themeCol
                anchors.fill: parent
                anchors.margins: 4
                spacing: 2

                Repeater {
                    model: ["dark", "light", "glass"]
                    delegate: Rectangle {
                        width: parent.width
                        height: 30
                        radius: 4
                        color: themeItemMouse.containsMouse ? Qt.rgba(Theme.accentColor.r, Theme.accentColor.g, Theme.accentColor.b, 0.15) : "transparent"

                        Row {
                            anchors.left: parent.left
                            anchors.leftMargin: 10
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 8

                            Rectangle {
                                width: 16; height: 16; radius: 8
                                border.color: Theme.theme === modelData ? Theme.accentColor : Theme.borderColor
                                border.width: 2
                                color: Theme.theme === modelData ? Theme.accentColor : "transparent"

                                Rectangle {
                                    anchors.centerIn: parent
                                    width: 6; height: 6; radius: 3
                                    color: Theme.theme === modelData ? Theme.accentTextColor : "transparent"
                                }
                            }
                            Text {
                                text: modelData === "dark" ? "Dark" : modelData === "light" ? "Light" : "Glass"
                                color: Theme.theme === modelData ? Theme.accentColor : Theme.textColor
                                font.pixelSize: 12
                                font.bold: Theme.theme === modelData
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }
                        MouseArea {
                            id: themeItemMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: {
                                Theme.theme = modelData
                                themeDropdownOpen = false
                            }
                        }
                    }
                }
            }
        }

        // Close dropdown on outside click
        MouseArea {
            anchors.fill: parent
            enabled: themeDropdownOpen
            z: -1
            onPressed: { themeDropdownOpen = false; mouse.accepted = false }
        }

        // --- Main Content ---
        SplitView {
            anchors.top: titleBar.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            orientation: Qt.Horizontal

            // Left: Window List
            Rectangle {
                SplitView.preferredWidth: parent.width * 0.38
                SplitView.minimumWidth: 200
                color: Theme.panelColor

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 6

                    // Search
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 30
                        radius: Theme.radius
                        color: Theme.cardColor
                        border.color: searchField.activeFocus ? Theme.accentColor : Theme.borderColor
                        border.width: 1

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 1
                            spacing: 6

                            Text {
                                text: "🔍"
                                color: Theme.textSecondaryColor
                                font.pixelSize: 12
                                Layout.leftMargin: 8
                            }
                            TextField {
                                id: searchField
                                Layout.fillWidth: true
                                placeholderText: "Search windows..."
                                placeholderTextColor: Theme.textSecondaryColor
                                color: Theme.textColor
                                background: Item {}
                                font.pixelSize: 12
                                onTextChanged: WindowProxyModel.setFilterText(text)
                            }
                        }
                    }

                    // Empty state
                    Text {
                        text: "No windows found"
                        color: Theme.textSecondaryColor
                        font.pixelSize: 12
                        visible: windowList.count === 0
                        horizontalAlignment: Text.AlignHCenter
                        Layout.alignment: Qt.AlignHCenter
                        Layout.topMargin: 20
                    }

                    // List
                    ListView {
                        id: windowList
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        model: WindowProxyModel
                        clip: true
                        spacing: 2

                        delegate: Rectangle {
                            width: windowList.width
                            height: 34
                            radius: Theme.radius - 1
                            color: windowList.currentIndex === index ? Qt.rgba(Theme.accentColor.r, Theme.accentColor.g, Theme.accentColor.b, 0.15) : (itemMouse.containsMouse ? Qt.rgba(Theme.textColor.r, Theme.textColor.g, Theme.textColor.b, 0.05) : "transparent")

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 8
                                anchors.rightMargin: 6
                                spacing: 8

                                Image {
                                    source: icon || ""
                                    width: 16; height: 16
                                    visible: icon && icon !== ""
                                    Layout.alignment: Qt.AlignVCenter
                                }
                                // Fallback icon placeholder
                                Rectangle {
                                    width: 16; height: 16
                                    radius: 3
                                    color: Theme.accentColor
                                    visible: !icon || icon === ""
                                    Layout.alignment: Qt.AlignVCenter
                                }
                                Text {
                                    text: title
                                    color: windowList.currentIndex === index ? Theme.textColor : Theme.textSecondaryColor
                                    font.pixelSize: 11
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                                // Capture mini button
                                Rectangle {
                                    Layout.preferredWidth: capText.implicitWidth + 12
                                    Layout.preferredHeight: 18
                                    radius: 4
                                    color: capMouse.containsMouse ? Qt.rgba(Theme.accentColor.r, Theme.accentColor.g, Theme.accentColor.b, 0.2) : Qt.rgba(Theme.accentColor.r, Theme.accentColor.g, Theme.accentColor.b, 0.1)
                                    Text {
                                        id: capText
                                        anchors.centerIn: parent
                                        text: "Cap"
                                        color: Theme.accentColor
                                        font.pixelSize: 9
                                        font.bold: true
                                    }
                                    MouseArea {
                                        id: capMouse
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        onClicked: {
                                            var p = WindowProxyModel.screenshot(index)
                                            if (p) Qt.openUrlExternally(p)
                                        }
                                    }
                                }
                            }

                            MouseArea {
                                id: itemMouse
                                anchors.fill: parent
                                anchors.rightMargin: 50 // don't capture clicks on Cap button
                                hoverEnabled: true
                                onClicked: windowList.currentIndex = index
                            }
                        }

                        onCurrentIndexChanged: {
                            if (currentIndex >= 0) {
                                var m = WindowProxyModel.get(currentIndex)
                                titleField.text = m.title || ""
                                detailExe.text = m.exe || ""
                                detailClass.text = m.class || ""
                                detailPid.text = m.pid ? ("PID: " + m.pid) : ""
                                if (Theme.previewEnabled) {
                                    if (m.thumbnail && m.thumbnail !== "") {
                                        previewImg.source = m.thumbnail
                                    } else {
                                        var p = WindowProxyModel.screenshot(currentIndex)
                                        previewImg.source = p || ""
                                    }
                                }
                            } else {
                                titleField.text = ""
                                previewImg.source = ""
                                detailExe.text = ""
                                detailClass.text = ""
                                detailPid.text = ""
                            }
                        }

                        Connections {
                            target: WindowListModel
                            function onWindowsUpdated() {
                                if (WindowListModel.count() > 0) windowList.currentIndex = 0
                            }
                        }
                    }

                    // Status bar
                    Text {
                        text: WindowListModel.count() + " windows"
                        color: Theme.textSecondaryColor
                        font.pixelSize: 10
                        Layout.leftMargin: 4
                    }
                }
            }

            // Right: Controls & Preview
            Rectangle {
                SplitView.fillWidth: true
                color: Theme.bgColor

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 10

                    // Window info row
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        Rectangle {
                            Layout.preferredWidth: 36
                            Layout.preferredHeight: 36
                            radius: 8
                            color: Theme.cardColor
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Text {
                                id: titleField
                                text: ""
                                color: Theme.textColor
                                font.pixelSize: 12
                                font.bold: true
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }
                            Row {
                                spacing: 8
                                Text { id: detailExe; text: ""; color: Theme.textSecondaryColor; font.pixelSize: 10; elide: Text.ElideRight; width: 200 }
                                Text { id: detailClass; text: ""; color: Theme.textSecondaryColor; font.pixelSize: 10 }
                                Text { id: detailPid; text: ""; color: Theme.textSecondaryColor; font.pixelSize: 10 }
                            }
                        }
                    }

                    // Opacity slider
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        Text {
                            text: "Opacity"
                            color: Theme.textSecondaryColor
                            font.pixelSize: 11
                        }
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 20
                            color: "transparent"

                            Rectangle {
                                anchors.verticalCenter: parent.verticalCenter
                                width: parent.width
                                height: 6
                                radius: 3
                                color: Theme.sliderTrackColor

                                Rectangle {
                                    width: parent.width * (alphaSlider.value / 255)
                                    height: parent.height
                                    radius: 3
                                    color: Theme.accentColor
                                }
                            }
                            Slider {
                                id: alphaSlider
                                anchors.fill: parent
                                from: 0
                                to: 255
                                value: 255
                                background: Item {}
                                handle: Rectangle {
                                    x: alphaSlider.leftPadding + alphaSlider.visualPosition * (alphaSlider.availableWidth - width)
                                    y: (alphaSlider.height - height) / 2
                                    width: 14; height: 14; radius: 7
                                    color: Theme.textColor
                                    border.color: Theme.borderColor
                                }
                            }
                        }
                        Text {
                            text: Math.round(alphaSlider.value)
                            color: Theme.textColor
                            font.pixelSize: 11
                            Layout.preferredWidth: 28
                            horizontalAlignment: Text.AlignRight
                        }
                    }

                    // Action buttons
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        Repeater {
                            model: [
                                { text: "Apply", accent: true },
                                { text: "Capture", accent: false },
                                { text: "Front", accent: false },
                                { text: "Save Preset", accent: false, success: true }
                            ]
                            delegate: Rectangle {
                                Layout.preferredHeight: 28
                                Layout.preferredWidth: actionText.implicitWidth + 20
                                radius: Theme.radius - 1
                                color: {
                                    if (modelData.accent) return Theme.accentColor
                                    if (modelData.success) return Qt.rgba(Theme.successColor.r, Theme.successColor.g, Theme.successColor.b, 0.15)
                                    return Theme.cardColor
                                }
                                border.color: modelData.success ? Theme.successColor : "transparent"
                                border.width: modelData.success ? 1 : 0

                                Text {
                                    id: actionText
                                    anchors.centerIn: parent
                                    text: modelData.text
                                    color: {
                                        if (modelData.accent) return Theme.accentTextColor
                                        if (modelData.success) return Theme.successColor
                                        return Theme.textColor
                                    }
                                    font.pixelSize: 11
                                    font.bold: modelData.accent
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: {
                                        if (modelData.text === "Apply") {
                                            AppBridge.setAlphaByTitle(titleField.text, Math.round(alphaSlider.value))
                                        } else if (modelData.text === "Capture") {
                                            if (windowList.currentIndex >= 0) {
                                                var p = WindowProxyModel.screenshot(windowList.currentIndex)
                                                if (p) previewImg.source = p
                                            }
                                        } else if (modelData.text === "Front") {
                                            if (windowList.currentIndex >= 0) WindowProxyModel.bringToFront(windowList.currentIndex)
                                        } else if (modelData.text === "Save Preset") {
                                            var name = titleField.text + "_" + Date.now()
                                            PresetManager.savePreset(name, titleField.text, Math.round(alphaSlider.value))
                                        }
                                    }
                                }
                            }
                        }
                        Item { Layout.fillWidth: true }
                    }

                    // Preview area
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        radius: Theme.radius
                        color: Theme.panelColor
                        border.color: Theme.borderColor
                        border.width: 1

                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 0

                            // Preview header with toggle
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 30
                                color: "transparent"

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 10
                                    anchors.rightMargin: 8
                                    spacing: 0

                                    Text {
                                        text: "Preview"
                                        color: Theme.textSecondaryColor
                                        font.pixelSize: 11
                                    }
                                    Item { Layout.fillWidth: true }

                                    // Toggle
                                    Row {
                                        spacing: 6
                                        Layout.alignment: Qt.AlignVCenter

                                        Text {
                                            text: Theme.previewEnabled ? "ON" : "OFF"
                                            color: Theme.previewEnabled ? Theme.successColor : Theme.textSecondaryColor
                                            font.pixelSize: 9
                                            font.bold: true
                                            anchors.verticalCenter: parent.verticalCenter
                                        }
                                        Rectangle {
                                            width: 28; height: 14; radius: 7
                                            color: Theme.previewEnabled ? Theme.successColor : Theme.sliderTrackColor
                                            anchors.verticalCenter: parent.verticalCenter

                                            Rectangle {
                                                width: 10; height: 10; radius: 5
                                                anchors.verticalCenter: parent.verticalCenter
                                                x: Theme.previewEnabled ? parent.width - width - 2 : 2
                                                color: Theme.panelColor

                                                Behavior on x { NumberAnimation { duration: 120 } }
                                            }
                                            MouseArea {
                                                anchors.fill: parent
                                                onClicked: Theme.previewEnabled = !Theme.previewEnabled
                                            }
                                        }
                                    }
                                }
                            }

                            // Separator
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 1
                                color: Theme.borderColor
                            }

                            // Preview content
                            Item {
                                Layout.fillWidth: true
                                Layout.fillHeight: true

                                // Preview image (when enabled and captured)
                                Image {
                                    id: previewImg
                                    anchors.fill: parent
                                    anchors.margins: 8
                                    fillMode: Image.PreserveAspectFit
                                    visible: Theme.previewEnabled && source !== ""
                                }

                                // Disabled message
                                Text {
                                    anchors.centerIn: parent
                                    text: "Preview disabled — click Capture to force capture"
                                    color: Theme.textSecondaryColor
                                    font.pixelSize: 11
                                    visible: !Theme.previewEnabled
                                }

                                // No preview captured
                                Text {
                                    anchors.centerIn: parent
                                    text: "No preview captured"
                                    color: Theme.textSecondaryColor
                                    font.pixelSize: 11
                                    visible: Theme.previewEnabled && previewImg.source === ""
                                }
                            }
                        }
                    }
                }
            }
        }

        // --- Resize handles ---
        // Top edge
        MouseArea {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 4
            cursorShape: Qt.SizeVerCursor
            onPressed: (mouse) => { root.startSystemResize(Qt.TopEdge) }
        }
        // Bottom edge
        MouseArea {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: 4
            cursorShape: Qt.SizeVerCursor
            onPressed: (mouse) => { root.startSystemResize(Qt.BottomEdge) }
        }
        // Left edge
        MouseArea {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: 4
            cursorShape: Qt.SizeHorCursor
            onPressed: (mouse) => { root.startSystemResize(Qt.LeftEdge) }
        }
        // Right edge
        MouseArea {
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: 4
            cursorShape: Qt.SizeHorCursor
            onPressed: (mouse) => { root.startSystemResize(Qt.RightEdge) }
        }
        // Bottom-right corner
        MouseArea {
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            width: 12; height: 12
            cursorShape: Qt.SizeFDiagCursor
            onPressed: (mouse) => { root.startSystemResize(Qt.BottomEdge | Qt.RightEdge) }
        }
    }

    // --- Error toast ---
    Connections {
        target: AppBridge
        function onOperationFailed(msg) {
            errorText.text = msg
            errorToast.opacity = 1
            errorTimer.restart()
        }
    }

    Rectangle {
        id: errorToast
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 16
        width: errorText.implicitWidth + 32
        height: errorText.implicitHeight + 16
        radius: Theme.radius
        color: Theme.dangerColor
        opacity: 0
        visible: opacity > 0

        Text {
            id: errorText
            anchors.centerIn: parent
            color: Theme.bgColor
            font.pixelSize: 13
            font.bold: true
        }

        Behavior on opacity { NumberAnimation { duration: 200 } }

        Timer {
            id: errorTimer
            interval: 3000
            onTriggered: errorToast.opacity = 0
        }
    }
}
```

- [ ] **Step 2: Verify** — read the file back, check no syntax errors in QML

---

### Task 6: Build and test

**Files:**
- None (build output only)

- [ ] **Step 1: Build**

```bash
cd C:/Users/24311/Documents/coding/TLM/build_mingw64
cmake -DQt6_DIR=C:/Qt/6.9.2/mingw_64/lib/cmake/Qt6 .. && cmake --build .
```

Expected: `[100%] Built target tlmapp` with no errors.

- [ ] **Step 2: Launch and verify**

```bash
cd src && ./tlmapp.exe
```

Test checklist:
- [ ] Window appears with no system title bar
- [ ] Custom title bar with TLM label + theme button + min/max/close
- [ ] Click theme button → dropdown with Dark/Light/Glass options
- [ ] Select Light → all colors change instantly
- [ ] Select Glass → semi-transparent panels appear
- [ ] Window list populates with visible windows
- [ ] Search field filters the list
- [ ] Select a window → info updates, preview shows screenshot
- [ ] Toggle preview OFF → preview area shows "Preview disabled"
- [ ] Toggle preview ON → preview resumes
- [ ] Apply transparency → works, error toast appears if no window selected
- [ ] Drag title bar → window moves
- [ ] Drag edges → window resizes
- [ ] Close button → app exits

- [ ] **Step 3: Mark plan complete**
