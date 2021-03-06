#ifndef CAMERA_HPP
#define CAMERA_HPP

#include "Point.hpp"
#include "Vector.hpp"
#include "Quaternion.hpp"

namespace midnight
{


class Camera
{
    /// The position of this Camera in world space
    Point3F position;
    
    /// The quaternion representing the orientation of this Camera
    Quaternion<float> orientation;
    
    /// The angle of the field of view (in degrees)
    float fieldOfView;

    /// The aspect ratio
    float aspectRatio;

    /// The near clipping plane
    float zNear;

    /// The far clipping plane
    float zFar;
    
  public:
    
    /**
     * Constructs a Camera with the provided field of view angle, aspect ratio, and clipping planes
     * 
     * @param fieldOfView the field of view angle of this Camera
     * 
     * @param aspectRatio the aspect ratio of this Camera
     * 
     * @param zNear the distance of the near clipping plane of the view frustum
     * 
     * @param zFar the distance of the far clipping plane of the view frustum
     * 
     */
    Camera(float fieldOfView, float aspectRatio, float zNear, float zFar) noexcept;
    
    /**
     * TODO: Docs
     * 
     */
    void reset();
    
    /**
     * Retrieves an projection matrix generated by this Camera
     * 
     * @return an projection matrix generated by this Camera
     * 
     */
    Matrix4x4F getProjection() const noexcept;
    
    /**
     * Retrieves an orientation matrix generated by this Camera
     * 
     * @return an orientation matrix generated by this Camera
     * 
     */
    Matrix4x4F getOrientation() const noexcept;

    /**
     * Rotates this Camera around the provided axis by the provided angle
     * 
     * @param axis the axis around which to rotate
     * 
     * @param angle the angle by which to rotate
     * 
     */
    void rotate(const Vector3F& axis, const Radians<float>& angle);
    
    /**
     * Rotates this Camera by the provided Quaternion
     * 
     * @param quaternion the Quaternion by which to rotate
     * 
     */
    void rotate(const Quaternion<float>& quaternion);
    
    /**
     * Retrieves the position of this camera
     * 
     * @return the position of this camera
     * 
     */
    const Point3F& getPosition() const noexcept;
    
    /**
     * Retrieves the position of this camera
     * 
     * @return the position of this camera
     * 
     */
    Point3F& getPosition() noexcept;
    
    /**
     * Sets the position of this camera
     * 
     * @param position the new position
     * 
     */
    void setPosition(const Point3F& position) noexcept;
    
    /**
     * Translates the position of this Camera by the provided Vector
     * 
     * @param translation the Vector by which to translate
     * 
     */
    void translate(const Vector3F& translation) noexcept;
    
    /**
     * Retrieves the field of view angle of this Camera
     * 
     * @return the field of view angle of this Camera
     * 
     */
    float getFieldOfView() const noexcept;
    
    /**
     * Sets the field of view angle of this Camera
     * 
     * @param fieldOfView the new field of view angle
     * 
     */
    void setFieldOfView(float fieldOfView) noexcept;
    
    /**
     * Retrieves the aspect ratio of this Camera
     * 
     * @return the aspect ratio of this Camera
     * 
     */
    float getAspectRatio() const noexcept;
    
    /**
     * Sets the aspect ratio of this Camera
     * 
     * @param aspectRatio the new aspect ratio
     * 
     */
    void setAspectRatio(float aspectRatio) noexcept;
    
    /**
     * Retrieves the near clipping plane of the view frustum
     * 
     * @return the near clipping plane of the view frustum
     * 
     */
    float getNearClippingPlane() const noexcept;
    
    /**
     * Sets the distance of the near clipping plane of the view frustum
     * 
     * @param zNear the new near clipping plane to set
     * 
     */
    void setNearClippingPlane(float zNear) noexcept;

    /**
     * Retrieves the far clipping plane of the view frustum
     * 
     * @return the far clipping plane of the view frustum
     * 
     */
    float getFarClippingPlane() const noexcept;
    
    /**
     * Sets the distance of the far clipping plane of the view frustum
     * 
     * @param zFar the new far clipping plane to set
     * 
     */
    void setFarClippingPlane(float zFar) noexcept;
    
    /**
     * Default destructor
     * 
     */
    virtual ~Camera() noexcept = default;
};

}

#include "Camera.inl"

#endif