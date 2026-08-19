#include "filtered_camera.h"

#include <opencv2/imgproc.hpp>
#include "QPainter"

filtered_camera::filtered_camera(std::unique_ptr<camera> cam, QQuickItem *parent)
    : QQuickPaintedItem(parent), camera_(std::move(cam)),
      worker_(&filtered_camera::fetch_images, this)
{}

filtered_camera::~filtered_camera()
{
    stop_ = true;
    worker_.join();
}


hsv_thresholds *filtered_camera::thresholds() { return thresholds_; }

void filtered_camera::set_thresholds(hsv_thresholds *thresholds)
{
    std::unique_lock lock(thresholds_lock_);
    thresholds_ = thresholds;
    lock.unlock();
    emit thresholdsChanged();
}


void filtered_camera::paint(QPainter *painter)
{
    {
        std::scoped_lock lock(image_lock_);
        const auto w = static_cast<int>(width());
        const auto h = static_cast<int>(height());

        const auto recording_scaled =
            recording_.scaled(w, h / 2, Qt::AspectRatioMode::KeepAspectRatio);
        painter->drawImage(
            QRectF(QPoint{ 0, 0 }, QPoint{ recording_scaled.width(), recording_scaled.height() }),
            recording_scaled);

        const auto segmented_scaled =
            segmented_.scaled(w, h / 2, Qt::AspectRatioMode::KeepAspectRatio);
        painter->drawImage(
            QRectF(QPoint{ 0, h / 2 },
                QPoint{ recording_scaled.width(), h / 2 + recording_scaled.height() }),
            segmented_scaled);
    }
    update();
}

void filtered_camera::fetch_images()
{
    while (!stop_) {
        try {

            std::unique_lock t_lock(thresholds_lock_);
            if (!thresholds_) continue;

            const auto lower =
                cv::Scalar(thresholds_->h_min, thresholds_->s_min, thresholds_->v_min);
            const auto higher =
                cv::Scalar(thresholds_->h_max, thresholds_->s_max, thresholds_->v_max);
            auto [cv_img, cloud] = camera_->record();

            t_lock.unlock();
            cv::Mat record;
            cv::cvtColor(cv_img, record, cv::COLOR_BGR2RGB);
            auto recording =
                QImage(record.data, record.cols, record.rows, QImage::Format_RGB888).copy();

            cv::Mat mask, result;

            cv::cvtColor(cv_img, cv_img, cv::COLOR_BGR2HSV);
            cv::inRange(cv_img, lower, higher, mask);
            cv::copyTo(cv_img, result, mask);
            cv::cvtColor(result, result, cv::COLOR_HSV2RGB);

            auto segmented =
                QImage(result.data, result.cols, result.rows, QImage::Format_RGB888).copy();

            std::scoped_lock lock(image_lock_);
            recording_ = std::move(recording);
            segmented_ = std::move(segmented);

        } catch (const std::exception &e) {
            std::cout << e.what() << "\n";
            return;
        }
    }
}
