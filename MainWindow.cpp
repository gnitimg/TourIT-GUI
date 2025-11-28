#include "MainWindow.h"
#include "PathCalculator.h"
#include <QDebug>
#include <QWebEngineSettings>
#include <QUrl>

// 高德地图API密钥 - 请替换为您自己的密钥
const QString GAODE_API_KEY = "7e85eb64a850bbd44d757f23ba6d7e8e";

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , mapView(nullptr)
    , startEdit(nullptr)
    , endEdit(nullptr)
    , waypointEdit(nullptr)
    , confirmStartEndBtn(nullptr)
    , confirmWaypointBtn(nullptr)
    , finishBtn(nullptr)
    , waypointList(nullptr)
    , networkManager(new QNetworkAccessManager(this))
    , pathCalculator(new PathCalculator(this))
{
    setupUI();

    // 连接信号槽
    connect(networkManager, &QNetworkAccessManager::finished,
            this, &MainWindow::onGeocodeReply);

    // 连接地图加载完成信号
    if (mapView) {
        connect(mapView, &QWebEngineView::loadFinished,
                this, &MainWindow::onMapLoadFinished);
    }

    // 连接路径计算信号
    if (pathCalculator) {
        connect(pathCalculator, &PathCalculator::pathCalculated,
                this, &MainWindow::displayRoute);
    }

    // 初始化地图
    updateMap();
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUI()
{
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);

    // 左侧控制面板
    QWidget *controlPanel = new QWidget(centralWidget);
    QVBoxLayout *controlLayout = new QVBoxLayout(controlPanel);
    controlPanel->setMaximumWidth(350);

    // 起点终点输入
    QLabel *startLabel = new QLabel("起点:", controlPanel);
    startEdit = new QLineEdit(controlPanel);
    startEdit->setPlaceholderText("例如：北京");

    QLabel *endLabel = new QLabel("终点:", controlPanel);
    endEdit = new QLineEdit(controlPanel);
    endEdit->setPlaceholderText("例如：上海");

    confirmStartEndBtn = new QPushButton("确认起点终点", controlPanel);

    // 途径点输入
    QLabel *waypointLabel = new QLabel("途径点:", controlPanel);
    waypointEdit = new QLineEdit(controlPanel);
    waypointEdit->setPlaceholderText("例如：天津");
    confirmWaypointBtn = new QPushButton("确认途径点", controlPanel);

    // 完成按钮
    finishBtn = new QPushButton("计算最短路径", controlPanel);
    finishBtn->setEnabled(false);

    // 途径点列表
    QLabel *listLabel = new QLabel("已添加的途径点:", controlPanel);
    waypointList = new QListWidget(controlPanel);

    controlLayout->addWidget(startLabel);
    controlLayout->addWidget(startEdit);
    controlLayout->addWidget(endLabel);
    controlLayout->addWidget(endEdit);
    controlLayout->addWidget(confirmStartEndBtn);
    controlLayout->addSpacing(20);
    controlLayout->addWidget(waypointLabel);
    controlLayout->addWidget(waypointEdit);
    controlLayout->addWidget(confirmWaypointBtn);
    controlLayout->addSpacing(20);
    controlLayout->addWidget(listLabel);
    controlLayout->addWidget(waypointList);
    controlLayout->addWidget(finishBtn);
    controlLayout->addStretch();

    // 右侧地图
    mapView = new QWebEngineView(centralWidget);
    mapView->setMinimumSize(600, 500);

    // 启用JavaScript
    if (mapView->settings()) {
        mapView->settings()->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
        mapView->settings()->setAttribute(QWebEngineSettings::LocalStorageEnabled, true);
    }

    mainLayout->addWidget(controlPanel);
    mainLayout->addWidget(mapView);

    // 连接信号槽
    connect(confirmStartEndBtn, &QPushButton::clicked,
            this, &MainWindow::onConfirmStartEnd);
    connect(confirmWaypointBtn, &QPushButton::clicked,
            this, &MainWindow::onConfirmWaypoint);
    connect(finishBtn, &QPushButton::clicked,
            this, &MainWindow::onFinishInput);
}

