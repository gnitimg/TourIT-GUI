#include "PathCalculator.h"
#include <QDebug>
#include <algorithm>

PathCalculator::PathCalculator(QObject *parent)
    : QObject(parent)
    , currentAlgorithm(NearestNeighbor)
{
}

void PathCalculator::setAlgorithm(AlgorithmType algorithm)
{
    if (currentAlgorithm != algorithm) {
        currentAlgorithm = algorithm;
        emit algorithmChanged(getCurrentAlgorithmName());
    }
}

QStringList PathCalculator::getAlgorithmNames() const
{
    return {"最近邻算法", "Dijkstra算法"};
}

QString PathCalculator::getCurrentAlgorithmName() const
{
    switch (currentAlgorithm) {
    case NearestNeighbor: return "最近邻算法";
    case Dijkstra: return "Dijkstra算法";
    default: return "未知算法";
    }
}

void PathCalculator::calculateShortestPathWithMatrix(const QStringList &points,
                                                     const QVector<QVector<double>> &distanceMatrix)
{
    if (points.size() < 2) {
        emit calculationError("至少需要起点和终点两个点");
        return;
    }

    try {
        QVector<int> path;

        switch (currentAlgorithm) {
        case NearestNeighbor:
            path = solveNearestNeighbor(distanceMatrix, 0, points.size() - 1);
            break;
        case Dijkstra:
            path = solveDijkstra(distanceMatrix, 0, points.size() - 1);
            break;
        }

        if (path.isEmpty() || path[0] != 0 || path.last() != points.size() - 1) {
            emit calculationError("无法找到有效路径");
            return;
        }

        QString resultPath = buildResultPath(points, path, distanceMatrix);
        emit pathCalculated(resultPath);

    } catch (const std::exception &e) {
        emit calculationError(QString("计算错误: %1").arg(e.what()));
    }
}

QVector<int> PathCalculator::solveNearestNeighbor(const QVector<QVector<double>>& graph, int start, int end)
{
    int n = graph.size();
    if (n <= 1) return QVector<int>();

    QVector<bool> visited(n, false);
    QVector<int> path;

    int current = start;
    path.push_back(current);
    visited[current] = true;

    while (path.size() < n - 1) {
        int next = -1;
        double minDist = 1e9;

        for (int i = 0; i < n; ++i) {
            if (!visited[i] && i != end) {
                if (graph[current][i] > 0 && graph[current][i] < minDist) {
                    minDist = graph[current][i];
                    next = i;
                }
            }
        }

        if (next == -1) break;

        path.push_back(next);
        visited[next] = true;
        current = next;
    }

    if (!visited[end]) {
        path.push_back(end);
    }

    return path;
}

QVector<int> PathCalculator::solveDijkstra(const QVector<QVector<double>>& graph, int start, int end)
{
    int n = graph.size();
    if (n == 0) return QVector<int>();

    QVector<double> dist(n, 1e9);
    QVector<int> prev(n, -1);
    QVector<bool> visited(n, false);

    dist[start] = 0;

    for (int i = 0; i < n; ++i) {
        int u = -1;
        double minDist = 1e9;

        for (int j = 0; j < n; ++j) {
            if (!visited[j] && dist[j] < minDist) {
                minDist = dist[j];
                u = j;
            }
        }

        if (u == -1 || u == end) break;
        visited[u] = true;

        for (int v = 0; v < n; ++v) {
            if (!visited[v] && graph[u][v] > 0) {
                double alt = dist[u] + graph[u][v];
                if (alt < dist[v]) {
                    dist[v] = alt;
                    prev[v] = u;
                }
            }
        }
    }

    QVector<int> path;
    for (int at = end; at != -1; at = prev[at]) {
        path.push_back(at);
    }
    std::reverse(path.begin(), path.end());

    if (path.isEmpty() || path[0] != start) {
        return QVector<int>();
    }

    return path;
}

QString PathCalculator::buildResultPath(const QStringList &points,
                                        const QVector<int> &path,
                                        const QVector<QVector<double>> &distanceMatrix)
{
    QString resultPath = "完整路线:\n";
    double totalDistance = 0.0;

    for (int i = 0; i < path.size(); ++i) {
        resultPath += QString("%1. %2").arg(i + 1).arg(points[path[i]]);
        if (i < path.size() - 1) {
            resultPath += " → ";
            totalDistance += distanceMatrix[path[i]][path[i+1]];
        }
    }

    resultPath += QString("\n\n使用算法: %1").arg(getCurrentAlgorithmName());

    QString algorithmDescription = (currentAlgorithm == NearestNeighbor)
                                       ? "贪心策略，每次选择最近的未访问点"
                                       : "单源最短路径算法，保证找到最优解";

    resultPath += QString("\n算法描述: %1").arg(algorithmDescription);
    resultPath += QString("\n路线总距离: %1 公里").arg(QString::number(totalDistance, 'f', 2));
    resultPath += QString("\n途径点数量: %1 个").arg(points.size() - 2);

    return resultPath;
}
