#include "MainWindow.h"
#include "PathCalculator.h"
#include "LocationPoint.h"
#include <QDebug>
#include <QWebEngineSettings>
#include <QUrl>
#include <QGroupBox>
#include <memory>

const QString MAP_API_KEY = "4f8c908bd3a5f79a46ee5150f6a4083b";
const QString DIS_API_KEY = "d8def074f908341a84de7b9498492a42";
const double EARTH_RADIUS = 6371.0;
const double SAME_LOCATION_THRESHOLD = 0.5; // 500米误差范围

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , mapView(nullptr)
    , startEdit(nullptr)
    , endEdit(nullptr)
    , waypointEdit(nullptr)
    , confirmStartBtn(nullptr)
    , confirmEndBtn(nullptr)
    , confirmWaypointBtn(nullptr)
    , finishBtn(nullptr)
    , importFileBtn(nullptr)
    , clearAllBtn(nullptr)
    , swapBtn(nullptr)
    , algorithmComboBox(nullptr)
    , waypointList(nullptr)
    , routeTypeLabel(nullptr)
    , networkManager(new QNetworkAccessManager(this))
    , pathCalculator(new PathCalculator(this))
    , isLoopRoute(false)
{
    setupUI();
    initializeMockData();

    connect(networkManager, &QNetworkAccessManager::finished,
            this, &MainWindow::onGeocodeReply);

    if (mapView) {
        connect(mapView, &QWebEngineView::loadFinished,
                this, &MainWindow::onMapLoadFinished);
    }

    if (pathCalculator) {
        connect(pathCalculator, &PathCalculator::pathCalculated,
                this, &MainWindow::displayRoute);
        connect(pathCalculator, &PathCalculator::algorithmChanged,
                this, &MainWindow::onAlgorithmSelected);
    }

    updateMap();
}

MainWindow::~MainWindow()
{
}

std::shared_ptr<LocationPoint> MainWindow::createLocationPoint(const QString& name, double lng, double lat, LocationPoint::PointState state) {
    switch(state) {
    case LocationPoint::S:
        return std::make_shared<StartPoint>(name, lng, lat);
    case LocationPoint::E:
        return std::make_shared<EndPoint>(name, lng, lat);
    case LocationPoint::O:
        return std::make_shared<WayPoint>(name, lng, lat);
    default:
        return std::make_shared<WayPoint>(name, lng, lat);
    }
}

void MainWindow::initializeMockData()
{
    mockCoordinates = {
        {"北京", {116.397428, 39.90923}},
        {"上海", {121.473701, 31.230416}},
        {"广州", {113.264385, 23.129112}},
        {"深圳", {114.057868, 22.543099}},
        {"杭州", {120.155070, 30.274085}},
        {"南京", {118.796877, 32.060255}},
        {"武汉", {114.305392, 30.593099}},
        {"成都", {104.066541, 30.572269}},
        {"重庆", {106.551557, 29.563009}},
        {"西安", {108.940174, 34.341568}},
        {"天津", {117.190182, 39.125596}},
        {"苏州", {120.585315, 31.298886}},
        {"厦门", {118.089425, 24.479834}},
        {"青岛", {120.382639, 36.067082}},
        {"大连", {121.618622, 38.914590}}
    };
}

