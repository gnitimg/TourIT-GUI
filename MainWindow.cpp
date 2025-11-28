#include "MainWindow.h"
#include "PathCalculator.h"
#include <QDebug>
#include <QWebEngineSettings>
#include <QUrl>
#include <cmath>

// 高德地图API密钥
const QString MAP_API_KEY = "4f8c908bd3a5f79a46ee5150f6a4083b";
const QString DIS_API_KEY = "d8def074f908341a84de7b9498492a42";

// 地球半径（公里）
const double EARTH_RADIUS = 6371.0;

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

    // 检查是否已经添加过
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
    QString url = QString("https://restapi.amap.com/v3/geocode/geo?key=%1&address=%2&city=")
                     .arg(DIS_API_KEY)
                     .arg(QString::fromUtf8(QUrl::toPercentEncoding(location)));

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
                    waypointEdit->clear();
                }

                QMessageBox::information(this, "成功",
                    QString("已找到位置: %1").arg(gc["formatted_address"].toString()));

                reply->deleteLater();
                return;  // 必须在成功时 return
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

double MainWindow::calculateStraightDistance(double lng1, double lat1, double lng2, double lat2)
{
    // 将角度转换为弧度
    double radLat1 = lat1 * M_PI / 180.0;
    double radLat2 = lat2 * M_PI / 180.0;
    double a = radLat1 - radLat2;
    double b = lng1 * M_PI / 180.0 - lng2 * M_PI / 180.0;

    // 使用Haversine公式计算距离
    double s = 2 * asin(sqrt(pow(sin(a/2), 2) +
                   cos(radLat1) * cos(radLat2) * pow(sin(b/2), 2)));
    s = s * EARTH_RADIUS;

    return qAbs(s);  // 返回绝对值，距离不能为负
}

void MainWindow::centerMap(double lng, double lat, const QString &title, const QString &type)
{
    QString color = "blue";
    if (type == "start") color = "red";
    else if (type == "end") color = "green";
    else if (type == "waypoint") color = "orange";

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
    waypoints.append(waypoint);
    if (waypointList) {
        waypointList->addItem(waypoint);
    }
    qDebug() << "添加途径点:" << waypoint << "当前数量:" << waypoints.size();
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

                function initMap() {
                    try {
                        map = new AMap.Map('container', {
                            zoom: 10,
                            center: [116.397428, 39.90923],
                            viewMode: '2D'
                        });

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

                        const loadingEl = document.querySelector('.loading');
                        if (loadingEl) {
                            loadingEl.style.display = 'none';
                        }

                        return marker;
                    } catch (error) {
                        console.error('添加标记失败:', error);
                    }
                }

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

                if (typeof AMap !== 'undefined') {
                    initMap();
                } else {
                    window.onload = initMap;
                }
            </script>
        </body>
        </html>
    )").arg(MAP_API_KEY);

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

    // 检查所有点都有坐标数据
    for (const QString &point : allPoints) {
        if (!locationCoordinates.contains(point)) {
            QMessageBox::warning(this, "错误", QString("点 %1 没有坐标数据，请重新搜索").arg(point));
            return;
        }
    }

    // 计算距离矩阵
    int n = allPoints.size();
    QVector<QVector<double>> distanceMatrix(n, QVector<double>(n, 0.0));

    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            QPair<double, double> coord1 = locationCoordinates[allPoints[i]];
            QPair<double, double> coord2 = locationCoordinates[allPoints[j]];

            double distance = calculateStraightDistance(
                coord1.first, coord1.second,
                coord2.first, coord2.second
            );

            distanceMatrix[i][j] = distance;
            distanceMatrix[j][i] = distance;

            qDebug() << QString("距离 %1 -> %2: %3 公里")
                        .arg(allPoints[i]).arg(allPoints[j]).arg(distance, 0, 'f', 2);
        }
    }

    qDebug() << "开始计算路径，点数量:" << allPoints.size();

    // 开始计算
    if (pathCalculator) {
        pathCalculator->calculateShortestPathWithMatrix(allPoints, distanceMatrix);
    }
}

void MainWindow::displayRoute(const QString &resultPath)
{
    QMessageBox::information(this, "路径规划结果", resultPath);
    qDebug() << "路径计算结果:" << resultPath;
}