void MainWindow::onConfirmStartEnd()
{
    startPoint = startEdit->text().trimmed();
    endPoint = endEdit->text().trimmed();

    if (startPoint.isEmpty() || endPoint.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "请输入起点和终点");
        return;
    }

    // 修复：简化逻辑，直接搜索两个位置
    searchLocation(startPoint, "start");
    searchLocation(endPoint, "end");

    finishBtn->setEnabled(true);

    QMessageBox::information(this, "设置成功",
                             QString("起点: %1\n终点: %2").arg(startPoint).arg(endPoint));
}

void MainWindow::onConfirmWaypoint()
{
    QString waypoint = waypointEdit->text().trimmed();
    if (waypoint.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "请输入途径点");
        return;
    }

    // 修复：正确检查重复，基于原始输入
    bool alreadyExists = false;
    for (const QString &existing : waypoints) {
        if (existing == waypoint) {
            alreadyExists = true;
            break;
        }
    }

    if (alreadyExists) {
        QMessageBox::warning(this, "重复输入", "该途径点已经添加过了");
        return;
    }

    searchLocation(waypoint, "waypoint");
}

void MainWindow::searchLocation(const QString &location, const QString &type)
{
    // 高德地图地理编码API
    QString url = QString("https://restapi.amap.com/v3/geocode/geo?key=%1&address=%2&city=").arg(GAODE_API_KEY).arg(QString::fromUtf8(QUrl::toPercentEncoding(location)));

    QNetworkRequest request;
    request.setUrl(QUrl(url));
    request.setRawHeader("Accept", "application/json");

    // 存储搜索类型以便在回复中识别
    QNetworkReply *reply = networkManager->get(request);
    reply->setProperty("searchType", type);
    reply->setProperty("location", location);

    qDebug() << "搜索位置:" << location << "类型:" << type;
}

void MainWindow::onGeocodeReply(QNetworkReply *reply)
{
    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject obj = doc.object();

    QString searchType = reply->property("searchType").toString();
    QString location = reply->property("location").toString();

    qDebug() << "地理编码响应:" << obj;

    // 正确判断 status（高德返回字符串 "1"）
    if (obj["status"].toString() == "1") {

        // 正确读取 count（必须 toString().toInt()）
        int count = obj["count"].toString().toInt();
        if (count > 0) {

            QJsonObject gc = obj["geocodes"].toArray()[0].toObject();
            QString locStr = gc["location"].toString();
            QStringList coords = locStr.split(",");

            if (coords.size() == 2) {
                double lng = coords[0].toDouble();
                double lat = coords[1].toDouble();

                locationCoordinates[location] = qMakePair(lng, lat);

                // 更新地图
                centerMap(lng, lat, gc["formatted_address"].toString(), searchType);

                // 处理途径点
                if (searchType == "waypoint") {
                    waypoints.append(location);
                    waypointList->addItem(location);
                }

                reply->deleteLater();
                return;  // <<<<<< 必须在成功时 return
            }
        }
    }

    // ---- 若执行到这里 = 失败 ----
    qDebug() << "地理编码失败，错误信息:" << obj["info"].toString();
    QMessageBox::warning(this, "搜索失败",
                         QString("未找到位置: %1\n错误信息: %2")
                             .arg(location)
                             .arg(obj["info"].toString()));

    reply->deleteLater();
}