void MainWindow::setupUI()
{
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);

    QWidget *controlPanel = new QWidget(centralWidget);
    QVBoxLayout *controlLayout = new QVBoxLayout(controlPanel);
    controlPanel->setMaximumWidth(450);

    // 算法选择
    QGroupBox *algorithmGroup = new QGroupBox("路径算法设置", controlPanel);
    QVBoxLayout *algoLayout = new QVBoxLayout(algorithmGroup);
    QLabel *algoLabel = new QLabel("选择算法:", algorithmGroup);
    algorithmComboBox = new QComboBox(algorithmGroup);
    algorithmComboBox->addItems(pathCalculator->getAlgorithmNames());
    algoLayout->addWidget(algoLabel);
    algoLayout->addWidget(algorithmComboBox);
    algorithmGroup->setLayout(algoLayout);

    // 起点终点设置
    QGroupBox *startEndGroup = new QGroupBox("起点终点设置", controlPanel);
    QGridLayout *seLayout = new QGridLayout(startEndGroup);

    QLabel *startLabel = new QLabel("起点:", startEndGroup);
    startEdit = new QLineEdit(startEndGroup);
    startEdit->setPlaceholderText("例如：北京");
    confirmStartBtn = new QPushButton("确认起点", startEndGroup);
    confirmStartBtn->setMaximumWidth(80);

    QLabel *endLabel = new QLabel("终点:", startEndGroup);
    endEdit = new QLineEdit(startEndGroup);
    endEdit->setPlaceholderText("例如：上海");
    confirmEndBtn = new QPushButton("确认终点", startEndGroup);
    confirmEndBtn->setMaximumWidth(80);

    swapBtn = new QPushButton("↔ 交换", startEndGroup);
    swapBtn->setMaximumWidth(80);

    seLayout->addWidget(startLabel, 0, 0);
    seLayout->addWidget(startEdit, 0, 1);
    seLayout->addWidget(confirmStartBtn, 0, 2);
    seLayout->addWidget(endLabel, 1, 0);
    seLayout->addWidget(endEdit, 1, 1);
    seLayout->addWidget(confirmEndBtn, 1, 2);
    seLayout->addWidget(swapBtn, 2, 1);

    startEndGroup->setLayout(seLayout);

    // 路线类型显示
    routeTypeLabel = new QLabel("路线类型: 未设置", controlPanel);
    routeTypeLabel->setStyleSheet("font-weight: bold; padding: 5px;");

    // 途径点设置
    QGroupBox *waypointGroup = new QGroupBox("途径点设置", controlPanel);
    QVBoxLayout *wpLayout = new QVBoxLayout(waypointGroup);

    waypointEdit = new QLineEdit(waypointGroup);
    waypointEdit->setPlaceholderText("例如：天津");

    QHBoxLayout *wpButtonLayout = new QHBoxLayout();
    confirmWaypointBtn = new QPushButton("添加途径点", waypointGroup);
    importFileBtn = new QPushButton("从文件导入", waypointGroup);
    wpButtonLayout->addWidget(confirmWaypointBtn);
    wpButtonLayout->addWidget(importFileBtn);

    QLabel *listLabel = new QLabel("已添加的途径点:", waypointGroup);
    waypointList = new QListWidget(waypointGroup);
    waypointList->setMaximumHeight(200);

    wpLayout->addWidget(waypointEdit);
    wpLayout->addLayout(wpButtonLayout);
    wpLayout->addWidget(listLabel);
    wpLayout->addWidget(waypointList);

    waypointGroup->setLayout(wpLayout);

    // 控制按钮
    QHBoxLayout *controlBtnLayout = new QHBoxLayout();
    finishBtn = new QPushButton("计算最短路径", controlPanel);
    finishBtn->setEnabled(false);
    finishBtn->setMinimumHeight(40);

    clearAllBtn = new QPushButton("清空所有点", controlPanel);
    clearAllBtn->setMinimumHeight(40);

    controlBtnLayout->addWidget(finishBtn);
    controlBtnLayout->addWidget(clearAllBtn);

    // 添加到主布局
    controlLayout->addWidget(algorithmGroup);
    controlLayout->addSpacing(10);
    controlLayout->addWidget(startEndGroup);
    controlLayout->addSpacing(10);
    controlLayout->addWidget(routeTypeLabel);
    controlLayout->addSpacing(10);
    controlLayout->addWidget(waypointGroup);
    controlLayout->addSpacing(10);
    controlLayout->addLayout(controlBtnLayout);
    controlLayout->addStretch();

    // 地图视图
    mapView = new QWebEngineView(centralWidget);
    mapView->setMinimumSize(700, 600);

    if (mapView->settings()) {
        mapView->settings()->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
    }

    mainLayout->addWidget(controlPanel);
    mainLayout->addWidget(mapView);

    // 连接信号槽
    connect(confirmStartBtn, &QPushButton::clicked,
            this, &MainWindow::onConfirmStartPoint);
    connect(confirmEndBtn, &QPushButton::clicked,
            this, &MainWindow::onConfirmEndPoint);
    connect(confirmWaypointBtn, &QPushButton::clicked,
            this, &MainWindow::onConfirmWaypoint);
    connect(importFileBtn, &QPushButton::clicked,
            this, &MainWindow::onImportFromFile);
    connect(swapBtn, &QPushButton::clicked,
            this, &MainWindow::onSwapStartEnd);
    connect(finishBtn, &QPushButton::clicked,
            this, &MainWindow::onFinishInput);
    connect(clearAllBtn, &QPushButton::clicked,
            this, &MainWindow::onClearAllPoints);
    connect(algorithmComboBox, &QComboBox::currentTextChanged,
            this, &MainWindow::onAlgorithmChanged);
}

