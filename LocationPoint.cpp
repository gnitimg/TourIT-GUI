#include "LocationPoint.h"

const double EARTH_RADIUS = 6371.0;

LocationPoint::LocationPoint(const QString& name, double lng, double lat, PointState state)
    : name(name), longitude(lng), latitude(lat), state(state) {}

QString LocationPoint::getColor() const {
    switch(state) {
    case S: return "red";
    case E: return "green";
    case O: return "orange";
    default: return "blue";
    }
}

QString LocationPoint::getIcon() const {
    switch(state) {
    case S: return "mark_r";
    case E: return "mark_g";
    case O: return "mark_y";
    default: return "mark_b";
    }
}

double LocationPoint::calculateDistance(const LocationPoint& other) const {
    double radLat1 = latitude * M_PI / 180.0;
    double radLat2 = other.latitude * M_PI / 180.0;
    double a = radLat1 - radLat2;
    double b = longitude * M_PI / 180.0 - other.longitude * M_PI / 180.0;

    double s = 2 * asin(sqrt(pow(sin(a/2), 2) + cos(radLat1) * cos(radLat2) * pow(sin(b/2), 2)));
    return std::abs(s * EARTH_RADIUS);
}

// StartPoint 实现
StartPoint::StartPoint(const QString& name, double lng, double lat)
    : LocationPoint(name, lng, lat, S) {}

QString StartPoint::getDisplayName() const { return QString("起点: %1").arg(name); }
QString StartPoint::getType() const { return "start"; }
QString StartPoint::getColor() const { return "red"; }
QString StartPoint::getIcon() const { return "mark_r"; }

// EndPoint 实现
EndPoint::EndPoint(const QString& name, double lng, double lat)
    : LocationPoint(name, lng, lat, E) {}

QString EndPoint::getDisplayName() const { return QString("终点: %1").arg(name); }
QString EndPoint::getType() const { return "end"; }
QString EndPoint::getColor() const { return "green"; }
QString EndPoint::getIcon() const { return "mark_g"; }

// WayPoint 实现
WayPoint::WayPoint(const QString& name, double lng, double lat)
    : LocationPoint(name, lng, lat, O) {}

QString WayPoint::getDisplayName() const { return QString("途径点: %1").arg(name); }
QString WayPoint::getType() const { return "waypoint"; }
QString WayPoint::getColor() const { return "orange"; }
QString WayPoint::getIcon() const { return "mark_y"; }