void MainWindow::processLocationData(const QJsonObject &locationObj, const QString &searchType, const QString &originalLocation)
{
    QString formatted_address = locationObj["formatted_address"].toString();
    QString location_str = locationObj["location"].toString();

    qDebug() << "处理位置数据:" << formatted_address << "坐标:" << location_str;

    // 解析经纬度 "经度,纬度"
    QStringList coords = location_str.split(",");
    if (coords.size() == 2) {
        double lng = coords[0].toDouble();
        double lat = coords[1].toDouble();

        // 存储坐标
        locationCoordinates[originalLocation] = qMakePair(lng, lat);

        // 移动地图中心到该位置
        centerMap(lng, lat, formatted_address, searchType);

        // 显示成功消息
        QString typeText;
        if (searchType == "start") typeText = "起点";
        else if (searchType == "end") typeText = "终点";
        else typeText = "途径点";

        QMessageBox::information(this, "位置确认",
                                 QString("%1已设置:\n%2").arg(typeText).arg(formatted_address));

        // 如果是途径点，添加到列表
        if (searchType == "waypoint") {
            // 使用格式化地址添加到列表，这样更清晰
            waypoints.append(formatted_address);
            if (waypointList) {
                waypointList->addItem(formatted_address);
            }
            waypointEdit->clear();

            qDebug() << "添加途径点:" << formatted_address << "当前数量:" << waypoints.size();
        }

        qDebug() << "找到位置:" << formatted_address << "坐标:" << lng << "," << lat;
    } else {
        qDebug() << "坐标格式错误:" << location_str;
        QMessageBox::warning(this, "错误", "坐标格式错误: " + location_str);
    }
}

void MainWindow::centerMap(double lng, double lat, const QString &title, const QString &type)
{
    QString color = "blue";
    if (type == "start") color = "red";
    else if (type == "end") color = "green";
    else if (type == "waypoint") color = "orange";

    // 修复：简化JavaScript代码，确保执行
    QString script = QString(
                         "setTimeout(function() {"
                         "   try {"
                         "       if (typeof map !== 'undefined') {"
                         "           map.setCenter([%1, %2]);"
                         "           map.setZoom(15);"
                         "           if (typeof clearMarkers === 'function') {"
                         "               clearMarkers();"
                         "           }"
                         "           if (typeof addMarker === 'function') {"
                         "               addMarker(%1, %2, '%3', '%4');"
                         "           }"
                         "       }"
                         "   } catch(e) {"
                         "       console.error('地图操作错误:', e);"
                         "   }"
                         "}, 200);"
                         ).arg(lng).arg(lat).arg(title).arg(color);

    executeJavaScript(script);
}

void MainWindow::executeJavaScript(const QString &script)
{
    if (mapView && mapView->page()) {
        // 延迟执行，确保地图完全加载
        QTimer::singleShot(100, this, [this, script]() {
            mapView->page()->runJavaScript(script, [](const QVariant &result) {
                if (!result.isNull()) {
                    qDebug() << "JavaScript执行结果:" << result;
                }
            });
        });
    }
}

void MainWindow::onMapLoadFinished(bool ok)
{
    if (ok) {
        qDebug() << "地图加载成功";
    } else {
        qDebug() << "地图加载失败";
        QMessageBox::warning(this, "错误", "地图加载失败，请检查网络连接");
    }
}

void MainWindow::addWaypointToList(const QString &waypoint)
{
    // 使用原始输入名称，而不是格式化地址
    QString originalWaypoint = waypointEdit->text().trimmed();
    waypoints.append(originalWaypoint);
    if (waypointList) {
        waypointList->addItem(originalWaypoint);
    }

    qDebug() << "添加途径点:" << originalWaypoint << "当前数量:" << waypoints.size();
}