void MainWindow::onAlgorithmChanged()
{
    QString algorithmName = algorithmComboBox->currentText();
    if (algorithmName == "最近邻算法") {
        pathCalculator->setAlgorithm(PathCalculator::NearestNeighbor);
    } else if (algorithmName == "Dijkstra算法") {
        pathCalculator->setAlgorithm(PathCalculator::Dijkstra);
    }
}

void MainWindow::onAlgorithmSelected(const QString &algorithmName)
{
    statusBar()->showMessage("当前算法: " + algorithmName, 3000);
}

void MainWindow::onConfirmStartPoint()
{
    QString startName = startEdit->text().trimmed();

    if (startName.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "请输入起点名称");
        return;
    }

    searchLocation(startName, LocationPoint::S);
}

void MainWindow::onConfirmEndPoint()
{
    QString endName = endEdit->text().trimmed();

    if (endName.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "请输入终点名称");
        return;
    }

    searchLocation(endName, LocationPoint::E);
}

void MainWindow::onSwapStartEnd()
{
    if (startPoint && endPoint) {
        // 交换起点和终点
        auto temp = startPoint;
        startPoint = endPoint;
        endPoint = temp;

        // 更新显示
        startEdit->setText(startPoint->getName());
        endEdit->setText(endPoint->getName());

        // 重新检查路线类型
        checkRouteType();

        QMessageBox::information(this, "交换成功", "起点和终点已交换");
    } else {
        QMessageBox::warning(this, "错误", "请先设置起点和终点");
    }
}

void MainWindow::onClearAllPoints()
{
    // 清空所有点
    startPoint.reset();
    endPoint.reset();
    waypoints.clear();
    locationPoints.clear();
    waypointList->clear();

    // 清空输入框
    startEdit->clear();
    endEdit->clear();
    waypointEdit->clear();

    // 更新显示
    updateRouteTypeDisplay();
    finishBtn->setEnabled(false);

    // 清空地图标记
    QString script = R"(
        if (typeof clearMarkers === 'function') {
            clearMarkers();
        }
    )";
    executeJavaScript(script);

    QMessageBox::information(this, "清空完成", "所有点已清空");
}

void MainWindow::onConfirmWaypoint()
{
    QString waypointName = waypointEdit->text().trimmed();
    if (waypointName.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "请输入途径点");
        return;
    }

    // 检查是否已经存在
    for (const auto& wp : waypoints) {
        if (wp->getName() == waypointName) {
            QMessageBox::warning(this, "重复输入", "该途径点已经添加过了");
            return;
        }
    }

    searchLocation(waypointName, LocationPoint::O);
    waypointEdit->clear();
}

bool MainWindow::isSameLocation(double lng1, double lat1, double lng2, double lat2)
{
    double distance = calculateStraightDistance(lng1, lat1, lng2, lat2);
    return distance <= SAME_LOCATION_THRESHOLD; // 500米内视为同一位置
}

void MainWindow::checkRouteType()
{
    if (startPoint && endPoint) {
        double startLng = startPoint->getLongitude();
        double startLat = startPoint->getLatitude();
        double endLng = endPoint->getLongitude();
        double endLat = endPoint->getLatitude();

        isLoopRoute = isSameLocation(startLng, startLat, endLng, endLat);
        updateRouteTypeDisplay();
    }
}

void MainWindow::updateRouteTypeDisplay()
{
    if (startPoint && endPoint) {
        if (isLoopRoute) {
            routeTypeLabel->setText("路线类型: 环路（起点≈终点）");
            routeTypeLabel->setStyleSheet("font-weight: bold; color: #FF6B6B; padding: 5px; background-color: #FFF5F5; border-radius: 5px;");
        } else {
            routeTypeLabel->setText("路线类型: 非环路");
            routeTypeLabel->setStyleSheet("font-weight: bold; color: #4ECDC4; padding: 5px; background-color: #F7FFF7; border-radius: 5px;");
        }
    } else {
        routeTypeLabel->setText("路线类型: 未设置");
        routeTypeLabel->setStyleSheet("font-weight: bold; color: #666; padding: 5px;");
    }
}

