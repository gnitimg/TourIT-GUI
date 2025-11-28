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

        // 使用Dijkstra算法计算最短路径
        QVector<int> path = dijkstra(graph, 0, n - 1);

        if (path.isEmpty() || path[0] != 0) {
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

        // 发射信号
        emit pathCalculated(resultPath);

    } catch (const std::exception &e) {
        emit calculationError(QString("计算错误: %1").arg(e.what()));
    }
}

double PathCalculator::getDistance(const QString &point1, const QString &point2)
{
    // 模拟距离计算 - 实际项目中应该调用高德地图API
    // 这里使用简单的哈希函数生成伪随机但确定性的距离
    qint64 hash1 = qHash(point1);
    qint64 hash2 = qHash(point2);

    // 生成 1-50 公里范围内的距离
    double distance = (qAbs(hash1 - hash2) % 5000) / 100.0 + 1.0;

    qDebug() << "计算距离:" << point1 << "->" << point2 << "=" << distance << "公里";

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
        // 找到未访问的最小距离顶点
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

        // 更新相邻顶点距离
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

    // 重构路径
    QVector<int> path;
    for (int at = end; at != -1; at = prev[at]) {
        path.push_back(at);
    }
    std::reverse(path.begin(), path.end());

    // 检查路径是否有效
    if (path.isEmpty() || path[0] != start) {
        return QVector<int>();
    }

    return path;
}
