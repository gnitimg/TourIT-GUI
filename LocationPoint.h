#ifndef LOCATIONPOINT_H
#define LOCATIONPOINT_H

#include <QString>
#include <QPair>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class LocationPoint {
public:
    enum PointState { S, O, E }; // S:起点, O:途径点, E:终点

    LocationPoint(const QString& name, double lng, double lat, PointState state);
    virtual ~LocationPoint() = default;

    // 纯虚函数
    virtual QString getDisplayName() const = 0;
    virtual QString getType() const = 0;

    // 虚函数
    virtual QString getColor() const;
    virtual QString getIcon() const;

    // 具体函数
    QString getName() const { return name; }
    double getLongitude() const { return longitude; }
    double getLatitude() const { return latitude; }
    PointState getState() const { return state; }
    QPair<double, double> getCoordinate() const { return qMakePair(longitude, latitude); }
    double calculateDistance(const LocationPoint& other) const;

protected:
    QString name;
    double longitude;
    double latitude;
    PointState state;
};

class StartPoint : public LocationPoint {
public:
    StartPoint(const QString& name, double lng, double lat);
    QString getDisplayName() const override;
    QString getType() const override;
    QString getColor() const override;
    QString getIcon() const override;
};

class EndPoint : public LocationPoint {
public:
    EndPoint(const QString& name, double lng, double lat);
    QString getDisplayName() const override;
    QString getType() const override;
    QString getColor() const override;
    QString getIcon() const override;
};

class WayPoint : public LocationPoint {
public:
    WayPoint(const QString& name, double lng, double lat);
    QString getDisplayName() const override;
    QString getType() const override;
    QString getColor() const override;
    QString getIcon() const override;
};

#endif