void MainWindow::searchLocation(const QString &location, LocationPoint::PointState state)
{
    if (mockCoordinates.contains(location)) {
        QPair<double, double> coords = mockCoordinates[location];
        auto point = createLocationPoint(location, coords.first, coords.second, state);
        locationPoints[location] = point;

        centerMap(point);

        if (state == LocationPoint::O) {
            waypoints.append(point);
            addWaypointToList(location);
        } else if (state == LocationPoint::S) {
            startPoint = point;
        } else if (state == LocationPoint::E) {
            endPoint = point;
        }

        // 检查是否起点和终点都已设置
        if (startPoint && endPoint) {
            checkRouteType();
            finishBtn->setEnabled(true);
        }

        QMessageBox::information(this, "成功", QString("已找到位置: %1").arg(location));
        return;
    }

    QString url = QString("https://restapi.amap.com/v3/geocode/geo?key=%1&address=%2&city=")
                      .arg(DIS_API_KEY).arg(QString::fromUtf8(QUrl::toPercentEncoding(location)));

    QNetworkRequest request;
    request.setUrl(QUrl(url));
    request.setRawHeader("Accept", "application/json");

    QNetworkReply *reply = networkManager->get(request);
    reply->setProperty("location", location);
    reply->setProperty("state", state);
}

void MainWindow::onGeocodeReply(QNetworkReply *reply)
{
    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject obj = doc.object();

    QString location = reply->property("location").toString();
    LocationPoint::PointState state = static_cast<LocationPoint::PointState>(reply->property("state").toInt());

    if (obj["info"].toString().contains("EXCEEDED_THE_LIMIT")) {
        useMockData(location, state);
        reply->deleteLater();
        return;
    }

    if (obj["status"].toString() == "1") {
        int count = obj["count"].toString().toInt();
        if (count > 0) {
            QJsonObject gc = obj["geocodes"].toArray()[0].toObject();
            QString locStr = gc["location"].toString();
            QStringList coords = locStr.split(",");

            if (coords.size() == 2) {
                double lng = coords[0].toDouble();
                double lat = coords[1].toDouble();

                auto point = createLocationPoint(location, lng, lat, state);
                locationPoints[location] = point;

                centerMap(point);

                if (state == LocationPoint::O) {
                    waypoints.append(point);
                    addWaypointToList(location);
                } else if (state == LocationPoint::S) {
                    startPoint = point;
                } else if (state == LocationPoint::E) {
                    endPoint = point;
                }

                // 检查是否起点和终点都已设置
                if (startPoint && endPoint) {
                    checkRouteType();
                    finishBtn->setEnabled(true);
                }

                QMessageBox::information(this, "成功",
                                         QString("已找到位置: %1").arg(gc["formatted_address"].toString()));

                reply->deleteLater();
                return;
            }
        }
    }

    qDebug() << "地理编码失败:" << obj["info"].toString();
    QMessageBox::warning(this, "搜索失败",
                         QString("未找到位置: %1\n错误信息: %2")
                             .arg(location)
                             .arg(obj["info"].toString()));

    reply->deleteLater();
}

void MainWindow::useMockData(const QString &location, LocationPoint::PointState state)
{
    qint64 hash = qHash(location);
    double lng = 116.0 + (hash % 1000) / 1000.0 * 10.0;
    double lat = 20.0 + (hash % 1000) / 1000.0 * 20.0;

    auto point = createLocationPoint(location, lng, lat, state);
    locationPoints[location] = point;
    centerMap(point);

    if (state == LocationPoint::O) {
        waypoints.append(point);
        addWaypointToList(location);
    } else if (state == LocationPoint::S) {
        startPoint = point;
    } else if (state == LocationPoint::E) {
        endPoint = point;
    }

    // 检查是否起点和终点都已设置
    if (startPoint && endPoint) {
        checkRouteType();
        finishBtn->setEnabled(true);
    }

    QMessageBox::information(this, "使用模拟数据",
                             QString("位置 %1 使用模拟坐标").arg(location));
}

