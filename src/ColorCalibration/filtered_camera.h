#pragma once

#include <thread>

#include <QQuickPaintedItem>
#include <QImage>
#include <QtQml/qqml.h>

#include <opencv2/core.hpp>

#include "Camera.hpp"


class hsv_thresholds : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(HSVThresholds)
    Q_PROPERTY(int h_min MEMBER h_min NOTIFY changed)
    Q_PROPERTY(int h_max MEMBER h_max NOTIFY changed)
    Q_PROPERTY(int s_min MEMBER s_min NOTIFY changed)
    Q_PROPERTY(int s_max MEMBER s_max NOTIFY changed)
    Q_PROPERTY(int v_min MEMBER v_min NOTIFY changed)
    Q_PROPERTY(int v_max MEMBER v_max NOTIFY changed)

  public:
    int h_min = 0;
    int h_max = 180;
    int s_min = 0;
    int s_max = 255;
    int v_min = 0;
    int v_max = 255;

  signals:
    void changed();
};


class filtered_camera : public QQuickPaintedItem
{
    Q_OBJECT
    QML_NAMED_ELEMENT(FilteredCamera)
    Q_PROPERTY(hsv_thresholds *thresholds READ thresholds()
            WRITE set_thresholds NOTIFY thresholdsChanged REQUIRED)
                  
  public:
    explicit filtered_camera(std::unique_ptr<camera> cam = std::make_unique<realsense_camera>(),
        QQuickItem *parent = nullptr);
    ~filtered_camera();

    hsv_thresholds *thresholds();
    void set_thresholds(hsv_thresholds *thresholds);

  signals:
    void thresholdsChanged();

  private:
    hsv_thresholds *thresholds_ = nullptr;
    std::unique_ptr<camera> camera_;

    QImage recording_;
    QImage segmented_;
    void paint(QPainter *painter) override;

    std::atomic_bool stop_ = false;
    std::mutex image_lock_;
    std::mutex thresholds_lock_;

    std::thread worker_;
    void fetch_images();
};