QString MainWindow::generateMapHTML()
{
    QString html = QString(R"(
        <!DOCTYPE html>
        <html lang="zh-CN">
        <head>
            <meta charset="UTF-8">
            <meta name="viewport" content="width=device-width, initial-scale=1.0">
            <title>路径规划地图</title>
            <style>
                * { margin: 0; padding: 0; }
                #container {
                    width: 100%;
                    height: 100vh;
                    background: #f0f0f0;
                }
                .loading {
                    position: absolute;
                    top: 50%;
                    left: 50%;
                    transform: translate(-50%, -50%);
                    font-size: 16px;
                    color: #666;
                }
                .info {
                    padding: 8px;
                    background: white;
                    border-radius: 4px;
                    font-size: 12px;
                    border: 1px solid #ccc;
                    max-width: 200px;
                }
            </style>
        </head>
        <body>
            <div id="container">
                <div class="loading">地图加载中...</div>
            </div>

            <script type="text/javascript" src="https://webapi.amap.com/maps?v=2.0&key=%1"></script>
            <script>
                let map;
                let markers = [];
                let infoWindows = [];

                // 初始化地图
                function initMap() {
                    try {
                        map = new AMap.Map('container', {
                            zoom: 10,
                            center: [116.397428, 39.90923],
                            viewMode: '2D'
                        });

                        // 添加控件 - 使用正确的控件名称
                        if (AMap && AMap.Scale) {
                            map.addControl(new AMap.Scale());
                        }
                        if (AMap && AMap.ToolBar) {
                            map.addControl(new AMap.ToolBar());
                        }

                        console.log('高德地图 v2.0 初始化成功');

                    } catch (error) {
                        console.error('地图初始化失败:', error);
                        document.querySelector('.loading').textContent = '地图加载失败: ' + error.message;
                    }
                }

                // 清除所有标记
                function clearMarkers() {
                    markers.forEach(marker => {
                        map.remove(marker);
                    });
                    markers = [];

                    infoWindows.forEach(window => {
                        window.close();
                    });
                    infoWindows = [];
                }

                // 添加标记点
                function addMarker(lng, lat, title, color) {
                    try {
                        const iconUrl = getIconUrl(color);

                        const marker = new AMap.Marker({
                            position: [lng, lat],
                            title: title,
                            icon: iconUrl,
                            offset: new AMap.Pixel(-13, -30)
                        });

                        const infoContent = '<div class="info"><strong>' + title + '</strong><br/>坐标: ' + lng.toFixed(6) + ', ' + lat.toFixed(6) + '</div>';
                        const infoWindow = new AMap.InfoWindow({
                            content: infoContent,
                            offset: new AMap.Pixel(0, -30)
                        });

                        marker.on('click', () => {
                            infoWindows.forEach(win => win.close());
                            infoWindow.open(map, marker.getPosition());
                        });

                        map.add(marker);
                        markers.push(marker);
                        infoWindows.push(infoWindow);

                        // 隐藏加载提示
                        const loadingEl = document.querySelector('.loading');
                        if (loadingEl) {
                            loadingEl.style.display = 'none';
                        }

                        return marker;
                    } catch (error) {
                        console.error('添加标记失败:', error);
                    }
                }

                // 获取图标URL
                function getIconUrl(color) {
                    const baseUrl = 'https://webapi.amap.com/theme/v1.3/markers/n/mark_';
                    const colorMap = {
                        'red': 'r',
                        'green': 'g',
                        'orange': 'y',
                        'blue': 'b'
                    };
                    return baseUrl + (colorMap[color] || 'b') + '.png';
                }

                // 页面加载完成后初始化地图
                if (typeof AMap !== 'undefined') {
                    initMap();
                } else {
                    window.onload = initMap;
                }
            </script>
        </body>
        </html>
    )").arg(GAODE_API_KEY);

    return html;
}

void MainWindow::updateMap()
{
    if (mapView) {
        QString html = generateMapHTML();
        mapView->setHtml(html);
        qDebug() << "地图已更新";
    }
}

void MainWindow::onFinishInput()
{
    if (waypoints.isEmpty()) {
        QMessageBox::warning(this, "错误", "请至少添加一个途径点");
        return;
    }

    onCalculateShortestPath();
}

void MainWindow::onCalculateShortestPath()
{
    if (startPoint.isEmpty() || endPoint.isEmpty()) {
        QMessageBox::warning(this, "错误", "请先设置起点和终点");
        return;
    }

    if (waypoints.isEmpty()) {
        QMessageBox::warning(this, "错误", "请至少添加一个途径点");
        return;
    }

    // 构建所有点列表
    QStringList allPoints;
    allPoints << startPoint << waypoints << endPoint;

    qDebug() << "开始计算路径，点数量:" << allPoints.size();

    // 开始计算
    if (pathCalculator) {
        pathCalculator->calculateShortestPath(allPoints);
    }
}

void MainWindow::displayRoute(const QString &resultPath)
{
    QMessageBox::information(this, "路径规划结果", resultPath);
    qDebug() << "路径计算结果:" << resultPath;
}
