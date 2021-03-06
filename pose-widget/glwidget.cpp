/* Copyright (c) 2013, 2015 Stanislaw Halik <sthalik@misaki.pl>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 */

#include "glwidget.h"
#include <cmath>
#include <algorithm>

#include <QPainter>
#include <QPaintEvent>

GLWidget::GLWidget(QWidget *parent) : QWidget(parent)
{
    Q_INIT_RESOURCE(posewidget);

    front = QImage(QString(":/images/side1.png"));
    back = QImage(QString(":/images/side6.png"));
    rotateBy(0, 0, 0, 0, 0, 0);
}

GLWidget::~GLWidget()
{
}

void GLWidget::paintEvent ( QPaintEvent * event ) {
    QPainter p(this);
    project_quad_texture();
    p.drawImage(event->rect(), texture);
}

void GLWidget::rotateBy(float xAngle, float yAngle, float zAngle, float x, float y, float z)
{

    float c1 = cos(yAngle / 57.295781);
    float s1 = sin(yAngle / 57.295781);
    float c2 = cos(xAngle / 57.295781);
    float s2 = sin(xAngle / 57.295781);
    float c3 = cos(zAngle / 57.295781);
    float s3 = sin(zAngle / 57.295781);

    rotation = rmat(c2*c3,           -c2*s3,          s2,
                    c1*s3+c3*s1*s2,  c1*c3-s1*s2*s3,  -c2*s1,
                    s1*s3-c1*c3*s2,  c3*s1+c1*s2*s3,  c1*c2);
    translation = vec3(x, y, z);

    update();
}


class Triangle {
    using num = GLWidget::num;
    using vec2 = GLWidget::vec2;
    using vec3 = GLWidget::vec3;
public:
    Triangle(const vec2& p1,
             const vec2& p2,
             const vec2& p3)
    {
        origin = p1;
        v0 = vec2(p3.x() - p1.x(), p3.y() - p1.y());
        v1 = vec2(p2.x() - p1.x(), p2.y() - p1.y());
        dot00 = v0.dot(v0);
        dot01 = v0.dot(v1);
        dot11 = v1.dot(v1);
        invDenom = 1 / (dot00 * dot11 - dot01 * dot01);
    }
    bool barycentric_coords(const vec2& px, vec2& uv) const
    {
        const vec2 v2 = px - origin;
        const num dot12 = v1.dot(v2);
        const num dot02 = v0.dot(v2);
        const num u = (dot11 * dot02 - dot01 * dot12) * invDenom;
        const num v = (dot00 * dot12 - dot01 * dot02) * invDenom;
        uv = vec2(u, v);
        return (u >= 0) && (v >= 0) && (u + v <= 1);
    }
    
private:
    num dot00, dot01, dot11, invDenom;
    vec2 v0, v1, origin;
};

inline GLWidget::vec3 GLWidget::normal(const vec3& p1, const vec3& p2, const vec3& p3)
{
    vec3 u = p2 - p1;
    vec3 v = p3 - p1;
     
    vec3 tmp = u.cross(v);
    
    num i = 1./sqrt(tmp.dot(tmp));
    
    return tmp * i;
}

void GLWidget::project_quad_texture() {
    const int sx = width(), sy = height();
    vec2 pt[4];
    const vec3 corners[] = {
        vec3(0., 0, 0),
        vec3(sx-1, 0, 0),
        vec3(0, sy-1, 0),
        vec3(sx-1, sy-1, 0.)
    };
    
    for (int i = 0; i < 4; i++)
        pt[i] = project(vec3(corners[i].x() - sx/2, corners[i].y() - sy/2, 0)) + vec2(sx/2, sy/2);
    
    vec3 normal1(0, 0, 1);
    vec3 normal2;
    {
        vec3 foo[3];
        for (int i = 0; i < 3; i++)
            foo[i] = project2(corners[i]);
        normal2 = normal(foo[0], foo[1], foo[2]);
    }
    
    num dir = normal1.dot(normal2);
    
    QImage& tex = dir < 0 ? back : front;
    
    int ow = tex.width(), oh = tex.height();

    QImage texture(QSize(sx, sy), QImage::Format_RGB888);
    QColor bgColor = palette().color(QPalette::Current, QPalette::Window);
    texture.fill(bgColor);
    
    const vec2 projected[2][3] = { { pt[0], pt[1], pt[2] }, { pt[3], pt[1], pt[2] } };
    const vec2 origs[2][3] = {
        { vec2(0, 0), vec2(ow-1, 0), vec2(0, oh-1) },
        { vec2(ow-1, oh-1), vec2(ow-1, 0), vec2(0, oh-1) }
    };
    const Triangle triangles[2] = {
        Triangle(projected[0][0], projected[0][1], projected[0][2]),
        Triangle(projected[1][0], projected[1][1], projected[1][2])
    };
  
    const int orig_pitch = tex.bytesPerLine();
    const int dest_pitch = texture.bytesPerLine();
    
    const unsigned char* orig = tex.bits();
    unsigned char* dest = texture.bits();
    
    const int orig_depth = tex.depth() / 8;
    const int dest_depth = texture.depth() / 8;
    
    /* image breakage? */
    if (orig_depth < 3)
        return;

    for (int y = 0; y < sy; y++)
        for (int x = 0; x < sx; x++) {
            vec2 pos(x, y);
            for (int i = 0; i < 2; i++) {
                vec2 uv;
                // XXX knowing center of the lookup pos,
                // we have symmetry so only one lookup is needed -sh 20150831
                if (triangles[i].barycentric_coords(pos, uv))
                {
                    const int px = origs[i][0].x()
                            + uv.x() * (origs[i][2].x() - origs[i][0].x())
                            + uv.y() * (origs[i][1].x() - origs[i][0].x());
                    const int py = origs[i][0].y()
                            + uv.x() * (origs[i][2].y() - origs[i][0].y())
                            + uv.y() * (origs[i][1].y() - origs[i][0].y());
                    
                    const unsigned char r = orig[py * orig_pitch + px * orig_depth + 2];
                    const unsigned char g = orig[py * orig_pitch + px * orig_depth + 1];
                    const unsigned char b = orig[py * orig_pitch + px * orig_depth + 0];

                    dest[y * dest_pitch + x * dest_depth + 0] = r;
                    dest[y * dest_pitch + x * dest_depth + 1] = g;
                    dest[y * dest_pitch + x * dest_depth + 2] = b;

                    break;
                }
            }
        }
    this->texture = texture;
}

GLWidget::vec2 GLWidget::project(const vec3 &point)
{
    vec3 ret = rotation * point;
    num z = std::max<num>(.75, 1. + translation.z()/-60);
    int w = width(), h = height();
    num x = w * translation.x() / 2 / -40;
    if (std::abs(x) > w/2)
        x = x > 0 ? w/2 : w/-2;
    num y = h * translation.y() / 2 / -40;
    if (std::abs(y) > h/2)
        y = y > 0 ? h/2 : h/-2;
    return vec2(z * (ret.x() + x), z * (ret.y() + y));
}

GLWidget::vec3 GLWidget::project2(const vec3 &point)
{
    return rotation * point;
}