void MainWindow::addWaypointToList(const QString &waypoint)
{
    if (waypointList) {
        waypointList->addItem(waypoint);
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
    if (!startPoint || !endPoint) {
        QMessageBox::warning(this, "错误", "请先设置起点和终点");
        return;
    }

    if (waypoints.isEmpty()) {
        QMessageBox::warning(this, "错误", "请至少添加一个途径点");
        return;
    }

    QStringList allPoints;
    allPoints << startPoint->getName();

    for (const auto& wp : waypoints) {
        allPoints << wp->getName();
    }

    // 如果是环路，不添加终点（因为起点和终点是同一个）
    if (!isLoopRoute) {
        allPoints << endPoint->getName();
    }

    for (const QString &point : allPoints) {
        if (!locationPoints.contains(point)) {
            QMessageBox::warning(this, "错误", QString("点 %1 没有坐标数据").arg(point));
            return;
        }
    }

    int n = allPoints.size();
    QVector<QVector<double>> distanceMatrix(n, QVector<double>(n, 0.0));

    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            auto point1 = locationPoints[allPoints[i]];
            auto point2 = locationPoints[allPoints[j]];

            double distance = point1->calculateDistance(*point2);
            distanceMatrix[i][j] = distance;
            distanceMatrix[j][i] = distance;
        }
    }

    if (pathCalculator) {
        pathCalculator->calculateShortestPathWithMatrix(allPoints, distanceMatrix);
    }
}

double MainWindow::calculateStraightDistance(double lng1, double lat1, double lng2, double lat2)
{
    double radLat1 = lat1 * M_PI / 180.0;
    double radLat2 = lat2 * M_PI / 180.0;
    double a = radLat1 - radLat2;
    double b = lng1 * M_PI / 180.0 - lng2 * M_PI / 180.0;

    double s = 2 * asin(sqrt(pow(sin(a/2), 2) +
                             cos(radLat1) * cos(radLat2) * pow(sin(b/2), 2)));
    s = s * EARTH_RADIUS;

    return qAbs(s);
}

void MainWindow::centerMap(std::shared_ptr<LocationPoint> point)
{
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
                         ).arg(point->getLongitude())
                         .arg(point->getLatitude())
                         .arg(point->getDisplayName())
                         .arg(point->getColor());

    executeJavaScript(script);
}

void MainWindow::executeJavaScript(const QString &script)
{
    if (mapView && mapView->page()) {
        QTimer::singleShot(100, this, [this, script]() {
            mapView->page()->runJavaScript(script);
        });
    }
}

void MainWindow::onMapLoadFinished(bool ok)
{
    if (ok) {
        qDebug() << "地图加载成功";
    } else {
        qDebug() << "地图加载失败";
    }
}

void MainWindow::onImportFromFile()
{
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "选择地址文件",
        "",
        "文本文件 (*.txt);;所有文件 (*.*)"
        );

    if (!fileName.isEmpty()) {
        importAddressesFromFile(fileName);
    }
}

void MainWindow::importAddressesFromFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "文件错误", "无法打开文件: " + fileName);
        return;
    }

    QTextStream in(&file);
    QStringList newAddresses;
    int importedCount = 0;
    int duplicateCount = 0;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (!line.isEmpty()) {
            bool duplicate = false;
            for (const auto& wp : waypoints) {
                if (wp->getName() == line) {
                    duplicate = true;
                    break;
                }
            }

            if (duplicate) {
                duplicateCount++;
                continue;
            }

            newAddresses.append(line);
            importedCount++;
            searchLocation(line, LocationPoint::O);
        }
    }

    file.close();

    QString message = QString("成功导入 %1 个新地址").arg(importedCount);
    if (duplicateCount > 0) {
        message += QString("\n跳过 %1 个重复地址").arg(duplicateCount);
    }

    QMessageBox::information(this, "导入完成", message);
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
    }
}

void MainWindow::displayRoute(const QString &resultPath)
{
    // 构建包含路线类型信息的路径结果
    QString enhancedResult = resultPath;

    if (isLoopRoute) {
        enhancedResult = "【环路路径规划】\n" + enhancedResult;
        enhancedResult += "\n\n环路特性:";
        enhancedResult += QString("\n• 起点和终点在同一位置（距离 < %1公里）").arg(SAME_LOCATION_THRESHOLD);
        enhancedResult += QString("\n• 起点/终点: %1").arg(startPoint ? startPoint->getName() : "无");
    } else {
        enhancedResult = "【非环路路径规划】\n" + enhancedResult;
        enhancedResult += QString("\n\n起点到终点距离: %1 公里")
                              .arg(QString::number(startPoint->calculateDistance(*endPoint), 'f', 2));
    }

    enhancedResult += QString("\n路线类型: %1").arg(isLoopRoute ? "环路" : "非环路");

    QMessageBox::information(this, "路径规划结果", enhancedResult);
}
