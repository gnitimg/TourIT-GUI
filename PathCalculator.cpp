#include "PathCalculator.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <limits>
#include <algorithm>
#include <QDebug>
#include <QHash>

PathCalculator::PathCalculator(QObject *parent)
    : QObject(parent)
{
}

void PathCalculator::calculateShortestPath(const QStringList &points)
{
    if (points.size() < 2) {
        emit calculationError("至少需要起点和终点两个点");
        return;
    }

    try {
        int n = points.size();
        QVector<QVector<double>> graph(n, QVector<double>(n, 0));

        // 构建距离矩阵
        for (int i = 0; i < n; ++i) {
            for (int j = i + 1; j < n; ++j) {
                double dist = getDistance(points[i], points[j]);
                graph[i][j] = dist;
                graph[j][i] = dist;
            }
        }

        // 使用最近邻算法解决TSP问题
        QVector<int> path = nearestNeighborTSP(graph, 0, n - 1);

        if (path.isEmpty() || path[0] != 0 || path.last() != n - 1) {
            emit calculationError("无法找到有效路径");
            return;
        }

        // 构建路径字符串
        QString resultPath;
        double totalDistance = 0.0;

        for (int i = 0; i < path.size(); ++i) {
            resultPath += points[path[i]];
            if (i < path.size() - 1) {
                resultPath += " → ";
                totalDistance += graph[path[i]][path[i+1]];
            }
        }

        resultPath += QString("\n\n总距离: %1 公里").arg(QString::number(totalDistance, 'f', 2));
        resultPath += QString("\n途径点数量: %1 个").arg(points.size() - 2);

        emit pathCalculated(resultPath);

    } catch (const std::exception &e) {
        emit calculationError(QString("计算错误: %1").arg(e.what()));
    }
}

void PathCalculator::calculateShortestPathWithMatrix(const QStringList &points, const QVector<QVector<double>> &distanceMatrix)
{
    if (points.size() < 2) {
        emit calculationError("至少需要起点和终点两个点");
        return;
    }

    try {
        int n = points.size();

        // 使用最近邻算法解决TSP问题（固定起点和终点）
        QVector<int> path = nearestNeighborTSP(distanceMatrix, 0, n - 1);

        if (path.isEmpty() || path[0] != 0 || path.last() != n - 1) {
            emit calculationError("无法找到有效路径");
            return;
        }

        // 构建路径字符串
        QString resultPath = "完整路线:\n";
        double totalDistance = 0.0;

        for (int i = 0; i < path.size(); ++i) {
            resultPath += QString("%1. %2").arg(i + 1).arg(points[path[i]]);
            if (i < path.size() - 1) {
                resultPath += "\n↓\n";
                totalDistance += distanceMatrix[path[i]][path[i+1]];
            }
        }

        resultPath += QString("\n\n路线总距离: %1 公里").arg(QString::number(totalDistance, 'f', 2));
        resultPath += QString("\n途径点数量: %1 个").arg(points.size() - 2);

        emit pathCalculated(resultPath);

    } catch (const std::exception &e) {
        emit calculationError(QString("计算错误: %1").arg(e.what()));
    }
}

double PathCalculator::getDistance(const QString &point1, const QString &point2)
{
    // 模拟距离计算
    qint64 hash1 = qHash(point1);
    qint64 hash2 = qHash(point2);

    double distance = (qAbs(hash1 - hash2) % 5000) / 100.0 + 1.0;
    return distance;
}

QVector<int> PathCalculator::dijkstra(const QVector<QVector<double>> &graph, int start, int end)
{
    int n = graph.size();
    if (n == 0 || start < 0 || end >= n) {
        return QVector<int>();
    }

    QVector<double> dist(n, std::numeric_limits<double>::max());
    QVector<int> prev(n, -1);
    QVector<bool> visited(n, false);

    dist[start] = 0;

    for (int i = 0; i < n; ++i) {
        int u = -1;
        double minDist = std::numeric_limits<double>::max();
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

QVector<int> PathCalculator::nearestNeighborTSP(const QVector<QVector<double>> &graph, int start, int end)
{
    int n = graph.size();
    if (n <= 1 || start < 0 || end >= n || start == end) {
        return QVector<int>();
    }

    QVector<bool> visited(n, false);
    QVector<int> path;

    // 固定起点
    int current = start;
    path.push_back(current);
    visited[current] = true;

    // 如果只有起点和终点两个点
    if (n == 2) {
        path.push_back(end);
        return path;
    }

    // 访问中间点（不包括起点和终点）
    while (path.size() < n - 1) {
        int next = -1;
        double minDist = std::numeric_limits<double>::max();

        // 寻找最近的未访问点（但不能是终点，除非是最后一个点）
        for (int i = 0; i < n; ++i) {
            if (!visited[i] && i != end) {
                if (graph[current][i] > 0 && graph[current][i] < minDist) {
                    minDist = graph[current][i];
                    next = i;
                }
            }
        }

        // 如果找不到中间点，但还有终点没访问
        if (next == -1) {
            // 检查是否只剩下终点没访问
            int unvisitedCount = 0;
            int lastUnvisited = -1;
            for (int i = 0; i < n; ++i) {
                if (!visited[i]) {
                    unvisitedCount++;
                    lastUnvisited = i;
                }
            }

            if (unvisitedCount == 1 && lastUnvisited == end) {
                // 只剩下终点，直接添加
                path.push_back(end);
                break;
            } else {
                // 无法找到有效路径
                qDebug() << "无法找到有效路径，剩余未访问点:" << unvisitedCount;
                return QVector<int>();
            }
        }

        path.push_back(next);
        visited[next] = true;
        current = next;
    }

    // 确保终点在路径末尾
    if (path.last() != end) {
        // 如果终点不在路径中，添加到末尾
        if (!visited[end]) {
            path.push_back(end);
        } else {
            // 终点已经被访问，但不在末尾，需要调整
            qDebug() << "终点位置错误，重新构建路径";
            return QVector<int>();
        }
    }

    // 验证路径
    if (path.size() != n) {
        qDebug() << "路径长度错误，期望:" << n << "实际:" << path.size();
        return QVector<int>();
    }

    if (path[0] != start || path.last() != end) {
        qDebug() << "起点或终点位置错误";
        return QVector<int>();
    }

    qDebug() << "计算完成路径:" << path;
    return path;
}
