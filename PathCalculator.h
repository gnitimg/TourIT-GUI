#ifndef PATHCALCULATOR_H
#define PATHCALCULATOR_H

#include <QObject>
#include <QVector>
#include <QString>
#include <QMap>
#include <QPair>

class PathCalculator : public QObject
{
    Q_OBJECT

public:
    explicit PathCalculator(QObject *parent = nullptr);
    void calculateShortestPath(const QStringList &points);
    void calculateShortestPathWithMatrix(const QStringList &points, const QVector<QVector<double>> &distanceMatrix);

signals:
    void pathCalculated(const QString &path);
    void calculationError(const QString &error);

private:
    double getDistance(const QString &point1, const QString &point2);
    QVector<int> dijkstra(const QVector<QVector<double>> &graph, int start, int end);
    QVector<int> nearestNeighborTSP(const QVector<QVector<double>> &graph, int start, int end);

    QMap<QString, QPair<double, double>> locationCache;
};

#endif
