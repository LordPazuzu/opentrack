/* Copyright (c) 2012 Patrick Ruoff
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 */

#ifndef FTNOIR_TRACKER_PT_H
#define FTNOIR_TRACKER_PT_H

#include "opentrack/plugin-api.hpp"
#include "ftnoir_tracker_pt_settings.h"
#include "camera.h"
#include "point_extractor.h"
#include "point_tracker.h"
#include "pt_video_widget.h"
#include "opentrack-compat/timer.hpp"
#include "opentrack/opencv-camera-dialog.hpp"

#include <QThread>
#include <QMutex>
#include <QMutexLocker>
#include <QTime>
#include <atomic>
#include <memory>
#include <vector>

class TrackerDialog_PT;

//-----------------------------------------------------------------------------
// Constantly processes the tracking chain in a separate thread
class Tracker_PT : public QThread, public ITracker
{
    Q_OBJECT
    friend class camera_dialog<Tracker_PT>;
    friend class TrackerDialog_PT;
public:
    Tracker_PT();
    ~Tracker_PT() override;
    void start_tracker(QFrame* parent_window) override;
    void data(double* data) override;

    Affine pose() { QMutexLocker lock(&mutex); return point_tracker.pose(); }
    int  get_n_points() { QMutexLocker lock(&mutex); return point_extractor.get_points().size(); }
    bool get_cam_info(CamInfo* info) { QMutexLocker lock(&camera_mtx); return camera.get_info(*info); }
public slots:
    void apply_settings();
protected:
    void run() override;
private:
    QMutex mutex;
    // thread commands
    enum Command {
        ABORT = 1<<0
    };
    void set_command(Command command);
    void reset_command(Command command);
    
    bool get_focal_length(float &ret);
    
    volatile int commands;

    QMutex camera_mtx;
    CVCamera       camera;
    PointExtractor point_extractor;
    PointTracker   point_tracker;

    PTVideoWidget* video_widget;
    QFrame*      video_frame;

    settings_pt s;
    Timer time;
    
    volatile bool ever_success;

    static constexpr double rad2deg = 180.0/3.14159265;
    static constexpr double deg2rad = 3.14159265/180.0;
};

class TrackerDll : public Metadata
{
    QString name() { return QString("PointTracker 1.1"); }
    QIcon icon() { return QIcon(":/Resources/Logo_IR.png"); }
};

#endif // FTNOIR_TRACKER_PT_H
