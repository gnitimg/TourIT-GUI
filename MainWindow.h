#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QListWidget>
#include <QLabel>
#include <QWebEngineView>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>
#include <QWebChannel>
#include <QWebEnginePage>
#include <QUrl>
#include <QTimer>
#include <QInputDialog>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QComboBox>
#include <QStatusBar>
#include <QGroupBox>
#include <cmath>
#include <memory>

#include "PathCalculator.h"
#include "LocationPoint.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onConfirmStartPoint();
    void onConfirmEndPoint();
    void onConfirmWaypoint();
    void onFinishInput();
    void onGeocodeReply(QNetworkReply *reply);
    void onCalculateShortestPath();
    void displayRoute(const QString &resultPath);
    void onMapLoadFinished(bool ok);
    void onImportFromFile();
    void onAlgorithmChanged();
    void onAlgorithmSelected(const QString &algorithmName);
    void onClearAllPoints();
    void onSwapStartEnd();

private:
    void setupUI();
    void searchLocation(const QString &location, LocationPoint::PointState state);
    void addWaypointToList(const QString &waypoint);
    void updateMap();
    QString generateMapHTML();
    void centerMap(std::shared_ptr<LocationPoint> point);
    void executeJavaScript(const QString &script);
    double calculateStraightDistance(double lng1, double lat1, double lng2, double lat2);
    void importAddressesFromFile(const QString &fileName);
    void initializeMockData();
    void useMockData(const QString &location, LocationPoint::PointState state);
    std::shared_ptr<LocationPoint> createLocationPoint(const QString& name, double lng, double lat, LocationPoint::PointState state);
    void checkRouteType();
    bool isSameLocation(double lng1, double lat1, double lng2, double lat2);
    void updateRouteTypeDisplay();

    QWebEngineView *mapView;
    QLineEdit *startEdit;
    QLineEdit *endEdit;
    QLineEdit *waypointEdit;
    QPushButton *confirmStartBtn;
    QPushButton *confirmEndBtn;
    QPushButton *confirmWaypointBtn;
    QPushButton *finishBtn;
    QPushButton *importFileBtn;
    QPushButton *clearAllBtn;
    QPushButton *swapBtn;
    QComboBox *algorithmComboBox;
    QListWidget *waypointList;
    QLabel *routeTypeLabel;

    QNetworkAccessManager *networkManager;
    PathCalculator *pathCalculator;

    std::shared_ptr<LocationPoint> startPoint;
    std::shared_ptr<LocationPoint> endPoint;
    QList<std::shared_ptr<LocationPoint>> waypoints;
    QMap<QString, std::shared_ptr<LocationPoint>> locationPoints;
    QMap<QString, QPair<double, double>> mockCoordinates;

    bool isLoopRoute; // 是否为环路

};

#endif
