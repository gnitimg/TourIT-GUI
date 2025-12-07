#ifndef PATHCALCULATOR_H
#define PATHCALCULATOR_H

#include <QObject>
#include <QVector>
#include <QString>
#include <QMap>

class PathCalculator : public QObject
{
    Q_OBJECT

public:
    enum AlgorithmType {
        NearestNeighbor,
        Dijkstra
    };

    explicit PathCalculator(QObject *parent = nullptr);

    void setAlgorithm(AlgorithmType algorithm);
    void calculateShortestPathWithMatrix(const QStringList &points,
                                         const QVector<QVector<double>> &distanceMatrix);

    QStringList getAlgorithmNames() const;
    QString getCurrentAlgorithmName() const;

signals:
    void pathCalculated(const QString &path);
    void calculationError(const QString &error);
    void algorithmChanged(const QString &algorithmName);

private:
    QVector<int> solveNearestNeighbor(const QVector<QVector<double>>& graph, int start, int end);
    QVector<int> solveDijkstra(const QVector<QVector<double>>& graph, int start, int end);
    QString buildResultPath(const QStringList &points,
                            const QVector<int> &path,
                            const QVector<QVector<double>> &distanceMatrix);

    AlgorithmType currentAlgorithm;
};

#endif
