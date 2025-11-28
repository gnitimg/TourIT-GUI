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

class PathCalculator;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onConfirmStartEnd();
    void onConfirmWaypoint();
    void onFinishInput();
    void onGeocodeReply(QNetworkReply *reply);
    void onCalculateShortestPath();
    void displayRoute(const QString &resultPath);
    void onMapLoadFinished(bool ok);

private:
    void setupUI();
    void searchLocation(const QString &location, const QString &type);
    void addWaypointToList(const QString &waypoint);
    void updateMap();
    QString generateMapHTML();
    void centerMap(double lng, double lat, const QString &title, const QString &type);
    void executeJavaScript(const QString &script);
    double calculateStraightDistance(double lng1, double lat1, double lng2, double lat2);

    // UI 组件
    QWebEngineView *mapView;
    QLineEdit *startEdit;
    QLineEdit *endEdit;
    QLineEdit *waypointEdit;
    QPushButton *confirmStartEndBtn;
    QPushButton *confirmWaypointBtn;
    QPushButton *finishBtn;
    QListWidget *waypointList;

    // 网络和计算组件
    QNetworkAccessManager *networkManager;
    PathCalculator *pathCalculator;

    // 数据存储
    QString startPoint;
    QString endPoint;
    QStringList waypoints;
    QMap<QString, QPair<double, double>> locationCoordinates;
};

#endif
